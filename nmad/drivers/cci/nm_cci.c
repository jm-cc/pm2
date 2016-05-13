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

#include <cci.h>
#undef PACKAGE_NAME
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_SUPPORT
#undef PACKAGE_TARNAME
#undef PACKAGE_BUGREPORT

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#include <nm_private.h>

#ifdef PUKABI
#include <Padico/Puk-ABI.h>
#endif /* PUKABI */

#include <Padico/Module.h>

#ifdef PUKABI
#define NM_CCI_RCACHE 1
#endif

#define NM_CCI_URL_SIZE_MAX 64
/** identity of peer node */
struct nm_cci_peer_s
{
  nm_trk_id_t trk_id;
  char url[NM_CCI_URL_SIZE_MAX];
};
/** header for control messages on trk #1 */
struct nm_cci_header_s
{
  uint64_t op;
  cci_rma_handle_t handle;
  uint64_t offset;
};
#define NM_CCI_OP_NONE     0x00
#define NM_CCI_OP_RTR      0x01
#define NM_CCI_OP_COMPLETE 0x02

PUK_LIST_TYPE(nm_cci_unexpected,
	      void*ptr;
	      size_t size;
	      );

/** connection (trk, gate) */
struct nm_cci_connection_s
{
  cci_connection_t*connection;
  struct nm_cci_peer_s peer;
  union
  {
    struct
    {
      /** pw posted for short recv */
      struct nm_pkt_wrap*p_pw;
      /** unexpected chunks of data */
      nm_cci_unexpected_list_t unexpected;
    } trk_small;
    struct
    {
      /** pw posted for large recv */
      struct nm_pkt_wrap*p_recv_pw;
#ifdef NM_CCI_RCACHE
      /** rcache entry for large send */
      const struct puk_mem_reg_s*send_rcache;
      /** rcache entry for large recv */
      const struct puk_mem_reg_s*recv_rcache;
#else
      /** rma handle for large send */
      cci_rma_handle_t*local_handle;
#endif /* NM_CCI_RCACHE */
      /** received header for large send */
      struct nm_cci_header_s hdr;
    } trk_large;
  } info;
};
PUK_VECT_TYPE(nm_cci_connection, struct nm_cci_connection_s*);

/** 'cci' driver per-instance data.
 */
struct nm_cci_drv
{
  cci_endpoint_t*endpoint;
  char*url;
  int nb_trks;
  nm_cci_connection_vect_t pending;
};

/** 'cci' per-gate data.
 */
struct nm_cci
{
  /** one connection per track.
   */
  struct nm_cci_connection_s*conns[2];
};



/** CCI NewMad Driver */
static const char*nm_cci_get_driver_url(nm_drv_t p_drv);
static int nm_cci_query(nm_drv_t p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_cci_init(nm_drv_t p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_cci_exit(nm_drv_t p_drv);
static int nm_cci_connect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_cci_disconnect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id);
static int nm_cci_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_cci_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_cci_poll_send(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_cci_poll_recv(void*_status, struct nm_pkt_wrap *p_pw);

static const struct nm_drv_iface_s nm_cci_driver =
  {
    .name               = "cci",
    
    .query              = &nm_cci_query,
    .init               = &nm_cci_init,
    .close              = &nm_cci_exit,

    .connect		= &nm_cci_connect,
    .disconnect         = &nm_cci_disconnect,

    .post_send_iov	= &nm_cci_send_iov,
    .post_recv_iov      = &nm_cci_recv_iov,

    .poll_send_iov	= &nm_cci_poll_send,
    .poll_recv_iov	= &nm_cci_poll_recv,

    .get_driver_url     = &nm_cci_get_driver_url,

    .capabilities.is_exportable = 0,
    .capabilities.min_period    = 0,
    .capabilities.max_unexpected = 1024
  };

/** 'PadicoComponent' facet for cci driver */
static void*nm_cci_instantiate(puk_instance_t, puk_context_t);
static void nm_cci_destroy(void*);

static const struct puk_component_driver_s nm_cci_component_driver =
  {
    .instantiate = &nm_cci_instantiate,
    .destroy     = &nm_cci_destroy
  };


/** Component declaration */
PADICO_MODULE_COMPONENT(NewMad_Driver_cci,
  puk_component_declare("NewMad_Driver_cci",
			puk_component_provides("PadicoComponent", "component", &nm_cci_component_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_cci_driver)));


static void*nm_cci_register(void*context, const void*ptr, size_t len)
{
  cci_endpoint_t*endpoint = context;
  cci_rma_handle_t*handle = NULL;
  int rc = cci_rma_register(endpoint,(void*)ptr, len, CCI_FLAG_WRITE | CCI_FLAG_READ, &handle);
  if(rc != CCI_SUCCESS)
    {
      fprintf(stderr, "nmad: cci- error in register.\n");
      abort();
    }
  return (void*)handle;
}

static void nm_cci_unregister(void*context, const void*ptr, void*key)
{
  cci_rma_handle_t*handle = key;
  cci_endpoint_t*endpoint = context;
  int rc = cci_rma_deregister(endpoint, handle);
  if(rc != CCI_SUCCESS)
    {
      fprintf(stderr, "nmad: cci- error in deregister.\n");
      abort();
    }
}

/** Instanciate functions */
static void*nm_cci_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_cci*status = TBX_MALLOC(sizeof(struct nm_cci));
  memset(status, 0, sizeof(struct nm_cci));
  return status;
}

static void nm_cci_destroy(void*_status)
{
  TBX_FREE(_status);
}

/** Url function */
static const char*nm_cci_get_driver_url(nm_drv_t p_drv)
{
  struct nm_cci_drv *p_cci_drv = p_drv->priv;
  return p_cci_drv->url;
}


static int nm_cci_query(nm_drv_t p_drv,
			struct nm_driver_query_param *params TBX_UNUSED,
			int nparam TBX_UNUSED)
{
  struct nm_cci_drv* p_cci_drv = TBX_MALLOC(sizeof(struct nm_cci_drv));
  memset(p_cci_drv, 0, sizeof(struct nm_cci_drv));
  p_drv->profile.latency = 10000;
  p_drv->profile.bandwidth = 1000;
  p_drv->priv = p_cci_drv;
  return NM_ESUCCESS;
}

static int nm_cci_init(nm_drv_t p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_cci_drv*p_cci_drv = p_drv->priv;

  uint32_t caps;
  int rc = cci_init(CCI_ABI_VERSION , 0, &caps);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: cci- initialization error %d.\n", rc);
      abort();
    }

  rc = cci_create_endpoint(NULL, 0, &p_cci_drv->endpoint, NULL);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: cci- create endpoint error %d.\n", rc);
      abort();
    }

  char*url = NULL;
  rc = cci_get_opt(p_cci_drv->endpoint, CCI_OPT_ENDPT_URI, &url);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: cci- get opt error %d.\n", rc);
      abort();
    }
  p_cci_drv->url = strdup(url);

  p_cci_drv->pending = nm_cci_connection_vect_new();

#ifdef PUKABI
  puk_mem_set_handlers(&nm_cci_register, &nm_cci_unregister);
#endif /* PUKABI */

  /* open the requested number of tracks */
  int i;
  for(i = 0; i < nb_trks; i++)
    {
      /* track capabilities encoding */
      trk_caps[i].rq_type	= nm_trk_rq_dgram;
      trk_caps[i].iov_type	= nm_trk_iov_both_assym;
      trk_caps[i].max_pending_send_request  = 1;
      trk_caps[i].max_pending_recv_request  = 1;
      trk_caps[i].max_single_request_length = SSIZE_MAX;
      trk_caps[i].max_iovec_request_length  = 0;
#ifdef IOV_MAX
      trk_caps[i].max_iovec_size	    = IOV_MAX;
#else /* IOV_MAX */
      trk_caps[i].max_iovec_size	    = sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */
    }

  return NM_ESUCCESS;
}

static int nm_cci_exit(nm_drv_t p_drv)
{
  struct nm_cci_drv*p_cci_drv = p_drv->priv;
  TBX_FREE(p_cci_drv->url);
  TBX_FREE(p_cci_drv);
  return NM_ESUCCESS;
}

static struct nm_cci_connection_s*nm_cci_connection_new(void)
{
  struct nm_cci_connection_s*conn = TBX_MALLOC(sizeof(struct nm_cci_connection_s));
  memset(conn, 0, sizeof(struct nm_cci_connection_s));
  conn->connection = NULL;
  return conn;
}


static void nm_cci_poll(struct nm_cci_drv*p_cci_drv)
{
  cci_event_t*event;
  int rc = cci_get_event(p_cci_drv->endpoint, &event);
  if(rc == CCI_SUCCESS)
    {
      switch(event->type)
	{
	case CCI_EVENT_SEND:
	  {
	    struct nm_pkt_wrap*p_pw = event->send.context;
	    if(p_pw != NULL)
	      {
		if(p_pw->trk_id == NM_TRK_LARGE)
		  {
		    struct nm_cci_connection_s*conn = p_pw->drv_priv;
#ifdef NM_CCI_RCACHE
		    puk_mem_unreg(conn->info.trk_large.send_rcache);
#else /* NM_CCI_RCACHE */
		    int rc = cci_rma_deregister(p_cci_drv->endpoint, conn->info.trk_large.local_handle);
		    if(rc != CCI_SUCCESS)
		      {
			fprintf(stderr, "nmad: cci- error in deregister.\n");
			abort();
		      }
#endif /* NM_CCI_RCACHE */
		  }
		p_pw->drv_priv = NULL;
	      }
	  }
	  break;

	case CCI_EVENT_RECV:
	  {
	    struct nm_cci_connection_s*conn = event->recv.connection->context;
	    const void*ptr = event->recv.ptr;
	    const size_t size = event->recv.len;
	    if(conn->peer.trk_id == NM_TRK_SMALL)
	      {
		struct nm_pkt_wrap*p_pw = conn->info.trk_small.p_pw;
		if(p_pw == NULL)
		  {
		    if(conn->info.trk_small.unexpected == NULL)
		      {
			conn->info.trk_small.unexpected = nm_cci_unexpected_list_new();
		      }
		    nm_cci_unexpected_t u = nm_cci_unexpected_new();
		    u->ptr = TBX_MALLOC(size);
		    u->size = size;
		    memcpy(u->ptr, ptr, size);
		    nm_cci_unexpected_list_push_back(conn->info.trk_small.unexpected, u);
		  }
		else
		  {
		    if(size > p_pw->v[0].iov_len)
		      {
			fprintf(stderr, "nmad: cci- received more data than posted.\n");
			abort();
		      }
		    memcpy(p_pw->v[0].iov_base, ptr, size);
		    p_pw->drv_priv = (void*)0x00;
		  }
	      }
	    else
	      {
		assert(size == sizeof(struct nm_cci_header_s));
		const struct nm_cci_header_s*hdr = ptr;
		switch(hdr->op)
		  {
		  case NM_CCI_OP_RTR:
		    {
		      memcpy(&conn->info.trk_large.hdr, ptr, size);
		    }
		    break;
		  case NM_CCI_OP_COMPLETE:
		    {
		      struct nm_pkt_wrap*p_pw = conn->info.trk_large.p_recv_pw;
		      p_pw->drv_priv = (void*)0x00;
#ifdef NM_CCI_RCACHE
		      puk_mem_unreg(conn->info.trk_large.recv_rcache);
#else /* NM_CCI_RCACHE */
		      const cci_rma_handle_t handle = hdr->handle;
		      int rc = cci_rma_deregister(p_cci_drv->endpoint, &handle);
		      if(rc != CCI_SUCCESS)
			{
			  fprintf(stderr, "nmad: cci- error in deregister.\n");
			  abort();
			}
#endif /* NM_CCI_RCACHE */
		    }
		    break;
		  case NM_CCI_OP_NONE:
		    break;
		  }
	      }
	  }
	  break;

	case CCI_EVENT_CONNECT:
	  if(event->connect.status == CCI_SUCCESS)
	    {
	      struct nm_cci_connection_s*conn = event->connect.context;
	      conn->connection = event->connect.connection;
	    }
	  break;

	case CCI_EVENT_CONNECT_REQUEST:
	  {
	    struct nm_cci_connection_s*conn = nm_cci_connection_new();
	    memcpy(&conn->peer, event->request.data_ptr, sizeof(struct nm_cci_peer_s));
	    rc = cci_accept(event, conn);
	    if(rc != CCI_SUCCESS)
	      {
		fprintf(stderr, "nmad: cci- error in accept.\n");
		abort();
	      }
	  }
	  break;

	case CCI_EVENT_ACCEPT:
	  {
	    struct nm_cci_connection_s*conn = event->accept.context;
	    cci_connection_t*connection = event->accept.connection;
	    conn->connection = connection;
	    nm_cci_connection_vect_push_back(p_cci_drv->pending, conn);
	  }
	  break;

	default:
	  fprintf(stderr, "nmad: cci- unexpected event %d.\n", event->type);
	  break;
	}
      cci_return_event(event);
    }
}

static int nm_cci_connect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  struct nm_cci_drv*p_cci_drv = p_drv->priv;
  struct nm_cci*status = (struct nm_cci*)_status;

  if(strcmp(p_cci_drv->url, remote_url) > 0)
    {
      /* ** connect */
      struct nm_cci_peer_s local;
      strncpy(local.url, p_cci_drv->url, NM_CCI_URL_SIZE_MAX);
      local.trk_id = trk_id;
      struct nm_cci_connection_s*conn = nm_cci_connection_new();
      strncpy(conn->peer.url, remote_url, NM_CCI_URL_SIZE_MAX);
      conn->peer.trk_id = trk_id;
      int rc = cci_connect(p_cci_drv->endpoint, remote_url, &local, sizeof(local), CCI_CONN_ATTR_RU, conn, 0, NULL);
      if(rc != CCI_SUCCESS)
	{
	  fprintf(stderr, "nmad: cci- connect error.\n");
	  abort();
	}
      while(conn->connection == NULL)
	{
	  nm_cci_poll(p_cci_drv);
	}
      status->conns[trk_id] = conn;
    }
  else
    {
      /* ** accept */
      while(status->conns[trk_id] == NULL)
	{
	  nm_cci_poll(p_cci_drv);
	  nm_cci_connection_vect_itor_t i;
	  puk_vect_foreach(i, nm_cci_connection, p_cci_drv->pending)
	    {
	      if((strcmp((*i)->peer.url, remote_url) == 0) && ((*i)->peer.trk_id == trk_id))
		{
		  status->conns[trk_id] = (*i);
		  nm_cci_connection_vect_erase(p_cci_drv->pending, i);
		  break;
		}
	    }
	}
    }
  return NM_ESUCCESS;
}

static int nm_cci_disconnect(void*_status, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id)
{
  struct nm_cci*status = (struct nm_cci*)_status;
#warning TODO-
  return NM_ESUCCESS;
}

static int nm_cci_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_cci*status = (struct nm_cci*)_status;
  struct nm_cci_connection_s*conn = status->conns[p_pw->trk_id];
  p_pw->drv_priv = conn;
  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      struct iovec*iov = p_pw->v;
      const int n = p_pw->v_nb;
      int rc = cci_sendv(conn->connection, iov, n, p_pw, CCI_FLAG_NO_COPY);
      if(rc != CCI_SUCCESS)
	{
	  fprintf(stderr, "nmad: cci- error while sending.\n");
	  abort();
	}
    }
  else
    {
      struct nm_cci_drv*p_cci_drv = p_pw->p_drv->priv;
#ifdef NM_CCI_RCACHE
      conn->info.trk_large.send_rcache = puk_mem_reg((void*)p_cci_drv->endpoint, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
#else /* NM_CCI_RCACHE */
      int rc = cci_rma_register(p_cci_drv->endpoint, p_pw->v[0].iov_base, p_pw->v[0].iov_len, CCI_FLAG_WRITE | CCI_FLAG_READ, &conn->info.trk_large.local_handle);
      if(rc != CCI_SUCCESS)
	{
	  fprintf(stderr, "nmad: cci- error in register.\n");
	  abort();
	}
#endif
    }
  return nm_cci_poll_send(_status, p_pw);
}

static int nm_cci_poll_send(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_cci*status = (struct nm_cci*)_status;
  struct nm_cci_drv*p_cci_drv = p_pw->p_drv->priv;
  nm_cci_poll(p_cci_drv);
  if(p_pw->trk_id == NM_TRK_LARGE)
    {
      struct nm_cci_connection_s*conn = status->conns[p_pw->trk_id];
      if(conn->info.trk_large.hdr.op == NM_CCI_OP_RTR)
	{
	  conn->info.trk_large.hdr.op = NM_CCI_OP_NONE;
	  struct nm_cci_header_s hdr;
	  hdr.op = NM_CCI_OP_COMPLETE;
	  memcpy((void*)&hdr.handle, &conn->info.trk_large.hdr.handle, sizeof(cci_rma_handle_t));
	  hdr.offset = conn->info.trk_large.hdr.offset;
	  cci_rma_handle_t*handle = NULL;
#ifdef NM_CCI_RCACHE
	  handle = conn->info.trk_large.send_rcache->key;
#else
	  handle = conn->info.trk_large.local_handle;
#endif
	  int rc = cci_rma(conn->connection, &hdr, sizeof(hdr), handle, 0, &conn->info.trk_large.hdr.handle, conn->info.trk_large.hdr.offset, p_pw->v[0].iov_len, p_pw, CCI_FLAG_WRITE);
	  if(rc != CCI_SUCCESS)
	    {
	      fprintf(stderr, "nmad: cci- error in rma.\n");
	      abort();
	    }
	}
    }
  return (p_pw->drv_priv == NULL) ? NM_ESUCCESS : -NM_EAGAIN;
}

static int nm_cci_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_cci*status = (struct nm_cci*)_status;
  struct nm_cci_connection_s*conn = status->conns[p_pw->trk_id];
  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      assert(conn->info.trk_small.p_pw == NULL);
      if(p_pw->v_nb != 1)
	{
	  fprintf(stderr, "nmad: cci- iovec not supported on recv side yet.\n");
	  abort();
	}
      if(conn->info.trk_small.unexpected != NULL && !nm_cci_unexpected_list_empty(conn->info.trk_small.unexpected))
	{
	  nm_cci_unexpected_t u = nm_cci_unexpected_list_pop_front(conn->info.trk_small.unexpected);
	  memcpy(p_pw->v[0].iov_base, u->ptr, u->size);
	  nm_cci_unexpected_delete(u);
	}
      else
	{
	  p_pw->drv_priv = (void*)0x01;
	  conn->info.trk_small.p_pw = p_pw;
	}
    }
  else
    {
      int rc;
      struct nm_cci_drv*p_cci_drv = p_pw->p_drv->priv;
      conn->info.trk_large.p_recv_pw = p_pw;
      p_pw->drv_priv = (void*)0x01;
      cci_rma_handle_t*handle = NULL;
#ifdef NM_CCI_RCACHE
      conn->info.trk_large.recv_rcache = puk_mem_reg((void*)p_cci_drv->endpoint, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
      handle = conn->info.trk_large.recv_rcache->key;
#else /* NM_CCI_RCACHE */
      rc = cci_rma_register(p_cci_drv->endpoint, p_pw->v[0].iov_base, p_pw->v[0].iov_len, CCI_FLAG_WRITE, &handle);
      if(rc != CCI_SUCCESS)
	{
	  fprintf(stderr, "nmad: cci- error in register.\n");
	  abort();
	}
#endif /* NM_CCI_RCACHE */
      struct nm_cci_header_s hdr = { .op = NM_CCI_OP_RTR, .handle = *handle, .offset = 0 };
      rc = cci_send(conn->connection, &hdr, sizeof(hdr), NULL, CCI_FLAG_SILENT);
      if(rc != CCI_SUCCESS)
	{
	  fprintf(stderr, "nmad: cci- error while sending header.\n");
	  abort();
	}
    }
  return nm_cci_poll_recv(_status, p_pw);
}


static int nm_cci_poll_recv(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_cci*status = (struct nm_cci*)_status;
  struct nm_cci_connection_s*conn = status->conns[p_pw->trk_id];
  struct nm_cci_drv*p_cci_drv = p_pw->p_drv->priv;
  nm_cci_poll(p_cci_drv);
  if(p_pw->drv_priv == (void*)0x00)
    {
      conn->info.trk_small.p_pw = NULL;
      return NM_ESUCCESS;
    }
  return -NM_EAGAIN;
}

