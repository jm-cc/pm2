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

/* *** method: 'lr2' *************************************** */

#define NM_IBVERBS_LR2_BUFSIZE (16*1024*1024)

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
    { 160*1024, 8192 },
    { 200*1024, 8192 },
    { 288*1024, 8192 },
    { 0, 0}
  };
static const int lr2_nsteps = sizeof(lr2_steps) / sizeof(struct lr2_step_s) - 1;

/** on the wire header of method 'lr2' */
struct lr2_header_s
{
#ifdef NM_IBVERBS_CHECKSUM
  volatile uint64_t checksum;
#endif
  volatile uint32_t busy; /* 'busy' has to be the last field in the struct */
} __attribute__((packed));

/** Connection state for tracks sending with lr2 */
struct nm_ibverbs_lr2
{  
  struct ibv_mr*mr;
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
  struct
  {
    char sbuf[NM_IBVERBS_LR2_BUFSIZE];
    char rbuf[NM_IBVERBS_LR2_BUFSIZE];
  } buffer;

  struct
  {
    const char*message;
    int todo;
    int done;
    void*rbuf;
    void*sbuf;
    int step;
  } send;

  struct
  {
    char*message;
    int size;
    void*rbuf;
    int done;
    int step;
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
PADICO_MODULE_BUILTIN(NewMad_ibverbs_lr2, &nm_ibverbs_lr2_load, NULL, NULL);

/* ********************************************************* */

static void* nm_ibverbs_lr2_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_lr2*lr2 = TBX_MALLOC(sizeof(struct nm_ibverbs_lr2));
  memset(&lr2->buffer, 0, sizeof(lr2->buffer));
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
  lr2->send.todo    = v[0].iov_len;
  lr2->send.done    = 0;
  lr2->send.rbuf    = lr2->buffer.rbuf;
  lr2->send.sbuf    = lr2->buffer.sbuf;
  lr2->send.step    = 0;
}

static int nm_ibverbs_lr2_send_poll(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;

  while(lr2->send.todo > 0)
    {
      const int chunk_size = lr2_steps[lr2->send.step].chunk_size;
      const int block_size = lr2_steps[lr2->send.step].block_size;
      const int chunk_payload_max = chunk_size - sizeof(struct lr2_header_s) * chunk_size / block_size;
      const int chunk_payload = lr2->send.todo > chunk_payload_max ? chunk_payload_max : lr2->send.todo;
      int chunk_todo = chunk_payload;
      void*base_sbuf = lr2->send.sbuf;
      void*base_rbuf = lr2->send.rbuf;
      while(chunk_todo > 0)
	{
	  const int block_payload = (chunk_todo > block_size - sizeof(struct lr2_header_s)) ? 
	    (block_size - sizeof(struct lr2_header_s)) : chunk_todo;
	  memcpy(lr2->send.sbuf, &lr2->send.message[lr2->send.done + chunk_payload - chunk_todo], block_payload);
#ifdef NM_IBVERBS_CHECKSUM
	  const uint64_t checksum = nm_ibverbs_checksum(lr2->send.sbuf, block_payload);
#endif
	  lr2->send.sbuf += block_payload;
	  lr2->send.rbuf += block_payload;
	  chunk_todo -= block_payload;
	  struct lr2_header_s*h = lr2->send.sbuf;
	  h->busy = 1;
#ifdef NM_IBVERBS_CHECKSUM
	  h->checksum = checksum;
#endif
	  lr2->send.sbuf += sizeof(struct lr2_header_s);
	  lr2->send.rbuf += sizeof(struct lr2_header_s);
	}
      while(lr2->cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 1)
	{
	  nm_ibverbs_rdma_poll(lr2->cnx);
	}
      nm_ibverbs_rdma_send(lr2->cnx, lr2->send.sbuf - base_sbuf, base_sbuf, base_rbuf,
		       &lr2->buffer,
		       &lr2->seg,
		       lr2->mr,
		       NM_IBVERBS_WRID_PACKET);
      lr2->send.done += chunk_payload;
      lr2->send.todo -= chunk_payload;
      if(lr2->send.step < lr2_nsteps - 1)
	lr2->send.step++;
    }
  while(lr2->cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0)
    {
      nm_ibverbs_rdma_poll(lr2->cnx);
    }
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
}


static int nm_ibverbs_lr2_poll_one(void*_status)
{
  struct nm_ibverbs_lr2*lr2 = _status;

  while(lr2->recv.done < lr2->recv.size)
    {
      const int chunk_size = lr2_steps[lr2->recv.step].chunk_size;
      const int block_size = lr2_steps[lr2->recv.step].block_size;
      const int message_todo = lr2->recv.size - lr2->recv.done;
      const int chunk_payload_max = chunk_size - sizeof(struct lr2_header_s) * chunk_size / block_size;
      const int chunk_payload = message_todo > chunk_payload_max ? chunk_payload_max : message_todo;
      int chunk_todo = chunk_payload;
      while(chunk_todo > 0)
	{
	  const int block_payload = (chunk_todo > block_size - sizeof(struct lr2_header_s)) ? 
	    (block_size - sizeof(struct lr2_header_s)) : chunk_todo;
	  struct lr2_header_s*h = lr2->recv.rbuf + block_payload;
	  if((chunk_todo == chunk_payload) && !h->busy)
	    goto wouldblock;
	  else
	    while(!h->busy)
	      {
	      }
	  memcpy(&lr2->recv.message[lr2->recv.done], lr2->recv.rbuf, block_payload);
#ifdef NM_IBVERBS_CHECKSUM
	  const uint64_t checksum = nm_ibverbs_checksum(lr2->recv.rbuf, block_payload);
	  if(h->checksum != checksum)
	    {
	      fprintf(stderr, "Infiniband: lr2- checksum failed; step = %d; received = %llX; expected = %llX.\n",
		      lr2->recv.step, h->checksum, checksum);
	      abort();
	    }
#endif
    	  h->busy = 0;
	  chunk_todo -= block_payload;
	  lr2->recv.done += block_payload;
	  lr2->recv.rbuf += block_payload + sizeof(struct lr2_header_s);
	}
      if(lr2->recv.step < lr2_nsteps - 1)
	lr2->recv.step++;
    }
  lr2->recv.message = NULL;
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

