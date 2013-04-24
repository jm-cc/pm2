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

#ifdef PUKABI
#include <Padico/Puk-ABI.h>
#endif /* PUKABI */

#include <Padico/Module.h>


/* *** method: 'rcache' ********************************* */

/** Header sent by the receiver to ask for RDMA data (rcache rdv) */
struct nm_ibverbs_rcache_rdvhdr 
{
  uint64_t raddr;
  uint32_t rkey;
  volatile int busy;
};

/** Header sent to signal presence of rcache data */
struct nm_ibverbs_rcache_sighdr 
{
  volatile int busy;
};

/** Connection state for 'rcache' tracks */
struct nm_ibverbs_rcache
{
  struct ibv_mr*mr;              /**< global MR (used for headers) */
  struct ibv_pd*pd;              /**< protection domain */
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
  puk_context_t context;
  struct
  {
    char*message;
    int size;
#ifdef PUKABI
    const struct puk_mem_reg_s*reg;
#else
    struct ibv_mr*mr;
#endif /* PUKABI */
  } recv;
  struct
  {
    const char*message;
    int size;
#ifdef PUKABI
    const struct puk_mem_reg_s*reg;
#else
    struct ibv_mr*mr;
#endif /* PUKABI */
  } send;
  struct
  {
    struct nm_ibverbs_rcache_rdvhdr shdr, rhdr;
    struct nm_ibverbs_rcache_sighdr ssig, rsig;
  } headers;
};

static void nm_ibverbs_rcache_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_rcache_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_rcache_send_poll(void*_status);
static void nm_ibverbs_rcache_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_rcache_poll_one(void*_status);

static const struct nm_ibverbs_method_iface_s nm_ibverbs_rcache_method =
  {
    .init        = &nm_ibverbs_rcache_init,
    .connect     = &nm_ibverbs_rcache_connect,
    .send_post   = &nm_ibverbs_rcache_send_post,
    .send_poll   = &nm_ibverbs_rcache_send_poll,
    .recv_init   = &nm_ibverbs_rcache_recv_init,
    .poll_one    = &nm_ibverbs_rcache_poll_one,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_rcache_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_rcache_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_rcache_adapter =
  {
    .instanciate = &nm_ibverbs_rcache_instanciate,
    .destroy = &nm_ibverbs_rcache_destroy
  };


static void*nm_ibverbs_mem_reg(void*context, const void*ptr, size_t len)
{
  struct ibv_pd*pd = context;
  struct ibv_mr*mr = ibv_reg_mr(pd, (void*)ptr, len, 
				IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(mr == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: error while registering memory- ptr = %p; len = %d.\n", ptr, (int)len);
      abort();
    }
  return mr;
}
static void nm_ibverbs_mem_unreg(void*context, const void*ptr, void*key)
{
  struct ibv_mr*mr = key;
  int rc = ibv_dereg_mr(mr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: error while deregistering memory.\n");
      abort();
    }
}


PADICO_MODULE_COMPONENT(NewMad_ibverbs_rcache,
  puk_component_declare("NewMad_ibverbs_rcache",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_rcache_adapter),
			puk_component_provides("NewMad_ibverbs_method", "method", &nm_ibverbs_rcache_method)));


static void* nm_ibverbs_rcache_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_rcache*rcache = TBX_MALLOC(sizeof(struct nm_ibverbs_rcache));
 /* init state */
  memset(&rcache->headers, 0, sizeof(rcache->headers));
  memset(&rcache->send, 0, sizeof(rcache->send));
  memset(&rcache->recv, 0, sizeof(rcache->recv));
  rcache->mr  = NULL;
  rcache->cnx = NULL;
  rcache->pd  = NULL;
  rcache->context = context;
#ifdef PUKABI
  puk_mem_set_handlers(&nm_ibverbs_mem_reg, &nm_ibverbs_mem_unreg);
#endif /* PUKABI */
  return rcache;
}

static void nm_ibverbs_rcache_destroy(void*_status)
{
  /* TODO */
}

/* *** rcache connection *********************************** */

static void nm_ibverbs_rcache_init(puk_context_t context, const void**drv_url, size_t*url_size)
{ 
  const char*url = NULL;
  nm_ibverbs_connect_create(&url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_rcache*rcache = _status; 
  const char*s_index = puk_context_getattr(rcache->context, "index");
  const int index= atoi(s_index);
  struct nm_ibverbs_hca_s*p_hca = nm_ibverbs_hca_resolve(index);
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_cnx_new(p_hca);
  rcache->cnx = p_ibverbs_cnx;
  rcache->pd = p_hca->pd;
  /* register Memory Region */
  rcache->mr = ibv_reg_mr(p_hca->pd, &rcache->headers, sizeof(rcache->headers),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(rcache->mr == NULL)
    {
      TBX_FAILURE("Infiniband: rcache cannot register MR.\n");
    }
  struct nm_ibverbs_segment*seg = &p_ibverbs_cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&rcache->headers;
  seg->rkey  = rcache->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(rcache->context, "local_url");
  int rc = nm_ibverbs_connect_exchange(local_url, remote_url,
				       &p_ibverbs_cnx->local_addr, &p_ibverbs_cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: timeout in address exchange.\n");
    }
  rcache->seg = p_ibverbs_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_ibverbs_cnx);
}


/* ** reg cache I/O **************************************** */

static void nm_ibverbs_rcache_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_rcache*rcache = _status;
  char*message = v[0].iov_base;
  const int size = v[0].iov_len;
  assert(n == 1);
  if(rcache->send.message != NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: rendez-vous already posted on sender side.\n");
      abort();
    }
  rcache->send.message = message;
  rcache->send.size = size;
#ifdef PUKABI
  rcache->send.reg = puk_mem_reg(rcache->pd, message, size);
#else /* PUKABI */
  rcache->send.mr = nm_ibverbs_mem_reg(rcache->pd, message, size);
#endif /* PUKABI */
}

static int nm_ibverbs_rcache_send_poll(void*_status)
{
  struct nm_ibverbs_rcache*rcache = _status;
  struct nm_ibverbs_rcache_rdvhdr*const h = &rcache->headers.rhdr;
  if(h->busy)
    {
      const uint64_t raddr = h->raddr;
      const uint32_t rkey  = h->rkey;
      struct ibv_mr*mr = NULL;
#ifdef PUKABI
      mr = rcache->send.reg->key;
#else
      mr = rcache->send.mr;      
#endif
      h->raddr = 0;
      h->rkey  = 0;
      h->busy  = 0;
      nm_ibverbs_do_rdma(rcache->cnx, 
			 rcache->send.message, rcache->send.size,
			 raddr, IBV_WR_RDMA_WRITE, IBV_SEND_SIGNALED,
			 mr->lkey, rkey, NM_IBVERBS_WRID_DATA);
      rcache->headers.ssig.busy = 1;
      nm_ibverbs_rdma_send(rcache->cnx, sizeof(struct nm_ibverbs_rcache_sighdr),
			   &rcache->headers.ssig,
			   &rcache->headers.rsig,
			   &rcache->headers,
			   &rcache->seg,
			   rcache->mr,
			   NM_IBVERBS_WRID_HEADER);
      nm_ibverbs_send_flush(rcache->cnx, NM_IBVERBS_WRID_DATA);
      nm_ibverbs_send_flush(rcache->cnx, NM_IBVERBS_WRID_HEADER);
#ifdef PUKABI
      puk_mem_unreg(rcache->send.reg);
      rcache->send.reg = NULL;
#else /* PUKABI */
      nm_ibverbs_mem_unreg(rcache->pd, rcache->send.message, rcache->send.mr);
      rcache->send.mr = NULL;
#endif /* PUKABI */
      rcache->send.message = NULL;
      rcache->send.size    = -1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static void nm_ibverbs_rcache_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_rcache*rcache = _status;
  if(rcache->recv.message != NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: rendez-vous already posted on receiver side.\n");
      abort();
    }
  rcache->headers.rsig.busy = 0;
  rcache->recv.message = v->iov_base;
  rcache->recv.size = v->iov_len;
  struct nm_ibverbs_rcache_rdvhdr*const h = &rcache->headers.shdr;
  h->raddr =  (uintptr_t)rcache->recv.message;
  h->busy  = 1;
#ifdef PUKABI
  rcache->recv.reg = puk_mem_reg(rcache->pd, rcache->recv.message, rcache->recv.size);
  struct ibv_mr*mr = rcache->recv.reg->key;
  h->rkey  = mr->rkey;
#else /* PUKABI */
  rcache->recv.mr = nm_ibverbs_mem_reg(rcache->pd, rcache->recv.message, rcache->recv.size);
  h->rkey  = rcache->recv.mr->rkey;
#endif /* PUKABI */

  nm_ibverbs_rdma_send(rcache->cnx, sizeof(struct nm_ibverbs_rcache_rdvhdr),
		       &rcache->headers.shdr, 
		       &rcache->headers.rhdr,
		       &rcache->headers,
		       &rcache->seg,
		       rcache->mr,
		       NM_IBVERBS_WRID_RDV);
  nm_ibverbs_send_flush(rcache->cnx, NM_IBVERBS_WRID_RDV);
}

static int nm_ibverbs_rcache_poll_one(void*_status)
{
  struct nm_ibverbs_rcache*rcache = _status;
  struct nm_ibverbs_rcache_sighdr*rsig = &rcache->headers.rsig;
  if(rsig->busy)
    {
      rsig->busy = 0;
#ifdef PUKABI
      puk_mem_unreg(rcache->recv.reg);
      rcache->recv.reg = NULL;
#else /* PUKABI */
      nm_ibverbs_mem_unreg(rcache->pd, rcache->recv.message, rcache->recv.mr);
      rcache->recv.mr = NULL;
#endif /* PUKABI */
      rcache->recv.message = NULL;
      rcache->recv.size = -1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}
