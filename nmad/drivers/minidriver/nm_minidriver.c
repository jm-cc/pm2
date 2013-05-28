/*
 * NewMadeleine
 * Copyright (C) 2013 (see AUTHORS file)
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

#include <limits.h>

#include <Padico/Module.h>
#include <tbx.h>
#include <nm_private.h>

/** driver private state */
struct nm_minidriver_drv 
{
  struct { puk_context_t minidriver; } trks_array[2]; /**< driver contexts for tracks */
  char*url;                   /**< driver url for this node (used by connector) */
};

/** status for an instance. */
struct nm_minidriver
{
  struct
  {
    struct puk_receptacle_NewMad_minidriver_s minidriver;
    puk_instance_t minidriver_instance;
  } trks[2];
};

/* ********************************************************* */

/* ** component declaration */

static int nm_minidriver_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_minidriver_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_minidriver_close(struct nm_drv*p_drv);
static int nm_minidriver_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_minidriver_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id);
static int nm_minidriver_post_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_minidriver_poll_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_minidriver_send_prefetch(void*_status,  struct nm_pkt_wrap *p_pw);
static int nm_minidriver_post_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_minidriver_poll_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_minidriver_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static const char* nm_minidriver_get_driver_url(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_minidriver_driver =
  {
    .name               = "minidriver",

    .query              = &nm_minidriver_query,
    .init               = &nm_minidriver_init,
    .close              = &nm_minidriver_close,

    .connect		= &nm_minidriver_connect,
    .disconnect         = &nm_minidriver_disconnect,
    
    .post_send_iov	= &nm_minidriver_post_send_iov,
    .post_recv_iov      = &nm_minidriver_post_recv_iov,
    
    .poll_send_iov      = &nm_minidriver_poll_send_iov,
    .poll_recv_iov      = &nm_minidriver_poll_recv_iov,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .prefetch_send      = &nm_minidriver_send_prefetch,

    .cancel_recv_iov    = &nm_minidriver_cancel_recv_iov,

    .get_driver_url     = &nm_minidriver_get_driver_url,

    .capabilities.min_period    = 0,
    .capabilities.rdv_threshold = 32 * 1024
  };

/** 'PadicoAdapter' facet for Minidriver driver */
static void*nm_minidriver_instanciate(puk_instance_t, puk_context_t);
static void nm_minidriver_destroy(void*);

static const struct puk_adapter_driver_s nm_minidriver_adapter_driver =
  {
    .instanciate = &nm_minidriver_instanciate,
    .destroy     = &nm_minidriver_destroy
  };


PADICO_MODULE_COMPONENT(NewMad_Driver_minidriver,
  puk_component_declare("NewMad_Driver_minidriver",
			puk_component_provides("PadicoAdapter", "adapter", &nm_minidriver_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_minidriver_driver)) );



/** Instanciate functions */
static void* nm_minidriver_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_minidriver*status = TBX_MALLOC(sizeof(struct nm_minidriver));
  return status;
}

static void nm_minidriver_destroy(void*_status)
{
  struct nm_minidriver*status = _status;
  TBX_FREE(status);
}

const static char*nm_minidriver_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  return p_minidriver_drv->url;
}

/* ** init & connection ************************************ */

static int nm_minidriver_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam)
{
  int err = NM_ESUCCESS;
  struct nm_minidriver_drv*p_minidriver_drv = TBX_MALLOC(sizeof(struct nm_minidriver_drv));
  
  if (!p_minidriver_drv) 
    {
      err = -NM_ENOMEM;
      goto out;
    }
  p_minidriver_drv->url = NULL;

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
      p_minidriver_drv->trks_array[i].minidriver = context;
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

  p_drv->priv = p_minidriver_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_minidriver_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  
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
      puk_context_t context = p_minidriver_drv->trks_array[i].minidriver;
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
  p_minidriver_drv->url = puk_hex_encode(url_chunks, &url_chunks_size, NULL);
  free(url_chunks);
  return NM_ESUCCESS;
}

static int nm_minidriver_close(struct nm_drv *p_drv)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  TBX_FREE(p_minidriver_drv->url);
  TBX_FREE(p_minidriver_drv);
  return NM_ESUCCESS;
}


static int nm_minidriver_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  struct nm_minidriver*status = _status;
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  puk_context_t context = p_minidriver_drv->trks_array[trk_id].minidriver;
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

static int nm_minidriver_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id)
{
  int err;
  
  /* TODO */
  
  err = NM_ESUCCESS;
  
  return err;
}

/* ********************************************************* */

/* ** driver I/O functions ********************************* */

static int nm_minidriver_post_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  (*minidriver->driver->send_post)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
  int err = nm_minidriver_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_minidriver_poll_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  int err = (*minidriver->driver->send_poll)(minidriver->_status);
  return err;
}

static int nm_minidriver_send_prefetch(void*_status, struct nm_pkt_wrap *p_pw)
{
  const nm_trk_id_t trk_id = NM_TRK_LARGE;
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[trk_id].minidriver;
  if(minidriver->driver->send_prefetch)
    {
      (*minidriver->driver->send_prefetch)(minidriver->_status, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
    }
  return NM_ESUCCESS;
}

static int nm_minidriver_poll_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = -NM_EAGAIN;
  if(_status)
    {
      struct nm_minidriver*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      err = (*minidriver->driver->poll_one)(minidriver->_status);
    }
  else
    {
      TBX_FAILURE("poll_any not implemented yet.");
    }
  return err;
}

static int nm_minidriver_post_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = NM_ESUCCESS;
  if(_status)
    {
      struct nm_minidriver*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      (*minidriver->driver->recv_init)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
    }
  err = nm_minidriver_poll_recv_iov(_status, p_pw);
  return err;
}

static int nm_minidriver_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  int err = -NM_ENOTIMPL;
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  if(minidriver->driver->cancel_recv)
    {
      err = (*minidriver->driver->cancel_recv)(minidriver->_status);
    }
  return err;
}
