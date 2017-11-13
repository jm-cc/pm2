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


/** Load and initialize a driver with a parameter,
 * and applying numa binding in-between.
 */
int nm_core_driver_load_init(nm_core_t p_core, puk_component_t driver_component,
			     struct nm_driver_query_param*param,
			     nm_drv_t *pp_drv, const char**p_url)
{
  /* ** allocate & init nm_drv */
  nm_drv_t p_drv = nm_drv_new();
  assert(driver_component != NULL);
  p_drv->p_core    = p_core;
  p_drv->assembly  = driver_component;
  p_drv->driver    = puk_component_get_driver_NewMad_minidriver(p_drv->assembly, NULL);
  p_drv->minidriver_context = puk_component_get_context(driver_component, puk_iface_NewMad_minidriver(), NULL);
  p_drv->p_pw_recv_any = NULL;
  p_drv->props.capabilities = (struct nm_minidriver_capabilities_s){ 0 };
#ifdef PM2_TOPOLOGY
  p_drv->props.profile.cpuset = NULL;
#endif /* PM2_TOPOLOGY */
  nm_drv_list_push_back(&p_core->driver_list, p_drv);
  p_core->nb_drivers++;
  if(!p_drv->minidriver_context)
    {
      NM_FATAL("nmad: FATAL- cannot find context in minidriver component %s. Bad assembly.\n",
	       driver_component->name);
    }

  /* ** driver pre-init */
  if(!p_drv->driver->getprops)
    {
      NM_FATAL("nmad: FATAL- driver %s has no 'getprops' method.\n", driver_component->name);
    }
  (*p_drv->driver->getprops)(p_drv->minidriver_context, &p_drv->props);

#ifdef PM2_TOPOLOGY
  /* ** NUIOA */
  if(!getenv("NMAD_NUIOA_DISABLE"))
    {
      hwloc_cpuset_t cpuset = p_drv->props.profile.cpuset;
      if(cpuset != NULL 
	 && !hwloc_bitmap_isequal(cpuset, hwloc_topology_get_complete_cpuset(topology)))
	{
	  /* if this driver wants something */
	  hwloc_cpuset_t current = hwloc_bitmap_alloc();
	  int rc = hwloc_get_cpubind(topology, current, HWLOC_CPUBIND_THREAD);
	  char*s_current = NULL;
	  hwloc_bitmap_asprintf(&s_current, current);
	  char*s_drv_cpuset = NULL;
	  hwloc_bitmap_asprintf(&s_drv_cpuset, p_drv->props.profile.cpuset);
	  if((rc == 0) && !hwloc_bitmap_isequal(current, hwloc_topology_get_complete_cpuset(topology)))
	    {
	      NM_DISPF("# nmad: nuioa- thread already bound to %s. Not binding to %s.\n", s_current, s_drv_cpuset);
	    }
	  else
	    {
	      NM_DISPF("# nmad: nuioa- driver '%s' has a preferred location. Binding to %s.\n",
		       p_drv->assembly->name, s_drv_cpuset);
	      rc = hwloc_set_cpubind(topology, p_drv->props.profile.cpuset, HWLOC_CPUBIND_THREAD);
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

  /* ** driver init*/
  if(!p_drv->driver->init)
    {
      NM_FATAL("nmad: FATAL- driver %s has no 'init' method.\n", p_drv->assembly->name);
    }
  /* init driver & get url */
  const void*url = NULL;
  size_t url_size = 0;
  (*p_drv->driver->init)(p_drv->minidriver_context, &url, &url_size);
  int iurl_size = url_size;
  const char*drv_url = puk_hex_encode(url, &iurl_size, NULL);
  *p_url = drv_url;

#ifdef PIOMAN
  p_drv->ltask_binding = NULL;
#endif

  nm_ns_update(p_core, p_drv);

  if(p_drv->props.capabilities.max_msg_size == 0)
    {
      p_drv->props.capabilities.max_msg_size = NM_LEN_MAX;
    }

  NM_DISPF("# nmad: driver name = %s; url = %s; max_msg_size = %llu\n",
	   driver_component->name, *p_url, (unsigned long long)p_drv->props.capabilities.max_msg_size);
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
      if(p_drv->p_pw_recv_any)
	{
	  struct nm_pkt_wrap_s*p_pw = p_drv->p_pw_recv_any;
          if(p_drv->driver->recv_cancel_any)
            {
              nm_pw_ref_inc(p_pw);
              (*p_drv->driver->recv_cancel_any)(p_drv->minidriver_context);
            }
#ifndef PIOMAN
	  nm_pkt_wrap_list_remove(&p_core->pending_recv_list, p_pw);
	  nm_pkt_wrap_list_push_back(pending_pw, p_pw);
#else
	  int rc = piom_ltask_cancel_request(&p_pw->ltask);
	  if(rc == 0)
	    {
	      nm_pkt_wrap_list_push_back(pending_pw, p_pw);
	    }
#endif /* PIOMAN */
	  p_drv->p_pw_recv_any = NULL;
	}
    }
  
  /* cancel pre-posted pw on trk #0 for each gate */
  nm_gate_t p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      int i;
      for(i = 0; i < p_gate->n_trks; i++)
	{
	  struct nm_trk_s*p_trk = &p_gate->trks[i];
	  struct nm_pkt_wrap_s*p_pw = p_trk->p_pw_recv;
	  if(p_pw)
	    {
	      struct puk_receptacle_NewMad_minidriver_s*r = &p_trk->receptacle;
	      if(r->driver->recv_cancel)
                {
                  nm_pw_ref_inc(p_pw);
                  (*r->driver->recv_cancel)(r->_status);
                }
	      p_trk->p_pw_recv = NULL;
	      
#ifndef PIOMAN
	      nm_pkt_wrap_list_remove(&p_core->pending_recv_list, p_pw);
	      nm_pkt_wrap_list_push_back(pending_pw, p_pw);
#else
	      int rc = piom_ltask_cancel_request(&p_pw->ltask);
	      if(rc == 0)
		{
		  nm_pkt_wrap_list_push_back(pending_pw, p_pw);
		}
#endif
	    }
	  p_trk->p_pw_recv= NULL;
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
	  piom_ltask_cancel_wait(&p_pw->ltask);
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
      int i;
      for(i = 0; i < p_gate->n_trks; i++)
	{
	  struct nm_trk_s*p_trk = &p_gate->trks[i];
	  if(p_trk->instance != NULL)
	    {
              puk_hashtable_remove(p_core->trk_table, p_trk->receptacle._status);
              puk_instance_destroy(p_trk->instance);
	      p_trk->instance = NULL;
	    }
	}
    }
  /* close all drivers */
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      if(p_drv->driver->close)
	{
	  (*p_drv->driver->close)(p_drv->minidriver_context);
	}
    }
  /* close all gates */
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      nm_gtag_table_destroy(&p_gate->tags);
      puk_instance_destroy(p_gate->strategy_instance);
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

