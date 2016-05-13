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

PADICO_MODULE_HOOK(NewMad_Core);

static int nm_core_driver_load(nm_core_t p_core, puk_component_t driver, nm_drv_t*pp_drv);

static int nm_core_driver_init(nm_core_t p_core, nm_drv_t p_drv, const char**p_url);


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
  nm_drv_t p_drv = TBX_MALLOC(sizeof(struct nm_drv));
  memset(p_drv, 0, sizeof(struct nm_drv));
  p_drv->p_core   = p_core;
  p_drv->assembly = driver_assembly;
  p_drv->driver   = puk_component_get_driver_NewMad_Driver(p_drv->assembly, NULL);
  p_drv->p_in_rq  = NULL;
  p_drv->index = -1;
#ifdef PM2_TOPOLOGY
  p_drv->profile.cpuset = NULL;
#endif /* PM2_TOPOLOGY */

  nm_pw_post_lfqueue_init(&p_drv->post_recv);
  nm_pw_post_lfqueue_init(&p_drv->post_send);

  tbx_fast_list_add_tail(&p_drv->_link, &p_core->driver_list);
  p_core->nb_drivers++;

  *pp_drv = p_drv;

  err = NM_ESUCCESS;

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
  /* Track 0- for unexpected packets */
  p_drv->trk_caps[NM_TRK_SMALL] = (struct nm_trk_cap)
    {
      .rq_type		   = nm_trk_rq_unspecified,
      .iov_type		   = nm_trk_iov_unspecified,
      .max_pending_send_request  = 0,
      .max_pending_recv_request  = 0,
      .min_single_request_length = 0,
      .max_single_request_length = 0,
      .max_iovec_request_length  = 0,
      .max_iovec_size		   = 0,
      .supports_data             = 0
    };
  /* Track 1- for long packets with rendezvous */
  p_drv->trk_caps[NM_TRK_LARGE] = (struct nm_trk_cap)
    {
      .rq_type		   = nm_trk_rq_rdv,
      .iov_type		   = nm_trk_iov_unspecified,
      .max_pending_send_request  = 0,
      .max_pending_recv_request  = 0,
      .min_single_request_length = 0,
      .max_single_request_length = 0,
      .max_iovec_request_length  = 0,
      .max_iovec_size	           = 0,
      .supports_data             = 0
    };

  err = p_drv->driver->init(p_drv, p_drv->trk_caps, nb_trks);
  if (err != NM_ESUCCESS)
    {
      NM_WARN("drv.init returned %d", err);
      goto out;
    }
  p_drv->nb_tracks = nb_trks;

  const char*drv_url = (*p_drv->driver->get_driver_url)(p_drv);
  *p_url = drv_url;

#ifdef PIOMAN_POLL
  p_drv->ltask_binding = NULL;
  nm_ltask_submit_post_drv(p_drv);
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
  nm_drv_t p_drv = NULL;
  int err = nm_core_driver_load(p_core, driver, &p_drv);
  if (err != NM_ESUCCESS) 
    {
      NM_WARN("nm_core_driver_load returned %d", err);
      return err;
    }
  
  if(param && (param->key == NM_DRIVER_QUERY_BY_INDEX))
    p_drv->index = param->value.index;
  else
    p_drv->index = 0;

  if(!p_drv->driver->query)
    {
      TBX_FAILUREF("nmad: FATAL- driver %s has no 'query' method.\n", p_drv->driver->name);
    }
  err = p_drv->driver->query(p_drv, param, 1);
  if(err != NM_ESUCCESS)
    {
      TBX_FAILUREF("nmad: FATAL- error %d while querying driver %s\n", err, p_drv->driver->name);
    }

#ifdef PM2_TOPOLOGY
  if(!getenv("NMAD_NUIOA_DISABLE"))
    {
      hwloc_cpuset_t cpuset = p_drv->profile.cpuset;
      if(cpuset != NULL 
	 && !hwloc_bitmap_isequal(cpuset, hwloc_topology_get_complete_cpuset(topology)))
	{
	  /* if this driver wants something */
	  hwloc_cpuset_t current = hwloc_bitmap_alloc();
	  int rc = hwloc_get_cpubind(topology, current, HWLOC_CPUBIND_THREAD);
	  char*s_current = NULL;
	  hwloc_bitmap_asprintf(&s_current, current);
	  char*s_drv_cpuset = NULL;
	  hwloc_bitmap_asprintf(&s_drv_cpuset, p_drv->profile.cpuset);
	  if((rc == 0) && !hwloc_bitmap_isequal(current, hwloc_topology_get_complete_cpuset(topology)))
	    {
	      NM_DISPF("# nmad: nuioa- thread already bound to %s. Not binding to %s.\n", s_current, s_drv_cpuset);
	    }
	  else
	    {
	      NM_DISPF("# nmad: nuioa- driver '%s' has a preferred location. Binding to %s.\n",
		       p_drv->assembly->name, s_drv_cpuset);
	      rc = hwloc_set_cpubind(topology, p_drv->profile.cpuset, HWLOC_CPUBIND_THREAD);
	      if(rc)
		{
		  fprintf(stderr, "nmad: WARNING- hwloc_set_cpubind failed.\n");
		}
	    }
	  free(s_drv_cpuset);
	  free(s_current);
	  hwloc_bitmap_free(current);
	}
      else
	{
	  NM_DISPF("# nmad: nuioa- driver '%s' has no preferred location. Not binding.\n", p_drv->assembly->name);
	}
    }
  else
    {
      NM_DISPF("# nmad: nuioa- disabled by user\n");
    }
#else /* PM2_TOPOLOGY */
  NM_DISPF("# nmad: nuioa- hwloc not available\n");
#endif /* PM2_TOPOLOGY */

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

  nm_lock_interface(p_core);
  nm_drv_t p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
#ifdef PIOMAN_POLL
      /* stop polling
       */
      nmad_unlock();
      nm_unlock_interface(p_core);
      piom_ltask_cancel(&p_drv->p_ltask);
#endif /* PIOMAN_POLL */
      /* cancel any pending active recv request 
       */

      if(p_drv->p_in_rq)
	{
	  struct nm_pkt_wrap*p_pw = p_drv->p_in_rq;
	  p_drv->p_in_rq = NULL;
	  if(p_drv->driver->cancel_recv_iov)
	    p_drv->driver->cancel_recv_iov(NULL, p_pw);
#if defined(NMAD_POLL)
	  tbx_fast_list_del(&p_pw->link);
#else
	  piom_ltask_cancel(&p_pw->ltask);
#endif
	  nm_pw_ref_dec(p_pw);
	}

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
#else
		  piom_ltask_cancel(&p_pw->ltask);
#endif
		  nm_pw_ref_dec(p_pw);
		}
	      p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
	      p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	    }
	}
#ifdef PIOMAN_POLL
      nm_lock_interface(p_core);
      nmad_lock();
#endif
    }

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
		  p_gdrv->receptacle.driver->disconnect(p_gdrv->receptacle._status, p_gate, p_drv, trk_id);
		  p_gdrv->p_in_rq_array[trk_id] = NULL;
		}
	    }
	}
    }
  /* deinstantiate all drivers */
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      nm_gdrv_vect_itor_t i;
      puk_vect_foreach(i, nm_gdrv, &p_gate->gdrv_array)
	{
	  struct nm_gate_drv*p_gdrv = *i;
	  *i = NULL;
	  if(p_gdrv->instance != NULL)
	    {
	      puk_instance_destroy(p_gdrv->instance);
	      p_gdrv->instance = NULL;
	    }
	  TBX_FREE(p_gdrv);
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
  nm_drv_t tmp_drv = NULL;
  tbx_fast_list_for_each_entry_safe(p_drv, tmp_drv, &p_core->driver_list, _link)
    {
      tbx_fast_list_del(&p_drv->_link);
      TBX_FREE(p_drv);
    }

  nm_unlock_interface(p_core);

#if(defined(PIOMAN))
  pioman_exit();
#endif

  return err;
}

