/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include <Padico/Module.h>

static int nm_ibverbs_regrdma_load(void);

PADICO_MODULE_BUILTIN(NewMad_ibverbs_regrdma, &nm_ibverbs_regrdma_load, NULL, NULL);

/* *** method: 'regrdma' *********************************** */

static const int nm_ibverbs_regrdma_frag_base    = 256 * 1024;
static const int nm_ibverbs_regrdma_frag_inc     = 512 * 1024;
static const int nm_ibverbs_regrdma_frag_overrun = 64  * 1024;
static const int nm_ibverbs_regrdma_frag_max     = 4096 * 1024;

struct nm_ibverbs_regrdma_cell
{
  struct ibv_mr*mr;
  int packet_size;
  int offset;
};

/** Header sent to ask for RDMA data (regrdma rdv) */
struct nm_ibverbs_regrdma_rdvhdr 
{
  uint64_t raddr;
  uint32_t rkey;
  volatile int busy;
};

/** Header sent to signal presence of regrdma data */
struct nm_ibverbs_regrdma_sighdr 
{
  int k;
  volatile int busy;
};

/** Connection state for tracks sending with on-the-fly memory registration */
struct nm_ibverbs_regrdma 
{
  struct ibv_mr*mr;              /**< global MR (used for 'headers') */
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
  struct {
    char*message;
    int todo;
    int max_cell;
    struct nm_ibverbs_regrdma_cell*cells;
    int k;
    int pending_reg;
    int frag_size;
  } recv;
  
  struct {
    const char*message;
    int size;
    int todo;
    int max_cell;
    struct nm_ibverbs_regrdma_cell*cells;
    int k;
    int pending_reg;
    int pending_packet;
    int frag_size;
  } send;
  
  struct {
    struct nm_ibverbs_regrdma_rdvhdr shdr, rhdr[2];
    struct nm_ibverbs_regrdma_sighdr ssig, rsig[2];
  } headers;
};

static void nm_ibverbs_regrdma_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv);
static void nm_ibverbs_regrdma_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_regrdma_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_regrdma_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_regrdma_send_poll(void*_status);
static void nm_ibverbs_regrdma_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_regrdma_poll_one(void*_status);

static const struct nm_ibverbs_method_iface_s nm_ibverbs_regrdma_method =
  {
    .cnx_create  = &nm_ibverbs_regrdma_cnx_create,
    .addr_pack   = &nm_ibverbs_regrdma_addr_pack,
    .addr_unpack = &nm_ibverbs_regrdma_addr_unpack,
    .send_post   = &nm_ibverbs_regrdma_send_post,
    .send_poll   = &nm_ibverbs_regrdma_send_poll,
    .recv_init   = &nm_ibverbs_regrdma_recv_init,
    .poll_one    = &nm_ibverbs_regrdma_poll_one,
    .poll_any    = NULL,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_regrdma_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_regrdma_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_regrdma_adapter =
  {
    .instanciate = &nm_ibverbs_regrdma_instanciate,
    .destroy = &nm_ibverbs_regrdma_destroy
  };

static int nm_ibverbs_regrdma_load(void)
{
  puk_component_declare("NewMad_ibverbs_regrdma",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_regrdma_adapter),
			puk_component_provides("NewMad_ibverbs_method", "method", &nm_ibverbs_regrdma_method));
  return 0;
}

static void* nm_ibverbs_regrdma_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_regrdma*regrdma = TBX_MALLOC(sizeof(struct nm_ibverbs_regrdma));
  memset(&regrdma->headers, 0, sizeof(regrdma->headers));
  regrdma->mr = NULL;
  return regrdma;
}

static void nm_ibverbs_regrdma_destroy(void*_status)
{
  /* TODO */
}

static void nm_ibverbs_regrdma_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  regrdma->cnx = p_ibverbs_cnx;
  /* register Memory Region */
  regrdma->mr = ibv_reg_mr(p_ibverbs_drv->pd, &regrdma->headers,
			   sizeof(regrdma->headers),
			   IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(regrdma->mr == NULL)
    {
      TBX_FAILURE("Infiniband: regrdma cannot register MR.\n");
    }
}

static void nm_ibverbs_regrdma_addr_pack(void*_status,  struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  struct nm_ibverbs_segment*seg = &addr->segments[addr->n];
  seg->kind  = NM_IBVERBS_CNX_REGRDMA;
  seg->raddr = (uintptr_t)&regrdma->headers;
  seg->rkey  = regrdma->mr->rkey;
  addr->n++;
}

static void nm_ibverbs_regrdma_addr_unpack(void*_status,  struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  int i;
  for(i = 0; addr->segments[i].raddr; i++)
    {
      struct nm_ibverbs_segment*seg = &addr->segments[i];
      if(seg->kind == NM_IBVERBS_CNX_REGRDMA)
	{
	  regrdma->seg = *seg;
	  break;
	}
    }
}

/* ** regrdma I/O ****************************************** */

static inline int nm_ibverbs_regrdma_count(size_t size)
{
  return 1 /* first block */
    + (size - nm_ibverbs_regrdma_frag_base) / nm_ibverbs_regrdma_frag_inc /* blocks */
    + 2 /* rounding */
    + 1; /* guard to detect overrun */
}

static inline size_t nm_ibverbs_regrdma_step_next(size_t frag_size)
{
  const size_t s = (frag_size <= nm_ibverbs_regrdma_frag_max) ?
    (nm_ibverbs_regrdma_frag_inc + frag_size) :
    (nm_ibverbs_regrdma_frag_max);
  return s;
}

static inline void nm_ibverbs_regrdma_send_init(struct nm_ibverbs_regrdma*__restrict__ regrdma, const char*message, int size)
{
  regrdma->send.message        = message;
  regrdma->send.size           = size;
  regrdma->send.todo           = size;
  regrdma->send.k              = -1;
  regrdma->send.pending_reg    = 0;
  regrdma->send.pending_packet = 0;
  regrdma->send.frag_size      = nm_ibverbs_regrdma_frag_base;
  regrdma->send.max_cell       = nm_ibverbs_regrdma_count(size);
  regrdma->send.cells = TBX_MALLOC(sizeof(struct nm_ibverbs_regrdma_cell) * regrdma->send.max_cell);
  memset(regrdma->send.cells, 0, sizeof(struct nm_ibverbs_regrdma_cell) *   regrdma->send.max_cell);
}

static inline int nm_ibverbs_regrdma_send_done(struct nm_ibverbs_regrdma*__restrict__ regrdma)
{
  return (regrdma->send.todo == 0) && (regrdma->send.pending_reg == 0);
}

static inline int nm_ibverbs_regrdma_send_wouldblock(struct nm_ibverbs_regrdma*__restrict__ regrdma)
{
  const int k = regrdma->send.k;
  return (k >= 0 &&
	  regrdma->send.cells[regrdma->send.k].mr && 
	  !regrdma->headers.rhdr[k & 0x01].busy);
}

static inline void nm_ibverbs_regrdma_send_step_send(struct nm_ibverbs_regrdma*__restrict__ const regrdma)
{
  const int prev = regrdma->send.k - 1;
  if(regrdma->send.k >= 0 && regrdma->send.cells[regrdma->send.k].mr)
    {
      /* ** send current packet */
      const int k = regrdma->send.k;
      struct nm_ibverbs_regrdma_rdvhdr*const h = &regrdma->headers.rhdr[k & 0x01];
      assert(h->busy);
      const uint64_t raddr = h->raddr;
      const uint32_t rkey  = h->rkey;
      h->raddr = 0;
      h->rkey  = 0;
      h->busy  = 0;
      nm_ibverbs_send_flushn_total(regrdma->cnx, 2); /* TODO */
      nm_ibverbs_do_rdma(regrdma->cnx, 
			 regrdma->send.message + regrdma->send.cells[k].offset,
			 regrdma->send.cells[k].packet_size, raddr, IBV_WR_RDMA_WRITE,
			 (regrdma->send.cells[k].packet_size > regrdma->cnx->max_inline ?
			  IBV_SEND_SIGNALED : (IBV_SEND_SIGNALED | IBV_SEND_INLINE)),
			 regrdma->send.cells[k].mr->lkey, rkey, NM_IBVERBS_WRID_DATA);
      regrdma->send.pending_packet++;
      assert(regrdma->send.pending_packet <= 2);
      nm_ibverbs_send_flush(regrdma->cnx, NM_IBVERBS_WRID_HEADER);
      regrdma->headers.ssig.k    = k;
      regrdma->headers.ssig.busy = 1;
      nm_ibverbs_rdma_send(regrdma->cnx, sizeof(struct nm_ibverbs_regrdma_sighdr),
			   &regrdma->headers.ssig,
			   &regrdma->headers.rsig[k & 0x01],
			   &regrdma->headers,
			   &regrdma->seg,
			   regrdma->mr,
			   NM_IBVERBS_WRID_HEADER);
    }
  if(prev >= 0 && regrdma->send.cells[prev].mr) 
    {
      regrdma->send.pending_packet--;
      regrdma->send.pending_reg--;
      nm_ibverbs_send_flushn(regrdma->cnx, NM_IBVERBS_WRID_DATA, regrdma->send.pending_packet);
      ibv_dereg_mr(regrdma->send.cells[prev].mr);
      regrdma->send.cells[prev].mr = NULL;
    }
}

static inline void nm_ibverbs_regrdma_send_step_reg(struct nm_ibverbs_regrdma*__restrict__ const regrdma)
{
  if(regrdma->send.todo > 0) 
    {
      const int next = regrdma->send.k + 1;
      /* ** prepare next */
      regrdma->send.cells[next].packet_size = 
	regrdma->send.todo > (regrdma->send.frag_size + nm_ibverbs_regrdma_frag_overrun) ?
	regrdma->send.frag_size : regrdma->send.todo;
      regrdma->send.cells[next].offset = regrdma->send.size - regrdma->send.todo;
      regrdma->send.cells[next].mr     =
	ibv_reg_mr(regrdma->cnx->qp->pd, (void*)(regrdma->send.message + regrdma->send.cells[next].offset),
		   regrdma->send.cells[next].packet_size, IBV_ACCESS_LOCAL_WRITE);
      regrdma->send.pending_reg++;
      assert(regrdma->send.pending_reg <= 2);
      regrdma->send.todo -= regrdma->send.cells[next].packet_size;
      regrdma->send.frag_size = nm_ibverbs_regrdma_step_next(regrdma->send.frag_size);
    }
}

static void nm_ibverbs_regrdma_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  const char*message = v[0].iov_base;
  const int size = v[0].iov_len;
  assert(n == 1);
  nm_ibverbs_regrdma_send_init(regrdma, message, size);
}

static int nm_ibverbs_regrdma_send_poll(void*_status)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  if(!nm_ibverbs_regrdma_send_done(regrdma) &&
     !nm_ibverbs_regrdma_send_wouldblock(regrdma))
    {
      nm_ibverbs_regrdma_send_step_send(regrdma);
      nm_ibverbs_regrdma_send_step_reg(regrdma);
      regrdma->send.k++;
      if(regrdma->send.k >= regrdma->send.max_cell)
	{
	  TBX_FAILUREF("Infiniband: bad regrdma cells count while sending (k = %d; max = %d).\n",
		       regrdma->send.k, regrdma->send.max_cell);
	}
    }
  if(nm_ibverbs_regrdma_send_done(regrdma))
    {
      nm_ibverbs_send_flush(regrdma->cnx, NM_IBVERBS_WRID_HEADER);
      TBX_FREE(regrdma->send.cells);
      regrdma->send.cells = NULL;
      regrdma->send.max_cell = 0;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}


static void nm_ibverbs_regrdma_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  const int size = v->iov_len;
  regrdma->recv.message     = v->iov_base;
  regrdma->recv.todo        = size;
  regrdma->recv.max_cell    = nm_ibverbs_regrdma_count(size);
  regrdma->recv.cells       = TBX_MALLOC(sizeof(struct nm_ibverbs_regrdma_cell) * regrdma->recv.max_cell);
  memset(regrdma->recv.cells, 0, sizeof(struct nm_ibverbs_regrdma_cell) * regrdma->recv.max_cell);
  regrdma->recv.k           = 0;
  regrdma->recv.pending_reg = 0;
  regrdma->recv.frag_size   = nm_ibverbs_regrdma_frag_base;
}

static inline int nm_ibverbs_regrdma_recv_isdone(struct nm_ibverbs_regrdma*__restrict__ regrdma)
{
  return (regrdma->recv.todo <= 0 && regrdma->recv.pending_reg == 0);
}
static inline void nm_ibverbs_regrdma_recv_step_reg(struct nm_ibverbs_regrdma*__restrict__ regrdma)
{
  const int k = regrdma->recv.k;
  if(regrdma->recv.todo > 0 &&
     regrdma->recv.cells[k].mr == NULL)
    {
      const int packet_size = regrdma->recv.todo > (regrdma->recv.frag_size + nm_ibverbs_regrdma_frag_overrun) ?
	regrdma->recv.frag_size : regrdma->recv.todo;
      regrdma->recv.cells[k].mr = ibv_reg_mr(regrdma->cnx->qp->pd, 
					     regrdma->recv.message,
					     packet_size,
					     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      regrdma->recv.pending_reg++;
      nm_ibverbs_send_flush(regrdma->cnx, NM_IBVERBS_WRID_RDV);
      regrdma->headers.shdr.raddr = (uintptr_t)regrdma->recv.message;
      regrdma->headers.shdr.rkey  = regrdma->recv.cells[k].mr->rkey;
      regrdma->headers.shdr.busy  = 1;
      nm_ibverbs_rdma_send(regrdma->cnx, sizeof(struct nm_ibverbs_regrdma_rdvhdr),
			   &regrdma->headers.shdr, 
			   &regrdma->headers.rhdr[k & 0x01],
			   &regrdma->headers,
			   &regrdma->seg,
			   regrdma->mr,
			   NM_IBVERBS_WRID_RDV);
      regrdma->recv.todo     -= packet_size;
      regrdma->recv.message  += packet_size;
      regrdma->recv.frag_size = nm_ibverbs_regrdma_step_next(regrdma->recv.frag_size);
      nm_ibverbs_send_flush(regrdma->cnx, NM_IBVERBS_WRID_RDV);
    }
}
static inline void nm_ibverbs_regrdma_recv_step_dereg(struct nm_ibverbs_regrdma*__restrict__ regrdma)
{
  const int prev = regrdma->recv.k - 1;
  struct nm_ibverbs_regrdma_sighdr*rsig = &regrdma->headers.rsig[prev & 0x01];
  if( (prev >= 0) &&
      (regrdma->recv.cells[prev].mr) &&
      (rsig->busy) )
    {
      assert(rsig->k == prev);
      rsig->busy = 0;
      rsig->k    = 0xffff;
      ibv_dereg_mr(regrdma->recv.cells[prev].mr);
      regrdma->recv.pending_reg--;
      regrdma->recv.cells[prev].mr = NULL;
    }
  if(prev < 0 ||
     regrdma->recv.cells[prev].mr == NULL)
    {
      regrdma->recv.k++;
      if(regrdma->recv.k >= regrdma->recv.max_cell)
	{
	  TBX_FAILUREF("Infiniband: bad regrdma cells count while receiving (k=%d; max=%d).\n",
		       regrdma->recv.k, regrdma->recv.max_cell);
	}
    }
}

static int nm_ibverbs_regrdma_poll_one(void*_status)
{
  struct nm_ibverbs_regrdma*regrdma = _status;
  if(!nm_ibverbs_regrdma_recv_isdone(regrdma))
    {
      nm_ibverbs_regrdma_recv_step_reg(regrdma);
      nm_ibverbs_regrdma_recv_step_dereg(regrdma);
    }
  if(nm_ibverbs_regrdma_recv_isdone(regrdma)) 
    {
      TBX_FREE(regrdma->recv.cells);
      regrdma->recv.cells = NULL;
      regrdma->recv.max_cell = 0;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}
