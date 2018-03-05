/*
 * NewMadeleine
 * Copyright (C) 2014-2017 (see AUTHORS file)
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

/** @file Shared-memory driver with 'minidriver' interface.
 */

#include <Padico/Puk.h>
#include <Padico/Module.h>
#include <Padico/Shm.h>
#include <nm_minidriver.h>

/* ********************************************************* */

static void*nm_minidriver_shm_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_minidriver_shm_destroy(void*);

static const struct puk_component_driver_s nm_minidriver_shm_component =
  {
    .instantiate = &nm_minidriver_shm_instantiate,
    .destroy = &nm_minidriver_shm_destroy
  };

static void nm_minidriver_shm_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_minidriver_shm_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_minidriver_shm_close(puk_context_t context);
static void nm_minidriver_shm_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_minidriver_shm_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static void nm_minidriver_shm_send_post(void*_status, const struct iovec*v, int n);
static int  nm_minidriver_shm_send_poll(void*_status);
static void nm_minidriver_shm_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_minidriver_shm_send_buf_post(void*_status, nm_len_t len);
static void nm_minidriver_shm_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static void nm_minidriver_shm_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_minidriver_shm_recv_poll_one(void*_status);
static int  nm_minidriver_shm_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len);
static void nm_minidriver_shm_recv_buf_release(void*_status);
static int  nm_minidriver_shm_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s nm_minidriver_shm_minidriver =
  {
    .getprops         = &nm_minidriver_shm_getprops,
    .init             = &nm_minidriver_shm_init,
    .close            = &nm_minidriver_shm_close,
    .connect          = &nm_minidriver_shm_connect,
    .send_data        = &nm_minidriver_shm_send_data,
    .send_post        = &nm_minidriver_shm_send_post,
    .send_poll        = &nm_minidriver_shm_send_poll,
    .send_buf_get     = &nm_minidriver_shm_send_buf_get,
    .send_buf_post    = &nm_minidriver_shm_send_buf_post,
    .recv_data_post   = &nm_minidriver_shm_recv_data_post,
    .recv_iov_post    = &nm_minidriver_shm_recv_iov_post,
    .recv_poll_one    = &nm_minidriver_shm_recv_poll_one,
    .recv_cancel      = &nm_minidriver_shm_recv_cancel,
    .recv_buf_poll    = &nm_minidriver_shm_recv_buf_poll,
    .recv_buf_release = &nm_minidriver_shm_recv_buf_release
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_shm,
  puk_component_declare("Minidriver_shm",
			puk_component_provides("PadicoComponent", "component", &nm_minidriver_shm_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_minidriver_shm_minidriver)));

/* ********************************************************* */


/** 'shm' per-context data.
 */
struct nm_minidriver_shm_context_s
{
  struct nm_minidriver_shm_s*conns[PADICO_SHM_NUMNODES]; /**< connections in the context, indexed by destination rank */
  struct padico_shm_s*shm;                 /**< shm segment */
  char*url;
};

/** 'shm' per-instance status.
 */
struct nm_minidriver_shm_s
{
  struct padico_shm_node_s*dest;           /**< destination node */
  int dest_rank;                           /**< destination rank in shm directory */
  struct nm_minidriver_shm_context_s*p_shm_context;
  struct
  {
    nm_data_slicer_t slicer;
    nm_len_t chunk_len;
    struct iovec*v;
    int n;
    int block_num;
  } recv;
  struct
  {
    nm_data_slicer_t slicer;
    int block_num;
    size_t len;
  } send;
};


/* ********************************************************* */

static void*nm_minidriver_shm_instantiate(puk_instance_t instance, puk_context_t context)
{  
  struct nm_minidriver_shm_s*p_status = padico_malloc(sizeof(struct nm_minidriver_shm_s));
  p_status->dest = NULL;
  p_status->dest_rank = -1;
  p_status->p_shm_context = puk_context_get_status(context);
  assert(p_status->p_shm_context->shm->seg != NULL);
  p_status->send.block_num = -1;
  return p_status;

}

static void nm_minidriver_shm_destroy(void*_status)
{
  struct nm_minidriver_shm_s*p_status = _status;
  if(p_status->dest_rank > -1)
    p_status->p_shm_context->conns[p_status->dest_rank] = NULL;
  padico_free(_status);
}

/* ********************************************************* */

static void nm_minidriver_shm_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  p_props->profile.latency = 200;
  p_props->profile.bandwidth = 10000;
  p_props->capabilities.supports_data = 1;
  p_props->capabilities.supports_buf_send = 1;
  p_props->capabilities.supports_buf_recv = 1;
  p_props->capabilities.max_msg_size = PADICO_SHM_SHORT_PAYLOAD;
}

static void nm_minidriver_shm_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_minidriver_shm_context_s*p_shm_context = padico_malloc(sizeof(struct nm_minidriver_shm_context_s));
  padico_string_t segment_name = padico_string_new();
  padico_string_printf(segment_name, "minidriver-shm-id%s", context->id);
  p_shm_context->shm = padico_shm_init(padico_string_get(segment_name));
  int i;
  for(i = 0; i < PADICO_SHM_NUMNODES; i++)
    {
      p_shm_context->conns[i] = NULL;
    }
  puk_context_set_status(context, p_shm_context);
  p_shm_context->url = padico_strdup((char*)padico_na_node_getuuid(padico_na_getlocalnode()));
  padico_string_delete(segment_name);
  *drv_url = p_shm_context->url;
  *url_size = strlen(p_shm_context->url);
}

static void nm_minidriver_shm_close(puk_context_t context)
{
  struct nm_minidriver_shm_context_s*p_shm_context = puk_context_get_status(context);
  padico_shm_close(p_shm_context->shm);
  puk_context_set_status(context, NULL);
  padico_free(p_shm_context->url);
  padico_free(p_shm_context);
}

static void nm_minidriver_shm_connect(void*_status, const void*remote_url, size_t url_size)
{ 
  struct nm_minidriver_shm_s*p_status = _status;
  const char*remote_uuid = remote_url;
  padico_na_node_t remote_node = padico_na_getnodebyuuid((padico_topo_uuid_t)remote_uuid);
  assert(p_status->p_shm_context->shm->seg != NULL);
  p_status->dest = padico_shm_directory_node_lookup(p_status->p_shm_context->shm, remote_node);
  if(p_status->dest == NULL)
    {
      padico_fatal("minidriver-shm: FATAL- cannot find peer node %s in shm directory.\n", remote_uuid);
    }
  else
    {
      p_status->dest_rank = p_status->dest->rank;
      assert(p_status->dest_rank >= 0);
      assert(p_status->dest_rank < PADICO_SHM_NUMNODES);
      p_status->p_shm_context->conns[p_status->dest_rank] = p_status;
    }
}

static void nm_minidriver_shm_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_minidriver_shm_s*p_status = _status;
  const int block_num = padico_shm_block_alloc(p_status->p_shm_context->shm);
  void*base = padico_shm_short_get_ptr(p_status->p_shm_context->shm, block_num);
  if(chunk_len > PADICO_SHM_SHORT_PAYLOAD)
    padico_fatal("minidriver-shm: FATAL- cannot send, payload too large for short buffer.\n");
  assert(chunk_len <= PADICO_SHM_SHORT_PAYLOAD);
  assert(p_status->send.block_num == -1);
  p_status->send.block_num = block_num;
  p_status->send.len = chunk_len;
  nm_data_slicer_init(&p_status->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_status->send.slicer, chunk_offset);
  nm_data_slicer_copy_from(&p_status->send.slicer, base, chunk_len);
  nm_data_slicer_destroy(&p_status->send.slicer);
  p_status->send.slicer = NM_DATA_SLICER_NULL;
}

static void nm_minidriver_shm_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_minidriver_shm_s*p_status = _status;
  const int block_num = padico_shm_block_alloc(p_status->p_shm_context->shm);
  void*base = padico_shm_short_get_ptr(p_status->p_shm_context->shm, block_num);
  size_t len = 0;
  int i;
  for(i = 0; i < n; i++)
    {
      if(len + v[i].iov_len > PADICO_SHM_SHORT_PAYLOAD)
	padico_fatal("minidriver-shm: FATAL- cannot send, payload too large for short buffer (max payload = %d; packet len = %d).\n", (int)PADICO_SHM_SHORT_PAYLOAD, (int)(len + v[i].iov_len));
      memcpy(base + len, v[i].iov_base, v[i].iov_len);
      len += v[i].iov_len;
    }
  assert(len <= PADICO_SHM_SHORT_PAYLOAD);
  assert(p_status->send.block_num == -1);
  p_status->send.block_num = block_num;
  p_status->send.len = len;
  p_status->send.slicer = NM_DATA_SLICER_NULL;
}

static void nm_minidriver_shm_send_buf_get(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_minidriver_shm_s*p_status = _status;
  const int block_num = padico_shm_block_alloc(p_status->p_shm_context->shm);
  void*base = padico_shm_short_get_ptr(p_status->p_shm_context->shm, block_num);
  *p_buffer = base;
  *p_len = PADICO_SHM_SHORT_PAYLOAD;
  p_status->send.block_num = block_num;
}

static void nm_minidriver_shm_send_buf_post(void*_status, nm_len_t len)
{
 struct nm_minidriver_shm_s*p_status = _status;
 p_status->send.len = len;
}

static int nm_minidriver_shm_send_poll(void*_status)
{
  struct nm_minidriver_shm_s*p_status = _status;
  int rc = padico_shm_short_send_commit(p_status->p_shm_context->shm, p_status->dest, p_status->send.block_num, p_status->send.len, 0);
  if(rc)
    {
      return -NM_EAGAIN;
    }
  else
    {
      p_status->send.block_num = -1;
      return NM_ESUCCESS;
    }
}

static int  nm_minidriver_shm_recv_buf_poll(void*_status, void**p_buffer, nm_len_t*p_len)
{
  struct nm_minidriver_shm_s*p_status = _status;
  const int block = padico_shm_short_recv_poll(p_status->p_shm_context->shm, p_status->dest);
  if(block != -1)
    {
      struct padico_shm_short_header_s*header = padico_shm_block_get_ptr(p_status->p_shm_context->shm, block);
      *p_buffer = header + 1;
      *p_len = header->size;
      p_status->recv.block_num = block;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static void nm_minidriver_shm_recv_buf_release(void*_status)
{
  struct nm_minidriver_shm_s*p_status = _status;
  padico_shm_block_free(p_status->p_shm_context->shm, p_status->recv.block_num);
}

static void nm_minidriver_shm_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_minidriver_shm_s*p_status = _status;
  nm_data_slicer_init(&p_status->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_status->send.slicer, chunk_offset);
  p_status->recv.chunk_len = chunk_len;
}

static void nm_minidriver_shm_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_minidriver_shm_s*p_status = _status;
  p_status->recv.v = v;
  p_status->recv.n = n;
  p_status->recv.slicer = NM_DATA_SLICER_NULL;
}

static int nm_minidriver_shm_recv_poll_one(void*_status)
{
  struct nm_minidriver_shm_s*p_status = _status;
  const int block = padico_shm_short_recv_poll(p_status->p_shm_context->shm, p_status->dest);
  if(block != -1)
    {
      struct padico_shm_short_header_s*header = padico_shm_block_get_ptr(p_status->p_shm_context->shm, block);
      void*buffer = header + 1;
      if(nm_data_slicer_isnull(&p_status->recv.slicer))
	{
	  assert(p_status->recv.n == 1);
	  if(p_status->recv.v->iov_len < header->size)
	    {
	      return -NM_EINVAL;
	    }
	  memcpy(p_status->recv.v->iov_base, buffer, header->size);
	  p_status->recv.v = NULL;
	  p_status->recv.n = -1;
	}
      else
	{
	  assert(header->size <= p_status->recv.chunk_len);
	  nm_data_slicer_copy_to(&p_status->recv.slicer, buffer, header->size);
	  nm_data_slicer_destroy(&p_status->recv.slicer);
	  p_status->recv.slicer = NM_DATA_SLICER_NULL;
	}
      padico_shm_block_free(p_status->p_shm_context->shm, block);
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int nm_minidriver_shm_recv_cancel(void*_status)
{ 
  return -NM_ENOTIMPL;
}
