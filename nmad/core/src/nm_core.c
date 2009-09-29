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
#include <sys/uio.h>
#include <assert.h>
#ifdef PM2_NUIOA
#include <numa.h>
#endif

#include <nm_private.h>

debug_type_t debug_nm_so_trace = NEW_DEBUG_TYPE("NM_SO: ", "nm_so_trace");

#ifdef PIOMAN

static piom_spinlock_t piom_big_lock;

void  nmad_lock(void)
{
  piom_spin_lock(&piom_big_lock);
}

void nmad_unlock(void)
{
  piom_spin_unlock(&piom_big_lock);
}
/* Initialisation de PIOMan  */
int nm_core_init_piom(struct nm_core *p_core)
{
  LOG_IN();

  piom_spin_lock_init(&piom_big_lock);
  return 0;
}

/** Initialize PIOMan server for the given driver */
static int nm_core_init_piom_drv(struct nm_core*p_core, struct nm_drv *p_drv)
{
  LOG_IN();
  piom_server_init(&p_drv->server, "NMad IO Server");

#ifdef  MARCEL_REMOTE_TASKLETS
  marcel_vpset_t vpset;
  marcel_vpset_vp(&vpset, 0);
  ma_remote_tasklet_set_vpset(&p_drv->server.poll_tasklet, &vpset);
#endif

  piom_server_set_poll_settings(&p_drv->server,
				PIOM_POLL_AT_TIMER_SIG
				| PIOM_POLL_AT_IDLE
				| PIOM_POLL_WHEN_FORCED, 1, -1);

  /* Définition des callbacks */
  piom_server_add_callback(&p_drv->server, PIOM_FUNCTYPE_POLL_POLLONE,
			   (piom_pcallback_t) {
			     .func = &nm_piom_poll,
			     .speed = PIOM_CALLBACK_NORMAL_SPEED
			     }
			   );

#warning "TODO: Fix the poll_any issues."
  //	piom_server_add_callback(&p_drv->server,
  //				  PIOM_FUNCTYPE_POLL_POLLANY,
  //				  (piom_pcallback_t) {
  //		.func = &nm_piom_poll_any,
  //			 .speed = PIOM_CALLBACK_NORMAL_SPEED});

#ifdef PIOM_BLOCKING_CALLS
  if((p_drv->driver->get_capabilities(p_drv))->is_exportable)
    {
      //		piom_server_start_lwp(&p_drv->server, 1);
      piom_server_add_callback(&p_drv->server, PIOM_FUNCTYPE_BLOCK_WAITONE,
			       (piom_pcallback_t) {
				 .func = &nm_piom_block,
				 .speed = PIOM_CALLBACK_NORMAL_SPEED
			       });
#if 0
      piom_server_add_callback(&p_drv->server, PIOM_FUNCTYPE_BLOCK_WAITANY,
			       (piom_pcallback_t) {
				 .func = &nm_piom_block_any,
				 .speed = PIOM_CALLBACK_NORMAL_SPEED
			       });
#endif

    }
#endif /* PIOM_BLOCKING_CALLS */

  piom_server_start(&p_drv->server);

  /* Very ugly for now */
  /* todo: "unugly" me ! */
  struct nm_pkt_wrap *post_rq = &p_drv->post_rq;

  post_rq->p_drv  = p_drv;
  post_rq->trk_id = -1;
  post_rq->p_gate = NULL;
  post_rq->p_gdrv = NULL;
  post_rq->drv_priv   = NULL;

  post_rq->flags = 0;
  post_rq->length = 0;

  post_rq->v_size          = 0;
  post_rq->v_nb            = 0;
  post_rq->v = NULL;

  post_rq->contribs = NULL;
  post_rq->contribs_size = 0;
  post_rq->n_contribs = 0;

  piom_req_init(&post_rq->inst);
  post_rq->inst.server=&p_drv->server;
  post_rq->which=NONE;
  post_rq->inst.priority=PIOM_REQ_PRIORITY_LOW;
  post_rq->inst.state|=PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
  piom_req_submit(&p_drv->server, &post_rq->inst);

  return 0;
}
#endif /* PIOMAN */


/** Load a driver.
 *
 * Out parameters:
 * p_id  - contains the id of the new driver
 */
int nm_core_driver_load(nm_core_t p_core,
			puk_component_t driver,
			nm_drv_id_t *p_id)
{
  int err;

  NM_LOG_IN();
  if (p_core->nb_drivers == NM_DRV_MAX) {
    err	= -NM_ENOMEM;
    goto out;
  }
  assert(driver != NULL);
  struct nm_drv	*p_drv = p_core->driver_array + p_core->nb_drivers;
  memset(p_drv, 0, sizeof(struct nm_drv));
  p_drv->p_core   = p_core;
  p_drv->id       = p_core->nb_drivers;
  p_drv->assembly = driver;
  p_drv->driver   = puk_adapter_get_driver_NewMad_Driver(p_drv->assembly, NULL);

  if (p_id) {
    *p_id	= p_drv->id;
  }
  p_core->nb_drivers++;

  err = NM_ESUCCESS;

 out:
  NM_LOG_OUT();

  return err;
}


/** Query resources and register them for a driver.
 *
 */
int nm_core_driver_query(nm_core_t p_core,
			 nm_drv_id_t id,
			 struct nm_driver_query_param *params,
			 int nparam)
{
  int err;

  NM_LOG_IN();
  struct nm_drv	*p_drv = &p_core->driver_array[id];

  if (!p_drv->driver->query) {
    err = -NM_EINVAL;
    goto out;
  }

  err = p_drv->driver->query(p_drv, params, nparam);
  if (err != NM_ESUCCESS) {
    NM_DISPF("drv.query returned %d", err);
    goto out;
  }

  err = NM_ESUCCESS;

 out:
  NM_LOG_OUT();

  return err;
}

/** Initialize a driver using previously registered resources.
 *
 * Out parameters:
 * p_url - contains the URL of the driver (memory for the URL is allocated by
 * nm_core)
 */
int nm_core_driver_init(nm_core_t p_core, nm_drv_id_t drv_id, char **p_url)
{
  struct nm_drv	*p_drv = NULL;
  int err;

  NM_LOG_IN();
  p_drv	= &p_core->driver_array[drv_id];
  p_drv->p_core = p_core;

  if (!p_drv->driver->init) {
    err = -NM_EINVAL;
    goto out;
  }
  /* open tracks */
  const int nb_trks = NM_SO_MAX_TRACKS;
  struct nm_trk_cap trk_caps[NM_SO_MAX_TRACKS] =
    {
      /* Track 0- for unexpected packets */
      [NM_TRK_SMALL] = {
	.rq_type		   = nm_trk_rq_unspecified,
	.iov_type		   = nm_trk_iov_unspecified,
	.max_pending_send_request  = 0,
	.max_pending_recv_request  = 0,
	.min_single_request_length = 0,
	.max_single_request_length = 0,
	.max_iovec_request_length  = 0,
	.max_iovec_size		   = 0
      },
      /* Track 1- for long packets with rendezvous */
      [NM_TRK_LARGE] = {
	.rq_type		   = nm_trk_rq_rdv,
	.iov_type		   = nm_trk_iov_unspecified,
	.max_pending_send_request  = 0,
	.max_pending_recv_request  = 0,
	.min_single_request_length = 0,
	.max_single_request_length = 0,
	.max_iovec_request_length  = 0,
	.max_iovec_size	           = 0
      }
    };

  err = p_drv->driver->init(p_drv, trk_caps, nb_trks);
  if (err != NM_ESUCCESS) {
    NM_DISPF("drv.init returned %d", err);
    goto out;
  }
  p_drv->nb_tracks = nb_trks;

  nm_trk_id_t trk_id;
  for(trk_id = 0; trk_id < p_drv->nb_tracks; trk_id++)
    {
      FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_INPUT_LIST, p_drv->id, trk_id, p_drv->nb_tracks);
      FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_OUTPUT_LIST, p_drv->id, trk_id, p_drv->nb_tracks);
    }

  /* encode URL */
  const char*drv_url = p_drv->driver->get_driver_url ? p_drv->driver->get_driver_url(p_drv) : NULL;
  p_tbx_string_t url = tbx_string_init_to_cstring(drv_url?:"-");
  for(trk_id = 0; trk_id < p_drv->nb_tracks; trk_id++)
    {
      const char*trk_url = p_drv->driver->get_track_url ? p_drv->driver->get_track_url(p_drv, trk_id) : NULL;
      tbx_string_append_char(url, '#');
      tbx_string_append_cstring(url, trk_url?:"-");
    }
  *p_url = tbx_string_to_cstring(url);
  tbx_string_free(url);
  FUT_DO_PROBE1(FUT_NMAD_INIT_NIC, p_drv->id);
  FUT_DO_PROBESTR(FUT_NMAD_INIT_NIC_URL, p_drv->assembly->name);

#ifdef PIOMAN
  nm_core_init_piom_drv(p_core, p_drv);
#endif

  nm_ns_update(p_core);
  err = NM_ESUCCESS;

 out:
  NM_LOG_OUT();

  return err;
}

/** Helper to load and init several drivers at once,
 * with an array of parameters for each driver,
 * and applying numa binding in-between.
 */
int nm_core_driver_load_init_some_with_params(nm_core_t p_core,
					      int count,
					      puk_component_t*driver_array,
					      struct nm_driver_query_param **params_array,
					      int *nparam_array,
					      nm_drv_id_t *p_id_array,
					      char **p_url_array)
{
  nm_drv_id_t id;
  int i;
#ifdef PM2_NUIOA
  int preferred_node = PM2_NUIOA_ANY_NODE;
  int nuioa = (numa_available() >= 0);
  char * nuioa_criteria = getenv("PM2_NUIOA_CRITERIA");
  int nuioa_with_latency = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "latency"));
  int nuioa_with_bandwidth = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "bandwidth"));
  int nuioa_current_best = 0;
#endif /* PM2_NUIOA */

  for(i=0; i<count; i++) {
    int err;

    err = nm_core_driver_load(p_core, driver_array[i], &id);
    if (err != NM_ESUCCESS) {
      NM_DISPF("nm_core_driver_load returned %d", err);
      return err;
    }

    p_id_array[i] = id;

    err = nm_core_driver_query(p_core, id, params_array[i], nparam_array[i]);
    if (err != NM_ESUCCESS) {
      NM_DISPF("nm_core_driver_query returned %d", err);
      return err;
    }

#ifdef PM2_NUIOA
    if (nuioa) {
      struct nm_drv *p_drv = p_core->driver_array + id;
      int node = p_drv->driver->get_capabilities(p_drv)->numa_node;
      if (node != PM2_NUIOA_ANY_NODE) {
	/* if this driver wants something */
	DISP("# nmad: marking nuioa node %d as preferred for driver %d", node, id);

	if (nuioa_with_latency) {
	  /* choosing by latency: take this network if it's the first one
	   * or if its latency is lower than the previous one */
	  if (preferred_node == PM2_NUIOA_ANY_NODE
	      || p_drv->driver->get_capabilities(p_drv)->latency < nuioa_current_best) {
	    preferred_node = node;
	    nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->latency;
	  }

	} else if (nuioa_with_bandwidth) {
	  /* choosing by bandwidth: take this network if it's the first one
	   * or if its bandwidth is higher than the previous one */
	  if (preferred_node == PM2_NUIOA_ANY_NODE
	      || p_drv->driver->get_capabilities(p_drv)->bandwidth > nuioa_current_best) {
	    preferred_node = node;
	    nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->bandwidth;
	  }

	} else if (preferred_node == PM2_NUIOA_ANY_NODE) {
	  /* if it's the first driver, take its preference for now */
	  preferred_node = node;

	} else if (preferred_node != node) {
	  /* if the first driver wants something else, it's a conflict,
	   * display a message once */
	  if (preferred_node != PM2_NUIOA_CONFLICTING_NODES)
	    DISP("found conflicts between preferred nuioa nodes of drivers");
	  preferred_node = PM2_NUIOA_CONFLICTING_NODES;
	}
      }
    }
#endif /* PM2_NUIOA */
  }

#ifdef PM2_NUIOA
  if (nuioa && preferred_node != PM2_NUIOA_ANY_NODE && preferred_node != PM2_NUIOA_CONFLICTING_NODES) {
#if (defined LIBNUMA_API_VERSION) && LIBNUMA_API_VERSION == 2
    struct bitmask * mask = numa_bitmask_alloc(numa_num_possible_nodes());
    numa_bitmask_setbit(mask, preferred_node);
    numa_bind(mask);
    numa_bitmask_free(mask);
#else
    nodemask_t mask;
    nodemask_zero(&mask);
    nodemask_set(&mask, preferred_node);
    numa_bind(&mask);
#endif
    DISP("# nmad: binding to nuioa node %d", preferred_node);
  }
#endif /* PM2_NUIOA */

  for(i=0; i<count; i++) {
    int err;

    err = nm_core_driver_init(p_core, p_id_array[i], &p_url_array[i]);
    if (err != NM_ESUCCESS) {
      NM_DISPF("nm_core_driver_init returned %d", err);
      return err;
    }

#ifndef CONFIG_PROTO_MAD3
    printf("# nmad: driver #%d- name = %s; url = %s\n", i, driver_array[i]->name, p_url_array[i]);
#endif
  }
  return NM_ESUCCESS;
}

/** Shutdown all drivers.
 *
 */
static int nm_core_driver_exit(struct nm_core *p_core)
{
  int i, j, err = NM_ESUCCESS;

  nmad_lock();

  nm_drv_id_t drv_id;
  for(drv_id = 0; drv_id < p_core->nb_drivers; drv_id++)
    {
      int i;
#ifdef PIOMAN
      /* stop polling
       */
      struct nm_drv*p_drv = &p_core->driver_array[drv_id];
      nmad_unlock();
      piom_server_stop(&p_drv->server);
      nmad_lock();
#endif /* PIOMAN */
      /* cancel any pending active recv request 
       */
      for(i = 0; i < p_core->nb_gates; i++)
	{
	  struct nm_gate*p_gate = &p_core->gate_array[i];
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, drv_id);
	  struct nm_pkt_wrap*p_pw = p_gdrv->p_in_rq_array[NM_TRK_SMALL];
	  if(p_pw)
	    {
	      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
	      if(r->driver->cancel_recv_iov)
		r->driver->cancel_recv_iov(r->_status, p_pw);
	      p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
#ifdef PIOMAN
	      piom_req_success(&p_pw->inst);
#else /* PIOMAN */
	      tbx_slist_search_and_extract(p_core->so_sched.pending_recv_req, NULL, p_pw);
#endif /* PIOMAN */
	      nm_so_pw_free(p_pw);
	    }
	  p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
	  p_gate->active_recv[drv_id][NM_TRK_SMALL] = 0;
	}
    }

  /* disconnect all gates */
  for(i = 0 ; i < p_core->nb_gates ; i++)
    {
      struct nm_gate *p_gate = &p_core->gate_array[i];
      p_gate->status = NM_GATE_STATUS_DISCONNECTED;
      for(j = 0 ; j < NM_DRV_MAX ; j++)
	{
	  struct nm_gate_drv *p_gdrv = nm_gate_drv_get(p_gate, j);
	  if (p_gdrv != NULL)
	    {
	      struct nm_drv *p_drv = p_drv = p_gdrv->p_drv;
	      nm_trk_id_t trk_id;
	      for(trk_id = 0 ; trk_id < p_drv->nb_tracks; trk_id++)
		{
		  struct nm_cnx_rq	 rq	= {
		    .p_gate			= p_gate,
		    .p_drv			= p_drv,
		    .trk_id			= trk_id,
		    .remote_drv_url		= NULL,
		    .remote_trk_url		= NULL
		  };
		  p_gdrv->receptacle.driver->disconnect(p_gdrv->receptacle._status, &rq);
		  p_gdrv->p_in_rq_array[trk_id] = NULL;
		}
	      TBX_FREE(p_gdrv->p_in_rq_array);
	      p_gdrv->p_in_rq_array = NULL;
	    }
	}
    }
  /* deinstantiate all drivers */
  for(i = 0 ; i < p_core->nb_gates ; i++)
    {
      struct nm_gate *p_gate = &p_core->gate_array[i];
      for(j = 0 ; j < NM_DRV_MAX ; j++)
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, j);
	  if (p_gdrv != NULL)
	    {
	      struct nm_drv *p_drv = p_gdrv->p_drv;
	      p_drv->nb_tracks = 0;
	      if(p_gdrv->instance != NULL)
		{
		  puk_instance_destroy(p_gdrv->instance);
		  p_gdrv->instance = NULL;
		}
	      TBX_FREE(p_gdrv);
	    }
	}
    }
  for(i = 0; i < p_core->nb_drivers; i++)
    {
      struct nm_drv*p_drv = &p_core->driver_array[i];
      if(p_drv->driver->close)
	{
	  (*p_drv->driver->close)(p_drv);
	}
    }
  /* close all gates */
  for(i = 0 ; i < p_core->nb_gates ; i++)
    {
      struct nm_gate *p_gate = &p_core->gate_array[i];
      nm_so_tag_table_destroy(&p_gate->tags);
      puk_instance_destroy(p_gate->strategy_instance);
    }
  nmad_unlock();
  return err;
}

/** Initialize a new gate.
 *
 * out parameters:
 * p_id - id of the gate
 */
int nm_core_gate_init(nm_core_t p_core, nm_gate_t*pp_gate)
{
  int err = NM_ESUCCESS;

  if (p_core->nb_gates == NUMBER_OF_GATES) {
    err	= -NM_ENOMEM;
    goto out;
  }

  struct nm_gate *p_gate = &p_core->gate_array[p_core->nb_gates];

  p_core->nb_gates++;

  memset(p_gate, 0, sizeof(struct nm_gate));

  p_gate->status = NM_GATE_STATUS_INIT;
  p_gate->id	 = p_core->nb_gates;
  p_gate->p_core = p_core;
  p_gate->ref    = NULL;

  FUT_DO_PROBE1(FUT_NMAD_INIT_GATE, p_gate->id);

  nm_so_tag_table_init(&p_gate->tags);

  memset(p_gate->active_recv, 0, sizeof(p_gate->active_recv));
  memset(p_gate->active_send, 0, sizeof(p_gate->active_send));

  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_recv);
  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_send);

  p_gate->strategy_instance = puk_adapter_instanciate(p_core->so_sched.strategy_adapter);
  puk_instance_indirect_NewMad_Strategy(p_gate->strategy_instance, NULL,
					&p_gate->strategy_receptacle);

  *pp_gate = p_gate;

 out:
  return err;
}

/** Connect the process through a gate using a specified driver.
 */
static int nm_core_gate_connect_accept(struct nm_core	*p_core,
				       nm_gate_t	 p_gate,
				       nm_drv_id_t	 drv_id,
				       const char	*drv_trk_url,
				       int		 connect_flag)
{
  struct nm_cnx_rq	 rq	= {
    .p_gate			= NULL,
    .p_drv			= NULL,
    .trk_id			= 0,
    .remote_drv_url		= NULL,
    .remote_trk_url		= NULL
  };

  struct nm_gate_drv	*p_gdrv	= TBX_MALLOC(sizeof(struct nm_gate_drv));

  char	*urls[255];
  nm_trk_id_t trk_id;
  int err;

  p_gate->status = NM_GATE_STATUS_CONNECTING;

  /* set gate */
  rq.p_gate = p_gate;

  if (!rq.p_gate) {
    err = -NM_EINVAL;
    goto out;
  }

  /* set drv */
  if (drv_id >= p_core->nb_drivers) {
    err = -NM_EINVAL;
    goto out;
  }
  rq.p_drv = &p_core->driver_array[drv_id];

  /* instanciate driver */
  p_gdrv->instance = puk_adapter_instanciate(rq.p_drv->assembly);
  puk_instance_indirect_NewMad_Driver(p_gdrv->instance, NULL, &p_gdrv->receptacle);

  /* init gate/driver fields */
  p_gdrv->p_drv	= rq.p_drv;
  p_gdrv->p_in_rq_array = TBX_MALLOC(rq.p_drv->nb_tracks * sizeof(struct nm_pkt_wrap*));
  if (!p_gdrv->p_in_rq_array) {
    err	= -NM_ENOMEM;
    goto out;
  }
  p_gate->p_gate_drv_array[drv_id] = p_gdrv;

  /* split drv/trk url */
  if (drv_trk_url)
    {
      rq.remote_drv_url	= tbx_strdup(drv_trk_url);
      {
	char *ptr = rq.remote_drv_url;
	int i;
	for (i = 0 ; ptr && i < 255; i++) {
	  ptr = strchr(ptr, '#');

	  if (ptr) {
	    *ptr	= '\0';
	    urls[i]	= ++ptr;
	  }
	}
      }
    } else {
      rq.remote_drv_url	= NULL;
      memset(urls, 0, 255*sizeof(NULL));
    }

  /* connect/accept connections */
  for (trk_id = 0; trk_id < rq.p_drv->nb_tracks; trk_id++)
    {
      p_gdrv->p_in_rq_array[trk_id] = NULL;
      rq.trk_id = trk_id;
      rq.remote_trk_url	= urls[trk_id];

      if (connect_flag) {
	err = p_gdrv->receptacle.driver->connect(p_gdrv->receptacle._status, &rq);
	if (err != NM_ESUCCESS) {
	  NM_DISPF("drv.ops.connect returned %d", err);
	  goto out;
	}
      } else {
	err = p_gdrv->receptacle.driver->accept(p_gdrv->receptacle._status, &rq);
	if (err != NM_ESUCCESS) {
	  NM_DISPF("drv.ops.accept returned %d", err);
	  goto out;
	}
      }

    }

  err = NM_ESUCCESS;
  p_gate->status = NM_GATE_STATUS_CONNECTED;

 out:
  if (rq.remote_drv_url) {
    TBX_FREE(rq.remote_drv_url);
  }
  return err;

}

/** Server side of connection establishment.
 */
int nm_core_gate_accept(nm_core_t	 p_core,
			nm_gate_t	 p_gate,
			nm_drv_id_t	 drv_id,
			const char	*drv_trk_url)
{
  return nm_core_gate_connect_accept(p_core, p_gate, drv_id, drv_trk_url, 0);
}

/** Client side of connection establishment.
 */
int nm_core_gate_connect(nm_core_t p_core,
                         nm_gate_t p_gate,
                         nm_drv_id_t drv_id,
                         const char*drv_trk_url)
{
  return nm_core_gate_connect_accept(p_core, p_gate, drv_id, drv_trk_url, !0);
}

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(struct nm_gate*p_gate)
{
  return p_gate->ref;
}

/** Set the user-registered per-gate data */
void nm_gate_ref_set(struct nm_gate*p_gate, void*ref)
{
  p_gate->ref = ref;
}


/** Load a newmad component from disk. The actual path loaded is
 * ${PM2_CONF_DIR}/'entity'/'entity'_'driver'.xml
 */
puk_component_t nm_core_component_load(const char*entity, const char*name)
{
  char filename[1024];
  int rc = 0;
  const char*pm2_conf_dir = getenv("PM2_CONF_DIR");
  const char*pm2_home = getenv("PM2_HOME");
  const char*pm2_root = getenv("PM2_ROOT");
  if(pm2_conf_dir)
    {
      /* $PM2_CONF_DIR/<entity>/<entity>_<name>.xml */
      rc = snprintf(filename, 1024, "%s/%s/%s_%s.xml", pm2_conf_dir, entity, entity, name);
    }
  else if(pm2_home)
    {
      /* $PM2_HOME/<entity>/<entity>_<name>.xml */
      rc = snprintf(filename, 1024, "%s/%s/%s_%s.xml", pm2_home, entity, entity, name);
    }
  else if(pm2_root)
    {
      /* $PM2_ROOT/build/home/<entity>/<entity>_<name>.xml */
      rc = snprintf(filename, 1024, "%s/build/home/%s/%s_%s.xml", pm2_root, entity, entity, name);
    }
  else
    {
      TBX_FAILURE("nmad: cannot compute PM2_CONF_DIR. Please set $PM2_CONF_DIR, $PM2_HOME, or $PM2_ROOT.\n");
    }
  assert(rc < 1024 && rc > 0);
  puk_component_t component = puk_adapter_parse_file(filename);
  if(component == NULL)
    {
      padico_fatal("nmad: failed to load component '%s' (%s)\n", name, filename);
    }
  return component;
}



/** Initialize NewMad core and SchedOpt.
 */
int nm_core_init(int*argc, char *argv[], nm_core_t*pp_core)
{
  int err = NM_ESUCCESS;

  /*
   * Lazy Puk initialization (it may already have been initialized in PadicoTM or MPI_Init)
   */
  if(!padico_puk_initialized())
    {
      padico_puk_init(*argc, argv);
    }

  /*
   * Lazy PM2 initialization (it may already have been initialized in PadicoTM or ForestGOMP)
   */
  if(!tbx_initialized())
    {
      common_pre_init(argc, argv, NULL);
      common_post_init(argc, argv, NULL);
    }

  /* init debug system */
  pm2debug_register(&debug_nm_so_trace);
  pm2debug_init_ext(argc, argv, 0 /* debug_flags */);

  FUT_DO_PROBE0(FUT_NMAD_INIT_CORE);

  struct nm_core *p_core = TBX_MALLOC(sizeof(struct nm_core));
  memset(p_core, 0, sizeof(struct nm_core));

  p_core->nb_gates   = 0;
  p_core->nb_drivers = 0;

#ifdef PIOMAN
  nm_core_init_piom(p_core);
#endif

  err = nm_so_schedule_init(p_core);
  if (err != NM_ESUCCESS)
    goto out_free;

  *pp_core	= p_core;

 out:
  return err;

 out_free:
  TBX_FREE(p_core);
  goto out;
}

/** Shutdown the core struct and the main scheduler.
 */
int nm_core_exit(nm_core_t p_core)
{
  nm_core_driver_exit(p_core);
  nm_so_schedule_exit(p_core);
  nm_ns_exit(p_core);
  TBX_FREE(p_core);

  return NM_ESUCCESS;
}

