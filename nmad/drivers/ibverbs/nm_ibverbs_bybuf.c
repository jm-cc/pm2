/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <nm_private.h>


/* *** method: 'bybuf' ************************************ */

#define NM_IBVERBS_BYBUF_BLOCKSIZE (24 * 1024)
#define NM_IBVERBS_BYBUF_RBUF_NUM  8

#define NM_IBVERBS_BYBUF_BUFSIZE     (NM_IBVERBS_BYBUF_BLOCKSIZE - sizeof(struct nm_ibverbs_bybuf_header_s))
#define NM_IBVERBS_BYBUF_DATA_SIZE    NM_IBVERBS_BYBUF_BUFSIZE

#define NM_IBVERBS_BYBUF_CREDITS_THR ((NM_IBVERBS_BYBUF_RBUF_NUM / 2) + 1)

#define NM_IBVERBS_BYBUF_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_BYBUF_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */

struct nm_ibverbs_bybuf_header_s
{
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bybuf' minidriver */
struct nm_ibverbs_bybuf_packet_s
{
  char data[NM_IBVERBS_BYBUF_BUFSIZE];
  struct nm_ibverbs_bybuf_header_s header;
} __attribute__((packed));

/** context for ibverbs bybuf */
struct nm_ibverbs_bybuf_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bybuf
{
  struct
  {
    struct nm_ibverbs_bybuf_packet_s sbuf;
    struct nm_ibverbs_bybuf_packet_s rbuf[NM_IBVERBS_BYBUF_RBUF_NUM];
    volatile uint16_t rack, sack;     /**< credits acknowledgement (recv, send) */
  } buffer;
  
  struct
  {
    uint32_t next_out;  /**< next sequence number for outgoing packet */
    uint32_t next_in;   /**< cell number of next expected packet */
    int credits;        /**< remaining credits for sending */
    int to_ack;         /**< credits not acked yet by the receiver */
  } window;
  
  struct
  {
    void*buf;                 /**< buffer to store recevived data */
    nm_len_t chunk_len;       /**< length of chunk to send */
  } recv;
  
  struct
  {
    nm_len_t chunk_len;            /**< length of chunk to send */
    nm_len_t done;                 /**< total amount of data sent */
  } send;

  struct nm_ibverbs_segment seg;   /**< remote segment */
  struct ibv_mr*mr;                /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx*cnx;
  puk_context_t context;
};

static void nm_ibverbs_bybuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_ibverbs_bybuf_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_bybuf_close(puk_context_t context);
static void nm_ibverbs_bybuf_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_bybuf_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_bybuf_buf_send_post(void*_status, nm_len_t len);
static int  nm_ibverbs_bybuf_send_poll(void*_status);
static int  nm_ibverbs_bybuf_cancel_recv(void*_status);
static int  nm_ibverbs_bybuf_buf_recv_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_bybuf_buf_recv_release(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_bybuf_minidriver =
  {
    .getprops         = &nm_ibverbs_bybuf_getprops,
    .init             = &nm_ibverbs_bybuf_init,
    .close            = &nm_ibverbs_bybuf_close,
    .connect          = &nm_ibverbs_bybuf_connect,
    .send_post        = NULL,
    .send_data        = NULL,
    .send_poll        = &nm_ibverbs_bybuf_send_poll,
    .buf_send_get     = &nm_ibverbs_bybuf_buf_send_get,
    .buf_send_post    = &nm_ibverbs_bybuf_buf_send_post,
    .recv_init        = NULL,
    .recv_data        = NULL,
    .poll_one         = NULL,
    .cancel_recv      = &nm_ibverbs_bybuf_cancel_recv,
    .buf_recv_poll    = &nm_ibverbs_bybuf_buf_recv_poll,
    .buf_recv_release = &nm_ibverbs_bybuf_buf_recv_release
  };

static void*nm_ibverbs_bybuf_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_bybuf_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_bybuf_component =
  {
    .instantiate = &nm_ibverbs_bybuf_instantiate,
    .destroy = &nm_ibverbs_bybuf_destroy
  };

PADICO_MODULE_COMPONENT(NewMad_ibverbs_bybuf,
  puk_component_declare("NewMad_ibverbs_bybuf",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_bybuf_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_bybuf_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));

/** atomically get the number of credits to ACK and reset it to 0 */
static inline int nm_ibverbs_bybuf_to_ack(struct nm_ibverbs_bybuf*__restrict__ bybuf)
{
  const int to_ack = bybuf->window.to_ack;
  if(to_ack > 0)
    {
      return nm_atomic_compare_and_swap(&bybuf->window.to_ack, to_ack, 0) ? to_ack : 0;
    }
  else
    {
      return 0;
    }
}


static void* nm_ibverbs_bybuf_instantiate(puk_instance_t instance, puk_context_t context)
{
  /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_bybuf_packet_s) % 1024 == 0);
  assert(NM_IBVERBS_BYBUF_CREDITS_THR > NM_IBVERBS_BYBUF_RBUF_NUM / 2);
  /* init */
  struct nm_ibverbs_bybuf*bybuf = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&bybuf, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_bybuf));
    }
  else
    {
      bybuf = malloc(sizeof(struct nm_ibverbs_bybuf));
    }
  memset(&bybuf->buffer, 0, sizeof(bybuf->buffer));
  bybuf->window.next_out = 1;
  bybuf->window.next_in  = 1;
  bybuf->window.credits  = NM_IBVERBS_BYBUF_RBUF_NUM;
  bybuf->window.to_ack   = 0;
  bybuf->mr              = NULL;
  bybuf->recv.buf        = NULL;
  bybuf->send.done       = NM_LEN_UNDEFINED;
  bybuf->send.chunk_len  = NM_LEN_UNDEFINED;
  bybuf->context         = context;
  bybuf->cnx             = NULL;
  return bybuf;
}

static void nm_ibverbs_bybuf_destroy(void*_status)
{
  struct nm_ibverbs_bybuf*bybuf = _status;
  if(bybuf->cnx)
    {
      nm_ibverbs_cnx_close(bybuf->cnx);
    }
  if(bybuf->mr)
    {
      ibv_dereg_mr(bybuf->mr);
    }
  free(bybuf);
}


static void nm_ibverbs_bybuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  assert(context != NULL);
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = malloc(sizeof(struct nm_ibverbs_bybuf_context_s));
  p_bybuf_context->p_hca = nm_ibverbs_hca_from_context(context);
  puk_context_set_status(context, p_bybuf_context);
  nm_ibverbs_hca_get_profile(p_bybuf_context->p_hca, &props->profile);
  props->capabilities.supports_data = 0;
  props->capabilities.supports_buf_send = 1;
  props->capabilities.supports_buf_recv = 1;
  props->capabilities.max_msg_size = NM_IBVERBS_BYBUF_DATA_SIZE;
}

static void nm_ibverbs_bybuf_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(context);
  const char*url = NULL;
  p_bybuf_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_bybuf_close(puk_context_t context)
{
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(context);
  nm_connector_destroy(p_bybuf_context->p_connector);
  nm_ibverbs_hca_release(p_bybuf_context->p_hca);
  puk_context_set_status(context, NULL);
  free(p_bybuf_context);
}


static void nm_ibverbs_bybuf_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_bybuf*bybuf = _status;
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(bybuf->context);
  bybuf->cnx = nm_ibverbs_cnx_new(p_bybuf_context->p_hca);
  bybuf->mr = ibv_reg_mr(p_bybuf_context->p_hca->pd, &bybuf->buffer, sizeof(bybuf->buffer),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(bybuf->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: bybuf cannot register MR (errno = %d; %s).\n",
	       err, strerror(err));
    }

  struct nm_ibverbs_segment*seg = &bybuf->cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&bybuf->buffer;
  seg->rkey  = bybuf->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(bybuf->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &bybuf->cnx->local_addr, &bybuf->cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: timeout in address exchange.\n");
    }
  bybuf->seg = bybuf->cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(bybuf->cnx);
}



/* ** bybuf I/O ******************************************* */

static void nm_ibverbs_bybuf_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;
  assert(bybuf->send.done == NM_LEN_UNDEFINED);
  assert(bybuf->send.chunk_len == NM_LEN_UNDEFINED);
  *p_buffer = &bybuf->buffer.sbuf;
  *p_len = NM_IBVERBS_BYBUF_DATA_SIZE;
  bybuf->send.done = 0;
}

static void nm_ibverbs_bybuf_buf_send_post(void*_status, nm_len_t len)
{
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;
  bybuf->send.chunk_len = len;
  assert(len <= NM_IBVERBS_BYBUF_DATA_SIZE);
}
static int nm_ibverbs_bybuf_send_poll(void*_status)
{
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;

  if(bybuf->send.done == bybuf->send.chunk_len)
    {
      /* already sent; just poll */
      goto fastpoll;
    }
  
  /* 1- check credits */
  const int rack = bybuf->buffer.rack;
  if(rack)
    {
      nm_atomic_add(&bybuf->window.credits, rack);
      bybuf->buffer.rack = 0;
    }
  if(bybuf->window.credits <= 1)
    {
      goto wouldblock;
    }

  /* 2- check window availability */
  assert(bybuf->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] == 0);

  /* 3- prepare and send packet */
  const nm_len_t offset0 = NM_IBVERBS_BYBUF_DATA_SIZE - bybuf->send.chunk_len;
  const nm_len_t padding = (nm_ibverbs_alignment > 0 &&  bybuf->send.chunk_len >= 1024) ?
    (offset0 % nm_ibverbs_alignment) : 0 ;
  assert((nm_ibverbs_alignment == 0) || (padding < nm_ibverbs_alignment));
  const nm_len_t offset = offset0 - padding;
  assert(offset >= 0);
  assert(offset <= NM_IBVERBS_BYBUF_DATA_SIZE);
  void*p_packet = &bybuf->buffer.sbuf;
  struct nm_ibverbs_bybuf_header_s*p_header = p_packet + bybuf->send.chunk_len + padding;
  assert(bybuf->send.chunk_len <= NM_IBVERBS_BYBUF_DATA_SIZE);
  p_header->offset = offset;
  p_header->ack    = nm_ibverbs_bybuf_to_ack(bybuf);
  p_header->status = NM_IBVERBS_BYBUF_STATUS_DATA;
  bybuf->send.done = bybuf->send.chunk_len;
  nm_ibverbs_rdma_send(bybuf->cnx,
		       sizeof(struct nm_ibverbs_bybuf_header_s) + bybuf->send.chunk_len + padding,
		       p_packet,
		       &bybuf->buffer.rbuf[bybuf->window.next_out].data[offset],
		       &bybuf->buffer,
		       &bybuf->seg,
		       bybuf->mr,
		       NM_IBVERBS_WRID_RDMA);
  bybuf->window.next_out = (bybuf->window.next_out + 1) % NM_IBVERBS_BYBUF_RBUF_NUM;
  nm_atomic_dec(&bybuf->window.credits);
 fastpoll:
  nm_ibverbs_rdma_poll(bybuf->cnx);
  if(bybuf->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  assert(bybuf->send.done <= bybuf->send.chunk_len);
  bybuf->send.done = NM_LEN_UNDEFINED;
  bybuf->send.chunk_len = NM_LEN_UNDEFINED;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static int nm_ibverbs_bybuf_buf_recv_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  int err = -NM_EUNKNOWN;
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;
  struct nm_ibverbs_bybuf_packet_s*__restrict__ packet = &bybuf->buffer.rbuf[bybuf->window.next_in];
  if(packet->header.status != 0)
    {
      assert((packet->header.status & NM_IBVERBS_BYBUF_STATUS_DATA) != 0);
      assert(bybuf->recv.buf == NULL);
      const int offset = packet->header.offset;
      assert(offset >= 0);
      assert(offset <= NM_IBVERBS_BYBUF_DATA_SIZE);
      const int packet_size = NM_IBVERBS_BYBUF_DATA_SIZE - offset;
      *p_buffer = &packet->data[offset];
      *p_len = packet_size;
      const int credits = packet->header.ack;
      if(credits)
	nm_atomic_add(&bybuf->window.credits, credits);
      err = NM_ESUCCESS;
    }
  else
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static void nm_ibverbs_bybuf_buf_recv_release(void*_status)
{
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;
  struct nm_ibverbs_bybuf_packet_s*__restrict__ packet = &bybuf->buffer.rbuf[bybuf->window.next_in];
  assert((packet->header.status & NM_IBVERBS_BYBUF_STATUS_DATA) != 0);
  packet->header.ack = 0;
  packet->header.status = 0;
  const int to_ack = nm_atomic_inc(&bybuf->window.to_ack);
  if(to_ack > NM_IBVERBS_BYBUF_CREDITS_THR) 
    {
      bybuf->buffer.sack = nm_ibverbs_bybuf_to_ack(bybuf);
      if(bybuf->buffer.sack > 0)
	{
	  nm_ibverbs_rdma_send(bybuf->cnx,
			       sizeof(uint16_t),
			       (void*)&bybuf->buffer.sack,
			       (void*)&bybuf->buffer.rack,
			       &bybuf->buffer,
			       &bybuf->seg,
			       bybuf->mr,
			       NM_IBVERBS_WRID_ACK);
	}
    }
  bybuf->window.next_in = (bybuf->window.next_in + 1) % NM_IBVERBS_BYBUF_RBUF_NUM;
  nm_ibverbs_rdma_poll(bybuf->cnx);
  bybuf->recv.buf = NULL;
  nm_ibverbs_send_flush(bybuf->cnx, NM_IBVERBS_WRID_ACK);
}

static int nm_ibverbs_bybuf_cancel_recv(void*_status)
{
  int err = -NM_EAGAIN;
  struct nm_ibverbs_bybuf*__restrict__ bybuf = _status;
  if(bybuf->recv.buf != NULL)
    {
      bybuf->recv.buf = NULL;
      err = NM_ESUCCESS;
    }
  return err;
}
