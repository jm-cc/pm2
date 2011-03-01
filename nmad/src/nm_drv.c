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

#include <nm_private.h>



/** Load a driver.
 *
 * Out parameters:
 * p_id  - contains the id of the new driver
 */
int nm_core_driver_load(nm_core_t p_core,
			puk_component_t driver_assembly,
			nm_drv_t*pp_drv)
{
  int err;

  NM_LOG_IN();

  assert(driver_assembly != NULL);
  struct nm_drv*p_drv = TBX_MALLOC(sizeof(struct nm_drv));
  memset(p_drv, 0, sizeof(struct nm_drv));
  p_drv->p_core   = p_core;
  p_drv->assembly = driver_assembly;
  p_drv->driver   = puk_component_get_driver_NewMad_Driver(p_drv->assembly, NULL);

  nm_trk_id_t trk_id;;
  for(trk_id = 0; trk_id < NM_SO_MAX_TRACKS; trk_id++)
    {
      TBX_INIT_FAST_LIST_HEAD(&p_drv->post_recv_list[trk_id]);
      TBX_INIT_FAST_LIST_HEAD(&p_drv->post_sched_out_list[trk_id]);
    }
  nm_so_lock_out_init(p_core, p_drv);
  nm_so_lock_in_init(p_core, p_drv);

#ifdef NMAD_POLL
  TBX_INIT_FAST_LIST_HEAD(&p_drv->pending_recv_list);
  TBX_INIT_FAST_LIST_HEAD(&p_drv->pending_send_list);
  nm_poll_lock_in_init(p_core, p_drv);
  nm_poll_lock_out_init(p_core, p_drv);
#endif /* NMAD_POLL*/

  tbx_fast_list_add_tail(&p_drv->_link, &p_core->driver_list);
  p_core->nb_drivers++;

  *pp_drv = p_drv;

  err = NM_ESUCCESS;

  NM_LOG_OUT();

  return err;
}


/** Query resources and register them for a driver.
 *
 */
int nm_core_driver_query(nm_core_t p_core,
			 nm_drv_t p_drv,
			 struct nm_driver_query_param *param)
{
  int err;

  NM_LOG_IN();

  if (!p_drv->driver->query)
    {
      err = -NM_EINVAL;
      goto out;
    }

  err = p_drv->driver->query(p_drv, param, 1);
  if (err != NM_ESUCCESS)
    {
      NM_WARN("drv.query returned %d", err);
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
int nm_core_driver_init(nm_core_t p_core, nm_drv_t p_drv, const char **p_url)
{
  int err;

  NM_LOG_IN();
  p_drv->p_core = p_core;

  if (!p_drv->driver->init)
    {
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
  if (err != NM_ESUCCESS)
    {
      NM_WARN("drv.init returned %d", err);
      goto out;
    }
  p_drv->nb_tracks = nb_trks;

  nm_trk_id_t trk_id;
  for(trk_id = 0; trk_id < p_drv->nb_tracks; trk_id++)
    {
      FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_INPUT_LIST, p_drv, trk_id, p_drv->nb_tracks);
      FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_OUTPUT_LIST, p_drv, trk_id, p_drv->nb_tracks);
    }

  const char*drv_url = (*p_drv->driver->get_driver_url)(p_drv);
  *p_url = drv_url;
  FUT_DO_PROBE1(FUT_NMAD_INIT_NIC, p_drv);
  FUT_DO_PROBESTR(FUT_NMAD_INIT_NIC_URL, p_drv->assembly->name);

#ifdef PIOMAN_POLL
  nm_submit_post_drv_ltask(&p_drv->task, p_drv);
#endif

  nm_ns_update(p_core, p_drv);
  err = NM_ESUCCESS;

 out:
  NM_LOG_OUT();

  return err;
}

/** Helper to load and init a driver with a parameter,
 * and applying numa binding in-between.
 */
int nm_core_driver_load_init(nm_core_t p_core, puk_component_t driver,
			     struct nm_driver_query_param*param,
			     nm_drv_t *pp_drv, const char**p_url)
{
#ifdef PM2_NUIOA
  int preferred_node = PM2_NUIOA_ANY_NODE;
  int nuioa = (numa_available() >= 0);
  char * nuioa_criteria = getenv("PM2_NUIOA_CRITERIA");
  int nuioa_with_latency = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "latency"));
  int nuioa_with_bandwidth = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "bandwidth"));
  int nuioa_current_best = 0;
#endif /* PM2_NUIOA */

  nm_drv_t p_drv = NULL;
  int err = nm_core_driver_load(p_core, driver, &p_drv);
  if (err != NM_ESUCCESS) 
    {
      NM_WARN("nm_core_driver_load returned %d", err);
      return err;
    }
  
  err = nm_core_driver_query(p_core, p_drv, param);
  if (err != NM_ESUCCESS) 
    {
      NM_WARN("nm_core_driver_query returned %d", err);
      return err;
    }
#ifdef PM2_NUIOA
  if (nuioa) 
    {
      const int node = p_drv->driver->get_capabilities(p_drv)->numa_node;
      if (node != PM2_NUIOA_ANY_NODE) 
	{
	  /* if this driver wants something */
	  NM_DISPF("# nmad: marking nuioa node %d as preferred for driver %s", node, p_drv->driver->name);
	  if (nuioa_with_latency) 
	    {
	      /* choosing by latency: take this network if it's the first one
	       * or if its latency is lower than the previous one */
	      if (preferred_node == PM2_NUIOA_ANY_NODE
		  || p_drv->driver->get_capabilities(p_drv)->latency < nuioa_current_best) {
		preferred_node = node;
		nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->latency;
	      }
	    }
	  else if (nuioa_with_bandwidth) 
	    {
	      /* choosing by bandwidth: take this network if it's the first one
	       * or if its bandwidth is higher than the previous one */
	      if (preferred_node == PM2_NUIOA_ANY_NODE
		  || p_drv->driver->get_capabilities(p_drv)->bandwidth > nuioa_current_best) {
		preferred_node = node;
		nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->bandwidth;
	      }
	    }
	  else if (preferred_node == PM2_NUIOA_ANY_NODE)
	    {
	      /* if it's the first driver, take its preference for now */
	      preferred_node = node;
	    }
	  else if (preferred_node != node) 
	    {
	      /* if the first driver wants something else, it's a conflict,
	       * display a message once */
	      if (preferred_node != PM2_NUIOA_CONFLICTING_NODES)
		NM_WARN("found conflicts between preferred nuioa nodes of drivers");
	      preferred_node = PM2_NUIOA_CONFLICTING_NODES;
	    }
	  }
    }
#endif /* PM2_NUIOA */

  
#ifdef PM2_NUIOA
  if(!nuioa)
    {
      NM_DISPF("# nmad: nuioa- NUMA not available\n");
    }
  else if(preferred_node ==  PM2_NUIOA_ANY_NODE)
    {
      NM_DISPF("# nmad: nuioa- any node. Not binding.\n");
    }
  else if(preferred_node == PM2_NUIOA_CONFLICTING_NODES)
    {
      NM_DISPF("# nmad: nuioa- conflicting nodes.\n");
    }
  else
    {
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
      NM_DISPF("# nmad: binding to nuioa node %d", preferred_node);
    }
#endif /* PM2_NUIOA */

  err = nm_core_driver_init(p_core, p_drv, p_url);
  if (err != NM_ESUCCESS) 
    {
      NM_WARN("nm_core_driver_init returned %d", err);
      return err;
    }
  NM_DISPF("# nmad: driver name = %s; url = %s\n", driver->name, *p_url);

  *pp_drv = p_drv;
  return NM_ESUCCESS;
}

/** Shutdown all drivers.
 *
 */
int nm_core_driver_exit(struct nm_core *p_core)
{
  int err = NM_ESUCCESS;
#if(defined(PIOMAN))
  nmad_unlock();
  piom_ltask_pause();
  nmad_lock();
#endif	/* PIOMAN */

  nm_lock_interface(p_core);
  struct nm_drv*p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
#ifdef PIOMAN_POLL
      /* stop polling
       */
      nmad_unlock();
      nm_unlock_interface(p_core);
      piom_ltask_cancel(&p_drv->task);
      nm_lock_interface(p_core);
      nmad_lock();
#endif /* PIOMAN_POLL */
      /* cancel any pending active recv request 
       */

      struct nm_gate*p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv != NULL)
	    {
	      struct nm_pkt_wrap*p_pw = p_gdrv->p_in_rq_array[NM_TRK_SMALL];
	      if(p_pw)
		{
		  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
		  if(r->driver->cancel_recv_iov)
		    r->driver->cancel_recv_iov(r->_status, p_pw);
		  p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;

#if defined(NMAD_POLL)
		  tbx_fast_list_del(&p_pw->link);
		  nm_so_pw_free(p_pw);
#else
		  piom_ltask_cancel(&p_pw->ltask);
		  nm_pw_free(p_pw);		  
#endif
		}
	      p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
	      p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	    }
	}
    }
#if(defined(PIOMAN))
  piom_ltask_resume();
  piom_exit_ltasks();
#endif
  /* disconnect all gates */
  struct nm_gate*p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      p_gate->status = NM_GATE_STATUS_DISCONNECTED;
      NM_FOR_EACH_DRIVER(p_drv, p_core)
	{
	  struct nm_gate_drv *p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv != NULL)
	    {
	      nm_trk_id_t trk_id;
	      for(trk_id = 0 ; trk_id < p_drv->nb_tracks; trk_id++)
		{
		  struct nm_cnx_rq	 rq	= {
		    .p_gate			= p_gate,
		    .p_drv			= p_drv,
		    .trk_id			= trk_id,
		    .remote_drv_url		= NULL,
		  };
		  p_gdrv->receptacle.driver->disconnect(p_gdrv->receptacle._status, &rq);
		  p_gdrv->p_in_rq_array[trk_id] = NULL;
		}
	    }
	}
    }
  /* deinstantiate all drivers */
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      NM_FOR_EACH_DRIVER(p_drv, p_core)
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if (p_gdrv != NULL)
	    {
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
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      if(p_drv->driver->close)
	{
	  (*p_drv->driver->close)(p_drv);
	}
    }
  /* close all gates */
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      nm_so_tag_table_destroy(&p_gate->tags);
      puk_instance_destroy(p_gate->strategy_instance);
      nm_gdrv_vect_destroy(&p_gate->gdrv_array);
    }
  struct nm_gate*tmp_gate = NULL;
  tbx_fast_list_for_each_entry_safe(p_gate, tmp_gate, &p_core->gate_list, _link)
    {
      tbx_fast_list_del(&p_gate->_link);
      TBX_FREE(p_gate);
    }
  struct nm_drv*tmp_drv = NULL;
  tbx_fast_list_for_each_entry_safe(p_drv, tmp_drv, &p_core->driver_list, _link)
    {
      tbx_fast_list_del(&p_drv->_link);
      TBX_FREE(p_drv);
    }

  nm_unlock_interface(p_core);

  return err;
}

