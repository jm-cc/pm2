/*
 * NewMadeleine
 * Copyright (C) 2013 (see AUTHORS file)
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

#include <dcfa.h>
#include <nm_connector.h>
#include <errno.h>

#include <Padico/Module.h>

#include "nm_dcfa.h"

/* *** method: 'bycopy' ************************************ */

#define NM_DCFA_BYCOPY_BLOCKSIZE 8192
#define NM_DCFA_BYCOPY_RBUF_NUM  8

#define NM_DCFA_BYCOPY_BUFSIZE     (NM_DCFA_BYCOPY_BLOCKSIZE - sizeof(struct nm_dcfa_bycopy_header))
#define NM_DCFA_BYCOPY_DATA_SIZE (nm_dcfa_checksum_enabled() ? NM_DCFA_BYCOPY_BUFSIZE : NM_DCFA_BYCOPY_BUFSIZE + sizeof(uint32_t))
#define NM_DCFA_BYCOPY_SBUF_NUM    2
#define NM_DCFA_BYCOPY_CREDITS_THR ((NM_DCFA_BYCOPY_RBUF_NUM / 2) + 1)

#define NM_DCFA_BYCOPY_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_DCFA_BYCOPY_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_DCFA_BYCOPY_STATUS_LAST    0x02  /**< last data block */
#define NM_DCFA_BYCOPY_STATUS_CREDITS 0x04  /**< message contains credits */

struct nm_dcfa_bycopy_header 
{
  uint32_t checksum;       /**< data checksum, if enabled; contains data otherwise */
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bycopy' minidriver */
struct nm_dcfa_bycopy_packet 
{
  char data[NM_DCFA_BYCOPY_BUFSIZE];
  struct nm_dcfa_bycopy_header header;
} __attribute__((packed));

/** Connection state for tracks sending by copy
 */
struct nm_dcfa_bycopy
{
  struct {
    struct nm_dcfa_bycopy_packet sbuf[NM_DCFA_BYCOPY_SBUF_NUM];
    struct nm_dcfa_bycopy_packet rbuf[NM_DCFA_BYCOPY_RBUF_NUM];
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
    int done;           /**< size of data received so far */
    int msg_size;       /**< size of whole message */
    void*buf_posted;    /**< buffer posted for receive */
    int size_posted;    /**< length of above buffer */
  } recv;
  
  struct {
    const struct iovec*v;
    int n;              /**< size of above iovec */
    int current_packet; /**< current buffer for sending */
    int msg_size;       /**< total size of message to send */
    int done;           /**< total amount of data sent */
    int v_done;         /**< size of current iovec segment already sent */
    int v_current;      /**< current iovec segment beeing sent */
  } send;
  
  struct nm_dcfa_segment seg; /**< remote segment */
  struct ibv_mr*mr;           /**< global MR (used for 'buffer') */
  struct nm_dcfa_cnx*cnx;
  puk_context_t context;
};

static void nm_dcfa_bycopy_getprops(int index, struct nm_minidriver_properties_s*props);
static void nm_dcfa_bycopy_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_dcfa_bycopy_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_dcfa_bycopy_send_post(void*_status, const struct iovec*v, int n);
static int  nm_dcfa_bycopy_send_poll(void*_status);
static void nm_dcfa_bycopy_recv_init(void*_status,  struct iovec*v, int n);
static int  nm_dcfa_bycopy_poll_one(void*_status);
static int  nm_dcfa_bycopy_cancel_recv(void*_status);

static const struct nm_minidriver_iface_s nm_dcfa_bycopy_minidriver =
  {
    .getprops    = &nm_dcfa_bycopy_getprops,
    .init        = &nm_dcfa_bycopy_init,
    .connect     = &nm_dcfa_bycopy_connect,
    .send_post   = &nm_dcfa_bycopy_send_post,
    .send_poll   = &nm_dcfa_bycopy_send_poll,
    .recv_init   = &nm_dcfa_bycopy_recv_init,
    .poll_one    = &nm_dcfa_bycopy_poll_one,
    .cancel_recv = &nm_dcfa_bycopy_cancel_recv
  };

static void*nm_dcfa_bycopy_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_dcfa_bycopy_destroy(void*);

static const struct puk_component_driver_s nm_dcfa_bycopy_component =
  {
    .instantiate = &nm_dcfa_bycopy_instantiate,
    .destroy = &nm_dcfa_bycopy_destroy
  };

PADICO_MODULE_COMPONENT(NewMad_dcfa_bycopy,
  puk_component_declare("NewMad_dcfa_bycopy",
			puk_component_provides("PadicoComponent", "component", &nm_dcfa_bycopy_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_dcfa_bycopy_minidriver)));


static void* nm_dcfa_bycopy_instantiate(puk_instance_t instance, puk_context_t context)
{
  /* check parameters consistency */
  assert(sizeof(struct nm_dcfa_bycopy_packet) % 1024 == 0);
  assert(NM_DCFA_BYCOPY_CREDITS_THR > NM_DCFA_BYCOPY_RBUF_NUM / 2);
  /* init */
  struct nm_dcfa_bycopy*bycopy = NULL;
  if(nm_dcfa_memalign > 0)
    {
      posix_memalign((void**)&bycopy, nm_dcfa_memalign, sizeof(struct nm_dcfa_bycopy));
    }
  else
    {
      bycopy = TBX_MALLOC(sizeof(struct nm_dcfa_bycopy));
    }
  memset(&bycopy->buffer, 0, sizeof(bycopy->buffer));
  bycopy->window.next_out = 1;
  bycopy->window.next_in  = 1;
  bycopy->window.credits  = NM_DCFA_BYCOPY_RBUF_NUM;
  bycopy->window.to_ack   = 0;
  bycopy->mr              = NULL;
  bycopy->recv.buf_posted = NULL;
  bycopy->send.v          = NULL;
  bycopy->context         = context;
  return bycopy;
}

static void nm_dcfa_bycopy_destroy(void*_status)
{
  /* TODO */
}


static void nm_dcfa_bycopy_getprops(int index, struct nm_minidriver_properties_s*props)
{
  nm_dcfa_hca_get_profile(index, &props->profile);
}

static void nm_dcfa_bycopy_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  const char*url = NULL;
  nm_connector_create(sizeof(struct nm_dcfa_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}


static void nm_dcfa_bycopy_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_dcfa_bycopy*bycopy = _status;
  const char*s_index = puk_context_getattr(bycopy->context, "index");
  const int index = atoi(s_index);
  struct nm_dcfa_hca_s*p_hca = nm_dcfa_hca_resolve(index);
  struct nm_dcfa_cnx*p_dcfa_cnx = nm_dcfa_cnx_new(p_hca);
  bycopy->cnx = p_dcfa_cnx;

  /* register Memory Region */
  bycopy->mr = ibv_reg_mr(p_hca->pd, &bycopy->buffer, sizeof(bycopy->buffer),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(bycopy->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: bycopy cannot register MR (errno = %d; %s).\n",
	       err, strerror(err));
    }

  struct nm_dcfa_segment*seg = &p_dcfa_cnx->local_addr.segment;
  seg->raddr = (uintptr_t)bycopy->mr->host_addr;
  seg->rkey  = bycopy->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(bycopy->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_dcfa_cnx->local_addr, &p_dcfa_cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: timeout in address exchange.\n");
    }
  bycopy->seg = p_dcfa_cnx->remote_addr.segment;
  bycopy->buffer.rready = 0;
  nm_dcfa_cnx_connect(p_dcfa_cnx);
  /*
  bycopy->buffer.sready = 1;
  while(!bycopy->buffer.rready)
    {
      nm_dcfa_sync_send((void*)&bycopy->buffer.sready, (void*)&bycopy->buffer.rready, sizeof(bycopy->buffer.sready), p_dcfa_cnx);
    }
  */
}



/* ** bycopy I/O ******************************************* */

static void nm_dcfa_bycopy_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_dcfa_bycopy*bycopy = _status;
  bycopy->send.v              = v;
  bycopy->send.n              = n;
  bycopy->send.current_packet = 0;
  bycopy->send.msg_size       = 0;
  bycopy->send.done           = 0;
  bycopy->send.v_done         = 0;
  bycopy->send.v_current      = 0;
  int i;
  for(i = 0; i < bycopy->send.n; i++)
    {
      bycopy->send.msg_size += bycopy->send.v[i].iov_len;
    }
}

static int nm_dcfa_bycopy_send_poll(void*_status)
{
  struct nm_dcfa_bycopy*__restrict__ bycopy = _status;
  while(bycopy->send.done < bycopy->send.msg_size)
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
	  for(j = 0; j < NM_DCFA_BYCOPY_RBUF_NUM; j++)
	    {
	      if(bycopy->buffer.rbuf[j].header.status)
		{
		  bycopy->window.credits += bycopy->buffer.rbuf[j].header.ack;
		  bycopy->buffer.rbuf[j].header.ack = 0;
		  bycopy->buffer.rbuf[j].header.status &= ~NM_DCFA_BYCOPY_STATUS_CREDITS;
		}
	    }
	}
      if(bycopy->window.credits <= 1) 
	{
	  goto wouldblock;
	}
      
      /* 2- check window availability */
      nm_dcfa_rdma_poll(bycopy->cnx);
      if(bycopy->cnx->pending.wrids[NM_DCFA_WRID_RDMA] >= NM_DCFA_BYCOPY_SBUF_NUM)
	{
	  goto wouldblock;
	}
      
      /* 3- prepare and send packet */
      struct nm_dcfa_bycopy_packet*__restrict__ packet = &bycopy->buffer.sbuf[bycopy->send.current_packet];
      const int remaining = bycopy->send.msg_size - bycopy->send.done;
      const int offset = (remaining > NM_DCFA_BYCOPY_DATA_SIZE) ? 0 : (NM_DCFA_BYCOPY_DATA_SIZE - remaining);
      int available   = NM_DCFA_BYCOPY_DATA_SIZE - offset;
      int packet_size = 0;
      while((available > 0) &&
	    (bycopy->send.v_current < bycopy->send.n))
	{
	  const int v_remaining = bycopy->send.v[bycopy->send.v_current].iov_len - bycopy->send.v_done;
	  const int fragment_size = (v_remaining > available) ? available : v_remaining;
	  memcpy(&packet->data[offset + packet_size],
		 bycopy->send.v[bycopy->send.v_current].iov_base + bycopy->send.v_done, fragment_size);
	  packet_size += fragment_size;
	  available   -= fragment_size;
	  bycopy->send.v_done += fragment_size;
	  if(bycopy->send.v_done >= bycopy->send.v[bycopy->send.v_current].iov_len) 
	    {
	      bycopy->send.v_current++;
	      bycopy->send.v_done = 0;
	    }
	}
      assert(NM_DCFA_BYCOPY_DATA_SIZE - offset == packet_size);
      
      packet->header.offset = offset;
      packet->header.ack    = bycopy->window.to_ack;
      packet->header.status = NM_DCFA_BYCOPY_STATUS_DATA;
      if(nm_dcfa_checksum_enabled())
	packet->header.checksum = nm_dcfa_checksum(&packet->data[offset], packet_size);
      bycopy->window.to_ack = 0;
      bycopy->send.done += packet_size;
      if(bycopy->send.done >= bycopy->send.msg_size)
	packet->header.status |= NM_DCFA_BYCOPY_STATUS_LAST;
      int padding = 0;
      const int size = sizeof(struct nm_dcfa_bycopy_packet) - offset;
      if(nm_dcfa_alignment > 0 && size >= 2048)
	padding = offset % nm_dcfa_alignment; 
      nm_dcfa_rdma_send(bycopy->cnx,
			   sizeof(struct nm_dcfa_bycopy_packet) - offset + padding,
			   &packet->data[offset - padding],
			   &bycopy->buffer.rbuf[bycopy->window.next_out].data[offset - padding],
			   &bycopy->buffer,
			   &bycopy->seg,
			   bycopy->mr,
			   NM_DCFA_WRID_RDMA);
      bycopy->send.current_packet = (bycopy->send.current_packet + 1) % NM_DCFA_BYCOPY_SBUF_NUM;
      bycopy->window.next_out     = (bycopy->window.next_out     + 1) % NM_DCFA_BYCOPY_RBUF_NUM;
      bycopy->window.credits--;
    }
  nm_dcfa_rdma_poll(bycopy->cnx);
  if(bycopy->cnx->pending.wrids[NM_DCFA_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static void nm_dcfa_bycopy_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_dcfa_bycopy*bycopy = _status;
  assert(bycopy->recv.buf_posted == NULL);
  bycopy->recv.done        = 0;
  bycopy->recv.msg_size    = -1;
  bycopy->recv.buf_posted  = v->iov_base;
  bycopy->recv.size_posted = v->iov_len;
}

static int nm_dcfa_bycopy_poll_one(void*_status)
{
  int err = -NM_EUNKNOWN;
  int complete = 0;
  struct nm_dcfa_bycopy*__restrict__ bycopy = _status;
  struct nm_dcfa_bycopy_packet*__restrict__ packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
  while( ( (bycopy->recv.msg_size == -1) || (!complete) ) &&
	 (packet->header.status != 0) ) 
    {
      if(bycopy->recv.msg_size == -1) 
	{
	  assert((packet->header.status & NM_DCFA_BYCOPY_STATUS_DATA) != 0);
	  bycopy->recv.msg_size = 0;
	}
      complete = (packet->header.status & NM_DCFA_BYCOPY_STATUS_LAST);
      const int offset = packet->header.offset;
      const int packet_size = NM_DCFA_BYCOPY_DATA_SIZE - offset;
      if(nm_dcfa_checksum_enabled())
	{
	  const uint32_t checksum = nm_dcfa_checksum(&packet->data[offset], packet_size);
	  if(checksum != packet->header.checksum)
	    {
	      fprintf(stderr, "nmad: FATAL- dcfa: checksum failed- received = %x; expected = %x.\n",
		      (unsigned)packet->header.checksum, (unsigned)checksum);
	      abort();
	    }
	}
      memcpy(bycopy->recv.buf_posted + bycopy->recv.done, &packet->data[offset], packet_size);
      bycopy->recv.msg_size += packet_size;
      bycopy->recv.done += packet_size;
      bycopy->window.credits += packet->header.ack;
      packet->header.ack = 0;
      packet->header.status = 0;
      bycopy->window.to_ack++;
      if(bycopy->window.to_ack > NM_DCFA_BYCOPY_CREDITS_THR) 
	{
	  bycopy->buffer.sack = bycopy->window.to_ack;
	  nm_dcfa_rdma_send(bycopy->cnx,
			    sizeof(uint16_t),
			    (void*)&bycopy->buffer.sack,
			    (void*)&bycopy->buffer.rack,
			    &bycopy->buffer,
			    &bycopy->seg,
			    bycopy->mr,
			    NM_DCFA_WRID_ACK);
	  bycopy->window.to_ack = 0;
	}
      nm_dcfa_rdma_poll(bycopy->cnx);
      bycopy->window.next_in = (bycopy->window.next_in + 1) % NM_DCFA_BYCOPY_RBUF_NUM;
      packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
    }
  nm_dcfa_rdma_poll(bycopy->cnx);
  if((bycopy->recv.msg_size > 0) && complete)
    {
      bycopy->recv.buf_posted = NULL;
      nm_dcfa_send_flush(bycopy->cnx, NM_DCFA_WRID_ACK);
      err = NM_ESUCCESS;
    }
  else 
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static int nm_dcfa_bycopy_cancel_recv(void*_status)
{
  int err = -NM_EAGAIN;
  struct nm_dcfa_bycopy*__restrict__ bycopy = _status;
  if(bycopy->recv.done == 0)
    {
      bycopy->recv.buf_posted = NULL;
      bycopy->recv.size_posted = 0;
      err = NM_ESUCCESS;
    }
  return err;
}
