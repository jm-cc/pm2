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

#include <Padico/Module.h>

static int nm_ibverbs_lr2_load(void);

PADICO_MODULE_BUILTIN(NewMad_ibverbs_lr2, &nm_ibverbs_lr2_load, NULL, NULL);


/* *** method: 'lr2' *************************************** */

#define NM_IBVERBS_LR2_BUFSIZE (512*1024)
#define NM_IBVERBS_LR2_NBUF 3

struct lr2_step_s
{
  int chunk_size;
  int block_size;
};

static const struct lr2_step_s lr2_steps[] =
  {
    {  6*1024, 1024 },
    { 16*1024, 1024 },
    { 24*1024, 4096 },
    { 40*1024, 8192 },
    { 64*1024, 8192 },
    { 88*1024, 8192 },
    { 128*1024, 8192 },
    /*
      { 160*1024, 8192 },
      { 200*1024, 8192 },
      { 256*1024, 8192 },
    */
    { 0, 0}
  };
static const int lr2_nsteps = sizeof(lr2_steps) / sizeof(struct lr2_step_s) - 1;

/** on the wire header of method 'lr2' */
struct lr2_header_s
{
#ifdef NM_IBVERBS_CHECKSUM
  volatile uint32_t checksum;
#endif
  volatile uint32_t busy; /* 'busy' has to be the last field in the struct */
} __attribute__((packed));

static const int lr2_hsize = sizeof(struct lr2_header_s);

static inline int nm_ibverbs_min(const int a, const int b)
{
  if(b > a)
    return a;
  else
    return b;
}


/** Connection state for tracks sending with lr2 */
struct nm_ibverbs_lr2
{  
  struct ibv_mr*mr;
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
  struct
  {
    char sbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    char rbuf[NM_IBVERBS_LR2_BUFSIZE * NM_IBVERBS_LR2_NBUF];
    volatile uint32_t rack, sack;
  } buffer;

  struct
  {
    const char*message;
    int size;
    int done;
    void*rbuf;
    void*sbuf;
    int step;
    int nbuffer;
  } send;

  struct
  {
    char*message;
    int size;
    void*rbuf;
    int done;
    int step;
    int nbuffer;
  } recv;
};

static void nm_ibverbs_lr2_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv);
static void nm_ibverbs_lr2_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_lr2_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_lr2_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_lr2_send_poll(void*_status);
static void nm_ibverbs_lr2_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_lr2_poll_one(void*_status);

static const struct nm_ibverbs_method_iface_s nm_ibverbs_lr2_method =
  {
    .cnx_create  = &nm_ibverbs_lr2_cnx_create,
    .addr_pack   = &nm_ibverbs_lr2_addr_pack,
    .addr_unpack = &nm_ibverbs_lr2_addr_unpack,
    .send_post   = &nm_ibverbs_lr2_send_post,
    .send_poll   = &nm_ibverbs_lr2_send_poll,
    .recv_init   = &nm_ibverbs_lr2_recv_init,
    .poll_one    = &nm_ibverbs_lr2_poll_one,
    .poll_any    = NULL,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_lr2_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_lr2_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_lr2_adapter =
  {
    .instanciate = &nm_ibverbs_lr2_instanciate,
    .destroy = &nm_ibverbs_lr2_destroy
  };

/* ********************************************************* */

static int nm_ibverbs_lr2_load(void)
{
  puk_component_declare("NewMad_ibverbs_lr2",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_lr2_adapter),
			puk_component_provides("NewMad_ibverbs_method", "method", &nm_ibverbs_lr2_method));
  return 0;
}

/* ********************************************************* */

static void* nm_ibverbs_lr2_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_lr2*lr2 = TBX_MALLOC(sizeof(struct nm_ibverbs_lr2));
  memset(&lr2->buffer, 0, sizeof(lr2->buffer));
  lr2->buffer.rack = 0;
  lr2->mr = NULL;
  return lr2;
}

static void nm_ibverbs_lr2_destroy(void*_status)
{
  TBX_FREE(_status);
  /* TODO */
}

/* *** lr2 connection ************************************** */

static void nm_ibverbs_lr2_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  lr2->cnx = p_ibverbs_cnx;
  lr2->mr = ibv_reg_mr(p_ibverbs_drv->pd, &lr2->buffer, sizeof(lr2->buffer),
		       IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(lr2->mr == NULL)
    {
      TBX_FAILURE("Infiniband: lr2 cannot register MR.\n");
    }
}

static void nm_ibverbs_lr2_addr_pack(void*_status,  struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  struct nm_ibverbs_segment*seg = &addr->segments[addr->n];
  seg->kind  = NM_IBVERBS_CNX_LR2;
  seg->raddr = (uintptr_t)&lr2->buffer;
  seg->rkey  = lr2->mr->rkey;
  addr->n++;
}

static void nm_ibverbs_lr2_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  int i;
  for(i = 0; addr->segments[i].raddr; i++)
    {
      struct nm_ibverbs_segment*seg = &addr->segments[i];
      if(seg->kind == NM_IBVERBS_CNX_LR2)
	{
	  lr2->seg = *seg;
	  break;
	}
    }
}

/* *** lr2 I/O ********************************************* */

static void nm_ibverbs_lr2_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  assert(n == 1);
  lr2->send.message = v[0].iov_base;
  lr2->send.size    = v[0].iov_len;
  lr2->send.done    = 0;
  lr2->send.rbuf    = lr2->buffer.rbuf;
  lr2->send.sbuf    = lr2->buffer.sbuf;
  lr2->send.step    = 0;
  lr2->send.nbuffer = 0;
  lr2->buffer.rack  = 0;
}

static int nm_ibverbs_lr2_send_poll(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;

  while(lr2->send.done < lr2->send.size)
    {
      const int chunk_size = lr2_steps[lr2->send.step].chunk_size;
      const int block_size = lr2_steps[lr2->send.step].block_size;
      const int chunk_payload = nm_ibverbs_min(lr2->send.size - lr2->send.done, chunk_size - lr2_hsize * chunk_size / block_size);
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
      int chunk_done = 0;
      int chunk_offset = 0; /**< offset in the sbuf/rbuf, i.e. payload + headers */
      while(chunk_done < chunk_payload)
	{
	  const int block_payload = nm_ibverbs_min(chunk_payload - chunk_done, block_size - lr2_hsize);
	  memcpy(lr2->send.sbuf + chunk_offset, lr2->send.message + lr2->send.done + chunk_done, block_payload);
	  struct lr2_header_s*h = lr2->send.sbuf + chunk_offset + block_payload;
	  h->busy = 1;
#ifdef NM_IBVERBS_CHECKSUM
	  h->checksum = nm_ibverbs_checksum(lr2->send.sbuf + chunk_offset, block_payload);
#endif
	  chunk_done   += block_payload;
	  chunk_offset += block_payload + lr2_hsize;
	}
      nm_ibverbs_send_flushn(lr2->cnx, NM_IBVERBS_WRID_PACKET, 1);
      nm_ibverbs_rdma_send(lr2->cnx, chunk_offset, lr2->send.sbuf, lr2->send.rbuf,
			   &lr2->buffer, &lr2->seg, lr2->mr, NM_IBVERBS_WRID_PACKET);
      lr2->send.done += chunk_payload;
      lr2->send.sbuf += chunk_offset;
      lr2->send.rbuf += chunk_offset;
      if(lr2->send.step < lr2_nsteps - 1)
	lr2->send.step++;
    }
  nm_ibverbs_send_flush(lr2->cnx, NM_IBVERBS_WRID_PACKET);
  return NM_ESUCCESS;
}

static void nm_ibverbs_lr2_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_lr2*lr2 = _status;
  lr2->recv.done    = 0;
  lr2->recv.message = v->iov_base;
  lr2->recv.size    = v->iov_len;
  lr2->recv.rbuf    = lr2->buffer.rbuf;
  lr2->recv.step    = 0;
  lr2->recv.nbuffer = 0;
}

static int nm_ibverbs_lr2_poll_one(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;

  while(lr2->recv.done < lr2->recv.size)
    {
      const int chunk_size = lr2_steps[lr2->recv.step].chunk_size;
      const int block_size = lr2_steps[lr2->recv.step].block_size;
      const int chunk_payload = nm_ibverbs_min(lr2->recv.size - lr2->recv.done, chunk_size - lr2_hsize * chunk_size / block_size);
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
      int chunk_todo = chunk_payload;
      while(chunk_todo > 0)
	{
	  const int block_payload = nm_ibverbs_min(chunk_todo, block_size - lr2_hsize);
	  struct lr2_header_s*h = lr2->recv.rbuf + block_payload;
	  if((chunk_todo == chunk_payload) && !h->busy)
	    goto wouldblock;
	  else
	    while(!h->busy)
	      {
	      }
	  memcpy(lr2->recv.message + lr2->recv.done, lr2->recv.rbuf, block_payload);
#ifdef NM_IBVERBS_CHECKSUM
	  const uint64_t checksum = nm_ibverbs_checksum(lr2->recv.message + lr2->recv.done, block_payload);
	  if(h->checksum != checksum)
	    {
	      fprintf(stderr, "Infiniband: lr2- checksum failed; step = %d; done = %d / %d;  received = %llX; expected = %llX.\n",
		      lr2->recv.step, lr2->recv.done, lr2->recv.size, 
		      (long long unsigned)h->checksum, (long long unsigned)checksum);
	      abort();
	    }
    	  h->checksum = 0;
#endif
    	  h->busy = 0;
	  memset(lr2->recv.rbuf, 0, block_payload);
	  chunk_todo -= block_payload;
	  lr2->recv.done += block_payload;
	  lr2->recv.rbuf += block_payload + lr2_hsize;
	}
      if(lr2->recv.step < lr2_nsteps - 1)
	lr2->recv.step++;
    }
  lr2->recv.message = NULL;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

