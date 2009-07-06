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


#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>
#include <assert.h>

#include <elan/elan.h>
#include <elan/capability.h>
#include <qsnet/fence.h>

#include <nm_private.h>


#define INITIAL_PW_NUM		16
#define NM_QSNET_USE_ELAN_DONE

#ifdef  NM_QSNET_USE_ELAN_DONE
#warning using elan_done event notification
#else
#warning using elan_TxWait/elan_RxWait event notification
#endif

struct nm_qsnet_drv
{
  /** memory allocator for private pw data */
  p_tbx_memory_t qsnet_pw_mem;
  
  /** main elan entry point */
  ELAN_BASE	*base;
  
  /** process rank from elan point of view */
  u_int		 proc;
  
  /** number of processes in the session */
  u_int		 nproc;

  /** Driver capabilities */
  struct nm_drv_cap caps;
  
  /** url */
  char*url;

  /** tracks of the QsNet driver */
  struct nm_qsnet_trk*trks_array;
  /** number of tracks */
  int nb_trks;
};

struct nm_qsnet_trk
{
  /** elan tagged port */
  ELAN_TPORT		*p;
  
  /** elan queue used by the tagged port */
  ELAN_QUEUE		*q;
  
  /** process rank from elan point of view */
  u_int		 	 proc;
  
  /** number of processes in the session */
  u_int		 	 nproc;
  
  /** match info to gate id reverse mapping */
  uint8_t	       	*gate_map;
  
  /** gate_map size */
  uint8_t	       	 gate_map_size;
};

struct nm_qsnet_cnx
{
  /** remote rank from elan point of view */
  u_int		 remote_proc;
};

struct nm_qsnet
{
  struct nm_qsnet_cnx	cnx_array[255];
};

struct nm_qsnet_pkt_wrap
{
  ELAN_EVENT	*ev;
};

struct nm_qsnet_adm_pkt
{
  char drv_url[16];
};

/** Qsnet NewMad Driver */
static int nm_qsnet_query(struct nm_drv *p_drv, struct nm_driver_query_param *params,int nparam);
static int nm_qsnet_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_qsnet_close(struct nm_drv *p_drv);
static int nm_qsnet_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_qsnet_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_qsnet_disconnect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_qsnet_post_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_qsnet_post_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_qsnet_poll_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_qsnet_poll_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static struct nm_drv_cap*nm_qsnet_get_capabilities(struct nm_drv *p_drv);
static const char*nm_qsnet_get_driver_url(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_qsnet_driver =
  {
    .name               = "qsnet",

    .query              = &nm_qsnet_query,
    .init               = &nm_qsnet_init,
    .close	        = &nm_qsnet_close,
    
    .connect		= &nm_qsnet_connect,
    .accept		= &nm_qsnet_accept,
    .disconnect         = &nm_qsnet_disconnect,
    
    .post_send_iov	= &nm_qsnet_post_send_iov,
    .post_recv_iov      = &nm_qsnet_post_recv_iov,
    
    .poll_send_iov      = &nm_qsnet_poll_send_iov,
    .poll_recv_iov      = &nm_qsnet_poll_recv_iov,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .get_driver_url     = &nm_qsnet_get_driver_url,
    .get_track_url      = NULL,
    .get_capabilities   = &nm_qsnet_get_capabilities

  };

/** 'PadicoAdapter' facet for Qsnet driver */
static void*nm_qsnet_instanciate(puk_instance_t, puk_context_t);
static void nm_qsnet_destroy(void*);

static const struct puk_adapter_driver_s nm_qsnet_adapter_driver =
  {
    .instanciate = &nm_qsnet_instanciate,
    .destroy     = &nm_qsnet_destroy
  };

/** Component declaration */
static int nm_qsnet_load(void)
{
  puk_component_declare("NewMad_Driver_qsnet",
			puk_component_provides("PadicoAdapter", "adapter", &nm_qsnet_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_qsnet_driver));
  return 0;
}
PADICO_MODULE_BUILTIN(NewMad_Driver_qsnet, &nm_qsnet_load, NULL, NULL);

/** Return capabilities */
static struct nm_drv_cap*nm_qsnet_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_qsnet_drv*p_qsnet_drv = p_drv->priv;
  return &p_qsnet_drv->caps;
}

/** Return url */
static const char*nm_qsnet_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_qsnet_drv*p_qsnet_drv = p_drv->priv;
  return p_qsnet_drv->url;
}
/** Component instanciation */
static void*nm_qsnet_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_qsnet*status = TBX_MALLOC(sizeof (struct nm_qsnet));
  memset(status, 0, sizeof(struct nm_qsnet));
  return status;
}

static void nm_qsnet_destroy(void*_status)
{
  TBX_FREE(_status);
}

/* prototypes
 */
static int nm_qsnet_poll_send_iov(void*_status, struct nm_pkt_wrap *p_pw);

static int nm_qsnet_poll_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);


static int nm_qsnet_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam)
{
  int err;
  struct nm_qsnet_drv*p_qsnet_drv = NULL;
  
  /* private data                                                 */
  p_qsnet_drv	= TBX_MALLOC(sizeof (struct nm_qsnet_drv));
  if (!p_qsnet_drv) {
    err = -NM_ENOMEM;
    goto out;
  }
  
  memset(p_qsnet_drv, 0, sizeof (struct nm_qsnet_drv));
  
  /* driver capabilities encoding					*/
  p_qsnet_drv->caps.has_trk_rq_dgram			= 1;
  p_qsnet_drv->caps.has_selective_receive		        = 1;
  p_qsnet_drv->caps.has_concurrent_selective_receive	= 0;
#ifdef PM2_NUIOA
  p_qsnet_drv->caps.numa_node = PM2_NUIOA_ANY_NODE;
  p_qsnet_drv->caps.latency = 225 ; /* from sr_ping */
  p_qsnet_drv->caps.bandwidth = 837; /* from sr_ping, hardcode 900 instead? */
#endif
  
  p_drv->priv = p_qsnet_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_qsnet_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  ELAN_BASE		*base		= NULL;
  char			*node_string	= NULL;
  p_tbx_string_t	 url_string	= NULL;
  int err = -NM_EINVAL;
  struct nm_qsnet_drv*p_qsnet_drv = p_drv->priv;
  
  NM_LOG_IN();

  node_string = getenv("LIBELAN_ECAP0");
  if (node_string)
    goto capability_generated;
  
  node_string = getenv("ELAN_NODE_STRING");
  if (!node_string)
    goto out;
  
  if (elan_generateCapability (node_string) < 0)
    goto out;
  
 capability_generated:
  err = -NM_ESCFAILD;
  if (!(base = elan_baseInit(0))) {
    perror("elan_baseInit");
    goto out;
  }

  p_qsnet_drv->base	= base;
  p_qsnet_drv->proc	= base->state->vp;
  p_qsnet_drv->nproc	= base->state->nvp;
  
  NM_TRACE_VAL("proc", p_qsnet_drv->proc);
  NM_TRACE_VAL("nproc", p_qsnet_drv->nproc);
  
  tbx_malloc_init(&(p_qsnet_drv->qsnet_pw_mem),   sizeof(struct nm_qsnet_pkt_wrap),
		  INITIAL_PW_NUM,   "nmad/qsnet/pw");
  
  url_string	= tbx_string_init_to_int(p_qsnet_drv->proc);
  p_qsnet_drv->url	= tbx_string_to_cstring(url_string);
  NM_TRACE_STR("p_qsnet_drv->url", p_qsnet_drv->url);
  tbx_string_free(url_string);
  
  /* open requested tracks */
  p_qsnet_drv->nb_trks = nb_trks;
  p_qsnet_drv->trks_array = TBX_MALLOC(nb_trks * sizeof(struct nm_qsnet_trk));
  nm_trk_id_t trk_id;
  for(trk_id = 0; trk_id < nb_trks; trk_id++)
    {
      struct nm_qsnet_drv *p_qsnet_drv = p_drv->priv;
      struct nm_qsnet_trk *p_qsnet_trk= &p_qsnet_drv->trks_array[trk_id];
      ELAN_TPORT *p = NULL;
      ELAN_QUEUE *q = NULL;
      
      memset(p_qsnet_trk, 0, sizeof(struct nm_qsnet_trk));
      
      /* track capabilities encoding					*/
      trk_caps[trk_id].rq_type	= nm_trk_rq_dgram;
      trk_caps[trk_id].iov_type	= nm_trk_iov_none;
      trk_caps[trk_id].max_pending_send_request  = 2;
      trk_caps[trk_id].max_pending_recv_request  = 2;
      trk_caps[trk_id].max_single_request_length = SSIZE_MAX;
      
      err	= -NM_ESCFAILD;
      if ((q = elan_gallocQueue(base, base->allGroup)) == NULL) {
	perror("elan_gallocQueue");
	goto out;
      }
      
      if (!(p = elan_tportInit(base->state,
			       q,
			       base->tport_nslots,
			       base->tport_smallmsg, base->tport_bigmsg,
			       base->tport_stripemsg,
			       base->waitType, base->retryCount,
			       &base->shm_key, base->shm_fifodepth,
			       base->shm_fragsize, 0))) {
	perror("elan_tportInit");
	goto out;
      }

      p_qsnet_trk->p		 = p;
      p_qsnet_trk->q		 = q;
      p_qsnet_trk->gate_map_size = 0;
      p_qsnet_trk->proc	         = p_qsnet_drv->proc;
      p_qsnet_trk->nproc	 = p_qsnet_drv->nproc;
      
    }

  err = NM_ESUCCESS;
  
 out:
  NM_LOG_OUT();
  
  return err;
}

static int nm_qsnet_close(struct nm_drv *p_drv)
{
  struct nm_qsnet_drv *p_qsnet_drv = p_drv->priv;
  int err = NM_ESUCCESS;
  TBX_FREE( p_qsnet_drv->trks_array);
  return err;
}

static int nm_qsnet_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_qsnet	*status	        = _status;
  struct nm_gate	*p_gate 	= p_crq->p_gate;
  struct nm_drv		*p_drv		= p_crq->p_drv;
  struct nm_qsnet_drv   *p_qsnet_drv    = p_drv->priv;
  struct nm_qsnet_trk	*p_qsnet_trk	= &p_qsnet_drv->trks_array[p_crq->trk_id];
  struct nm_qsnet_cnx	*p_qsnet_cnx	= &status->cnx_array[p_crq->trk_id];
  ELAN_TPORT		*p		= p_qsnet_trk->p;
  u_int			 local_proc	= p_qsnet_trk->proc;
  u_int			 remote_proc	= 0;
  char			*drv_url	= p_crq->remote_drv_url;
  int err;
  struct nm_qsnet_adm_pkt	 pkt;

  NM_LOG_IN();
  
  /* process remote process URL
   */
  NM_TRACEF("connect - drv_url: %s", drv_url);
  
  {
    char *ptr = NULL;
    remote_proc = strtol(drv_url, &ptr, 0);
    p_qsnet_cnx->remote_proc = remote_proc;
  }

  /* manage gate reverse map storage
   */
  if (remote_proc >= p_qsnet_trk->gate_map_size)
    {
      p_qsnet_trk->gate_map = TBX_REALLOC(p_qsnet_trk->gate_map, remote_proc+1);
    }
  
  p_qsnet_trk->gate_map[remote_proc]	= p_gate->id;
  NM_TRACEF("new gate mapping: gate %d = remote_proc %d", p_gate->id, remote_proc);
  
  
  /* send our own URL to remote process
   */
  NM_TRACEF("send pkt -->");
  {
    ELAN_EVENT	*ev	= NULL;
    
    strcpy(pkt.drv_url, p_qsnet_drv->url);
    NM_TRACEF("connect - pkt.drv_url: %s",		pkt.drv_url);
    
    /* we use tag 1 for administrative pkts and tag 2 for anything else
     */
    NM_TRACEF("remote = %d, local = %d", remote_proc, local_proc);
    ev = elan_tportTxStart(p, 0, remote_proc, local_proc,
			   1, &pkt, sizeof(pkt));
    elan_tportTxWait(ev);
  }
  NM_TRACEF("send pkt <--");
  
  err = NM_ESUCCESS;
  NM_LOG_OUT();
  
  return err;
}

static int nm_qsnet_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_qsnet	*status	        =  _status;
  struct nm_gate	*p_gate		= p_crq->p_gate;
  struct nm_drv		*p_drv		= p_crq->p_drv;
  struct nm_qsnet_drv   *p_qsnet_drv    = p_drv->priv;
  struct nm_qsnet_trk	*p_qsnet_trk	= &p_qsnet_drv->trks_array[p_crq->trk_id];
  struct nm_qsnet_cnx	*p_qsnet_cnx	= &status->cnx_array[p_crq->trk_id];
  ELAN_TPORT		*p		= p_qsnet_trk->p;
  int			 remote_proc	= 0;
  int err;
  struct nm_qsnet_adm_pkt pkt;
  
  NM_LOG_IN();
  
  NM_TRACEF("recv pkt -->");
  {
    ELAN_EVENT	*ev	= NULL;
    int tag;
    size_t size;
    
    ev	= elan_tportRxStart(p, 0, 0, 0, -1, 1, &pkt, sizeof(pkt));
    elan_tportRxWait(ev, &remote_proc, &tag, &size);
    NM_TRACEF("remote_proc = %d, tag = %d, size = %d",
	      remote_proc, tag, size);
    
  }
  NM_TRACEF("recv pkt <--");
  
  NM_TRACEF("accept - pkt.drv_url: %s",	pkt.drv_url);
  p_qsnet_cnx->remote_proc = remote_proc;
  
  /* manage gate reverse map storage
   */
  if (remote_proc >= p_qsnet_trk->gate_map_size)
    {
      p_qsnet_trk->gate_map = TBX_REALLOC(p_qsnet_trk->gate_map, remote_proc+1);
    }
  
  p_qsnet_trk->gate_map[remote_proc]	= p_gate->id;
  NM_TRACEF("new gate mapping: gate %d = remote_proc %d",
	    p_gate->id, remote_proc);
  
  err = NM_ESUCCESS;
  NM_LOG_OUT();
  
  return err;
}

static int nm_qsnet_disconnect(void*_status, struct nm_cnx_rq *p_crq)
{
  int err = NM_ESUCCESS;
  return err;
}

static int nm_qsnet_post_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_qsnet	*status	        = _status;
  struct nm_gate	*p_gate		= p_pw->p_gate;
  struct nm_drv		*p_drv		= p_pw->p_drv;
  struct nm_qsnet_drv   *p_qsnet_drv    = p_drv->priv;
  struct nm_qsnet_trk	*p_qsnet_trk	= &p_qsnet_drv->trks_array[p_pw->trk_id];
  struct nm_qsnet_cnx	*p_qsnet_cnx	= &status->cnx_array[p_pw->trk_id];
  struct nm_qsnet_pkt_wrap *p_qsnet_pw  = tbx_malloc(p_qsnet_drv->qsnet_pw_mem);
  struct iovec		*p_iov		= NULL;
  int err = -NM_EINVAL;
  assert(p_pw->v_nb == 1);
  if (p_pw->v_nb != 1)
    goto out;

  p_pw->drv_priv = p_qsnet_pw;
  p_iov		 = p_pw->v;
  p_qsnet_pw->ev = elan_tportTxStart(p_qsnet_trk->p, 0, p_qsnet_cnx->remote_proc,
				     p_qsnet_drv->proc,
				     2, p_iov->iov_base, p_iov->iov_len);
#if 1
  err = nm_qsnet_poll_send_iov(_status, p_pw);
#else
  err = -NM_EAGAIN;
#endif
  
 out:
  return err;
}

static int nm_qsnet_post_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_drv			*p_drv		= p_pw->p_drv;
  struct nm_qsnet_drv           *p_qsnet_drv    = p_drv->priv;
  struct nm_qsnet_trk		*p_qsnet_trk	= &p_qsnet_drv->trks_array[p_pw->trk_id];
  struct nm_qsnet_pkt_wrap	*p_qsnet_pw	= tbx_malloc(p_qsnet_drv->qsnet_pw_mem);
  struct iovec			*p_iov		= NULL;
  u_int				 remote_proc	= 0;
  u_int				 remote_proc_mask = 0;
  int err = -NM_EINVAL;
  assert(p_pw->v_nb == 1);
  if (p_pw->v_nb != 1)
    goto out;
  
  if (p_pw->p_gate)
    {
      struct nm_gate	*p_gate	= p_pw->p_gate;
      struct nm_qsnet	*status	= _status;
      struct nm_qsnet_cnx *p_qsnet_cnx = &status->cnx_array[p_pw->trk_id];
      
      remote_proc	= p_qsnet_cnx->remote_proc;
      remote_proc_mask	= (u_int)-1;
    }

  p_pw->drv_priv = p_qsnet_pw;
  p_iov	= p_pw->v;
  p_qsnet_pw->ev = elan_tportRxStart(p_qsnet_trk->p, 0, remote_proc_mask, remote_proc,
				     -1, 2, p_iov->iov_base, p_iov->iov_len);
  
#if 1
  err = nm_qsnet_poll_recv_iov(_status, p_pw);
#else
  err = -NM_EAGAIN;
#endif
  
 out:
  return err;
}

static int nm_qsnet_poll_send_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_qsnet_pkt_wrap *p_qsnet_pw = p_pw->drv_priv;
  struct nm_qsnet_drv      *p_qsnet_drv = p_pw->p_gdrv->p_drv->priv;
  int boolean;
  int err;
  
#ifdef NM_QSNET_USE_ELAN_DONE
  boolean		= elan_done(p_qsnet_pw->ev, 0);
#else
  boolean		= elan_tportTxDone(p_qsnet_pw->ev);
#endif
  
  if (!boolean) {
    err	= -NM_EAGAIN;
    goto out;
  }
  
  elan_tportTxWait(p_qsnet_pw->ev);
  
  tbx_free(p_qsnet_drv->qsnet_pw_mem, p_qsnet_pw);
  err = NM_ESUCCESS;

 out:
  return err;
}

static int nm_qsnet_poll_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  struct nm_qsnet_pkt_wrap *p_qsnet_pw	= p_pw->drv_priv;
  struct nm_qsnet_drv      *p_qsnet_drv = p_pw->p_gdrv->p_drv->priv;
  int			    remote_proc	= 0;
  int boolean;
  int err;

#ifdef NM_QSNET_USE_ELAN_DONE
  boolean = elan_done(p_qsnet_pw->ev, 0);
#else
  boolean = elan_tportRxDone(p_qsnet_pw->ev);
#endif
  if (!boolean) {
    err	= -NM_EAGAIN;
    goto out;
  }
  
  elan_tportRxWait(p_qsnet_pw->ev, &remote_proc, NULL, NULL);
  
  if (!p_pw->p_gate) 
    {
      /* gate-less request */
      struct nm_qsnet_trk *p_qsnet_trk = &p_qsnet_drv->trks_array[p_pw->trk_id];
      p_pw->p_gate	= p_pw->p_drv->p_core->gate_array
	+ p_qsnet_trk->gate_map[remote_proc];
      
      NM_TRACE_VAL("new pkt from qsnet proc", remote_proc);
      NM_TRACE_PTR("gate ptr", p_pw->p_gate);
    }
  
  tbx_free(p_qsnet_drv->qsnet_pw_mem, p_qsnet_pw);
  err = NM_ESUCCESS;
  
 out:
  return err;
}

