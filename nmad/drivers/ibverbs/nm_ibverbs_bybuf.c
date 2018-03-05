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

PUK_VECT_TYPE(nm_ibverbs_bybuf_status, struct nm_ibverbs_bybuf_s*);

/** context for ibverbs bybuf */
struct nm_ibverbs_bybuf_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
  struct nm_ibverbs_bybuf_status_vect_s p_statuses;
  int round_robin;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bybuf_s
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
    nm_len_t chunk_len;       /**< length of chunk to send */
  } recv;
  
  struct
  {
    nm_len_t chunk_len;            /**< length of chunk to send */
    nm_len_t done;                 /**< total amount of data sent */
  } send;

  struct nm_ibverbs_segment_s seg;   /**< remote segment */
  struct ibv_mr*mr;                  /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx_s*p_cnx;
  puk_context_t context;
};

static void nm_ibverbs_bybuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_bybuf_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_bybuf_close(puk_context_t context);
static void nm_ibverbs_bybuf_connect(void*_status, const void*p_remote_url, size_t url_size);
static void nm_ibverbs_bybuf_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_bybuf_send_buf_post(void*_status, nm_len_t len);
static int  nm_ibverbs_bybuf_send_poll(void*_status);
static int  nm_ibverbs_bybuf_recv_cancel(void*_status);
static int  nm_ibverbs_bybuf_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_ibverbs_bybuf_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_bybuf_recv_buf_release(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_bybuf_minidriver =
  {
    .getprops         = &nm_ibverbs_bybuf_getprops,
    .init             = &nm_ibverbs_bybuf_init,
    .close            = &nm_ibverbs_bybuf_close,
    .connect          = &nm_ibverbs_bybuf_connect,
    .send_post        = NULL,
    .send_data        = NULL,
    .send_poll        = &nm_ibverbs_bybuf_send_poll,
    .send_buf_get     = &nm_ibverbs_bybuf_send_buf_get,
    .send_buf_post    = &nm_ibverbs_bybuf_send_buf_post,
    .recv_iov_post    = NULL,
    .recv_data_post   = NULL,
    .recv_poll_one    = NULL,
    .recv_poll_any    = &nm_ibverbs_bybuf_recv_poll_any,
    .recv_buf_poll    = &nm_ibverbs_bybuf_recv_buf_poll,
    .recv_buf_release = &nm_ibverbs_bybuf_recv_buf_release,
    .recv_cancel      = &nm_ibverbs_bybuf_recv_cancel
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
static inline int nm_ibverbs_bybuf_to_ack(struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf)
{
  const int to_ack = p_bybuf->window.to_ack;
  if(to_ack > 0)
    {
      return nm_atomic_compare_and_swap(&p_bybuf->window.to_ack, to_ack, 0) ? to_ack : 0;
    }
  else
    {
      return 0;
    }
}


static void* nm_ibverbs_bybuf_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(context);
 /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_bybuf_packet_s) % 1024 == 0);
  assert(NM_IBVERBS_BYBUF_CREDITS_THR > NM_IBVERBS_BYBUF_RBUF_NUM / 2);
  /* init */
  struct nm_ibverbs_bybuf_s*p_bybuf = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&p_bybuf, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_bybuf_s));
    }
  else
    {
      p_bybuf = malloc(sizeof(struct nm_ibverbs_bybuf_s));
    }
  memset(&p_bybuf->buffer, 0, sizeof(p_bybuf->buffer));
  p_bybuf->window.next_out = 1;
  p_bybuf->window.next_in  = 1;
  p_bybuf->window.credits  = NM_IBVERBS_BYBUF_RBUF_NUM;
  p_bybuf->window.to_ack   = 0;
  p_bybuf->mr              = NULL;
  p_bybuf->send.done       = NM_LEN_UNDEFINED;
  p_bybuf->send.chunk_len  = NM_LEN_UNDEFINED;
  p_bybuf->context         = context;
  p_bybuf->p_cnx           = NULL;
  nm_ibverbs_bybuf_status_vect_push_back(&p_bybuf_context->p_statuses, p_bybuf);
  return p_bybuf;
}

static void nm_ibverbs_bybuf_destroy(void*_status)
{
  struct nm_ibverbs_bybuf_s*p_bybuf = _status;
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(p_bybuf->context);
  nm_ibverbs_bybuf_status_vect_itor_t itor = nm_ibverbs_bybuf_status_vect_find(&p_bybuf_context->p_statuses, p_bybuf);
  nm_ibverbs_bybuf_status_vect_erase(&p_bybuf_context->p_statuses, itor);
  if(p_bybuf->p_cnx)
    {
      nm_ibverbs_cnx_close(p_bybuf->p_cnx);
    }
  if(p_bybuf->mr)
    {
      ibv_dereg_mr(p_bybuf->mr);
    }
  free(p_bybuf);
}


static void nm_ibverbs_bybuf_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  assert(context != NULL);
  if(NM_IBVERBS_BYBUF_BLOCKSIZE > UINT16_MAX)
    {
      NM_FATAL("ibverbs: inconsistency detected in blocksize for 16 bits offsets.");
    }
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = malloc(sizeof(struct nm_ibverbs_bybuf_context_s));
  p_bybuf_context->p_hca = nm_ibverbs_hca_from_context(context);
  puk_context_set_status(context, p_bybuf_context);
  nm_ibverbs_hca_get_profile(p_bybuf_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data     = 0;
  p_props->capabilities.supports_buf_send = 1;
  p_props->capabilities.supports_buf_recv = 1;
  p_props->capabilities.has_recv_any      = 1;
  p_props->capabilities.max_msg_size      = NM_IBVERBS_BYBUF_DATA_SIZE;
}

static void nm_ibverbs_bybuf_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(context);
  const char*url = NULL;
  p_bybuf_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  nm_ibverbs_bybuf_status_vect_init(&p_bybuf_context->p_statuses);
  p_bybuf_context->round_robin = 0;
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
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
  struct nm_ibverbs_bybuf_s*p_bybuf = _status;
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(p_bybuf->context);
  p_bybuf->p_cnx = nm_ibverbs_cnx_new(p_bybuf_context->p_hca);
  p_bybuf->mr = ibv_reg_mr(p_bybuf_context->p_hca->pd, &p_bybuf->buffer, sizeof(p_bybuf->buffer),
                           IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(p_bybuf->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: bybuf cannot register MR (errno = %d; %s).\n",
	       err, strerror(err));
    }
  struct nm_ibverbs_segment_s*p_seg = &p_bybuf->p_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_bybuf->buffer;
  p_seg->rkey  = p_bybuf->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_bybuf->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_bybuf->p_cnx->local_addr, &p_bybuf->p_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_bybuf->seg = p_bybuf->p_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_bybuf->p_cnx);
}



/* ** bybuf I/O ******************************************* */

static void nm_ibverbs_bybuf_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;
  assert(p_bybuf->send.done == NM_LEN_UNDEFINED);
  assert(p_bybuf->send.chunk_len == NM_LEN_UNDEFINED);
  *p_buffer = &p_bybuf->buffer.sbuf;
  *p_len = NM_IBVERBS_BYBUF_DATA_SIZE;
  p_bybuf->send.done = 0;
}

static void nm_ibverbs_bybuf_send_buf_post(void*_status, nm_len_t len)
{
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;
  p_bybuf->send.chunk_len = len;
  assert(len <= NM_IBVERBS_BYBUF_DATA_SIZE);
}
static int nm_ibverbs_bybuf_send_poll(void*_status)
{
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;

  if(p_bybuf->send.done == p_bybuf->send.chunk_len)
    {
      /* already sent; just poll */
      goto fastpoll;
    }
  
  /* 1- check credits */
  const int rack = p_bybuf->buffer.rack;
  if(rack)
    {
      nm_atomic_add(&p_bybuf->window.credits, rack);
      p_bybuf->buffer.rack = 0;
    }
  if(p_bybuf->window.credits <= 1)
    {
      goto wouldblock;
    }

  /* 2- check window availability */
  assert(p_bybuf->p_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] == 0);

  /* 3- prepare and send packet */
  const nm_len_t offset0 = NM_IBVERBS_BYBUF_DATA_SIZE - p_bybuf->send.chunk_len;
  const nm_len_t padding = (nm_ibverbs_alignment > 0 &&  p_bybuf->send.chunk_len >= 1024) ?
    (offset0 % nm_ibverbs_alignment) : 0 ;
  assert((nm_ibverbs_alignment == 0) || (padding < nm_ibverbs_alignment));
  const nm_len_t offset = offset0 - padding;
  assert(offset >= 0);
  assert(offset <= NM_IBVERBS_BYBUF_DATA_SIZE);
  void*p_packet = &p_bybuf->buffer.sbuf;
  struct nm_ibverbs_bybuf_header_s*p_header = p_packet + p_bybuf->send.chunk_len + padding;
  assert(p_bybuf->send.chunk_len <= NM_IBVERBS_BYBUF_DATA_SIZE);
  p_header->offset   = offset;
  p_header->ack      = nm_ibverbs_bybuf_to_ack(p_bybuf);
  p_header->status   = NM_IBVERBS_BYBUF_STATUS_DATA;
  p_bybuf->send.done = p_bybuf->send.chunk_len;
  nm_ibverbs_rdma_send(p_bybuf->p_cnx,
		       sizeof(struct nm_ibverbs_bybuf_header_s) + p_bybuf->send.chunk_len + padding,
		       p_packet,
		       &p_bybuf->buffer.rbuf[p_bybuf->window.next_out].data[offset],
		       &p_bybuf->buffer,
		       &p_bybuf->seg,
		       p_bybuf->mr,
		       NM_IBVERBS_WRID_RDMA);
  p_bybuf->window.next_out = (p_bybuf->window.next_out + 1) % NM_IBVERBS_BYBUF_RBUF_NUM;
  nm_atomic_dec(&p_bybuf->window.credits);
 fastpoll:
  nm_ibverbs_rdma_poll(p_bybuf->p_cnx);
  if(p_bybuf->p_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  assert(p_bybuf->send.done <= p_bybuf->send.chunk_len);
  p_bybuf->send.done = NM_LEN_UNDEFINED;
  p_bybuf->send.chunk_len = NM_LEN_UNDEFINED;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static int  nm_ibverbs_bybuf_recv_poll_any(puk_context_t p_context, void**_status)
{
  struct nm_ibverbs_bybuf_context_s*p_bybuf_context = puk_context_get_status(p_context);
  nm_ibverbs_bybuf_status_vect_itor_t i;
  puk_vect_foreach(i, nm_ibverbs_bybuf_status, &p_bybuf_context->p_statuses)
    {
      if((*i)->buffer.rbuf[(*i)->window.next_in].header.status)
        {
          *_status = *i;
          return NM_ESUCCESS;
        }
    }
  *_status = NULL;
  return -NM_EAGAIN;
}

static int nm_ibverbs_bybuf_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  int err = -NM_EUNKNOWN;
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;
  struct nm_ibverbs_bybuf_packet_s*__restrict__ p_packet = &p_bybuf->buffer.rbuf[p_bybuf->window.next_in];
  if(p_packet->header.status != 0)
    {
      assert((p_packet->header.status & NM_IBVERBS_BYBUF_STATUS_DATA) != 0);
      const int offset = p_packet->header.offset;
      assert(offset >= 0);
      assert(offset <= NM_IBVERBS_BYBUF_DATA_SIZE);
      const int packet_size = NM_IBVERBS_BYBUF_DATA_SIZE - offset;
      *p_buffer = &p_packet->data[offset];
      *p_len = packet_size;
      const int credits = p_packet->header.ack;
      if(credits)
	nm_atomic_add(&p_bybuf->window.credits, credits);
      err = NM_ESUCCESS;
    }
  else
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static void nm_ibverbs_bybuf_recv_buf_release(void*_status)
{
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;
  struct nm_ibverbs_bybuf_packet_s*__restrict__ p_packet = &p_bybuf->buffer.rbuf[p_bybuf->window.next_in];
  assert((p_packet->header.status & NM_IBVERBS_BYBUF_STATUS_DATA) != 0);
  p_packet->header.ack = 0;
  p_packet->header.status = 0;
  const int to_ack = nm_atomic_inc(&p_bybuf->window.to_ack);
  if(to_ack > NM_IBVERBS_BYBUF_CREDITS_THR) 
    {
      p_bybuf->buffer.sack = nm_ibverbs_bybuf_to_ack(p_bybuf);
      if(p_bybuf->buffer.sack > 0)
	{
	  nm_ibverbs_rdma_send(p_bybuf->p_cnx,
			       sizeof(uint16_t),
			       (void*)&p_bybuf->buffer.sack,
			       (void*)&p_bybuf->buffer.rack,
			       &p_bybuf->buffer,
			       &p_bybuf->seg,
			       p_bybuf->mr,
			       NM_IBVERBS_WRID_ACK);
	}
    }
  p_bybuf->window.next_in = (p_bybuf->window.next_in + 1) % NM_IBVERBS_BYBUF_RBUF_NUM;
  nm_ibverbs_rdma_poll(p_bybuf->p_cnx);
  nm_ibverbs_send_flush(p_bybuf->p_cnx, NM_IBVERBS_WRID_ACK);
}

static int nm_ibverbs_bybuf_recv_cancel(void*_status)
{
  struct nm_ibverbs_bybuf_s*__restrict__ p_bybuf = _status;
  return NM_ESUCCESS;
}
