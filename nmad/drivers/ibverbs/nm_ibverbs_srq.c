/*
 * NewMadeleine
 * Copyright (C) 2006-2018 (see AUTHORS file)
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


/* *** method: 'sr' ************************************ */

#define NM_IBVERBS_SRQ_BLOCKSIZE (48 * 1024)
#define NM_IBVERBS_SRQ_BUFSIZE   ( NM_IBVERBS_SRQ_BLOCKSIZE - sizeof(struct nm_ibverbs_srq_header_s) )

/** header for each packet */
struct nm_ibverbs_srq_header_s
{
  uint32_t rank; /**< the rank this packet comes from */
} __attribute__((packed));

/** An "on the wire" packet for 'sr' minidriver */
struct nm_ibverbs_srq_packet_s
{
  struct nm_ibverbs_srq_header_s header;
  char data[NM_IBVERBS_SRQ_BUFSIZE];
} __attribute__((packed));

struct nm_ibverbs_srq_addr_s
{
  struct nm_ibverbs_cnx_addr_s cnx_addr;   /**< address for connection (LID/QPN + seg) */
  struct nm_ibverbs_segment_s context_seg; /**< segment for the buffer in context (SRQ) [ @note currently unused on remote side ] */
  uint32_t rank;                           /**< rank of this conn in statuses vect */
} __attribute__((packed));

PUK_VECT_TYPE(nm_ibverbs_srq_status, struct nm_ibverbs_srq_s*);

/** context for ibverbs sr */
struct nm_ibverbs_srq_context_s
{
  struct nm_ibverbs_srq_packet_s rbuf; /**< buffer for SRQ */
  struct ibv_mr*mr;                   /**< MR used for rbuf in context */
  struct nm_ibverbs_segment_s seg;    /**< MR segment for local SRQ buffer */
  struct nm_ibverbs_context_s*p_ibverbs_context;
  struct nm_connector_s*p_connector;
  struct nm_ibverbs_srq_status_vect_s p_statuses;
  int round_robin;
  int srq_posted;
  int running;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_srq_s
{
  struct
  {
    struct nm_ibverbs_srq_packet_s sbuf;
  } buffer;
  
  struct
  {
    nm_len_t chunk_len;              /**< length of chunk to send */
  } recv;
  
  struct
  {
    nm_len_t chunk_len;              /**< length of chunk to send */
  } send;

  struct ibv_mr*mr;                  /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx_s*p_cnx;
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context;
  puk_context_t context;
  uint32_t lrank;                    /**< rank of this connection in local statuses vect */
  uint32_t rrank;                    /**< rank of this connection in remote statuses vect */
};

static void nm_ibverbs_srq_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_srq_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_srq_close(puk_context_t context);
static void nm_ibverbs_srq_connect(void*_status, const void*p_remote_url, size_t url_size);
static void nm_ibverbs_srq_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_srq_send_buf_post(void*_status, nm_len_t len);
static int  nm_ibverbs_srq_send_poll(void*_status);
static int  nm_ibverbs_srq_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_ibverbs_srq_recv_wait_any(puk_context_t p_context, void**_status);
static int  nm_ibverbs_srq_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_srq_recv_buf_release(void*_status);
static int  nm_ibverbs_srq_recv_cancel_any(puk_context_t p_context);

static const struct nm_minidriver_iface_s nm_ibverbs_srq_minidriver =
  {
    .getprops         = &nm_ibverbs_srq_getprops,
    .init             = &nm_ibverbs_srq_init,
    .close            = &nm_ibverbs_srq_close,
    .connect          = &nm_ibverbs_srq_connect,
    .send_iov_post    = NULL,
    .send_data_post   = NULL,
    .send_poll        = &nm_ibverbs_srq_send_poll,
    .send_buf_get     = &nm_ibverbs_srq_send_buf_get,
    .send_buf_post    = &nm_ibverbs_srq_send_buf_post,
    .recv_iov_post    = NULL,
    .recv_data_post   = NULL,
    .recv_poll_one    = NULL,
    .recv_poll_any    = &nm_ibverbs_srq_recv_poll_any,
    .recv_wait_any    = &nm_ibverbs_srq_recv_wait_any,
    .recv_buf_poll    = &nm_ibverbs_srq_recv_buf_poll,
    .recv_buf_release = &nm_ibverbs_srq_recv_buf_release,
    .recv_cancel      = NULL,
    .recv_cancel_any  = &nm_ibverbs_srq_recv_cancel_any
  };

static void*nm_ibverbs_srq_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_srq_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_srq_component =
  {
    .instantiate = &nm_ibverbs_srq_instantiate,
    .destroy = &nm_ibverbs_srq_destroy
  };

PADICO_MODULE_COMPONENT(NewMad_ibverbs_srq,
  puk_component_declare("NewMad_ibverbs_srq",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_srq_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_srq_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));


static void* nm_ibverbs_srq_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(context);
 /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_srq_packet_s) % 1024 == 0);
  /* init */
  struct nm_ibverbs_srq_s*p_ibverbs_srq = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&p_ibverbs_srq, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_srq_s));
    }
  else
    {
      p_ibverbs_srq = malloc(sizeof(struct nm_ibverbs_srq_s));
    }
  memset(&p_ibverbs_srq->buffer, 0, sizeof(p_ibverbs_srq->buffer));
  p_ibverbs_srq->mr              = NULL;
  p_ibverbs_srq->send.chunk_len  = NM_LEN_UNDEFINED;
  p_ibverbs_srq->recv.chunk_len  = NM_LEN_UNDEFINED;
  p_ibverbs_srq->context         = context;
  p_ibverbs_srq->p_cnx           = NULL;
  p_ibverbs_srq->p_ibverbs_srq_context = p_ibverbs_srq_context;
  nm_ibverbs_srq_status_vect_push_back(&p_ibverbs_srq_context->p_statuses, p_ibverbs_srq);
  p_ibverbs_srq->lrank = nm_ibverbs_srq_status_vect_size(&p_ibverbs_srq_context->p_statuses) - 1;
  return p_ibverbs_srq;
}

static void nm_ibverbs_srq_destroy(void*_status)
{
  struct nm_ibverbs_srq_s*p_ibverbs_srq = _status;
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(p_ibverbs_srq->context);
  nm_ibverbs_srq_status_vect_itor_t itor = nm_ibverbs_srq_status_vect_find(&p_ibverbs_srq_context->p_statuses, p_ibverbs_srq);
  nm_ibverbs_srq_status_vect_erase(&p_ibverbs_srq_context->p_statuses, itor);
  if(p_ibverbs_srq->p_cnx)
    {
      nm_ibverbs_cnx_close(p_ibverbs_srq->p_cnx);
    }
  if(p_ibverbs_srq->mr)
    {
      ibv_dereg_mr(p_ibverbs_srq->mr);
    }
  free(p_ibverbs_srq);
}


static void nm_ibverbs_srq_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  assert(context != NULL);
  if(NM_IBVERBS_SRQ_BLOCKSIZE > UINT16_MAX)
    {
      NM_FATAL("ibverbs- inconsistency detected in blocksize for 16 bits offsets.");
    }
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = malloc(sizeof(struct nm_ibverbs_srq_context_s));

  /* force SRQ for now */
  puk_context_putattr(context, "ibv_srq", "1");
  
  p_ibverbs_srq_context->p_ibverbs_context = nm_ibverbs_context_new(context);
  puk_context_set_status(context, p_ibverbs_srq_context);
  nm_ibverbs_hca_get_profile(p_ibverbs_srq_context->p_ibverbs_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data     = 0;
  p_props->capabilities.supports_buf_send = 1;
  p_props->capabilities.supports_buf_recv = 1;
  p_props->capabilities.supports_wait_any = 1;
  p_props->capabilities.prefers_wait_any  = 0;
  p_props->capabilities.has_recv_any      = 1;
  p_props->capabilities.max_msg_size      = NM_IBVERBS_SRQ_BUFSIZE;
}

static void nm_ibverbs_srq_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(context);
  const char*url = NULL;
  if(!p_ibverbs_srq_context->p_ibverbs_context->ib_opts.use_srq)
    {
      NM_FATAL("ibverbs- board does not provide SRQ. Cannot use sr driver.\n");
    }
  p_ibverbs_srq_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_srq_addr_s), &url);
  nm_ibverbs_srq_status_vect_init(&p_ibverbs_srq_context->p_statuses);
  p_ibverbs_srq_context->round_robin = 0;
  p_ibverbs_srq_context->srq_posted = 0;
  p_ibverbs_srq_context->running = 1;
  p_ibverbs_srq_context->mr = nm_ibverbs_reg_mr(p_ibverbs_srq_context->p_ibverbs_context->p_hca,
                                               &p_ibverbs_srq_context->rbuf, sizeof(p_ibverbs_srq_context->rbuf),
                                               &p_ibverbs_srq_context->seg);
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
}

static void nm_ibverbs_srq_close(puk_context_t context)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(context);
  nm_connector_destroy(p_ibverbs_srq_context->p_connector);
  nm_ibverbs_context_delete(p_ibverbs_srq_context->p_ibverbs_context);
  puk_context_set_status(context, NULL);
  free(p_ibverbs_srq_context);
}


static void nm_ibverbs_srq_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_srq_s*p_ibverbs_srq = _status;
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(p_ibverbs_srq->context);
  p_ibverbs_srq->p_cnx = nm_ibverbs_cnx_create(p_ibverbs_srq_context->p_ibverbs_context);
  p_ibverbs_srq->mr = nm_ibverbs_reg_mr(p_ibverbs_srq_context->p_ibverbs_context->p_hca,
                                       &p_ibverbs_srq->buffer, sizeof(p_ibverbs_srq->buffer),
                                       &p_ibverbs_srq->p_cnx->local_addr.segment);
  if(p_ibverbs_srq->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("ibverbs- sr cannot register MR (errno = %d; %s).\n", err, strerror(err));
    }
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_ibverbs_srq->context, "local_url");
  struct nm_ibverbs_srq_addr_s remote_addr;
  struct nm_ibverbs_srq_addr_s local_addr =
    {
      .cnx_addr    = p_ibverbs_srq->p_cnx->local_addr,
      .context_seg = p_ibverbs_srq_context->seg,
      .rank        = p_ibverbs_srq->lrank
    };  
  int rc = nm_connector_exchange(local_url, remote_url, &local_addr, &remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs- timeout in address exchange.\n");
    }
  p_ibverbs_srq->p_cnx->remote_addr = remote_addr.cnx_addr;
  p_ibverbs_srq->rrank = remote_addr.rank;
  nm_ibverbs_cnx_connect(p_ibverbs_srq->p_cnx);
}



/* ** sr I/O ******************************************* */

static void nm_ibverbs_srq_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_ibverbs_srq_s*__restrict__ p_ibverbs_srq = _status;
  struct nm_ibverbs_srq_packet_s*__restrict__ p_packet = &p_ibverbs_srq->buffer.sbuf;
  assert(p_ibverbs_srq->send.chunk_len == NM_LEN_UNDEFINED);
  p_packet->header.rank = p_ibverbs_srq->rrank;
  *p_buffer = p_packet->data;
  *p_len = NM_IBVERBS_SRQ_BUFSIZE;
}

static void nm_ibverbs_srq_send_buf_post(void*_status, nm_len_t len)
{
  struct nm_ibverbs_srq_s*__restrict__ p_ibverbs_srq = _status;
  p_ibverbs_srq->send.chunk_len = len;
  assert(len <= NM_IBVERBS_SRQ_BUFSIZE);
 
  struct ibv_sge list =
    {
      .addr   = (uintptr_t)&p_ibverbs_srq->buffer.sbuf,
      .length = len + sizeof(struct nm_ibverbs_srq_header_s),
      .lkey   = p_ibverbs_srq->mr->lkey
    };
  struct ibv_send_wr wr =
    {
      .wr_id      = NM_IBVERBS_WRID_SEND,
      .sg_list    = &list,
      .num_sge    = 1,
      .opcode     = IBV_WR_SEND,
      .send_flags = (len > p_ibverbs_srq->p_cnx->max_inline ? 
                     IBV_SEND_SIGNALED : (IBV_SEND_SIGNALED | IBV_SEND_INLINE)),
      .next       = NULL
    };
  struct ibv_send_wr*bad_wr = NULL;
  int rc = ibv_post_send(p_ibverbs_srq->p_cnx->qp, &wr, &bad_wr);
  if(rc)
    {
      NM_FATAL("ibverbs- post send failed.\n");
    }
}

static int nm_ibverbs_srq_send_poll(void*_status)
{
  struct nm_ibverbs_srq_s*__restrict__ p_ibverbs_srq = _status;
  assert(p_ibverbs_srq->send.chunk_len != NM_LEN_UNDEFINED);
  struct ibv_wc wc;
  int ne = ibv_poll_cq(p_ibverbs_srq->p_cnx->of_cq, 1, &wc);
  if(ne < 0)
    {
      NM_FATAL("ibverbs- poll out CQ failed.\n");
    }
  if(ne > 0)
    {
      p_ibverbs_srq->send.chunk_len = NM_LEN_UNDEFINED;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static inline void nm_ibverbs_srq_refill(struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context)
{
  if(!p_ibverbs_srq_context->srq_posted)
    {
      struct ibv_sge list =
        {
          .addr   = (uintptr_t)&p_ibverbs_srq_context->rbuf,
          .length = NM_IBVERBS_SRQ_BUFSIZE,
          .lkey   = p_ibverbs_srq_context->mr->lkey
        };
      struct ibv_recv_wr wr =
        {
          .wr_id   = NM_IBVERBS_WRID_RECV,
          .sg_list = &list,
          .num_sge = 1,
        };
      struct ibv_recv_wr*bad_wr = NULL;
      int rc = ibv_post_srq_recv(p_ibverbs_srq_context->p_ibverbs_context->p_srq, &wr, &bad_wr);
      if(rc)
        {
          NM_FATAL("ibverbs- ibv_post_srq_recv failed rc=%d (%s).\n", rc, strerror(rc));
        }
      p_ibverbs_srq_context->srq_posted = 1;
    }
}

static int nm_ibverbs_srq_recv_poll_any(puk_context_t p_context, void**_status)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(p_context);
  struct nm_ibverbs_srq_packet_s*__restrict__ p_packet = &p_ibverbs_srq_context->rbuf;
  nm_ibverbs_srq_refill(p_ibverbs_srq_context);
  struct ibv_wc wc;
  int ne = ibv_poll_cq(p_ibverbs_srq_context->p_ibverbs_context->srq_cq, 1, &wc);
  if(ne < 0)
    {
      NM_FATAL("ibverbs- poll srq CQ failed (status=%d; %s).\n", wc.status, nm_ibverbs_status_strings[wc.status]);
    }
  else if(ne > 0)
    {
      struct nm_ibverbs_srq_s*p_ibverbs_srq = nm_ibverbs_srq_status_vect_at(&p_ibverbs_srq_context->p_statuses, p_packet->header.rank);
      p_ibverbs_srq->recv.chunk_len = wc.byte_len - sizeof(struct nm_ibverbs_srq_header_s);
      *_status = p_ibverbs_srq;
      p_ibverbs_srq_context->srq_posted = 0;
      return NM_ESUCCESS;
    }
  *_status = NULL;
  return -NM_EAGAIN;
}

static int nm_ibverbs_srq_recv_wait_any(puk_context_t p_context, void**_status)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(p_context);
  int rc = -NM_EUNKNOWN;
  do
    {
      if(!p_ibverbs_srq_context->running)
        return -NM_ECANCELED;
      nm_ibverbs_srq_refill(p_ibverbs_srq_context);
      nm_ibverbs_context_wait_event(p_ibverbs_srq_context->p_ibverbs_context);
      rc = nm_ibverbs_srq_recv_poll_any(p_context, _status);
    }
  while(rc != NM_ESUCCESS);
  return NM_ESUCCESS;
}

static int nm_ibverbs_srq_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_ibverbs_srq_s*__restrict__ p_ibverbs_srq = _status;
  if(p_ibverbs_srq->recv.chunk_len != NM_LEN_UNDEFINED)
    {
      struct nm_ibverbs_srq_packet_s*__restrict__ p_packet = &p_ibverbs_srq->p_ibverbs_srq_context->rbuf;
      *p_buffer = p_packet->data;
      *p_len = p_ibverbs_srq->recv.chunk_len;
      return NM_ESUCCESS;
    }
  else
    {
      NM_WARN("ibverbs- EAGAIN in recv_buf_poll() with SRQ enabled. It should not happen.\n");
      return -NM_EAGAIN;
    }
}

static void nm_ibverbs_srq_recv_buf_release(void*_status)
{
  struct nm_ibverbs_srq_s*__restrict__ p_ibverbs_srq = _status;
  assert(p_ibverbs_srq->recv.chunk_len != NM_LEN_UNDEFINED);
  p_ibverbs_srq->recv.chunk_len = NM_LEN_UNDEFINED;
  nm_ibverbs_srq_refill(p_ibverbs_srq->p_ibverbs_srq_context);
}

static int  nm_ibverbs_srq_recv_cancel_any(puk_context_t p_context)
{
  struct nm_ibverbs_srq_context_s*p_ibverbs_srq_context = puk_context_get_status(p_context);
  p_ibverbs_srq_context->running = 0;
  return NM_ESUCCESS;
}
