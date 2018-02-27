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

#define NM_IBVERBS_SR_BLOCKSIZE (24 * 1024)
#define NM_IBVERBS_SR_BUFSIZE     NM_IBVERBS_SR_BLOCKSIZE 

/** An "on the wire" packet for 'sr' minidriver */
struct nm_ibverbs_sr_packet_s
{
  char data[NM_IBVERBS_SR_BUFSIZE];
} __attribute__((packed));

PUK_VECT_TYPE(nm_ibverbs_sr_status, struct nm_ibverbs_sr_s*);

/** context for ibverbs sr */
struct nm_ibverbs_sr_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
  struct nm_ibverbs_sr_status_vect_s p_statuses;
  int round_robin;
};

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_sr_s
{
  struct
  {
    struct nm_ibverbs_sr_packet_s sbuf;
    struct nm_ibverbs_sr_packet_s rbuf;
  } buffer;
  
  struct
  {
    nm_len_t chunk_len;       /**< length of chunk to send */
  } recv;
  
  struct
  {
    nm_len_t chunk_len;            /**< length of chunk to send */
  } send;

  struct nm_ibverbs_segment_s seg;   /**< remote segment */
  struct ibv_mr*mr;                  /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx_s*p_cnx;
  puk_context_t context;
};

static void nm_ibverbs_sr_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_sr_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_sr_close(puk_context_t context);
static void nm_ibverbs_sr_connect(void*_status, const void*p_remote_url, size_t url_size);
static void nm_ibverbs_sr_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_sr_buf_send_post(void*_status, nm_len_t len);
static int  nm_ibverbs_sr_send_poll(void*_status);
static int  nm_ibverbs_sr_recv_cancel(void*_status);
static int  nm_ibverbs_sr_recv_poll_any(puk_context_t p_context, void**_status);
static int  nm_ibverbs_sr_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_ibverbs_sr_recv_buf_release(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_sr_minidriver =
  {
    .getprops         = &nm_ibverbs_sr_getprops,
    .init             = &nm_ibverbs_sr_init,
    .close            = &nm_ibverbs_sr_close,
    .connect          = &nm_ibverbs_sr_connect,
    .send_post        = NULL,
    .send_data        = NULL,
    .send_poll        = &nm_ibverbs_sr_send_poll,
    .buf_send_get     = &nm_ibverbs_sr_buf_send_get,
    .buf_send_post    = &nm_ibverbs_sr_buf_send_post,
    .recv_iov_post    = NULL,
    .recv_data_post   = NULL,
    .recv_poll_one    = NULL,
    .recv_poll_any    = NULL, /* &nm_ibverbs_sr_recv_poll_any, */
    .recv_buf_poll    = &nm_ibverbs_sr_recv_buf_poll,
    .recv_buf_release = &nm_ibverbs_sr_recv_buf_release,
    .recv_cancel      = &nm_ibverbs_sr_recv_cancel
  };

static void*nm_ibverbs_sr_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_sr_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_sr_component =
  {
    .instantiate = &nm_ibverbs_sr_instantiate,
    .destroy = &nm_ibverbs_sr_destroy
  };

PADICO_MODULE_COMPONENT(NewMad_ibverbs_sr,
  puk_component_declare("NewMad_ibverbs_sr",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_sr_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_sr_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));


static void* nm_ibverbs_sr_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(context);
 /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_sr_packet_s) % 1024 == 0);
  /* init */
  struct nm_ibverbs_sr_s*p_ibverbs_sr = NULL;
  if(nm_ibverbs_memalign > 0)
    {
      posix_memalign((void**)&p_ibverbs_sr, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_sr_s));
    }
  else
    {
      p_ibverbs_sr = malloc(sizeof(struct nm_ibverbs_sr_s));
    }
  memset(&p_ibverbs_sr->buffer, 0, sizeof(p_ibverbs_sr->buffer));
  p_ibverbs_sr->mr              = NULL;
  p_ibverbs_sr->send.chunk_len  = NM_LEN_UNDEFINED;
  p_ibverbs_sr->recv.chunk_len  = NM_LEN_UNDEFINED;
  p_ibverbs_sr->context         = context;
  p_ibverbs_sr->p_cnx           = NULL;
  nm_ibverbs_sr_status_vect_push_back(&p_ibverbs_sr_context->p_statuses, p_ibverbs_sr);
  return p_ibverbs_sr;
}

static void nm_ibverbs_sr_destroy(void*_status)
{
  struct nm_ibverbs_sr_s*p_ibverbs_sr = _status;
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(p_ibverbs_sr->context);
  nm_ibverbs_sr_status_vect_itor_t itor = nm_ibverbs_sr_status_vect_find(&p_ibverbs_sr_context->p_statuses, p_ibverbs_sr);
  nm_ibverbs_sr_status_vect_erase(&p_ibverbs_sr_context->p_statuses, itor);
  if(p_ibverbs_sr->p_cnx)
    {
      nm_ibverbs_cnx_close(p_ibverbs_sr->p_cnx);
    }
  if(p_ibverbs_sr->mr)
    {
      ibv_dereg_mr(p_ibverbs_sr->mr);
    }
  free(p_ibverbs_sr);
}


static void nm_ibverbs_sr_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  assert(context != NULL);
  if(NM_IBVERBS_SR_BLOCKSIZE > UINT16_MAX)
    {
      NM_FATAL("ibverbs: inconsistency detected in blocksize for 16 bits offsets.");
    }
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = malloc(sizeof(struct nm_ibverbs_sr_context_s));
  p_ibverbs_sr_context->p_hca = nm_ibverbs_hca_from_context(context);
  puk_context_set_status(context, p_ibverbs_sr_context);
  nm_ibverbs_hca_get_profile(p_ibverbs_sr_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data     = 0;
  p_props->capabilities.supports_buf_send = 1;
  p_props->capabilities.supports_buf_recv = 1;
  p_props->capabilities.has_recv_any      = 0;
  p_props->capabilities.max_msg_size      = NM_IBVERBS_SR_BUFSIZE;
}

static void nm_ibverbs_sr_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(context);
  const char*url = NULL;
  p_ibverbs_sr_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  nm_ibverbs_sr_status_vect_init(&p_ibverbs_sr_context->p_statuses);
  p_ibverbs_sr_context->round_robin = 0;
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
}

static void nm_ibverbs_sr_close(puk_context_t context)
{
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(context);
  nm_connector_destroy(p_ibverbs_sr_context->p_connector);
  nm_ibverbs_hca_release(p_ibverbs_sr_context->p_hca);
  puk_context_set_status(context, NULL);
  free(p_ibverbs_sr_context);
}


static void nm_ibverbs_sr_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_sr_s*p_ibverbs_sr = _status;
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(p_ibverbs_sr->context);
  p_ibverbs_sr->p_cnx = nm_ibverbs_cnx_new(p_ibverbs_sr_context->p_hca);
  p_ibverbs_sr->mr = ibv_reg_mr(p_ibverbs_sr_context->p_hca->pd, &p_ibverbs_sr->buffer, sizeof(p_ibverbs_sr->buffer),
                           IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(p_ibverbs_sr->mr == NULL)
    {
      const int err = errno;
      NM_FATAL("Infiniband: sr cannot register MR (errno = %d; %s).\n",
	       err, strerror(err));
    }
  struct nm_ibverbs_segment_s*p_seg = &p_ibverbs_sr->p_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_ibverbs_sr->buffer;
  p_seg->rkey  = p_ibverbs_sr->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_ibverbs_sr->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_ibverbs_sr->p_cnx->local_addr, &p_ibverbs_sr->p_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_ibverbs_sr->seg = p_ibverbs_sr->p_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_ibverbs_sr->p_cnx);
}



/* ** sr I/O ******************************************* */

static void nm_ibverbs_sr_buf_send_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;
  assert(p_ibverbs_sr->send.chunk_len == NM_LEN_UNDEFINED);
  *p_buffer = &p_ibverbs_sr->buffer.sbuf;
  *p_len = NM_IBVERBS_SR_BUFSIZE;
}

static void nm_ibverbs_sr_buf_send_post(void*_status, nm_len_t len)
{
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;
  p_ibverbs_sr->send.chunk_len = len;
  assert(len <= NM_IBVERBS_SR_BUFSIZE);
 
  struct ibv_sge list =
    {
      .addr   = (uintptr_t)&p_ibverbs_sr->buffer.sbuf,
      .length = len,
      .lkey   = p_ibverbs_sr->mr->lkey
    };
  struct ibv_send_wr wr =
    {
      .wr_id      = NM_IBVERBS_WRID_SEND,
      .sg_list    = &list,
      .num_sge    = 1,
      .opcode     = IBV_WR_SEND,
      .send_flags = (len > p_ibverbs_sr->p_cnx->max_inline ? 
                     IBV_SEND_SIGNALED : (IBV_SEND_SIGNALED | IBV_SEND_INLINE)),
      .next       = NULL
    };
  struct ibv_send_wr*bad_wr = NULL;
  int rc = ibv_post_send(p_ibverbs_sr->p_cnx->qp, &wr, &bad_wr);
  if(rc)
    {
      NM_FATAL("ibverbs- post send failed.\n");
    }
}
static int nm_ibverbs_sr_send_poll(void*_status)
{
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;
  assert(p_ibverbs_sr->send.chunk_len != NM_LEN_UNDEFINED);
  struct ibv_wc wc;
  int ne = ibv_poll_cq(p_ibverbs_sr->p_cnx->of_cq, 1, &wc);
  if(ne < 0)
    {
      NM_FATAL("ibverbs- poll out CQ failed.\n");
    }
  if(ne > 0)
    {
      p_ibverbs_sr->send.chunk_len = NM_LEN_UNDEFINED;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int  nm_ibverbs_sr_recv_poll_any(puk_context_t p_context, void**_status)
{
  struct nm_ibverbs_sr_context_s*p_ibverbs_sr_context = puk_context_get_status(p_context);
  nm_ibverbs_sr_status_vect_itor_t i;
  puk_vect_foreach(i, nm_ibverbs_sr_status, &p_ibverbs_sr_context->p_statuses)
    {

    }
  *_status = NULL;
  return -NM_EAGAIN;
}

static int nm_ibverbs_sr_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  int err = -NM_EUNKNOWN;
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;  
  struct nm_ibverbs_sr_packet_s*__restrict__ p_packet = &p_ibverbs_sr->buffer.rbuf;

  if(p_ibverbs_sr->recv.chunk_len == NM_LEN_UNDEFINED)
    {
      struct ibv_sge list =
        {
          .addr   = (uintptr_t)p_packet,
          .length = NM_IBVERBS_SR_BUFSIZE,
          .lkey   = p_ibverbs_sr->mr->lkey
        };
      struct ibv_recv_wr wr =
        {
          .wr_id   = NM_IBVERBS_WRID_RECV,
          .sg_list = &list,
          .num_sge = 1,
        };
      struct ibv_recv_wr*bad_wr = NULL;
      int rc = ibv_post_recv(p_ibverbs_sr->p_cnx->qp, &wr, &bad_wr);
      if(rc)
        {
          NM_FATAL("ibverbs- post recv failed.\n");
        }
      p_ibverbs_sr->recv.chunk_len = NM_IBVERBS_SR_BUFSIZE;
    }
  struct ibv_wc wc;
  int ne = ibv_poll_cq(p_ibverbs_sr->p_cnx->if_cq, 1, &wc);
  if(ne < 0)
    {
      NM_FATAL("ibverbs- poll in CQ failed.\n");
    }
  else if(ne > 0)
    {
      *p_buffer = p_packet;
      *p_len = wc.byte_len;
      err = NM_ESUCCESS;
    }
  else
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static void nm_ibverbs_sr_recv_buf_release(void*_status)
{
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;
  assert(p_ibverbs_sr->recv.chunk_len != NM_LEN_UNDEFINED);
  p_ibverbs_sr->recv.chunk_len = NM_LEN_UNDEFINED;
}

static int nm_ibverbs_sr_recv_cancel(void*_status)
{
  struct nm_ibverbs_sr_s*__restrict__ p_ibverbs_sr = _status;
  return -NM_ENOTIMPL;
}
