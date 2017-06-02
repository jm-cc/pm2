/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
  nm_drv_t p_drv = nm_drv_new();
  assert(driver_assembly != NULL);
  p_drv->p_core    = p_core;
  p_drv->assembly  = driver_assembly;
  p_drv->driver    = puk_component_get_driver_NewMad_Driver(p_drv->assembly, NULL);
  p_drv->p_in_rq   = NULL;
  p_drv->max_small = NM_LEN_UNDEFINED;
  p_drv->priv = NULL;
#ifdef PM2_TOPOLOGY
  p_drv->profile.cpuset = NULL;
#endif /* PM2_TOPOLOGY */

  nm_drv_list_push_back(&p_core->driver_list, p_drv);
  p_core->nb_drivers++;

  *pp_drv = p_drv;
  return NM_ESUCCESS;
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

  p_drv->p_core = p_core;

  if (!p_drv->driver->init)
    {
      err = -NM_EINVAL;
      goto out;
    }
  /* open tracks */
  const int nb_trks = NM_SO_MAX_TRACKS;
  /* Track 0- for unexpected packets */
  p_drv->trk_caps[NM_TRK_SMALL] = (struct nm_minidriver_capabilities_s) { 0 };
  /* Track 1- for long packets with rendezvous */
  p_drv->trk_caps[NM_TRK_LARGE] = (struct nm_minidriver_capabilities_s) { 0 };

  err = p_drv->driver->init(p_drv, p_drv->trk_caps, nb_trks);
  if (err != NM_ESUCCESS)
    {
      NM_WARN("drv.init returned %d", err);
      goto out;
    }
  p_drv->nb_tracks = nb_trks;

  const char*drv_url = (*p_drv->driver->get_driver_url)(p_drv);
  *p_url = drv_url;

#ifdef PIOMAN
  p_drv->ltask_binding = NULL;
#endif

  nm_ns_update(p_core, p_drv);
  err = NM_ESUCCESS;

  const nm_len_t max_small = p_drv->trk_caps[NM_TRK_SMALL].max_msg_size;
  if(max_small > 0)
    {
      p_drv->max_small = max_small;
    }
  else
    {
      p_drv->max_small = (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE - NM_ALIGN_FRONTIER);
    }

 out:

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
  
  if(!p_drv->driver->query)
    {
      NM_FATAL("nmad: FATAL- driver %s has no 'query' method.\n", p_drv->driver->name);
    }
  err = p_drv->driver->query(p_drv, param, 1);
  if(err != NM_ESUCCESS)
    {
      NM_FATAL("nmad: FATAL- error %d while querying driver %s\n", err, p_drv->driver->name);
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
  NM_DISPF("# nmad: driver name = %s; url = %s; max_small = %d\n", driver->name, *p_url, (int)p_drv->max_small);

  *pp_drv = p_drv;
  return NM_ESUCCESS;
}

/** Flush all pending pw on all drivers.
 */
void nm_core_driver_flush(struct nm_core*p_core)
{
  nm_core_lock_assert(p_core);
  struct nm_pkt_wrap_list_s*pending_pw = nm_pkt_wrap_list_new();
  nm_drv_t p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      /* cancel pre-posted pw on trk #0 for anygate */
      if(p_drv->p_in_rq)
	{
	  struct nm_pkt_wrap_s*p_pw = p_drv->p_in_rq;
	  p_drv->p_in_rq = NULL;
	  if(p_drv->driver->cancel_recv_iov)
	    p_drv->driver->cancel_recv_iov(NULL, p_pw);
#ifndef PIOMAN
	  nm_pkt_wrap_list_erase(&p_core->pending_recv_list, p_pw);
#endif /* PIOMAN */
	  nm_pkt_wrap_list_push_back(pending_pw, p_pw);
	}
      
      /* cancel pre-posted pw on trk #0 for each gate */
      nm_gate_t p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv != NULL)
	    {
	      struct nm_pkt_wrap_s*p_pw = p_gdrv->p_in_rq_array[NM_TRK_SMALL];
	      if(p_pw)
		{
		  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
		  if(r->driver->cancel_recv_iov)
		    r->driver->cancel_recv_iov(r->_status, p_pw);
		  p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;

#ifndef PIOMAN
		  nm_pkt_wrap_list_erase(&p_core->pending_recv_list, p_pw);
#endif
		  nm_pkt_wrap_list_push_back(pending_pw, p_pw);
		}
	      p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
	      p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	    }
	}
    }
  /* cancel ltasks with core unlocked */
  nm_core_unlock(p_core);
  struct nm_pkt_wrap_s*p_pw = NULL;
  do
    {
      p_pw = nm_pkt_wrap_list_pop_front(pending_pw);
      if(p_pw)
	{
#ifdef PIOMAN
	  piom_ltask_cancel(&p_pw->ltask);
#endif
	  nm_pw_ref_dec(p_pw);
	}
    }
  while(p_pw != NULL);
  nm_core_lock(p_core);
  nm_pkt_wrap_list_delete(pending_pw);
}

/** Shutdown & disconnect all drivers.
 */
void nm_core_driver_exit(struct nm_core*p_core)
{
  /* disconnect all gates */
  nm_gate_t p_gate = NULL;
  struct nm_drv_s*p_drv = NULL;
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
      nm_gtag_table_destroy(&p_gate->tags);
      puk_instance_destroy(p_gate->strategy_instance);
      nm_gdrv_vect_destroy(&p_gate->gdrv_array);
    }
  do
    {
      p_gate = nm_gate_list_pop_front(&p_core->gate_list);
      if(p_gate)
	nm_gate_delete(p_gate);
    }
  while(p_gate);
  do
    {
      p_drv = nm_drv_list_pop_front(&p_core->driver_list);
      if(p_drv)
	nm_drv_delete(p_drv);
    }
  while(p_drv);
}

