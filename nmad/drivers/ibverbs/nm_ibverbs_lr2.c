/*
 * NewMadeleine
 * Copyright (C) 2010-2017 (see AUTHORS file)
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
struct nm_ibverbs_lr2_s
{  
  struct
  {
    char sbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    char rbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    volatile uint32_t sack, rack;
  } buffer; /* buffers should be 64-byte aligned */
  struct ibv_mr*mr;
  struct nm_ibverbs_segment_s seg; /**< remote segment */
  struct nm_ibverbs_cnx_s*p_cnx;
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

static void nm_ibverbs_lr2_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_lr2_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_lr2_close(puk_context_t context);
static void nm_ibverbs_lr2_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_lr2_send_iov_post(void*_status, const struct iovec*v, int n);
static void nm_ibverbs_lr2_send_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_lr2_send_poll(void*_status);
static void nm_ibverbs_lr2_send_prefetch(void*_status, const void*ptr, uint64_t size);
static void nm_ibverbs_lr2_recv_iov_post(void*_status, struct iovec*v, int n);
static void nm_ibverbs_lr2_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
static int  nm_ibverbs_lr2_recv_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_lr2_minidriver =
  {
    .getprops       = &nm_ibverbs_lr2_getprops,
    .init           = &nm_ibverbs_lr2_init,
    .close          = &nm_ibverbs_lr2_close,
    .connect        = &nm_ibverbs_lr2_connect,
    .send_iov_post      = &nm_ibverbs_lr2_send_iov_post,
    .send_data_post      = &nm_ibverbs_lr2_send_data_post,
    .send_poll      = &nm_ibverbs_lr2_send_poll,
    .send_prefetch  = &nm_ibverbs_lr2_send_prefetch,
    .recv_iov_post  = &nm_ibverbs_lr2_recv_iov_post,
    .recv_data_post = &nm_ibverbs_lr2_recv_data_post,
    .recv_poll_one  = &nm_ibverbs_lr2_recv_poll_one,
    .recv_cancel    = NULL
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
  struct nm_ibverbs_lr2_s*p_lr2 = NULL;
  if(nm_ibverbs_memalign > 0)
    posix_memalign((void**)&p_lr2, nm_ibverbs_memalign, sizeof(struct nm_ibverbs_lr2_s));
  else
    p_lr2 = malloc(sizeof(struct nm_ibverbs_lr2_s));
  memset(&p_lr2->buffer, 0, sizeof(p_lr2->buffer));
  p_lr2->buffer.rack = 0;
  p_lr2->mr = NULL;
  p_lr2->context = context;
  p_lr2->p_cnx = NULL;
  return p_lr2;
}

static void nm_ibverbs_lr2_destroy(void*_status)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  if(p_lr2->p_cnx)
    {
      nm_ibverbs_cnx_close(p_lr2->p_cnx);
    }
  if(p_lr2->mr)
    {
      ibv_dereg_mr(p_lr2->mr);
    }
  free(p_lr2);
}

/* *** lr2 connection ************************************** */

static void nm_ibverbs_lr2_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  struct nm_ibverbs_lr2_context_s*p_lr2_context = malloc(sizeof(struct nm_ibverbs_lr2_context_s));
  puk_context_set_status(context, p_lr2_context);
  p_lr2_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_lr2_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data = 1;
  p_props->capabilities.max_msg_size = 0; /* unlimited */
  p_props->capabilities.trk_rdv = 1;
}

static void nm_ibverbs_lr2_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{ 
  const char*url = NULL;
  struct nm_ibverbs_lr2_context_s*p_lr2_context = puk_context_get_status(context);
  p_lr2_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
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
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  struct nm_ibverbs_lr2_context_s*p_lr2_context = puk_context_get_status(p_lr2->context);
  p_lr2->p_cnx = nm_ibverbs_cnx_new(p_lr2_context->p_hca);
  p_lr2->mr = ibv_reg_mr(p_lr2_context->p_hca->pd, &p_lr2->buffer, sizeof(p_lr2->buffer),
                         IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  p_lr2->send.prefetch = NULL;
  if(p_lr2->mr == NULL)
    {
      NM_FATAL("Infiniband: lr2 cannot register MR.\n");
    }
  struct nm_ibverbs_segment_s*p_seg = &p_lr2->p_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_lr2->buffer;
  p_seg->rkey  = p_lr2->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_lr2->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_lr2->p_cnx->local_addr, &p_lr2->p_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_lr2->seg = p_lr2->p_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_lr2->p_cnx);
}

/* *** lr2 I/O ********************************************* */

static inline void nm_ibverbs_lr2_send_init(struct nm_ibverbs_lr2_s*p_lr2)
{
  assert(p_lr2->send.message == NULL);
  assert(nm_data_slicer_isnull(&p_lr2->send.slicer));
  p_lr2->send.done    = 0;
  p_lr2->send.rbuf    = p_lr2->buffer.rbuf;
  p_lr2->send.sbuf    = p_lr2->buffer.sbuf;
  p_lr2->send.step    = 0;
  p_lr2->send.nbuffer = 0;
  p_lr2->buffer.rack  = 0;
}
static void nm_ibverbs_lr2_send_iov_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  assert(n == 1);
  nm_ibverbs_lr2_send_init(p_lr2);
  p_lr2->send.message = v[0].iov_base;
  p_lr2->send.slicer  = NM_DATA_SLICER_NULL;
  p_lr2->send.size    = v[0].iov_len;
}
static void nm_ibverbs_lr2_send_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  assert(chunk_offset + chunk_len <= nm_data_size(p_data));
  nm_ibverbs_lr2_send_init(p_lr2);
  p_lr2->send.message = NULL;
  p_lr2->send.size    = chunk_len;
  nm_data_slicer_init(&p_lr2->send.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_lr2->send.slicer, chunk_offset);
}

static int nm_ibverbs_lr2_send_poll(void*_status)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  while(p_lr2->send.done < p_lr2->send.size)
    {
      const nm_len_t chunk_size = lr2_steps[p_lr2->send.step];
      const nm_len_t block_size = NM_IBVERBS_LR2_BLOCKSIZE;
      const nm_len_t block_max_payload = block_size - lr2_hsize;
      const nm_len_t chunk_max_payload = chunk_size - lr2_hsize * chunk_size / block_size;
      const nm_len_t chunk_payload = nm_ibverbs_min(p_lr2->send.size - p_lr2->send.done, chunk_max_payload);
      /* ** N-buffering */
      if((p_lr2->send.sbuf + chunk_size) > (((void*)p_lr2->buffer.sbuf) + (p_lr2->send.nbuffer + 1) * NM_IBVERBS_LR2_BUFSIZE))
	{
	  /* flow control rationale- receiver may be:
	   *   . reading buffer N -> ok for writing N + 1
	   *   . already be ready for N + 1 -> ok
	   *   . reading N - 1 (~= N + 2) -> wait
	   * */
	  const int n2 = (p_lr2->send.nbuffer + 2) % NM_IBVERBS_LR2_NBUF;
	  if(p_lr2->buffer.rack == n2)
	    { 
	      return -NM_EAGAIN;
	    }
	  /* swap buffers */
	  p_lr2->send.nbuffer = (p_lr2->send.nbuffer + 1) % NM_IBVERBS_LR2_NBUF;
	  p_lr2->send.sbuf = ((void*)p_lr2->buffer.sbuf) + p_lr2->send.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	  p_lr2->send.rbuf = ((void*)p_lr2->buffer.rbuf) + p_lr2->send.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	}
      /* ** fill buffer (one chunk made of multiple blocks) */
      nm_len_t chunk_offset = 0; /**< offset in the sbuf/rbuf, i.e. payload + headers */
      nm_len_t base_offset = 0;
      if((p_lr2->send.prefetch != NULL) && (p_lr2->send.prefetch == p_lr2->send.message) && (p_lr2->send.done == 0))
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
	      struct lr2_header_s*p_header = p_lr2->send.sbuf + chunk_offset + block_payload;
	      p_header->checksum = 1 | nm_ibverbs_copy_from_and_checksum(p_lr2->send.sbuf + chunk_offset, &p_lr2->send.slicer, p_lr2->send.message, p_lr2->send.done + (chunk_payload - chunk_todo), block_payload);
	      chunk_todo   -= block_payload;
	      chunk_offset += block_payload + lr2_hsize;
	    }
	}
      /* ** send chunk */
      nm_ibverbs_send_flushn(p_lr2->p_cnx, NM_IBVERBS_WRID_PACKET, 1);
      nm_ibverbs_rdma_send(p_lr2->p_cnx, chunk_offset - base_offset, p_lr2->send.sbuf + base_offset, p_lr2->send.rbuf + base_offset,
			   &p_lr2->buffer, &p_lr2->seg, p_lr2->mr, NM_IBVERBS_WRID_PACKET);
      p_lr2->send.done += chunk_payload;
      p_lr2->send.sbuf += chunk_offset;
      p_lr2->send.rbuf += chunk_offset;
      p_lr2->send.prefetch = NULL;
      if(p_lr2->send.step < lr2_nsteps - 1)
	p_lr2->send.step++;
    }
  nm_ibverbs_send_flush(p_lr2->p_cnx, NM_IBVERBS_WRID_PACKET);
  p_lr2->send.message = NULL;
  if(!nm_data_slicer_isnull(&p_lr2->send.slicer))
    {
      nm_data_slicer_destroy(&p_lr2->send.slicer);
      p_lr2->send.slicer = NM_DATA_SLICER_NULL;
    }
  return NM_ESUCCESS;
}

static void nm_ibverbs_lr2_send_prefetch(void*_status, const void*ptr, uint64_t size)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  if((p_lr2->send.prefetch == NULL) && (p_lr2->send.message == NULL))
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
	      struct lr2_header_s*p_header = (struct lr2_header_s*)(&p_lr2->buffer.sbuf[chunk_offset] + block_payload);
	      p_header->checksum = 1 | nm_ibverbs_memcpy_and_checksum(&p_lr2->buffer.sbuf[chunk_offset], ptr + chunk_done, block_payload);
	      chunk_done   += block_payload;
              chunk_offset += block_size;
	    }
	  p_lr2->send.prefetch = ptr;
	}
    }
}

static void nm_ibverbs_lr2_recv_iov_post(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  p_lr2->recv.done    = 0;
  p_lr2->recv.message = v->iov_base;
  p_lr2->recv.slicer  = NM_DATA_SLICER_NULL;
  p_lr2->recv.size    = v->iov_len;
  p_lr2->recv.rbuf    = p_lr2->buffer.rbuf;
  p_lr2->recv.step    = 0;
  p_lr2->recv.nbuffer = 0;
}

static void nm_ibverbs_lr2_recv_data_post(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  assert(chunk_offset + chunk_len <= nm_data_size(p_data));
  p_lr2->recv.done    = 0;
  p_lr2->recv.message = NULL;
  p_lr2->recv.size    = chunk_len;
  p_lr2->recv.rbuf    = p_lr2->buffer.rbuf;
  p_lr2->recv.step    = 0;
  p_lr2->recv.nbuffer = 0;
  nm_data_slicer_init(&p_lr2->recv.slicer, p_data);
  if(chunk_offset > 0)
    nm_data_slicer_forward(&p_lr2->recv.slicer, chunk_offset);
}

static int nm_ibverbs_lr2_recv_poll_one(void*_status)
{
  struct nm_ibverbs_lr2_s*p_lr2 = _status;
  while(p_lr2->recv.done < p_lr2->recv.size)
    {
      const nm_len_t chunk_size = lr2_steps[p_lr2->recv.step];
      const nm_len_t block_size = NM_IBVERBS_LR2_BLOCKSIZE;
      const nm_len_t block_max_payload = block_size - lr2_hsize;
      const nm_len_t chunk_max_payload = chunk_size - lr2_hsize * chunk_size / block_size;
      const nm_len_t chunk_payload = nm_ibverbs_min(p_lr2->recv.size - p_lr2->recv.done, chunk_max_payload);
      if((p_lr2->recv.rbuf + chunk_size) > (((void*)p_lr2->buffer.rbuf) + (p_lr2->recv.nbuffer + 1) * NM_IBVERBS_LR2_BUFSIZE))
      {
	/* swap buffers */
	p_lr2->recv.nbuffer = (p_lr2->recv.nbuffer + 1) % NM_IBVERBS_LR2_NBUF;
	p_lr2->recv.rbuf = ((void*)p_lr2->buffer.rbuf) + p_lr2->recv.nbuffer * NM_IBVERBS_LR2_BUFSIZE;
	nm_ibverbs_send_flush(p_lr2->p_cnx, NM_IBVERBS_WRID_ACK);
	p_lr2->buffer.sack = p_lr2->recv.nbuffer;
	nm_ibverbs_rdma_send(p_lr2->p_cnx, sizeof(uint32_t),
			     (void*)&p_lr2->buffer.sack, (void*)&p_lr2->buffer.rack,
			     &p_lr2->buffer, &p_lr2->seg, p_lr2->mr, NM_IBVERBS_WRID_ACK);
      }
      nm_len_t chunk_todo = chunk_payload;
      nm_len_t chunk_offset = (chunk_max_payload - chunk_payload) % block_max_payload;
      while(chunk_todo > 0)
	{
	  const nm_len_t block_payload = (chunk_todo % block_max_payload == 0) ?
	    block_max_payload : (chunk_todo % block_max_payload);
	  struct lr2_header_s*p_header = p_lr2->recv.rbuf + chunk_offset + block_payload;
	  if((chunk_todo == chunk_payload) && !p_header->checksum)
	    goto wouldblock;
	  else
	    while(!p_header->checksum)
	      {
	      }
	  const uint32_t checksum =
	    1 | nm_ibverbs_copy_to_and_checksum(p_lr2->recv.rbuf + chunk_offset, &p_lr2->recv.slicer, p_lr2->recv.message,
						p_lr2->recv.done, block_payload);
	  if(p_header->checksum != checksum)
	    {
	      NM_FATAL("ibverbs: checksum failed; step = %d; done = %d / %d;  received = %llX; expected = %llX.\n",
                       p_lr2->recv.step, (int)p_lr2->recv.done, (int)p_lr2->recv.size, 
                       (long long unsigned)p_header->checksum, (long long unsigned)checksum);
	    }
    	  p_header->checksum = 0;
	  chunk_todo -= block_payload;
	  p_lr2->recv.done += block_payload;
	  p_lr2->recv.rbuf += chunk_offset + block_payload + lr2_hsize;
	  chunk_offset = 0;
	}
      if(p_lr2->recv.step < lr2_nsteps - 1)
	p_lr2->recv.step++;
    }
  p_lr2->recv.message = NULL;
  if(!nm_data_slicer_isnull(&p_lr2->recv.slicer))
    {
      nm_data_slicer_destroy(&p_lr2->recv.slicer);
      p_lr2->recv.slicer = NM_DATA_SLICER_NULL;
    }
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

