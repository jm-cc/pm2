/*
 * NewMadeleine
 * Copyright (C) 2006-2013 (see AUTHORS file)
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

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h>


#include "nm_ibverbs.h"

#include <Padico/Module.h>
#include <tbx.h>


/* *********************************************************
 * Rationale of the newmad/ibverbs driver
 *
 * This is a multi-method driver. Four methods are available:
 *   -- bycopy: data is copied in a pre-allocated, pre-registered
 *      memory region, then sent through RDMA into a pre-registered
 *      memory region of the peer node.
 *   -- adaptrdma: data is copied in a pre-allocated, pre-registered
 *      memory region, with an adaptive super-pipeline. Memory blocks are 
 *      copied as long as the previous RDMA send doesn't complete.
 *      A guard check ensures that block size progression is at least
 *      2-base exponential to prevent artifacts to kill performance.
 *   -- regrdma: memory is registered on the fly on both sides, using 
 *      a pipeline with variable block size. For each block, the
 *      receiver sends an ACK with RDMA info (raddr, rkey), then 
 *      zero-copy RDMA is performed.
 *   -- rcache: memory is registered on the fly on both sides, 
 *      sequencially, using puk_mem_* functions from Puk-ABI that
 *      manage a cache.
 *
 * Method is chosen as follows:
 *   -- tracks for small messages always uses 'bycopy'
 *   -- tracks with rendez-vous use 'adaptrdma' for smaller messages 
 *      and 'regrdma' for larger messages. Usually, the threshold 
 *      is 224kB (from empirical results about registration/send overlap)
 *      on MT23108, and 2MB on ConnectX.
 *   -- tracks with rendez-vous use 'rcache' when ib_rcache is activated
 *      in the flavor.
 */


/** driver private state */
struct nm_ibverbs_drv 
{
  struct { puk_context_t minidriver; } trks_array[2]; /**< driver contexts for tracks */
  char*url;                   /**< driver url for this node (used by connector) */
};

/** status for an instance. */
struct nm_ibverbs
{
  struct
  {
    struct puk_receptacle_NewMad_minidriver_s minidriver;
    puk_instance_t minidriver_instance;
  } trks[2];
};

static tbx_checksum_t _nm_ibverbs_checksum = NULL;

/** ptr alignment enforced in every packets (through padding) */
int nm_ibverbs_alignment = 64;
/** ptr alignment enforced for buffer allocation */
int nm_ibverbs_memalign  = 4096;

/* ********************************************************* */

/* ** component declaration */

static int nm_ibverbs_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_ibverbs_close(struct nm_drv*p_drv);
static int nm_ibverbs_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_ibverbs_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id);
static int nm_ibverbs_post_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_send_prefetch(void*_status,  struct nm_pkt_wrap *p_pw);
static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static const char* nm_ibverbs_get_driver_url(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_ibverbs_driver =
  {
    .name               = "ibverbs",

    .query              = &nm_ibverbs_query,
    .init               = &nm_ibverbs_init,
    .close              = &nm_ibverbs_close,

    .connect		= &nm_ibverbs_connect,
    .disconnect         = &nm_ibverbs_disconnect,
    
    .post_send_iov	= &nm_ibverbs_post_send_iov,
    .post_recv_iov      = &nm_ibverbs_post_recv_iov,
    
    .poll_send_iov      = &nm_ibverbs_poll_send_iov,
    .poll_recv_iov      = &nm_ibverbs_poll_recv_iov,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .prefetch_send      = &nm_ibverbs_send_prefetch,

    .cancel_recv_iov    = &nm_ibverbs_cancel_recv_iov,

    .get_driver_url     = &nm_ibverbs_get_driver_url,

    .capabilities.min_period    = 0,
    .capabilities.rdv_threshold = 32 * 1024
  };

/** 'PadicoAdapter' facet for Ibverbs driver */
static void*nm_ibverbs_instanciate(puk_instance_t, puk_context_t);
static void nm_ibverbs_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_adapter_driver =
  {
    .instanciate = &nm_ibverbs_instanciate,
    .destroy     = &nm_ibverbs_destroy
  };


PADICO_MODULE_COMPONENT(NewMad_Driver_ibverbs,
  puk_component_declare("NewMad_Driver_ibverbs",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_ibverbs_driver)) );


/** Instanciate functions */
static void* nm_ibverbs_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs*status = TBX_MALLOC(sizeof(struct nm_ibverbs));
  const char*checksum_env = getenv("NMAD_IBVERBS_CHECKSUM");
  if(_nm_ibverbs_checksum == NULL && checksum_env != NULL)
    {
      tbx_checksum_t checksum = tbx_checksum_get(checksum_env);
      if(checksum == NULL)
	TBX_FAILUREF("# nmad: checksum algorithm *%s* not available.\n", checksum_env);
      _nm_ibverbs_checksum = checksum;
      NM_DISPF("# nmad ibverbs: checksum enabled (%s).\n", checksum->name);
    }
  const char*align_env = getenv("NMAD_IBVERBS_ALIGN");
  if(align_env != NULL)
    {
      nm_ibverbs_alignment = atoi(align_env);
      NM_DISPF("# nmad ibverbs: alignment forced to %d\n", nm_ibverbs_alignment);
    }
  const char*memalign_env = getenv("NMAD_IBVERBS_MEMALIGN");
  if(memalign_env != NULL)
    {
      nm_ibverbs_memalign = atoi(memalign_env);
      NM_DISPF("# nmad ibverbs: memalign forced to %d\n", nm_ibverbs_memalign);
    }
  return status;
}

static void nm_ibverbs_destroy(void*_status)
{
  struct nm_ibverbs*status = _status;
  TBX_FREE(status);
}

const static char*nm_ibverbs_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  return p_ibverbs_drv->url;
}


/* ** checksum ********************************************* */


/** checksum algorithm. Set NMAD_IBVERBS_CHECKSUM to non-null to enable checksums.
 */
uint32_t nm_ibverbs_checksum(const char*data, nm_len_t len)
{
  if(_nm_ibverbs_checksum)
    return (*_nm_ibverbs_checksum->func)(data, len);
  else
    return 0;
}

int nm_ibverbs_checksum_enabled(void)
{
  return _nm_ibverbs_checksum != NULL;
}

uint32_t nm_ibverbs_memcpy_and_checksum(void*_dest, const void*_src, nm_len_t len)
{
  if(_nm_ibverbs_checksum)
    {
      if(_nm_ibverbs_checksum->checksum_and_copy)
	{
	  return (*_nm_ibverbs_checksum->checksum_and_copy)(_dest, _src, len);
	}
      else
	{
	  memcpy(_dest, _src, len);
	  return nm_ibverbs_checksum(_dest, len);
	}
    }
  else
    {
      memcpy(_dest, _src, len);
    }
  return 0;
}



/* ** init & connection ************************************ */

static int nm_ibverbs_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam)
{
  int err = NM_ESUCCESS;
  struct nm_ibverbs_drv*p_ibverbs_drv = TBX_MALLOC(sizeof(struct nm_ibverbs_drv));
  
  if (!p_ibverbs_drv) 
    {
      err = -NM_ENOMEM;
      goto out;
    }
  p_ibverbs_drv->url = NULL;

  /* get driver properties */
  puk_component_t component = NULL;
  struct nm_minidriver_properties_s props[2];
  int i;
  for(i = 0; i < 2; i++)
    {
      if(i == 0)
	{
	  component = puk_adapter_resolve("NewMad_ibverbs_bycopy");
	}
      else
	{
	  static const char const ib_rcache[] = "NewMad_ibverbs_rcache";
	  static const char const ib_lr2[] = "NewMad_ibverbs_lr2";
	  static puk_component_t ib_minidriver = NULL;
	  if(ib_minidriver == NULL)
	    {
	      if(getenv("NMAD_IBVERBS_RCACHE") != NULL)
		{
		  ib_minidriver = puk_adapter_resolve(ib_rcache);
		  NM_DISPF("# nmad ibverbs: rcache forced by environment.\n");
		}
	      else
		{
		  ib_minidriver = puk_adapter_resolve(ib_lr2);
		}
	    }
	  component = ib_minidriver;
	}
      const struct nm_minidriver_iface_s*minidriver_iface =
	puk_component_get_driver_NewMad_minidriver(component, NULL);
      /* create component context */
      puk_context_t context = puk_context_new(component);
      p_ibverbs_drv->trks_array[i].minidriver = context;
      char s_index[16];
      sprintf(s_index, "%d", p_drv->index);
      puk_context_putattr(context, "index", s_index);
      (*minidriver_iface->getprops)(p_drv->index, &props[i]);
    }

  /* driver profile encoding */
#ifdef PM2_NUIOA
  p_drv->profile.numa_node = props[0].profile.numa_node;
#endif
  p_drv->profile.latency = props[0].profile.latency;
  p_drv->profile.bandwidth = props[1].profile.bandwidth;

  p_drv->priv = p_ibverbs_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  
  srand48(getpid() * time(NULL));
  
 
  /* open tracks */
  void*url_chunks = NULL;
  int url_chunks_size = 0;
  nm_trk_id_t i;
  for(i = 0; i < nb_trks; i++)
    {
      if(trk_caps[i].rq_type == nm_trk_rq_rdv)
	{
	  trk_caps[i].rq_type  = nm_trk_rq_rdv;
	  trk_caps[i].iov_type = nm_trk_iov_none;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 1;
	}
      else
	{
	  trk_caps[i].rq_type  = nm_trk_rq_dgram;
	  trk_caps[i].iov_type = nm_trk_iov_send_only;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 0;
	}
      puk_context_t context = p_ibverbs_drv->trks_array[i].minidriver;
      const struct nm_minidriver_iface_s*minidriver_iface = 
	puk_component_get_driver_NewMad_minidriver(context->component, "minidriver");
      const void*trk_url = NULL;
      size_t trk_url_size = 0;
      (*minidriver_iface->init)(context, &trk_url, &trk_url_size);
      /* encode url chunk */
      const size_t chunk_size = trk_url_size + sizeof(int);
      url_chunks = realloc(url_chunks, url_chunks_size + chunk_size);
      int*p_chunk_size = url_chunks + url_chunks_size;
      *p_chunk_size = chunk_size;
      void*p_chunk_content = url_chunks + url_chunks_size + sizeof(int);
      memcpy(p_chunk_content, trk_url, trk_url_size);
      url_chunks_size += chunk_size;
    }
  p_ibverbs_drv->url = puk_hex_encode(url_chunks, &url_chunks_size, NULL);
  free(url_chunks);
  return NM_ESUCCESS;
}

static int nm_ibverbs_close(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  TBX_FREE(p_ibverbs_drv->url);
  TBX_FREE(p_ibverbs_drv);
  return NM_ESUCCESS;
}


static int nm_ibverbs_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  struct nm_ibverbs*status = _status;
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  puk_context_t context = p_ibverbs_drv->trks_array[trk_id].minidriver;
  status->trks[trk_id].minidriver_instance = puk_context_instanciate(context);
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[trk_id].minidriver;
  puk_instance_indirect_NewMad_minidriver(status->trks[trk_id].minidriver_instance, NULL, minidriver);
  int url_chunks_size = strlen(remote_url);
  void*url_chunks = puk_hex_decode(remote_url, &url_chunks_size, NULL);
  void*orig_url_chunks = url_chunks;
  const int*p_chunk_size = url_chunks;
  nm_trk_id_t i;
  for(i = 0; i < trk_id; i++)
    {
      url_chunks += *p_chunk_size;
      p_chunk_size = url_chunks;
    }
  const void*p_chunk_content = ((void*)p_chunk_size) + sizeof(int);
  (*minidriver->driver->connect)(minidriver->_status, p_chunk_content, *p_chunk_size);

  free(orig_url_chunks);
  return NM_ESUCCESS;
}

static int nm_ibverbs_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id)
{
  int err;
  
  /* TODO */
  
  err = NM_ESUCCESS;
  
  return err;
}

/* ********************************************************* */

/* ** driver I/O functions ********************************* */

static int nm_ibverbs_post_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  (*minidriver->driver->send_post)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
  int err = nm_ibverbs_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  int err = (*minidriver->driver->send_poll)(minidriver->_status);
  return err;
}

static int nm_ibverbs_send_prefetch(void*_status, struct nm_pkt_wrap *p_pw)
{
  const nm_trk_id_t trk_id = NM_TRK_LARGE;
  struct nm_ibverbs*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[trk_id].minidriver;
  if(minidriver->driver->send_prefetch)
    {
      (*minidriver->driver->send_prefetch)(minidriver->_status, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
    }
  return NM_ESUCCESS;
}

static int nm_ibverbs_poll_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = -NM_EAGAIN;
  if(_status)
    {
      struct nm_ibverbs*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      err = (*minidriver->driver->poll_one)(minidriver->_status);
    }
  else
    {
      TBX_FAILURE("poll_any not implemented yet.");
    }
  return err;
}

static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = NM_ESUCCESS;
  if(_status)
    {
      struct nm_ibverbs*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      (*minidriver->driver->recv_init)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
    }
  err = nm_ibverbs_poll_recv_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  int err = -NM_ENOTIMPL;
  struct nm_ibverbs*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  if(minidriver->driver->cancel_recv)
    {
      err = (*minidriver->driver->cancel_recv)(minidriver->_status);
    }
  return err;
}
