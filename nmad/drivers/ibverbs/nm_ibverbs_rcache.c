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
#include <nm_connector.h>

#ifdef PUKABI
#include <Padico/Puk-ABI.h>
#else
#define MINI_PUKABI
#endif

#ifdef MINI_PUKABI
/** a registered memory entry */
struct puk_mem_reg_s
{
  void*context;
  const void*ptr;
  size_t len;
  void*key;
  int refcount;
};
/** hook to register memory- returned value is used as key */
typedef void*(*puk_mem_register_t)  (void*context, const void*ptr, size_t len);
/** hook to unregister memory */
typedef void (*puk_mem_unregister_t)(void*context, const void*ptr, void*key);
/** sets handlers to register memory */
void puk_mem_set_handlers(puk_mem_register_t reg, puk_mem_unregister_t unreg);
/** asks for memory registration- does nothing if registration has been cached */
const struct puk_mem_reg_s*puk_mem_reg(void*context, const void*ptr, size_t len);
/** marks registration entry as 'unused' */
void puk_mem_unreg(const struct puk_mem_reg_s*reg);
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

/** context for ibverbs 'rcache' */
struct nm_ibverbs_rcache_context_s
{
  struct nm_ibverbs_hca_s*p_hca;
  struct nm_connector_s*p_connector;
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
#if defined(PUKABI) || defined(MINI_PUKABI)
    const struct puk_mem_reg_s*reg;
#else
    struct ibv_mr*mr;
#endif /* PUKABI */
  } recv;
  struct
  {
    const char*message;
    int size;
#if defined(PUKABI) || defined(MINI_PUKABI)
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

static void nm_ibverbs_rcache_getprops(puk_context_t context, struct nm_minidriver_properties_s*props);
static void nm_ibverbs_rcache_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_rcache_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_rcache_send_poll(void*_status);
static void nm_ibverbs_rcache_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_rcache_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_rcache_minidriver =
  {
    .getprops    = &nm_ibverbs_rcache_getprops,
    .init        = &nm_ibverbs_rcache_init,
    .connect     = &nm_ibverbs_rcache_connect,
    .send_data   = NULL,
    .send_post   = &nm_ibverbs_rcache_send_post,
    .send_poll   = &nm_ibverbs_rcache_send_poll,
    .recv_init   = &nm_ibverbs_rcache_recv_init,
    .recv_data   = NULL,
    .poll_one    = &nm_ibverbs_rcache_poll_one,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_rcache_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_rcache_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_rcache_component =
  {
    .instantiate = &nm_ibverbs_rcache_instantiate,
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
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_rcache_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_rcache_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));


static void* nm_ibverbs_rcache_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_rcache*rcache = malloc(sizeof(struct nm_ibverbs_rcache));
 /* init state */
  memset(&rcache->headers, 0, sizeof(rcache->headers));
  memset(&rcache->send, 0, sizeof(rcache->send));
  memset(&rcache->recv, 0, sizeof(rcache->recv));
  rcache->mr  = NULL;
  rcache->cnx = NULL;
  rcache->pd  = NULL;
  rcache->context = context;
#if defined(PUKABI) || defined(MINI_PUKABI)
  puk_mem_set_handlers(&nm_ibverbs_mem_reg, &nm_ibverbs_mem_unreg);
#endif /* PUKABI */
  return rcache;
}

static void nm_ibverbs_rcache_destroy(void*_status)
{
  /* TODO */
}

/* *** rcache connection *********************************** */

static void nm_ibverbs_rcache_getprops(puk_context_t context, struct nm_minidriver_properties_s*props)
{
  struct nm_ibverbs_rcache_context_s*p_rcache_context = malloc(sizeof(struct nm_ibverbs_rcache_context_s));
  puk_context_set_status(context, p_rcache_context);
  p_rcache_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_rcache_context->p_hca, &props->profile);
  props->capabilities.supports_data = 0;
}


static void nm_ibverbs_rcache_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  struct nm_ibverbs_rcache_context_s*p_rcache_context = puk_context_get_status(context);
  const char*url = NULL;
  p_rcache_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_rcache*rcache = _status; 
  struct nm_ibverbs_rcache_context_s*p_rcache_context = puk_context_get_status(rcache->context);
  struct nm_ibverbs_hca_s*p_hca = p_rcache_context->p_hca;
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_cnx_new(p_hca);
  rcache->cnx = p_ibverbs_cnx;
  rcache->pd = p_hca->pd;
  /* register Memory Region */
  rcache->mr = ibv_reg_mr(p_hca->pd, &rcache->headers, sizeof(rcache->headers),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(rcache->mr == NULL)
    {
      NM_FATAL("Infiniband: rcache cannot register MR.\n");
    }
  struct nm_ibverbs_segment*seg = &p_ibverbs_cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&rcache->headers;
  seg->rkey  = rcache->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(rcache->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
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
#if defined(PUKABI) || (defined MINI_PUKABI)
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
#if defined(PUKABI) || defined(MINI_PUKABI)
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
#if defined(PUKABI) || defined(MINI_PUKABI)
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
#if defined(PUKABI) || defined(MINI_PUKABI)
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
#if defined(PUKABI) || defined(MINI_PUKABI)
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

/* ** cache ************************************************ */

#ifdef MINI_PUKABI


static void(*puk_mem_invalidate_hook)(void*ptr, size_t size) = NULL;

void puk_abi_mem_invalidate(void*ptr, int size)
{
  if(puk_mem_invalidate_hook)
    (*puk_mem_invalidate_hook)(ptr, size);
}


#define PUK_MEM_CACHE_SIZE 64

static struct
{
  struct puk_mem_reg_s cache[PUK_MEM_CACHE_SIZE];
  puk_mem_register_t   reg;
  puk_mem_unregister_t unreg;
} puk_mem =
  {
    .reg   = NULL,
    .unreg = NULL,
    .cache = { { .refcount = 0, .ptr = NULL, .len = 0 } }
  };


static void puk_mem_cache_flush(struct puk_mem_reg_s*r)
{
  assert(r->refcount == 0);
  (*puk_mem.unreg)(r->context, r->ptr, r->key);
  r->context = NULL;
  r->ptr = NULL;
  r->len = 0;
  r->key = 0;
  r->refcount = 0;
}

static struct puk_mem_reg_s*puk_mem_slot_lookup(void)
{
  static unsigned int victim = 0;
  int i;
  for(i = 0; i < PUK_MEM_CACHE_SIZE; i++)
    {
      struct puk_mem_reg_s*r = &puk_mem.cache[i];
      if(r->ptr == NULL && r->len == 0)
	{
	  return r;
	}
    }
  const int victim_first = victim;
  victim = (victim + 1) % PUK_MEM_CACHE_SIZE;
  struct puk_mem_reg_s*r = &puk_mem.cache[victim];
  while(r->refcount > 0 && victim != victim_first)
    {
      victim = (victim + 1) % PUK_MEM_CACHE_SIZE;
      r = &puk_mem.cache[victim];
    }
  if(r->refcount > 0)
    {
      return NULL;
    }
  puk_mem_cache_flush(r);
  return r;
}

extern void puk_mem_dump(void)
{
  int i;
  for(i = 0; i < PUK_MEM_CACHE_SIZE; i++)
    {
      struct puk_mem_reg_s*r = &puk_mem.cache[i];
      if(r->ptr != NULL)
	{
	  puk_mem_cache_flush(r);
	}
    }
}

/** Invalidate memory registration before it is freed.
 */
extern void puk_mem_invalidate(void*ptr, size_t size)
{
  if(ptr)
    {
      int i;
      for(i = 0; i < PUK_MEM_CACHE_SIZE; i++)
	{
	  struct puk_mem_reg_s*r = &puk_mem.cache[i];
	  const void*rbegin = r->ptr;
	  const void*rend   = r->ptr + r->len;
	  const void*ibegin = ptr;
	  const void*iend   = ptr + size;
	  if( (r->ptr != NULL) &&
	      ( ((ibegin >= rbegin) && (ibegin < rend)) ||
		((iend >= rbegin) && (iend < rend)) ||
		((ibegin >= rbegin) && (iend < rend)) ||
		((rbegin >= ibegin) && (rend < iend)) ) )
	    {
	      if(r->refcount > 0)
		{
		  fprintf(stderr, "PukABI: trying to invalidate registered memory still in use.\n");
		  abort();
		}
	      puk_mem_cache_flush(r);
	    }
	}
    }
}

/* ********************************************************* */

static inline void puk_spinlock_acquire(void)
{ }
static inline void puk_spinlock_release(void)
{ }

void puk_mem_set_handlers(puk_mem_register_t reg, puk_mem_unregister_t unreg)
{
  puk_spinlock_acquire();
  puk_mem.reg = reg;
  puk_mem.unreg = unreg;
  puk_mem_invalidate_hook = &puk_mem_invalidate;
  puk_spinlock_release();
}
void puk_mem_unreg(const struct puk_mem_reg_s*_r)
{
  struct puk_mem_reg_s*r = (struct puk_mem_reg_s*)_r;
  puk_spinlock_acquire();
  r->refcount--;
  if(r->refcount < 0)
    {
      fprintf(stderr, "PukABI: unbalanced registration detected.\n");
      abort();
    }
  puk_spinlock_release();
}
const struct puk_mem_reg_s*puk_mem_reg(void*context, const void*ptr, size_t len)
{
  assert(ptr != NULL);
  puk_spinlock_acquire();
  int i;
  for(i = 0; i < PUK_MEM_CACHE_SIZE; i++)
    {
      struct puk_mem_reg_s*r = &puk_mem.cache[i];
      if(context == r->context && ptr >= r->ptr && (ptr + len <= r->ptr + r->len))
	{
	  r->refcount++;
	  puk_spinlock_release();
	  return r;
	}
    }
  struct puk_mem_reg_s*r = puk_mem_slot_lookup();

  if(r)
    {
      r->context  = context;
      r->ptr      = ptr;
      r->len      = len;
      r->refcount = 1;
      r->key      = (*puk_mem.reg)(context, ptr, len);
    }
  else
    {
      padico_fatal("PukABI: rcache buffer full.\n");
    }
  puk_spinlock_release();
  return r;
}

/* ********************************************************* */

#include <malloc.h>

static void (*minipukabi_old_free_hook)(void*ptr, const void*caller) = NULL;
static void minipukabi_free_hook(void*ptr, const void*caller);

static void minipukabi_install_hooks(void)
{
  minipukabi_old_free_hook = __free_hook;
  __free_hook = minipukabi_free_hook;
}
static void minipukabi_remove_hooks(void)
{
  __free_hook = minipukabi_free_hook;
}
static void minipukabi_free_hook(void*ptr, const void*caller)
{
  puk_abi_mem_invalidate(ptr, 1);
  minipukabi_remove_hooks();
  free(ptr);
  minipukabi_install_hooks();
}

#endif /* PUKABI */
