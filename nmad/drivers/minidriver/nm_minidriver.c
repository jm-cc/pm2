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
  puk_context_t context;
};

/** status for a driver context */
struct nm_minidriver_context_s
{
  struct
  {
    puk_context_t minidriver; /**< driver contexts for given track */
    struct nm_minidriver_properties_s props;
  } trks_array[2]; 
  char*url;                   /**< driver url for this node (used by connector) */
};

/** status for an instance. */
struct nm_minidriver
{
  struct
  {
    struct puk_receptacle_NewMad_minidriver_s minidriver;
    struct nm_data_s sdata, rdata;
  } trks[2];
};

/* ********************************************************* */

/* ** component declaration */

static int nm_minidriver_query(nm_drv_t p_drv, struct nm_driver_query_param*params, int nparam);
static int nm_minidriver_init(nm_drv_t p_drv, struct nm_minidriver_capabilities_s*trk_caps, int nb_trks);
static int nm_minidriver_close(nm_drv_t p_drv);
static int nm_minidriver_connect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_minidriver_disconnect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id);
static int nm_minidriver_post_send_iov(void*_status, struct nm_pkt_wrap_s*p_pw);
static int nm_minidriver_poll_send_iov(void*_status, struct nm_pkt_wrap_s*p_pw);
static int nm_minidriver_send_prefetch(void*_status,  struct nm_pkt_wrap_s *p_pw);
static int nm_minidriver_post_recv_iov(void*_status, struct nm_pkt_wrap_s*p_pw);
static int nm_minidriver_poll_recv_iov(void*_status, struct nm_pkt_wrap_s*p_pw);
static int nm_minidriver_cancel_recv_iov(void*_status, struct nm_pkt_wrap_s*p_pw);
static const char* nm_minidriver_get_driver_url(nm_drv_t p_drv);

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
    
  };

/** 'PadicoComponent' facet for Minidriver driver */
static void*nm_minidriver_instantiate(puk_instance_t, puk_context_t);
static void nm_minidriver_destroy(void*);

static const struct puk_component_driver_s nm_minidriver_component_driver =
  {
    .instantiate = &nm_minidriver_instantiate,
    .destroy     = &nm_minidriver_destroy
  };


PADICO_MODULE_COMPONENT(NewMad_Driver_minidriver,
  puk_component_declare("NewMad_Driver_minidriver",
			puk_component_provides("PadicoComponent", "component", &nm_minidriver_component_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_minidriver_driver),
			puk_component_uses("NewMad_minidriver", "trk0"),
			puk_component_uses("NewMad_minidriver", "trk1")) );



/** Instanciate functions */
static void* nm_minidriver_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_minidriver*status = malloc(sizeof(struct nm_minidriver));
  puk_context_indirect_NewMad_minidriver(instance, "trk0", &status->trks[0].minidriver);
  puk_context_indirect_NewMad_minidriver(instance, "trk1", &status->trks[1].minidriver);
  return status;
}

static void nm_minidriver_destroy(void*_status)
{
  struct nm_minidriver*status = _status;
  free(status);
}

const static char*nm_minidriver_get_driver_url(nm_drv_t p_drv)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  puk_context_t context = p_minidriver_drv->context;
  struct nm_minidriver_context_s*p_minidriver_context = puk_context_get_status(context);
  return p_minidriver_context->url;
}

/* ** init & connection ************************************ */

static int nm_minidriver_query(nm_drv_t p_drv, struct nm_driver_query_param *params, int nparam)
{
  int err = NM_ESUCCESS;
  puk_context_t context = NULL;
  struct nm_minidriver_drv*p_minidriver_drv = malloc(sizeof(struct nm_minidriver_drv));
  p_minidriver_drv->context = NULL;
  /* hack here- find self context in the composite */
  const struct puk_composite_driver_s*const composite_driver = 
    puk_component_get_driver_PadicoComposite(p_drv->assembly, NULL);
  if(composite_driver)
    {
      struct puk_composite_content_s*content = composite_driver->content;
      puk_component_conn_vect_itor_t conn;
      puk_vect_foreach(conn, puk_component_conn, &content->entry_points)
	{
	  context = conn->context;
	  if(context->component == padico_module_component_self())
	    {
	      p_minidriver_drv->context = context;
	    }
	}
    }
  if(p_minidriver_drv->context == NULL)
    {
      padico_fatal("NewMad_Driver_minidriver: bad assembly.\n");
    }
  struct nm_minidriver_context_s*p_minidriver_context = malloc(sizeof(struct nm_minidriver_context_s));
  puk_context_set_status(context, p_minidriver_context);
  p_minidriver_context->url = NULL;
  /* resolve sub-drivers for trk#0 & trk#1 and get properties */
  puk_component_conn_t trk0 = puk_context_conn_lookup(context, NULL, "trk0");
  puk_component_conn_t trk1 = puk_context_conn_lookup(context, NULL, "trk1");
  int i;
  for(i = 0; i < 2; i++)
    {
      puk_component_conn_t conn = (i == 0) ? trk0 : trk1;
      const struct nm_minidriver_iface_s*minidriver_iface = conn->facet->driver;
      p_minidriver_context->trks_array[i].minidriver = conn->context;
      p_minidriver_context->trks_array[i].props.capabilities = (struct nm_minidriver_capabilities_s){ 0 };
#ifdef PM2_TOPOLOGY
      p_minidriver_context->trks_array[i].props.profile.cpuset = NULL;
#endif /* PM2_TOPOLOGY */
      (*minidriver_iface->getprops)(conn->context, &p_minidriver_context->trks_array[i].props);
      /* sanity check */
      assert( ((minidriver_iface->send_data == NULL) && (p_minidriver_context->trks_array[i].props.capabilities.supports_data == 0)) ||
	      ((minidriver_iface->send_data != NULL) && (p_minidriver_context->trks_array[i].props.capabilities.supports_data != 0)) );
      
    }

  
  /* driver profile encoding */
#ifdef PM2_TOPOLOGY
  if(p_minidriver_context->trks_array[0].props.profile.cpuset != NULL)
    {
      p_drv->profile.cpuset = hwloc_bitmap_alloc();
      hwloc_bitmap_copy(p_drv->profile.cpuset, p_minidriver_context->trks_array[0].props.profile.cpuset);
    }
#endif /* PM2_TOPOLOGY */
  p_drv->profile.latency = p_minidriver_context->trks_array[0].props.profile.latency;
  p_drv->profile.bandwidth = p_minidriver_context->trks_array[1].props.profile.bandwidth;

  p_drv->priv = p_minidriver_drv;
  err = NM_ESUCCESS;

  return err;
}

static int nm_minidriver_init(nm_drv_t p_drv, struct nm_minidriver_capabilities_s*trk_caps, int nb_trks)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  struct nm_minidriver_context_s*p_minidriver_context = puk_context_get_status(p_minidriver_drv->context);
 
  /* open tracks */
  void*url_chunks = NULL;
  int url_chunks_size = 0;
  nm_trk_id_t i;
  for(i = 0; i < nb_trks; i++)
    {
      trk_caps[i] = p_minidriver_context->trks_array[i].props.capabilities;
      puk_context_t context = p_minidriver_context->trks_array[i].minidriver;
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
  p_minidriver_context->url = puk_hex_encode(url_chunks, &url_chunks_size, NULL);
  free(url_chunks);
  return NM_ESUCCESS;
}

static int nm_minidriver_close(nm_drv_t p_drv)
{
  struct nm_minidriver_drv*p_minidriver_drv = p_drv->priv;
  struct nm_minidriver_context_s*p_minidriver_context = puk_context_get_status(p_minidriver_drv->context);
  int i;
  for(i = 0; i < 2; i++)
    {
      puk_context_t context = p_minidriver_context->trks_array[i].minidriver;
      const struct nm_minidriver_iface_s*minidriver_iface = 
	puk_component_get_driver_NewMad_minidriver(context->component, "minidriver");
      if(minidriver_iface->close)
	(*minidriver_iface->close)(context);
#ifdef PM2_TOPOLOGY
      hwloc_bitmap_free(p_minidriver_context->trks_array[i].props.profile.cpuset);
#endif
    }
#ifdef PM2_TOPOLOGY
  hwloc_bitmap_free(p_drv->profile.cpuset);
#endif
  free(p_minidriver_context->url);
  free(p_minidriver_context);
  free(p_minidriver_drv);
  return NM_ESUCCESS;
}


static int nm_minidriver_connect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[trk_id].minidriver;
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
  nm_data_null_build(&status->trks[trk_id].sdata);
  nm_data_null_build(&status->trks[trk_id].rdata);

  free(orig_url_chunks);
  return NM_ESUCCESS;
}

static int nm_minidriver_disconnect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id)
{
  int err;
  
  /* TODO */
  
  err = NM_ESUCCESS;
  
  return err;
}

/* ********************************************************* */

/* ** driver I/O functions ********************************* */

static int nm_minidriver_post_send_iov(void*_status, struct nm_pkt_wrap_s*__restrict__ p_pw)
{
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  if(p_pw->p_data != NULL)
    {
      assert(minidriver->driver->send_data != NULL);
      (*minidriver->driver->send_data)(minidriver->_status, p_pw->p_data, p_pw->chunk_offset, p_pw->length);
    }
  else
    {
      if(minidriver->driver->send_post)
	{
	  (*minidriver->driver->send_post)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
	}
      else
	{
	  assert(minidriver->driver->send_data);
	  struct nm_data_s*p_data = &status->trks[p_pw->trk_id].sdata;
	  assert(nm_data_isnull(p_data));
	  nm_data_iov_set(p_data, (struct nm_data_iov_s){ .v = &p_pw->v[0], .n = p_pw->v_nb });
	  (*minidriver->driver->send_data)(minidriver->_status, p_data, 0 /* chunk_offset */, p_pw->length);
	}
    }
  int err = nm_minidriver_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_minidriver_poll_send_iov(void*_status, struct nm_pkt_wrap_s*__restrict__ p_pw)
{
  struct nm_minidriver*status = _status;
  struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
  int err = (*minidriver->driver->send_poll)(minidriver->_status);
#ifdef DEBUG
  if((err == NM_ESUCCESS) && (p_pw->p_data == NULL) && (minidriver->driver->send_post == NULL))
    {
      struct nm_data_s*p_data = &status->trks[p_pw->trk_id].sdata;
      assert(!nm_data_isnull(p_data));
      nm_data_null_build(p_data);
    }
#endif /* DEBUG */
  return err;
}

static int nm_minidriver_send_prefetch(void*_status, struct nm_pkt_wrap_s *p_pw)
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

static int nm_minidriver_poll_recv_iov(void*_status, struct nm_pkt_wrap_s*__restrict__ p_pw)
{
  int err = -NM_EAGAIN;
  if(_status)
    {
      struct nm_minidriver*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      err = (*minidriver->driver->poll_one)(minidriver->_status);
#ifdef DEBUG
      if((err == NM_ESUCCESS) && (p_pw->p_data == NULL) && (minidriver->driver->recv_init == NULL))
	{
	  struct nm_data_s*p_data = &status->trks[p_pw->trk_id].rdata;
	  assert(!nm_data_isnull(p_data));
	  nm_data_null_build(p_data);
	}
#endif /* DEBUG */
    }
  else
    {
      NM_FATAL("poll_any not implemented yet.");
    }
  return err;
}

static int nm_minidriver_post_recv_iov(void*_status, struct nm_pkt_wrap_s*__restrict__ p_pw)
{
  int err = NM_ESUCCESS;
  if(_status)
    {
      struct nm_minidriver*status = _status;
      struct puk_receptacle_NewMad_minidriver_s*minidriver = &status->trks[p_pw->trk_id].minidriver;
      if((p_pw->p_data != NULL) && (p_pw->v_nb == 0))
	{
	  /* pw content is only p_data */
	  assert(minidriver->driver->recv_data != NULL);
	  (*minidriver->driver->recv_data)(minidriver->_status, p_pw->p_data, p_pw->chunk_offset, p_pw->length);
	}
      else
	{
	  /* no p_data, or data has been flattened */
	  if(minidriver->driver->recv_init)
	    {
	      (*minidriver->driver->recv_init)(minidriver->_status, &p_pw->v[0], p_pw->v_nb);
	    }
	  else
	    {
	      assert(minidriver->driver->recv_data);
	      struct nm_data_s*p_data = &status->trks[p_pw->trk_id].rdata;
	      assert(nm_data_isnull(p_data));
	      nm_data_iov_set(p_data, (struct nm_data_iov_s){ .v = &p_pw->v[0], .n = p_pw->v_nb });
	      (*minidriver->driver->recv_data)(minidriver->_status, p_data, 0 /* chunk_offset */, p_pw->length);
	    }
	}
    }
  err = nm_minidriver_poll_recv_iov(_status, p_pw);
  return err;
}

static int nm_minidriver_cancel_recv_iov(void*_status, struct nm_pkt_wrap_s *p_pw)
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
