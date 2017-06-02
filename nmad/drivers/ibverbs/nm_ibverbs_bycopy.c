/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */

#include "nm_ibverbs.h"
#include <nm_connector.h>
#include <errno.h>

#include <Padico/Module.h>


/* *** method: 'bycopy' ************************************ */

#define NM_IBVERBS_BYCOPY_BLOCKSIZE 8192
#define NM_IBVERBS_BYCOPY_RBUF_NUM  8

#define NM_IBVERBS_BYCOPY_BUFSIZE     (NM_IBVERBS_BYCOPY_BLOCKSIZE - sizeof(struct nm_ibverbs_bycopy_header))
#define NM_IBVERBS_BYCOPY_DATA_SIZE (nm_ibverbs_checksum_enabled() ? NM_IBVERBS_BYCOPY_BUFSIZE : NM_IBVERBS_BYCOPY_BUFSIZE + sizeof(uint32_t))
#define NM_IBVERBS_BYCOPY_SBUF_NUM    3
#define NM_IBVERBS_BYCOPY_CREDITS_THR ((NM_IBVERBS_BYCOPY_RBUF_NUM / 2) + 1)

#define NM_IBVERBS_BYCOPY_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_BYCOPY_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_IBVERBS_BYCOPY_STATUS_LAST    0x02  /**< last data block */
#define NM_IBVERBS_BYCOPY_STATUS_CREDITS 0x04  /**< message contains credits */

struct nm_ibverbs_bycopy_header 
{
  uint32_t checksum;       /**< data checksum, if enabled; contains data otherwise */
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bycopy' minidriver */
struct nm_ibverbs_bycopy_packet 
{
  char data[NM_IBVERBS_BYCOPY_BUFSIZE];
  struct nm_ibverbs_bycopy_header header;
} __attribute__((packed));

/** context for ibverbs bycopy */
struct nm_ibverbs_bycopy_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bycopy
{
  struct {
    struct nm_ibverbs_bycopy_packet sbuf[NM_IBVERBS_BYCOPY_SBUF_NUM];
    struct nm_ibverbs_bycopy_packet rbuf[NM_IBVERBS_BYCOPY_RBUF_NUM];
    volatile uint16_t rack, sack;     /**< credits acknowledgement (recv, send) */
    volatile uint64_t rready, sready; /**< peer is ready (recv, send) */
  } buffer;
  
  struct {
    uint32_t next_out;  /**< next sequence number for outgoing packet */
    uint32_t credits;   /**< remaining credits for sending */
    uint32_t next_in;   /**< cell number of next expected packet */
    uint32_t to_ack;    /**< credits not acked yet by the receiver */
  } window;
  
  struct {
    nm_data_slicer_t slicer;  /**< data slicer for data to recv */
    nm_len_t chunk_len;       /**< length of chunk to send */
    nm_len_t done;            /**< size of data received so far */
  } recv;
  
  struct {
    nm_data_slicer_t slicer;       /**< data slicer for data to send */
    nm_len_t chunk_len;            /**< length of chunk to send */
    nm_len_t done;                 /**< total amount of data sent */
    int current_packet;            /**< current buffer for sending */
  } send;

  struct nm_ibverbs_segment seg; /**< remote segment */
  struct ibv_mr*mr;           /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx*cnx;
  puk_context_t context;
};

static void nm_ibverbs_bycopy_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_ibverbs_bycopy_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_bycopy_close(puk_context_t context);
static void nm_ibverbs_bycopy_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_bycopy_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_bycopy_send_poll(void*_status);
static void nm_ibverbs_bycopy_recv_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_bycopy_poll_one(void*_status);
static int  nm_ibverbs_bycopy_cancel_recv(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_bycopy_minidriver =
  {
    .getprops    = &nm_ibverbs_bycopy_getprops,
    .init        = &nm_ibverbs_bycopy_init,
    .close       = &nm_ibverbs_bycopy_close,
    .connect     = &nm_ibverbs_bycopy_connect,
    .send_post   = NULL,
    .send_data   = &nm_ibverbs_bycopy_send_data,
    .send_poll   = &nm_ibverbs_bycopy_send_poll,
    .recv_init   = NULL,
    .recv_data   = &nm_ibverbs_bycopy_recv_data,
    .poll_one    = &nm_ibverbs_bycopy_poll_one,
    .cancel_recv = &nm_ibverbs_bycopy_cancel_recv
  };

static void*nm_ibverbs_bycopy_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_bycopy_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_bycopy_component =
  {
    .instantiate = &nm_ibverbs_bycopy_instantiate,
    .destroy = &nm_ibverbs_bycopy_destroy
  };

PADICO_MODULE_COMPONENT(NewMad_ibverbs_bycopy,
  puk_component_declare("NewMad_ibverbs_bycopy",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_bycopy_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_bycopy_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));


static void* nm_ibverbs_bycopy_instantiate(puk_instance_t instance, puk_context_t context)
{
  /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_bycopy_packet) % 1024 == 0);
  assert(NM_IBVERBS_BYCOPY_CREDITS_THR > NM_IBVERBS_BYCOPY_RBUF_NUM / 2);
  /* init */
  struct nm_ibverbs_bycopy*bycopy = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&bycopy, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_bycopy));
    }
  else
    {
      bycopy = malloc(sizeof(struct nm_ibverbs_bycopy));
    }
  memset(&bycopy->buffer, 0, sizeof(bycopy->buffer));
  bycopy->window.next_out = 1;
  bycopy->window.next_in  = 1;
  bycopy->window.credits  = NM_IBVERBS_BYCOPY_RBUF_NUM;
  bycopy->window.to_ack   = 0;
  bycopy->mr              = NULL;
  bycopy->recv.slicer     = NM_DATA_SLICER_NULL;
  bycopy->send.slicer     = NM_DATA_SLICER_NULL;
  bycopy->context         = context;
  bycopy->cnx             = NULL;
  return bycopy;
}

static void nm_ibverbs_bycopy_destroy(void*_status)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  if(bycopy->cnx)
    {
      nm_ibverbs_cnx_close(bycopy->cnx);
    }
  if(bycopy->mr)
    {
      ibv_dereg_mr(bycopy->mr);
    }
  free(bycopy);
}


static void nm_ibverbs_bycopy_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = malloc(sizeof(struct nm_ibverbs_bycopy_context_s));
  puk_context_set_status(context, p_bycopy_context);
  p_bycopy_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_bycopy_context->p_hca, &props->profile);
  props->capabilities.supports_data = 1;
  props->capabilities.max_msg_size = NM_IBVERBS_BYCOPY_DATA_SIZE * NM_IBVERBS_BYCOPY_SBUF_NUM;
}

static void nm_ibverbs_bycopy_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  const char*url = NULL;
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = puk_context_get_status(context);
  p_bycopy_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_bycopy_close(puk_context_t context)
{
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = puk_context_get_status(context);
  nm_connector_destroy(p_bycopy_context->p_connector);
  nm_ibverbs_hca_release(p_bycopy_context->p_hca);
  puk_context_set_status(context, NULL);
  free(p_bycopy_context);
}


static void nm_ibverbs_bycopy_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = puk_context_get_status(bycopy->context);
  bycopy->cnx = nm_ibverbs_cnx_new(p_bycopy_context->p_hca);
  bycopy->mr = ibv_reg_mr(p_bycopy_context->p_hca->pd, &bycopy->buffer, sizeof(bycopy->buffer),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(bycopy->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: bycopy cannot register MR (errno = %d; %s).\n",
	       err, strerror(err));
    }

  struct nm_ibverbs_segment*seg = &bycopy->cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&bycopy->buffer;
  seg->rkey  = bycopy->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(bycopy->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &bycopy->cnx->local_addr, &bycopy->cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: timeout in address exchange.\n");
    }
  bycopy->seg = bycopy->cnx->remote_addr.segment;
  bycopy->buffer.rready = 0;
  nm_ibverbs_cnx_connect(bycopy->cnx);
  /*
  bycopy->buffer.sready = 1;
  while(!bycopy->buffer.rready)
    {
      nm_ibverbs_sync_send((void*)&bycopy->buffer.sready, (void*)&bycopy->buffer.rready, sizeof(bycopy->buffer.sready), bycopy->cnx);
    }
  */
}



/* ** bycopy I/O ******************************************* */

static void nm_ibverbs_bycopy_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  nm_data_slicer_generator_init(&bycopy->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_generator_forward(&bycopy->send.slicer, chunk_offset);
  bycopy->send.chunk_len      = chunk_len;
  bycopy->send.current_packet = 0;
  bycopy->send.done           = 0;
}

static int nm_ibverbs_bycopy_send_poll(void*_status)
{
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  while(bycopy->send.done < bycopy->send.chunk_len)
    {
      /* 1- check credits */
      const int rack = bycopy->buffer.rack;
      if(rack) {
	bycopy->window.credits += rack;
	bycopy->buffer.rack = 0;
      }
      if(bycopy->window.credits <= 1)
	{
	  int j;
	  for(j = 0; j < NM_IBVERBS_BYCOPY_RBUF_NUM; j++)
	    {
	      if(bycopy->buffer.rbuf[j].header.status)
		{
		  bycopy->window.credits += bycopy->buffer.rbuf[j].header.ack;
		  bycopy->buffer.rbuf[j].header.ack = 0;
		  bycopy->buffer.rbuf[j].header.status &= ~NM_IBVERBS_BYCOPY_STATUS_CREDITS;
		}
	    }
	}
      if(bycopy->window.credits <= 1) 
	{
	  goto wouldblock;
	}
      
      /* 2- check window availability */
      nm_ibverbs_rdma_poll(bycopy->cnx);
      if(bycopy->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] >= NM_IBVERBS_BYCOPY_SBUF_NUM)
	{
	  goto wouldblock;
	}
      
      /* 3- prepare and send packet */
      struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.sbuf[bycopy->send.current_packet];
      const nm_len_t remaining   = bycopy->send.chunk_len - bycopy->send.done;
      const nm_len_t offset      = (remaining > NM_IBVERBS_BYCOPY_DATA_SIZE) ? 0 : (NM_IBVERBS_BYCOPY_DATA_SIZE - remaining);
      const nm_len_t available   = NM_IBVERBS_BYCOPY_DATA_SIZE - offset;
      const nm_len_t packet_size = (remaining <= available) ? remaining : available;
      nm_data_slicer_generator_copy_from(&bycopy->send.slicer, &packet->data[offset], packet_size);
      assert(NM_IBVERBS_BYCOPY_DATA_SIZE - offset == packet_size);
      
      packet->header.offset = offset;
      packet->header.ack    = bycopy->window.to_ack;
      packet->header.status = NM_IBVERBS_BYCOPY_STATUS_DATA;
      if(nm_ibverbs_checksum_enabled())
	packet->header.checksum = nm_ibverbs_checksum(&packet->data[offset], packet_size);
      bycopy->window.to_ack = 0;
      bycopy->send.done += packet_size;
      if(bycopy->send.done >= bycopy->send.chunk_len)
	packet->header.status |= NM_IBVERBS_BYCOPY_STATUS_LAST;
      int padding = 0;
      const int size = sizeof(struct nm_ibverbs_bycopy_packet) - offset;
      if(nm_ibverbs_alignment > 0 && size >= 2048)
	padding = offset % nm_ibverbs_alignment; 
      nm_ibverbs_rdma_send(bycopy->cnx,
			   sizeof(struct nm_ibverbs_bycopy_packet) - offset + padding,
			   &packet->data[offset - padding],
			   &bycopy->buffer.rbuf[bycopy->window.next_out].data[offset - padding],
			   &bycopy->buffer,
			   &bycopy->seg,
			   bycopy->mr,
			   NM_IBVERBS_WRID_RDMA);
      bycopy->send.current_packet = (bycopy->send.current_packet + 1) % NM_IBVERBS_BYCOPY_SBUF_NUM;
      bycopy->window.next_out     = (bycopy->window.next_out     + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      bycopy->window.credits--;
    }
  nm_ibverbs_rdma_poll(bycopy->cnx);
  if(bycopy->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  nm_data_slicer_generator_destroy(&bycopy->send.slicer);
  bycopy->send.slicer = NM_DATA_SLICER_NULL;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static void nm_ibverbs_bycopy_recv_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  assert(nm_data_slicer_isnull(&bycopy->recv.slicer));
  nm_data_slicer_generator_init(&bycopy->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_generator_forward(&bycopy->recv.slicer, chunk_offset);
  bycopy->recv.chunk_len = chunk_len;
  bycopy->recv.done      = 0;
}

static int nm_ibverbs_bycopy_poll_one(void*_status)
{
  int err = -NM_EUNKNOWN;
  int complete = 0;
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
  while( (!complete) && (packet->header.status != 0) ) 
    {
      assert(bycopy->recv.done <= bycopy->recv.chunk_len);
      if(bycopy->recv.done == 0)
	{
	  assert((packet->header.status & NM_IBVERBS_BYCOPY_STATUS_DATA) != 0);
	}
      complete = (packet->header.status & NM_IBVERBS_BYCOPY_STATUS_LAST);
      const int offset = packet->header.offset;
      const int packet_size = NM_IBVERBS_BYCOPY_DATA_SIZE - offset;
      if(nm_ibverbs_checksum_enabled())
	{
	  const uint32_t checksum = nm_ibverbs_checksum(&packet->data[offset], packet_size);
	  if(checksum != packet->header.checksum)
	    {
	      fprintf(stderr, "nmad: FATAL- ibverbs: checksum failed- received = %x; expected = %x.\n",
		      (unsigned)packet->header.checksum, (unsigned)checksum);
	      abort();
	    }
	}
      nm_data_slicer_generator_copy_to(&bycopy->recv.slicer, &packet->data[offset], packet_size);
      bycopy->recv.done += packet_size;
      bycopy->window.credits += packet->header.ack;
      packet->header.ack = 0;
      packet->header.status = 0;
      bycopy->window.to_ack++;
      if(bycopy->window.to_ack > NM_IBVERBS_BYCOPY_CREDITS_THR) 
	{
	  bycopy->buffer.sack = bycopy->window.to_ack;
	  nm_ibverbs_rdma_send(bycopy->cnx,
			       sizeof(uint16_t),
			       (void*)&bycopy->buffer.sack,
			       (void*)&bycopy->buffer.rack,
			       &bycopy->buffer,
			       &bycopy->seg,
			       bycopy->mr,
			       NM_IBVERBS_WRID_ACK);
	  bycopy->window.to_ack = 0;
	}
      nm_ibverbs_rdma_poll(bycopy->cnx);
      bycopy->window.next_in = (bycopy->window.next_in + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
    }
  nm_ibverbs_rdma_poll(bycopy->cnx);
  if(complete)
    {
      nm_data_slicer_generator_destroy(&bycopy->recv.slicer);
      bycopy->recv.slicer = NM_DATA_SLICER_NULL;
      nm_ibverbs_send_flush(bycopy->cnx, NM_IBVERBS_WRID_ACK);
      err = NM_ESUCCESS;
    }
  else 
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static int nm_ibverbs_bycopy_cancel_recv(void*_status)
{
  int err = -NM_EAGAIN;
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  if(bycopy->recv.done == 0)
    {
      if(!nm_data_slicer_isnull(&bycopy->recv.slicer))
	nm_data_slicer_generator_destroy(&bycopy->recv.slicer);
      bycopy->recv.slicer = NM_DATA_SLICER_NULL;
      err = NM_ESUCCESS;
    }
  return err;
}
