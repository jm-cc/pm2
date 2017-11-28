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

/** @file Cross-Memory Attach driver with 'minidriver' interface.
 */

#include <Padico/Puk.h>
#include <Padico/Module.h>
#include <nm_minidriver.h>
#include <Padico/Shm.h>

/* ********************************************************* */

static void*nm_minidriver_cma_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_minidriver_cma_destroy(void*);

static const struct puk_component_driver_s nm_minidriver_cma_component =
  {
    .instantiate = &nm_minidriver_cma_instantiate,
    .destroy     = &nm_minidriver_cma_destroy
  };

static void nm_minidriver_cma_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_minidriver_cma_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_minidriver_cma_close(puk_context_t context);
static void nm_minidriver_cma_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_minidriver_cma_send_post(void*_status, const struct iovec*v, int n);
static int  nm_minidriver_cma_send_poll(void*_status);
static void nm_minidriver_cma_recv_iov_post(void*_status,  struct iovec*v, int n);
static int  nm_minidriver_cma_recv_poll_one(void*_status);
static int  nm_minidriver_cma_recv_cancel(void*_status);

static const struct nm_minidriver_iface_s nm_minidriver_cma_minidriver =
  {
    .getprops       = &nm_minidriver_cma_getprops,
    .init           = &nm_minidriver_cma_init,
    .close          = &nm_minidriver_cma_close,
    .connect        = &nm_minidriver_cma_connect,
    .send_post      = &nm_minidriver_cma_send_post,
    .send_poll      = &nm_minidriver_cma_send_poll,
    .recv_iov_post  = &nm_minidriver_cma_recv_iov_post,
    .recv_poll_one  = &nm_minidriver_cma_recv_poll_one,
    .recv_cancel    = &nm_minidriver_cma_recv_cancel
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(Minidriver_CMA,
  puk_component_declare("Minidriver_CMA",
			puk_component_provides("PadicoComponent", "component", &nm_minidriver_cma_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_minidriver_cma_minidriver)));

/* ********************************************************* */

/** 'cma' per-context data.
 */
struct nm_minidriver_cma_context_s
{
  struct padico_shm_s*shm;                 /**< shm segment */
  char*url;
};

/** 'cma' per-instance status.
 */
struct nm_minidriver_cma_s
{
  struct padico_shm_node_s*dest;           /**< destination node */
  int dest_rank;                           /**< destination rank in shm directory */
  struct nm_minidriver_cma_context_s*shm_context;
  struct padico_shm_short_lfqueue_s queue; /**< per-connection receive queue */
  struct
  {
    int block_num;
  } recv;
};

struct nm_minidriver_cma_packet_s
{
  struct iovec v;
  volatile int done;
};

/* ********************************************************* */

static void*nm_minidriver_cma_instantiate(puk_instance_t instance, puk_context_t context)
{ 
  struct nm_minidriver_cma_s*status = padico_malloc(sizeof(struct nm_minidriver_cma_s));
  assert(context != NULL);
  status->dest = NULL;
  status->dest_rank = -1;
  padico_shm_short_lfqueue_init(&status->queue);
  status->shm_context = puk_context_get_status(context);
  assert(status->shm_context != NULL);
  status->recv.block_num = -1;
  return status;

}

static void nm_minidriver_cma_destroy(void*_status)
{
  struct nm_minidriver_cma_s*status = _status;
  status->dest = NULL;
  padico_free(_status);
}

/* ********************************************************* */

static void nm_minidriver_cma_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  p_props->profile.latency = 200;
  p_props->profile.bandwidth = 10000;
  p_props->capabilities.supports_data = 0;
}

static void nm_minidriver_cma_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_minidriver_cma_context_s*shm_context = padico_malloc(sizeof(struct nm_minidriver_cma_context_s));
  padico_string_t segment_name = padico_string_new();
  padico_string_printf(segment_name, "minidriver-id%s", context->id);
  shm_context->shm = padico_shm_init(padico_string_get(segment_name));
  puk_context_set_status(context, shm_context);
  shm_context->url = padico_strdup((char*)padico_na_node_getuuid(padico_na_getlocalnode()));
  padico_string_delete(segment_name);
  *drv_url = shm_context->url;
  *url_size = strlen(shm_context->url);
}

static void nm_minidriver_cma_close(puk_context_t context)
{
  struct nm_minidriver_cma_context_s*shm_context = puk_context_get_status(context);
  padico_shm_close(shm_context->shm);
  puk_context_set_status(context, NULL);
  padico_free(shm_context->url);
  padico_free(shm_context);
}

static void nm_minidriver_cma_connect(void*_status, const void*remote_url, size_t url_size)
{ 
  struct nm_minidriver_cma_s*status = (struct nm_minidriver_cma_s*)_status;
  const char*remote_uuid = remote_url;
  padico_na_node_t remote_node = padico_na_getnodebyuuid((padico_topo_uuid_t)remote_uuid);
  assert(status->shm_context->shm->seg != NULL);
  status->dest = padico_shm_directory_node_lookup(status->shm_context->shm, remote_node);
  if(status->dest == NULL)
    {
      padico_fatal("minidriver-cma: FATAL- cannot find peer node %s in shm directory.\n", remote_uuid);
    }
  else
    {
      status->dest_rank = status->dest->rank;
      assert(status->dest_rank >= 0);
      assert(status->dest_rank < PADICO_SHM_NUMNODES);
    }
}

static void nm_minidriver_cma_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_minidriver_cma_s*status = _status;
  assert(n == 1);
  while(status->dest->mailbox.large[status->shm_context->shm->rank].pipeline == -1)
    {
      /* wait the receiver to be ready to receive */
      __sync_synchronize();
    }
  const int block_num = status->dest->mailbox.large[status->shm_context->shm->rank].pipeline;
  struct nm_minidriver_cma_packet_s*block = padico_shm_short_get_ptr(status->shm_context->shm, block_num);
  status->dest->mailbox.large[status->shm_context->shm->rank].pipeline = -1; /* consume RTR */

  int rc = process_vm_writev(status->dest->pid,
			     v, 1,
			     &block->v, 1,
			     0);
  if(rc < 0)
    {
      padico_fatal("minidriver-cma: FATAL- error '%s' in process_vm_writev.\n", strerror(errno));
    }
  block->done = 1;
}

static int nm_minidriver_cma_send_poll(void*_status)
{
  return NM_ESUCCESS;
}


static void nm_minidriver_cma_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_minidriver_cma_s*status = _status;
  const int block_num = padico_shm_block_alloc(status->shm_context->shm);
  assert(status->recv.block_num == -1);
  status->recv.block_num = block_num;
  assert(n == 1);
  struct nm_minidriver_cma_packet_s*block = padico_shm_short_get_ptr(status->shm_context->shm, block_num);
  block->v = v[0];
  block->done = 0;
  status->shm_context->shm->self->mailbox.large[status->dest_rank].pipeline = block_num;
  __sync_synchronize();
}

static int nm_minidriver_cma_recv_poll_one(void*_status)
{
  struct nm_minidriver_cma_s*status = _status;
  struct nm_minidriver_cma_packet_s*block = padico_shm_short_get_ptr(status->shm_context->shm, status->recv.block_num);
  if(block->done)
    {
      padico_shm_block_free(status->shm_context->shm, status->recv.block_num);
      status->recv.block_num = -1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static int nm_minidriver_cma_recv_cancel(void*_status)
{ 
  return -NM_ENOTIMPL;
}
