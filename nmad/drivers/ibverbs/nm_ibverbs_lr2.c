/*
 * NewMadeleine
 * Copyright (C) 2010 (see AUTHORS file)
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

/* @file ibverbs lr2- Logarithmic super-pipeline RDMA.
 */

#include "nm_ibverbs.h"
#include <nm_connector.h>

#include <Padico/Module.h>


/* *** method: 'lr2' *************************************** */

#define NM_IBVERBS_LR2_BUFSIZE (512*1024)
#define NM_IBVERBS_LR2_NBUF 3

#define NM_IBVERBS_LR2_BLOCKSIZE 4096

static const nm_len_t lr2_steps[] =
  { 12*1024, 24*1024, 40*1024, 64*1024, 88*1024, 128*1024, /* 160*1024, 200*1024, 256*1024, */ 0};
static const int lr2_nsteps = sizeof(lr2_steps) / sizeof(nm_len_t) - 1;

/** on the wire header of minidriver 'lr2' */
struct lr2_header_s
{
  volatile uint32_t checksum; /* has to be the last field in the struct */
} __attribute__((packed));

static const nm_len_t lr2_hsize = sizeof(struct lr2_header_s);

static inline nm_len_t nm_ibverbs_min(const nm_len_t a, const nm_len_t b)
{
  if(b > a)
    return a;
  else
    return b;
}

/** context for ibverbs lr2 */
struct nm_ibverbs_lr2_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
};

/** Connection state for tracks sending with lr2 */
struct nm_ibverbs_lr2
{  
  struct
  {
    char sbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    char rbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    volatile uint32_t sack, rack;
  } buffer; /* buffers should be 64-byte aligned */
  struct ibv_mr*mr;
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
  puk_context_t context;

  struct
  {
    const void*message;
    nm_data_slicer_t slicer;
    nm_len_t size;
    nm_len_t done;
    void*rbuf;
    void*sbuf;
    int step;
    int nbuffer;
    const void*prefetch;
  } send;

  struct
  {
    void*message;
    nm_data_slicer_t slicer;
    nm_len_t size;
    void*rbuf;
    nm_len_t done;
    int step;
    int nbuffer;
  } recv;
};

static void nm_ibverbs_lr2_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_ibverbs_lr2_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_lr2_close(puk_context_t context);
static void nm_ibverbs_lr2_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_lr2_send_post(void*_status, const struct iovec*v, int n);
static void nm_ibverbs_lr2_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_lr2_send_poll(void*_status);
static void nm_ibverbs_lr2_send_prefetch(void*_status, const void*ptr, uint64_t size);
static void nm_ibverbs_lr2_recv_init(void*_status, struct iovec*v, int n);
static void nm_ibverbs_lr2_recv_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_lr2_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_lr2_minidriver =
  {
    .getprops    = &nm_ibverbs_lr2_getprops,
    .init        = &nm_ibverbs_lr2_init,
    .close       = &nm_ibverbs_lr2_close,
    .connect     = &nm_ibverbs_lr2_connect,
    .send_post   = &nm_ibverbs_lr2_send_post,
    .send_data   = &nm_ibverbs_lr2_send_data,
    .send_poll   = &nm_ibverbs_lr2_send_poll,
    .send_prefetch = &nm_ibverbs_lr2_send_prefetch,
    .recv_init   = &nm_ibverbs_lr2_recv_init,
    .recv_data   = &nm_ibverbs_lr2_recv_data,
    .poll_one    = &nm_ibverbs_lr2_poll_one,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_lr2_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_lr2_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_lr2_component =
  {
    .instantiate = &nm_ibverbs_lr2_instantiate,
    .destroy = &nm_ibverbs_lr2_destroy
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_ibverbs_lr2,
  puk_component_declare("NewMad_ibverbs_lr2",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_lr2_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_lr2_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto"))
);


/* ********************************************************* */

static void* nm_ibverbs_lr2_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_lr2*lr2 = NULL;
  if(nm_ibverbs_memalign > 0)
    posix_memalign((void**)&lr2, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_lr2));
  else
    lr2 = malloc(sizeof(struct nm_ibverbs_lr2));
  memset(&lr2->buffer, 0, sizeof(lr2->buffer));
  lr2->buffer.rack = 0;
  lr2->mr = NULL;
  lr2->context = context;
  lr2->cnx = NULL;
  return lr2;
}

static void nm_ibverbs_lr2_destroy(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  if(lr2->cnx)
    {
      nm_ibverbs_cnx_close(lr2->cnx);
    }
  if(lr2->mr)
    {
      ibv_dereg_mr(lr2->mr);
    }
  free(lr2);
}

/* *** lr2 connection ************************************** */

static void nm_ibverbs_lr2_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  struct nm_ibverbs_lr2_context_s*p_lr2_context = malloc(sizeof(struct nm_ibverbs_lr2_context_s));
  puk_context_set_status(context, p_lr2_context);
  p_lr2_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_lr2_context->p_hca, &props->profile);
  props->capabilities.supports_data = 1;
  props->capabilities.max_msg_size = 0; /* unlimited */
  props->capabilities.trk_rdv = 1;
}

static void nm_ibverbs_lr2_init(puk_context_t context, const void**drv_url, size_t*url_size)
{ 
  const char*url = NULL;
  struct nm_ibverbs_lr2_context_s*p_lr2_context = puk_context_get_status(context);
  p_lr2_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_lr2_close(puk_context_t context)
{
  struct nm_ibverbs_lr2_context_s*p_lr2_context = puk_context_get_status(context);
  nm_connector_destroy(p_lr2_context->p_connector);
  nm_ibverbs_hca_release(p_lr2_context->p_hca);
  puk_context_set_status(context, NULL);
  free(p_lr2_context);
}

static void nm_ibverbs_lr2_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  struct nm_ibverbs_lr2_context_s*p_lr2_context = puk_context_get_status(lr2->context);
  lr2->cnx = nm_ibverbs_cnx_new(p_lr2_context->p_hca);
  lr2->mr = ibv_reg_mr(p_lr2_context->p_hca->pd, &lr2->buffer, sizeof(lr2->buffer),
		       IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  lr2->send.prefetch = NULL;
  if(lr2->mr == NULL)
    {
      NM_FATAL("Infiniband: lr2 cannot register MR.\n");
    }
  struct nm_ibverbs_segment*seg = &lr2->cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&lr2->buffer;
  seg->rkey  = lr2->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(lr2->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &lr2->cnx->local_addr, &lr2->cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: timeout in address exchange.\n");
    }
  lr2->seg = lr2->cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(lr2->cnx);
}

/* *** lr2 I/O ********************************************* */

static inline void nm_ibverbs_lr2_send_init(struct nm_ibverbs_lr2*lr2)
{
  assert(lr2->send.message == NULL);
  assert(nm_data_slicer_isnull(&lr2->send.slicer));
  lr2->send.done    = 0;
  lr2->send.rbuf    = lr2->buffer.rbuf;
  lr2->send.sbuf    = lr2->buffer.sbuf;
  lr2->send.step    = 0;
  lr2->send.nbuffer = 0;
  lr2->buffer.rack  = 0;
}
static void nm_ibverbs_lr2_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  assert(n == 1);
  nm_ibverbs_lr2_send_init(lr2);
  lr2->send.message = v[0].iov_base;
  lr2->send.slicer  = NM_DATA_SLICER_NULL;
  lr2->send.size    = v[0].iov_len;
}
static void nm_ibverbs_lr2_send_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  assert(chunk_offset + chunk_len <= nm_data_size(p_data));
  nm_ibverbs_lr2_send_init(lr2);
  lr2->send.message = NULL;
  lr2->send.size    = chunk_len;
  nm_data_slicer_init(&lr2->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&lr2->send.slicer, chunk_offset);
}

static int nm_ibverbs_lr2_send_poll(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  while(lr2->send.done < lr2->send.size)
    {
      const nm_len_t chunk_size = lr2_steps[lr2->send.step];
      const nm_len_t block_size = NM_IBVERBS_LR2_BLOCKSIZE;
      const nm_len_t block_max_payload = block_size - lr2_hsize;
      const nm_len_t chunk_max_payload = chunk_size - lr2_hsize * chunk_size / block_size;
      const nm_len_t chunk_payload = nm_ibverbs_min(lr2->send.size - lr2->send.done, chunk_max_payload);
      /* ** N-buffering */
      if((lr2->send.sbuf + chunk_size) > (((void*)lr2->buffer.sbuf) + (lr2->send.nbuffer + 1) * NM_IBVERBS_LR2_BUFSIZE))
	{
	  /* flow control rationale- receiver may be:
	   *   . reading buffer N -> ok for writing N + 1
	   *   . already be ready for N + 1 -> ok
	   *   . reading N - 1 (~= N + 2) -> wait
	   * */
	  const int n2 = (lr2->send.nbuffer + 2) % NM_IBVERBS_LR2_NBUF;
	  if(lr2->buffer.rack == n2)
	    { 
	      return -NM_EAGAIN;
	    }
	  /* swap buffers */
	  lr2->send.nbuffer = (lr2->send.nbuffer + 1) % NM_IBVERBS_LR2_NBUF;
	  lr2->send.sbuf = ((void*)lr2->buffer.sbuf) + lr2->send.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	  lr2->send.rbuf = ((void*)lr2->buffer.rbuf) + lr2->send.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	}
      /* ** fill buffer (one chunk made of multiple blocks) */
      nm_len_t chunk_offset = 0; /**< offset in the sbuf/rbuf, i.e. payload + headers */
      nm_len_t base_offset = 0;
      if((lr2->send.prefetch != NULL) && (lr2->send.prefetch == lr2->send.message) && (lr2->send.done == 0))
	{
	  chunk_offset = chunk_size;
	}
      else
	{
	  nm_len_t chunk_todo = chunk_payload;
	  base_offset = (chunk_max_payload - chunk_payload) % block_max_payload;
	  const nm_len_t padding = (nm_ibverbs_alignment > 0) ? (base_offset % nm_ibverbs_alignment) : 0;
	  chunk_offset = base_offset;
	  base_offset -= padding;
	  while(chunk_todo > 0)
	    {
	      const nm_len_t block_payload = (chunk_todo % block_max_payload == 0) ?
		block_max_payload : (chunk_todo % block_max_payload);
	      struct lr2_header_s*h = lr2->send.sbuf + chunk_offset + block_payload;
	      h->checksum = 1 | nm_ibverbs_copy_from_and_checksum(lr2->send.sbuf + chunk_offset, &lr2->send.slicer, lr2->send.message, lr2->send.done + (chunk_payload - chunk_todo), block_payload);
	      chunk_todo   -= block_payload;
	      chunk_offset += block_payload + lr2_hsize;
	    }
	}
      /* ** send chunk */
      nm_ibverbs_send_flushn(lr2->cnx, NM_IBVERBS_WRID_PACKET, 1);
      nm_ibverbs_rdma_send(lr2->cnx, chunk_offset - base_offset, lr2->send.sbuf + base_offset, lr2->send.rbuf + base_offset,
			   &lr2->buffer, &lr2->seg, lr2->mr, NM_IBVERBS_WRID_PACKET);
      lr2->send.done += chunk_payload;
      lr2->send.sbuf += chunk_offset;
      lr2->send.rbuf += chunk_offset;
      lr2->send.prefetch = NULL;
      if(lr2->send.step < lr2_nsteps - 1)
	lr2->send.step++;
    }
  nm_ibverbs_send_flush(lr2->cnx, NM_IBVERBS_WRID_PACKET);
  lr2->send.message = NULL;
  if(!nm_data_slicer_isnull(&lr2->send.slicer))
    {
      nm_data_slicer_destroy(&lr2->send.slicer);
      lr2->send.slicer = NM_DATA_SLICER_NULL;
    }
  return NM_ESUCCESS;
}

static void nm_ibverbs_lr2_send_prefetch(void*_status, const void*ptr, uint64_t size)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  if((lr2->send.prefetch == NULL) && (lr2->send.message == NULL))
    {
      const nm_len_t block_size = NM_IBVERBS_LR2_BLOCKSIZE;
      const nm_len_t chunk_size = lr2_steps[0];
      const nm_len_t block_payload = block_size - lr2_hsize;
      nm_len_t chunk_done = 0;
      nm_len_t chunk_offset = 0;
      if(size > block_payload * (chunk_size/block_size))
	{
	  /* prefetch only in case first step is complete */
	  while(chunk_offset < chunk_size)
	    {
	      struct lr2_header_s*h = (struct lr2_header_s*)(&lr2->buffer.sbuf[chunk_offset] + block_payload);
	      h->checksum = 1 | nm_ibverbs_memcpy_and_checksum(&lr2->buffer.sbuf[chunk_offset], ptr + chunk_done, block_payload);
	      chunk_done   += block_payload;
	  chunk_offset += block_size;
	    }
	  lr2->send.prefetch = ptr;
	}
    }
}

static void nm_ibverbs_lr2_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  lr2->recv.done    = 0;
  lr2->recv.message = v->iov_base;
  lr2->recv.slicer  = NM_DATA_SLICER_NULL;
  lr2->recv.size    = v->iov_len;
  lr2->recv.rbuf    = lr2->buffer.rbuf;
  lr2->recv.step    = 0;
  lr2->recv.nbuffer = 0;
}

static void nm_ibverbs_lr2_recv_data(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  assert(chunk_offset + chunk_len <= nm_data_size(p_data));
  lr2->recv.done    = 0;
  lr2->recv.message = NULL;
  lr2->recv.size    = chunk_len;
  lr2->recv.rbuf    = lr2->buffer.rbuf;
  lr2->recv.step    = 0;
  lr2->recv.nbuffer = 0;
  nm_data_slicer_init(&lr2->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&lr2->recv.slicer, chunk_offset);
}

static int nm_ibverbs_lr2_poll_one(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  while(lr2->recv.done < lr2->recv.size)
    {
      const nm_len_t chunk_size = lr2_steps[lr2->recv.step];
      const nm_len_t block_size = NM_IBVERBS_LR2_BLOCKSIZE;
      const nm_len_t block_max_payload = block_size - lr2_hsize;
      const nm_len_t chunk_max_payload = chunk_size - lr2_hsize * chunk_size / block_size;
      const nm_len_t chunk_payload = nm_ibverbs_min(lr2->recv.size - lr2->recv.done, chunk_max_payload);
      if((lr2->recv.rbuf + chunk_size) > (((void*)lr2->buffer.rbuf) + (lr2->recv.nbuffer + 1) * NM_IBVERBS_LR2_BUFSIZE))
      {
	/* swap buffers */
	lr2->recv.nbuffer = (lr2->recv.nbuffer + 1) % NM_IBVERBS_LR2_NBUF;
	lr2->recv.rbuf = ((void*)lr2->buffer.rbuf) + lr2->recv.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	nm_ibverbs_send_flush(lr2->cnx, NM_IBVERBS_WRID_ACK);
	lr2->buffer.sack = lr2->recv.nbuffer;
	nm_ibverbs_rdma_send(lr2->cnx, sizeof(uint32_t),
			     (void*)&lr2->buffer.sack, (void*)&lr2->buffer.rack,
			     &lr2->buffer, &lr2->seg, lr2->mr, NM_IBVERBS_WRID_ACK);
      }
      nm_len_t chunk_todo = chunk_payload;
      nm_len_t chunk_offset = (chunk_max_payload - chunk_payload) % block_max_payload;
      while(chunk_todo > 0)
	{
	  const nm_len_t block_payload = (chunk_todo % block_max_payload == 0) ?
	    block_max_payload : (chunk_todo % block_max_payload);
	  struct lr2_header_s*h = lr2->recv.rbuf + chunk_offset + block_payload;
	  if((chunk_todo == chunk_payload) && !h->checksum)
	    goto wouldblock;
	  else
	    while(!h->checksum)
	      {
	      }
	  const uint32_t checksum =
	    1 | nm_ibverbs_copy_to_and_checksum(lr2->recv.rbuf + chunk_offset, &lr2->recv.slicer, lr2->recv.message,
						lr2->recv.done, block_payload);
	  if(h->checksum != checksum)
	    {
	      fprintf(stderr, "nmad: FATAL- ibverbs: checksum failed; step = %d; done = %d / %d;  received = %llX; expected = %llX.\n",
		      lr2->recv.step, (int)lr2->recv.done, (int)lr2->recv.size, 
		      (long long unsigned)h->checksum, (long long unsigned)checksum);
	      abort();
	    }
    	  h->checksum = 0;
	  chunk_todo -= block_payload;
	  lr2->recv.done += block_payload;
	  lr2->recv.rbuf += chunk_offset + block_payload + lr2_hsize;
	  chunk_offset = 0;
	}
      if(lr2->recv.step < lr2_nsteps - 1)
	lr2->recv.step++;
    }
  lr2->recv.message = NULL;
  if(!nm_data_slicer_isnull(&lr2->recv.slicer))
    {
      nm_data_slicer_destroy(&lr2->recv.slicer);
      lr2->recv.slicer = NM_DATA_SLICER_NULL;
    }
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

