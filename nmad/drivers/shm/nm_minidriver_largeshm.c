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

static void*nm_minidriver_largeshm_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_minidriver_largeshm_destroy(void*);

static const struct puk_component_driver_s nm_minidriver_largeshm_component =
  {
    .instantiate = &nm_minidriver_largeshm_instantiate,
    .destroy = &nm_minidriver_largeshm_destroy
  };

static void nm_minidriver_largeshm_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_minidriver_largeshm_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_minidriver_largeshm_close(puk_context_t context);
static void nm_minidriver_largeshm_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_minidriver_largeshm_send_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_minidriver_largeshm_send_poll(void*_status);
static void nm_minidriver_largeshm_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_minidriver_largeshm_recv_poll_one(void*_status);
static int  nm_minidriver_largeshm_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s nm_minidriver_largeshm_minidriver =
  {
    .getprops        = &nm_minidriver_largeshm_getprops,
    .init            = &nm_minidriver_largeshm_init,
    .close           = &nm_minidriver_largeshm_close,
    .connect         = &nm_minidriver_largeshm_connect,
    .send_data_post       = &nm_minidriver_largeshm_send_data_post,
    .send_iov_post       = NULL,
    .send_poll       = &nm_minidriver_largeshm_send_poll,
    .recv_iov_post   = NULL,
    .recv_data_post  = &nm_minidriver_largeshm_recv_data_post,
    .recv_poll_one   = &nm_minidriver_largeshm_recv_poll_one,
    .recv_cancel     = &nm_minidriver_largeshm_recv_cancel
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_largeshm,
  puk_component_declare("Minidriver_largeshm",
			puk_component_provides("PadicoComponent", "component", &nm_minidriver_largeshm_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_minidriver_largeshm_minidriver)));

/* ********************************************************* */


/** 'shm' per-context data.
 */
struct nm_minidriver_largeshm_context_s
{
  struct nm_minidriver_largeshm_s*conns[PADICO_SHM_NUMNODES]; /**< connections in the context, indexed by destination rank */
  struct padico_shm_s*shm;                 /**< shm segment */
  char*url;
};

/** 'shm' per-instance status.
 */
struct nm_minidriver_largeshm_s
{
  struct padico_shm_node_s*dest;           /**< destination node */
  int dest_rank;                           /**< destination rank in shm directory */
  struct nm_minidriver_largeshm_context_s*p_shm_context;
  struct
  {
    nm_data_slicer_t slicer;
    nm_len_t chunk_len;
    nm_len_t done;
    int queue_num;
  } recv;
  struct
  {
    nm_data_slicer_t slicer;
    nm_len_t len;
    nm_len_t done;
    int queue_num;
  } send;
};


/* ********************************************************* */

static void*nm_minidriver_largeshm_instantiate(puk_instance_t instance, puk_context_t context)
{  
  struct nm_minidriver_largeshm_s*p_status = padico_malloc(sizeof(struct nm_minidriver_largeshm_s));
  p_status->dest = NULL;
  p_status->dest_rank = -1;
  p_status->p_shm_context = puk_context_get_status(context);
  assert(p_status->p_shm_context->shm->seg != NULL);
  p_status->send.queue_num = -1;
  return p_status;

}

static void nm_minidriver_largeshm_destroy(void*_status)
{
  struct nm_minidriver_largeshm_s*p_status = _status;
  if(p_status->dest_rank > -1)
    p_status->p_shm_context->conns[p_status->dest_rank] = NULL;
  padico_free(_status);
}

/* ********************************************************* */

static void nm_minidriver_largeshm_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  p_props->profile.latency = 200;
  p_props->profile.bandwidth = 10000;
  p_props->capabilities.supports_data = 1;
  p_props->capabilities.trk_rdv = 1;
}

static void nm_minidriver_largeshm_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_minidriver_largeshm_context_s*p_shm_context = padico_malloc(sizeof(struct nm_minidriver_largeshm_context_s));
  padico_string_t segment_name = padico_string_new();
  padico_string_printf(segment_name, "minidriver-large-id%s", context->id);
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

static void nm_minidriver_largeshm_close(puk_context_t context)
{
  struct nm_minidriver_largeshm_context_s*p_shm_context = puk_context_get_status(context);
  padico_shm_close(p_shm_context->shm);
  puk_context_set_status(context, NULL);
  padico_free(p_shm_context->url);
  padico_free(p_shm_context);
}

static void nm_minidriver_largeshm_connect(void*_status, const void*remote_url, size_t url_size)
{ 
  struct nm_minidriver_largeshm_s*p_status = _status;
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

static void nm_minidriver_largeshm_send_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_minidriver_largeshm_s*p_status = _status;
  p_status->send.queue_num = -1;
  while(p_status->dest->mailbox.large[p_status->p_shm_context->shm->rank].pipeline == -1)
    {
      /* wait the receiver to be ready to receive */
      __sync_synchronize();
    }
  p_status->send.queue_num = p_status->dest->mailbox.large[p_status->p_shm_context->shm->rank].pipeline;
  p_status->dest->mailbox.large[p_status->p_shm_context->shm->rank].pipeline = -1; /* consume RTR before send to avoid races */
  assert(p_status->send.queue_num != -1);
  p_status->send.done = 0;
  p_status->send.len = chunk_len;
  nm_data_slicer_init(&p_status->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_status->send.slicer, chunk_offset);
}

static int nm_minidriver_largeshm_send_poll(void*_status)
{
  struct nm_minidriver_largeshm_s*p_status = _status;
  if(p_status->send.done < p_status->send.len)
    {
      const int block_num = padico_shm_block_alloc(p_status->p_shm_context->shm);
      void*block = padico_shm_block_get_ptr(p_status->p_shm_context->shm, block_num);
      const int todo = p_status->send.len - p_status->send.done;
      const int b = (todo > PADICO_SHM_BLOCKSIZE) ? PADICO_SHM_BLOCKSIZE : todo;
      nm_data_slicer_copy_from(&p_status->send.slicer, block, b);
      p_status->send.done += b;
      int rc = -1;
      do
	{
	  rc = padico_shm_large_lfqueue_enqueue(&p_status->p_shm_context->shm->seg->buffer.queues[p_status->send.queue_num], block_num);
	}
      while(rc != 0);
    }
  if(p_status->send.done == p_status->send.len)
    {
      p_status->send.queue_num = -1;
      nm_data_slicer_destroy(&p_status->send.slicer);
      p_status->send.slicer = NM_DATA_SLICER_NULL;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static void nm_minidriver_largeshm_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_minidriver_largeshm_s*p_status = _status;
  nm_data_slicer_init(&p_status->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_status->recv.slicer, chunk_offset);
  assert(p_status->p_shm_context->shm->self->mailbox.large[p_status->dest_rank].pipeline == -1);
  /* allocate queue and signal RTR */
  int queue_num = -1;
  while(queue_num == -1)
    queue_num = padico_shm_queue_lfqueue_dequeue(&p_status->p_shm_context->shm->seg->buffer.free_queues);
  p_status->p_shm_context->shm->self->mailbox.large[p_status->dest_rank].pipeline = queue_num;
  __sync_synchronize();
  p_status->recv.chunk_len = chunk_len;
  p_status->recv.done      = 0;
  p_status->recv.queue_num = queue_num;
}

static int nm_minidriver_largeshm_recv_poll_one(void*_status)
{
  struct nm_minidriver_largeshm_s*p_status = _status;
  int block_num = -1;
  do
    {
      block_num = padico_shm_large_lfqueue_dequeue(&p_status->p_shm_context->shm->seg->buffer.queues[p_status->recv.queue_num]);
      if(block_num == -1 && p_status->recv.done == 0)
	return -NM_EAGAIN;
    }
  while(block_num == -1);
  const int todo = p_status->recv.chunk_len - p_status->recv.done;
  int b = (todo > PADICO_SHM_BLOCKSIZE) ? PADICO_SHM_BLOCKSIZE : todo;
  void*block = padico_shm_block_get_ptr(p_status->p_shm_context->shm, block_num);
  nm_data_slicer_copy_to(&p_status->recv.slicer, block, b);
  p_status->recv.done += b;
  padico_shm_block_free(p_status->p_shm_context->shm, block_num);
  if(p_status->recv.done == p_status->recv.chunk_len)
    {
      nm_data_slicer_destroy(&p_status->recv.slicer);
      p_status->recv.slicer = NM_DATA_SLICER_NULL;
      padico_shm_queue_lfqueue_enqueue(&p_status->p_shm_context->shm->seg->buffer.free_queues, p_status->recv.queue_num);
      p_status->recv.queue_num = -1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int nm_minidriver_largeshm_recv_cancel(void*_status)
{ 
  return -NM_ENOTIMPL;
}
