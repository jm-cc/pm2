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

static int puk_mem_cache_evict(void);
#endif /* PUKABI */

#include <Padico/Module.h>


/* *** method: 'rcache' ********************************* */

/** Header sent by the receiver to ask for RDMA data (rcache rdv) */
struct nm_ibverbs_rcache_rdvhdr_s
{
  uint64_t raddr;
  uint32_t rkey;
  volatile int busy;
};

/** Header sent to signal presence of rcache data */
struct nm_ibverbs_rcache_sighdr_s
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
struct nm_ibverbs_rcache_s
{
  struct ibv_mr*mr;                /**< global MR (used for headers) */
  struct ibv_pd*pd;                /**< protection domain */
  struct nm_ibverbs_segment_s seg; /**< remote segment */
  struct nm_ibverbs_cnx_s*p_cnx;
  puk_context_t context;
  struct
  {
    char*message;
    int size;
#if defined(PUKABI) || defined(MINI_PUKABI)
    const struct puk_mem_reg_s*p_reg;
#else
    struct ibv_mr*mr;
#endif /* PUKABI */
  } recv;
  struct
  {
    const char*message;
    int size;
#if defined(PUKABI) || defined(MINI_PUKABI)
    const struct puk_mem_reg_s*p_reg;
#else
    struct ibv_mr*mr;
#endif /* PUKABI */
  } send;
  struct
  {
    struct nm_ibverbs_rcache_rdvhdr_s shdr, rhdr;
    struct nm_ibverbs_rcache_sighdr_s ssig, rsig;
  } headers;
};

static void nm_ibverbs_rcache_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_rcache_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_rcache_send_iov_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_rcache_send_poll(void*_status);
static void nm_ibverbs_rcache_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_rcache_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_rcache_minidriver =
  {
    .getprops        = &nm_ibverbs_rcache_getprops,
    .init            = &nm_ibverbs_rcache_init,
    .connect         = &nm_ibverbs_rcache_connect,
    .send_data_post       = NULL,
    .send_iov_post       = &nm_ibverbs_rcache_send_iov_post,
    .send_poll       = &nm_ibverbs_rcache_send_poll,
    .recv_iov_post   = &nm_ibverbs_rcache_recv_init,
    .recv_data_post  = NULL,
    .recv_poll_one   = &nm_ibverbs_rcache_poll_one,
    .recv_cancel     = NULL
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
  struct ibv_mr*mr = NULL;
 retry:
  mr = ibv_reg_mr(pd, (void*)ptr, len, 
                  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(mr == NULL)
    {
#ifdef MINI_PUKABI
      if(errno == ENOMEM)
        {
          int evicted = puk_mem_cache_evict();
          if(evicted)
            goto retry;
        }
#endif /* MINI_PUKABI */
      NM_FATAL("ibverbs: error %d (%s) while registering memory- ptr = %p; len = %d.\n",
               errno, strerror(errno), ptr, (int)len);
    }
  return mr;
}
static void nm_ibverbs_mem_unreg(void*context, const void*ptr, void*key)
{
  struct ibv_mr*mr = key;
  int rc = ibv_dereg_mr(mr);
  if(rc != 0)
    {
      NM_FATAL("ibverbs: error %d (%s) while deregistering memory.\n", rc, strerror(rc));
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
  struct nm_ibverbs_rcache_s*p_rcache = malloc(sizeof(struct nm_ibverbs_rcache_s));
  memset(&p_rcache->headers, 0, sizeof(p_rcache->headers));
  memset(&p_rcache->send, 0, sizeof(p_rcache->send));
  memset(&p_rcache->recv, 0, sizeof(p_rcache->recv));
  p_rcache->mr  = NULL;
  p_rcache->p_cnx = NULL;
  p_rcache->pd  = NULL;
  p_rcache->context = context;
#if defined(PUKABI) || defined(MINI_PUKABI)
  puk_mem_set_handlers(&nm_ibverbs_mem_reg, &nm_ibverbs_mem_unreg);
#endif /* PUKABI */
  return p_rcache;
}

static void nm_ibverbs_rcache_destroy(void*_status)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status;
  free(p_rcache);
}

/* *** rcache connection *********************************** */

static void nm_ibverbs_rcache_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  struct nm_ibverbs_rcache_context_s*p_rcache_context = malloc(sizeof(struct nm_ibverbs_rcache_context_s));
  puk_context_set_status(context, p_rcache_context);
  p_rcache_context->p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_rcache_context->p_hca, &p_props->profile);
  p_props->capabilities.supports_data = 0;
}

static void nm_ibverbs_rcache_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  struct nm_ibverbs_rcache_context_s*p_rcache_context = puk_context_get_status(context);
  const char*url = NULL;
  p_rcache_context->p_connector = nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
}

static void nm_ibverbs_rcache_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status; 
  struct nm_ibverbs_rcache_context_s*p_rcache_context = puk_context_get_status(p_rcache->context);
  struct nm_ibverbs_hca_s*p_hca = p_rcache_context->p_hca;
  struct nm_ibverbs_cnx_s*p_ibverbs_cnx = nm_ibverbs_cnx_new(p_hca);
  p_rcache->p_cnx = p_ibverbs_cnx;
  p_rcache->pd = p_hca->pd;
  /* register Memory Region */
  p_rcache->mr = ibv_reg_mr(p_hca->pd, &p_rcache->headers, sizeof(p_rcache->headers),
                            IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(p_rcache->mr == NULL)
    {
      NM_FATAL("ibverbs: rcache cannot register MR.\n");
    }
  struct nm_ibverbs_segment_s*p_seg = &p_ibverbs_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_rcache->headers;
  p_seg->rkey  = p_rcache->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_rcache->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_ibverbs_cnx->local_addr, &p_ibverbs_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_rcache->seg = p_ibverbs_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_ibverbs_cnx);
}


/* ** reg cache I/O **************************************** */

static void nm_ibverbs_rcache_send_iov_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status;
  char*message = v[0].iov_base;
  const int size = v[0].iov_len;
  assert(n == 1);
  if(p_rcache->send.message != NULL)
    {
      NM_FATAL("ibverbs: rendez-vous already posted on sender side.\n");
    }
  p_rcache->send.message = message;
  p_rcache->send.size = size;
#if defined(PUKABI) || (defined MINI_PUKABI)
  p_rcache->send.p_reg = puk_mem_reg(p_rcache->pd, message, size);
#else /* PUKABI */
  p_rcache->send.mr = nm_ibverbs_mem_reg(p_rcache->pd, message, size);
#endif /* PUKABI */
}

static int nm_ibverbs_rcache_send_poll(void*_status)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status;
  struct nm_ibverbs_rcache_rdvhdr_s*const p_rdvhdr = &p_rcache->headers.rhdr;
  if(p_rdvhdr->busy)
    {
      const uint64_t raddr = p_rdvhdr->raddr;
      const uint32_t rkey  = p_rdvhdr->rkey;
      struct ibv_mr*mr = NULL;
#if defined(PUKABI) || defined(MINI_PUKABI)
      mr = p_rcache->send.p_reg->key;
#else
      mr = p_rcache->send.mr;      
#endif
      p_rdvhdr->raddr = 0;
      p_rdvhdr->rkey  = 0;
      p_rdvhdr->busy  = 0;
      nm_ibverbs_do_rdma(p_rcache->p_cnx, 
			 p_rcache->send.message, p_rcache->send.size,
			 raddr, IBV_WR_RDMA_WRITE, IBV_SEND_SIGNALED,
			 mr->lkey, rkey, NM_IBVERBS_WRID_DATA);
      p_rcache->headers.ssig.busy = 1;
      nm_ibverbs_rdma_send(p_rcache->p_cnx, sizeof(struct nm_ibverbs_rcache_sighdr_s),
			   &p_rcache->headers.ssig,
			   &p_rcache->headers.rsig,
			   &p_rcache->headers,
			   &p_rcache->seg,
			   p_rcache->mr,
			   NM_IBVERBS_WRID_HEADER);
      nm_ibverbs_send_flush(p_rcache->p_cnx, NM_IBVERBS_WRID_DATA);
      nm_ibverbs_send_flush(p_rcache->p_cnx, NM_IBVERBS_WRID_HEADER);
#if defined(PUKABI) || defined(MINI_PUKABI)
      puk_mem_unreg(p_rcache->send.p_reg);
      p_rcache->send.p_reg = NULL;
#else /* PUKABI */
      nm_ibverbs_mem_unreg(p_rcache->pd, p_rcache->send.message, p_rcache->send.mr);
      p_rcache->send.mr = NULL;
#endif /* PUKABI */
      p_rcache->send.message = NULL;
      p_rcache->send.size    = -1;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

static void nm_ibverbs_rcache_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status;
  if(p_rcache->recv.message != NULL)
    {
      NM_FATAL("ibverbs: rendez-vous already posted on receiver side.\n");
    }
  p_rcache->headers.rsig.busy = 0;
  p_rcache->recv.message = v->iov_base;
  p_rcache->recv.size = v->iov_len;
  struct nm_ibverbs_rcache_rdvhdr_s*const p_rdvhdr = &p_rcache->headers.shdr;
  p_rdvhdr->raddr = (uintptr_t)p_rcache->recv.message;
  p_rdvhdr->busy  = 1;
#if defined(PUKABI) || defined(MINI_PUKABI)
  p_rcache->recv.p_reg = puk_mem_reg(p_rcache->pd, p_rcache->recv.message, p_rcache->recv.size);
  struct ibv_mr*mr = p_rcache->recv.p_reg->key;
  p_rdvhdr->rkey  = mr->rkey;
#else /* PUKABI */
  p_rcache->recv.mr = nm_ibverbs_mem_reg(p_rcache->pd, p_rcache->recv.message, p_rcache->recv.size);
  p_rdvhdr->rkey  = p_rcache->recv.mr->rkey;
#endif /* PUKABI */

  nm_ibverbs_rdma_send(p_rcache->p_cnx, sizeof(struct nm_ibverbs_rcache_rdvhdr_s),
		       &p_rcache->headers.shdr, 
		       &p_rcache->headers.rhdr,
		       &p_rcache->headers,
		       &p_rcache->seg,
		       p_rcache->mr,
		       NM_IBVERBS_WRID_RDV);
  nm_ibverbs_send_flush(p_rcache->p_cnx, NM_IBVERBS_WRID_RDV);
}

static int nm_ibverbs_rcache_poll_one(void*_status)
{
  struct nm_ibverbs_rcache_s*p_rcache = _status;
  struct nm_ibverbs_rcache_sighdr_s*p_rsig = &p_rcache->headers.rsig;
  if(p_rsig->busy)
    {
      p_rsig->busy = 0;
#if defined(PUKABI) || defined(MINI_PUKABI)
      puk_mem_unreg(p_rcache->recv.p_reg);
      p_rcache->recv.p_reg = NULL;
#else /* PUKABI */
      nm_ibverbs_mem_unreg(p_rcache->pd, p_rcache->recv.message, p_rcache->recv.mr);
      p_rcache->recv.mr = NULL;
#endif /* PUKABI */
      p_rcache->recv.message = NULL;
      p_rcache->recv.size = -1;
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

static int puk_mem_cache_evict(void)
{
  static unsigned int victim = 0;
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
      return 0;
    }
  if(r->ptr != NULL)
    {
      puk_mem_cache_flush(r);
      return 1;
    }
  return 0;
}

static struct puk_mem_reg_s*puk_mem_slot_lookup(void)
{
  int i;
 retry:
  for(i = 0; i < PUK_MEM_CACHE_SIZE; i++)
    {
      struct puk_mem_reg_s*r = &puk_mem.cache[i];
      if(r->ptr == NULL && r->len == 0)
	{
	  return r;
	}
    }
  int evicted = puk_mem_cache_evict();
  if(evicted)
    {
      goto retry;
    }
  return NULL;
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
