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

#define NM_IBVERBS_BYCOPY_BUFSIZE     (NM_IBVERBS_BYCOPY_BLOCKSIZE - sizeof(struct nm_ibverbs_bycopy_header_s))
#define NM_IBVERBS_BYCOPY_DATA_SIZE (nm_ibverbs_checksum_enabled() ? NM_IBVERBS_BYCOPY_BUFSIZE : NM_IBVERBS_BYCOPY_BUFSIZE + sizeof(uint32_t))
#define NM_IBVERBS_BYCOPY_SBUF_NUM    3
#define NM_IBVERBS_BYCOPY_CREDITS_THR ((NM_IBVERBS_BYCOPY_RBUF_NUM / 2) + 1)

#define NM_IBVERBS_BYCOPY_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_BYCOPY_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_IBVERBS_BYCOPY_STATUS_LAST    0x02  /**< last data block */
#define NM_IBVERBS_BYCOPY_STATUS_CREDITS 0x04  /**< message contains credits */

struct nm_ibverbs_bycopy_header_s
{
  uint32_t checksum;       /**< data checksum, if enabled; contains data otherwise */
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bycopy' minidriver */
struct nm_ibverbs_bycopy_packet_s
{
  char data[NM_IBVERBS_BYCOPY_BUFSIZE];
  struct nm_ibverbs_bycopy_header_s header;
} __attribute__((packed));

/** context for ibverbs bycopy */
struct nm_ibverbs_bycopy_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bycopy_s
{
  struct {
    struct nm_ibverbs_bycopy_packet_s sbuf[NM_IBVERBS_BYCOPY_SBUF_NUM];
    struct nm_ibverbs_bycopy_packet_s rbuf[NM_IBVERBS_BYCOPY_RBUF_NUM];
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

  struct nm_ibverbs_segment_s seg; /**< remote segment */
  struct ibv_mr*mr;                /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx_s*p_cnx;
  puk_context_t context;
};

static void nm_ibverbs_bycopy_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_bycopy_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_bycopy_close(puk_context_t context);
static void nm_ibverbs_bycopy_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_bycopy_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_bycopy_send_poll(void*_status);
static void nm_ibverbs_bycopy_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_bycopy_recv_poll_one(void*_status);
static int  nm_ibverbs_bycopy_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_bycopy_minidriver =
  {
    .getprops        = &nm_ibverbs_bycopy_getprops,
    .init            = &nm_ibverbs_bycopy_init,
    .close           = &nm_ibverbs_bycopy_close,
    .connect         = &nm_ibverbs_bycopy_connect,
    .send_post       = NULL,
    .send_data       = &nm_ibverbs_bycopy_send_data,
    .send_poll       = &nm_ibverbs_bycopy_send_poll,
    .recv_iov_post   = NULL,
    .recv_data_post  = &nm_ibverbs_bycopy_recv_data_post,
    .recv_poll_one   = &nm_ibverbs_bycopy_recv_poll_one,
    .recv_cancel     = &nm_ibverbs_bycopy_recv_cancel
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
  assert(sizeof(struct nm_ibverbs_bycopy_packet_s) % 1024 == 0);
  assert(NM_IBVERBS_BYCOPY_CREDITS_THR > NM_IBVERBS_BYCOPY_RBUF_NUM / 2);
  /* init */
  struct nm_ibverbs_bycopy_s*p_bycopy = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&p_bycopy, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_bycopy_s));
    }
  else
    {
      p_bycopy = malloc(sizeof(struct nm_ibverbs_bycopy_s));
    }
  memset(&p_bycopy->buffer, 0, sizeof(p_bycopy->buffer));
  p_bycopy->window.next_out = 1;
  p_bycopy->window.next_in  = 1;
  p_bycopy->window.credits  = NM_IBVERBS_BYCOPY_RBUF_NUM;
  p_bycopy->window.to_ack   = 0;
  p_bycopy->mr              = NULL;
  p_bycopy->recv.slicer     = NM_DATA_SLICER_NULL;
  p_bycopy->send.slicer     = NM_DATA_SLICER_NULL;
  p_bycopy->context         = context;
  p_bycopy->p_cnx           = NULL;
  return p_bycopy;
}

static void nm_ibverbs_bycopy_destroy(void*_status)
{
  struct nm_ibverbs_bycopy_s*p_bycopy = _status;
  if(p_bycopy->p_cnx)
    {
      nm_ibverbs_cnx_close(p_bycopy->p_cnx);
    }
  if(p_bycopy->mr)
    {
      ibv_dereg_mr(p_bycopy->mr);
    }
  free(p_bycopy);
}

static void nm_ibverbs_bycopy_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = malloc(sizeof(struct nm_ibverbs_bycopy_context_s));
  puk_context_set_status(context, p_bycopy_context);
  p_bycopy_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_bycopy_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data = 1;
  p_props->capabilities.max_msg_size = NM_IBVERBS_BYCOPY_DATA_SIZE * NM_IBVERBS_BYCOPY_SBUF_NUM;
}

static void nm_ibverbs_bycopy_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  const char*url = NULL;
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = puk_context_get_status(context);
  p_bycopy_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
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
  struct nm_ibverbs_bycopy_s*p_bycopy = _status;
  struct nm_ibverbs_bycopy_context_s*p_bycopy_context = puk_context_get_status(p_bycopy->context);
  p_bycopy->p_cnx = nm_ibverbs_cnx_new(p_bycopy_context->p_hca);
  p_bycopy->mr = ibv_reg_mr(p_bycopy_context->p_hca->pd, &p_bycopy->buffer, sizeof(p_bycopy->buffer),
                            IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(p_bycopy->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: bycopy cannot register MR (errno = %d; %s).\n", err, strerror(err));
    }
  struct nm_ibverbs_segment_s*p_seg = &p_bycopy->p_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_bycopy->buffer;
  p_seg->rkey  = p_bycopy->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_bycopy->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_bycopy->p_cnx->local_addr, &p_bycopy->p_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_bycopy->seg = p_bycopy->p_cnx->remote_addr.segment;
  p_bycopy->buffer.rready = 0;
  nm_ibverbs_cnx_connect(p_bycopy->p_cnx);
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
  struct nm_ibverbs_bycopy_s*p_bycopy = _status;
  nm_data_slicer_generator_init(&p_bycopy->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_generator_forward(&p_bycopy->send.slicer, chunk_offset);
  p_bycopy->send.chunk_len      = chunk_len;
  p_bycopy->send.current_packet = 0;
  p_bycopy->send.done           = 0;
}

static int nm_ibverbs_bycopy_send_poll(void*_status)
{
  struct nm_ibverbs_bycopy_s*__restrict__ p_bycopy = _status;
  while(p_bycopy->send.done < p_bycopy->send.chunk_len)
    {
      /* 1- check credits */
      const int rack = p_bycopy->buffer.rack;
      if(rack)
        {
          p_bycopy->window.credits += rack;
          p_bycopy->buffer.rack = 0;
        }
      if(p_bycopy->window.credits <= 1)
	{
	  int j;
	  for(j = 0; j < NM_IBVERBS_BYCOPY_RBUF_NUM; j++)
	    {
	      if(p_bycopy->buffer.rbuf[j].header.status)
		{
		  p_bycopy->window.credits += p_bycopy->buffer.rbuf[j].header.ack;
		  p_bycopy->buffer.rbuf[j].header.ack = 0;
		  p_bycopy->buffer.rbuf[j].header.status &= ~NM_IBVERBS_BYCOPY_STATUS_CREDITS;
		}
	    }
	}
      if(p_bycopy->window.credits <= 1) 
	{
	  goto wouldblock;
	}
      
      /* 2- check window availability */
      nm_ibverbs_rdma_poll(p_bycopy->p_cnx);
      if(p_bycopy->p_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] >= NM_IBVERBS_BYCOPY_SBUF_NUM)
	{
	  goto wouldblock;
	}
      
      /* 3- prepare and send packet */
      struct nm_ibverbs_bycopy_packet_s*__restrict__ p_packet = &p_bycopy->buffer.sbuf[p_bycopy->send.current_packet];
      const nm_len_t remaining   = p_bycopy->send.chunk_len - p_bycopy->send.done;
      const nm_len_t offset      = (remaining > NM_IBVERBS_BYCOPY_DATA_SIZE) ? 0 : (NM_IBVERBS_BYCOPY_DATA_SIZE - remaining);
      const nm_len_t available   = NM_IBVERBS_BYCOPY_DATA_SIZE - offset;
      const nm_len_t packet_size = (remaining <= available) ? remaining : available;
      nm_data_slicer_generator_copy_from(&p_bycopy->send.slicer, &p_packet->data[offset], packet_size);
      assert(NM_IBVERBS_BYCOPY_DATA_SIZE - offset == packet_size);
      
      p_packet->header.offset = offset;
      p_packet->header.ack    = p_bycopy->window.to_ack;
      p_packet->header.status = NM_IBVERBS_BYCOPY_STATUS_DATA;
      if(nm_ibverbs_checksum_enabled())
	p_packet->header.checksum = nm_ibverbs_checksum(&p_packet->data[offset], packet_size);
      p_bycopy->window.to_ack = 0;
      p_bycopy->send.done += packet_size;
      if(p_bycopy->send.done >= p_bycopy->send.chunk_len)
	p_packet->header.status |= NM_IBVERBS_BYCOPY_STATUS_LAST;
      int padding = 0;
      const int size = sizeof(struct nm_ibverbs_bycopy_packet_s) - offset;
      if(nm_ibverbs_alignment > 0 && size >= 2048)
	padding = offset % nm_ibverbs_alignment; 
      nm_ibverbs_rdma_send(p_bycopy->p_cnx,
			   sizeof(struct nm_ibverbs_bycopy_packet_s) - offset + padding,
			   &p_packet->data[offset - padding],
			   &p_bycopy->buffer.rbuf[p_bycopy->window.next_out].data[offset - padding],
			   &p_bycopy->buffer,
			   &p_bycopy->seg,
			   p_bycopy->mr,
			   NM_IBVERBS_WRID_RDMA);
      p_bycopy->send.current_packet = (p_bycopy->send.current_packet + 1) % NM_IBVERBS_BYCOPY_SBUF_NUM;
      p_bycopy->window.next_out     = (p_bycopy->window.next_out     + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      p_bycopy->window.credits--;
    }
  nm_ibverbs_rdma_poll(p_bycopy->p_cnx);
  if(p_bycopy->p_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  nm_data_slicer_generator_destroy(&p_bycopy->send.slicer);
  p_bycopy->send.slicer = NM_DATA_SLICER_NULL;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static void nm_ibverbs_bycopy_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_bycopy_s*p_bycopy = _status;
  assert(nm_data_slicer_isnull(&p_bycopy->recv.slicer));
  nm_data_slicer_generator_init(&p_bycopy->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_generator_forward(&p_bycopy->recv.slicer, chunk_offset);
  p_bycopy->recv.chunk_len = chunk_len;
  p_bycopy->recv.done      = 0;
}

static int nm_ibverbs_bycopy_recv_poll_one(void*_status)
{
  int err = -NM_EUNKNOWN;
  int complete = 0;
  struct nm_ibverbs_bycopy_s*__restrict__ p_bycopy = _status;
  struct nm_ibverbs_bycopy_packet_s*__restrict__ p_packet = &p_bycopy->buffer.rbuf[p_bycopy->window.next_in];
  while( (!complete) && (p_packet->header.status != 0) ) 
    {
      assert(p_bycopy->recv.done <= p_bycopy->recv.chunk_len);
      if(p_bycopy->recv.done == 0)
	{
	  assert((p_packet->header.status & NM_IBVERBS_BYCOPY_STATUS_DATA) != 0);
	}
      complete = (p_packet->header.status & NM_IBVERBS_BYCOPY_STATUS_LAST);
      const int offset = p_packet->header.offset;
      const int packet_size = NM_IBVERBS_BYCOPY_DATA_SIZE - offset;
      if(nm_ibverbs_checksum_enabled())
	{
	  const uint32_t checksum = nm_ibverbs_checksum(&p_packet->data[offset], packet_size);
	  if(checksum != p_packet->header.checksum)
	    {
	      NM_FATAL("ibverbs: checksum failed- received = %x; expected = %x.\n",
                       (unsigned)p_packet->header.checksum, (unsigned)checksum);
	    }
	}
      nm_data_slicer_generator_copy_to(&p_bycopy->recv.slicer, &p_packet->data[offset], packet_size);
      p_bycopy->recv.done += packet_size;
      p_bycopy->window.credits += p_packet->header.ack;
      p_packet->header.ack = 0;
      p_packet->header.status = 0;
      p_bycopy->window.to_ack++;
      if(p_bycopy->window.to_ack > NM_IBVERBS_BYCOPY_CREDITS_THR) 
	{
	  p_bycopy->buffer.sack = p_bycopy->window.to_ack;
	  nm_ibverbs_rdma_send(p_bycopy->p_cnx,
			       sizeof(uint16_t),
			       (void*)&p_bycopy->buffer.sack,
			       (void*)&p_bycopy->buffer.rack,
			       &p_bycopy->buffer,
			       &p_bycopy->seg,
			       p_bycopy->mr,
			       NM_IBVERBS_WRID_ACK);
	  p_bycopy->window.to_ack = 0;
	}
      nm_ibverbs_rdma_poll(p_bycopy->p_cnx);
      p_bycopy->window.next_in = (p_bycopy->window.next_in + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      p_packet = &p_bycopy->buffer.rbuf[p_bycopy->window.next_in];
    }
  nm_ibverbs_rdma_poll(p_bycopy->p_cnx);
  if(complete)
    {
      nm_data_slicer_generator_destroy(&p_bycopy->recv.slicer);
      p_bycopy->recv.slicer = NM_DATA_SLICER_NULL;
      nm_ibverbs_send_flush(p_bycopy->p_cnx, NM_IBVERBS_WRID_ACK);
      err = NM_ESUCCESS;
    }
  else 
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static int nm_ibverbs_bycopy_recv_cancel(void*_status)
{
  int err = -NM_EAGAIN;
  struct nm_ibverbs_bycopy_s*__restrict__ p_bycopy = _status;
  if(p_bycopy->recv.done == 0)
    {
      if(!nm_data_slicer_isnull(&p_bycopy->recv.slicer))
	nm_data_slicer_generator_destroy(&p_bycopy->recv.slicer);
      p_bycopy->recv.slicer = NM_DATA_SLICER_NULL;
      err = NM_ESUCCESS;
    }
  return err;
}
