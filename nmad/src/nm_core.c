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

#include <Padico/Module.h>
PADICO_MODULE_BUILTIN(NewMad_Core, NULL, NULL, NULL);

#ifdef PIOMAN
#  ifdef FINE_GRAIN_LOCKING
piom_spinlock_t nm_tac_lock;
piom_spinlock_t nm_status_lock;
#  else
piom_spinlock_t piom_big_lock;
#    ifdef DEBUG
volatile piom_thread_t piom_big_lock_holder = PIOM_THREAD_NULL;
#    endif /* DEBUG */
#  endif /* FINE_GRAIN_LOCKING */
#endif  /* PIOMAN */


void nm_drv_post_all(struct nm_drv*p_drv)
{ 
 /* schedule & post out requests */
  nm_drv_post_send(p_drv);
  
  /* post new receive requests */
  nm_drv_refill_recv(p_drv);
  nm_drv_post_recv(p_drv);
}

/** Main function of the core scheduler loop.
 *
 * This is the heart of NewMadeleine...
 */
int nm_schedule(struct nm_core *p_core)
{
#ifdef NMAD_POLL
  nmad_lock();  

#ifdef DEBUG
  static int scheduling_in_progress = 0;
  assert(!scheduling_in_progress);
  scheduling_in_progress = 1;  
#endif /* DEBUG */

  nm_try_and_commit(p_core);

  /* post new requests	*/
  struct nm_drv*p_drv = NULL;
  NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core)
  {
    nm_drv_post_all(p_drv);
  }

  /* poll pending recv requests */
  if (!tbx_fast_list_empty(&p_core->pending_recv_list))
    {
      NM_TRACEF("polling inbound requests");
      struct nm_pkt_wrap*p_pw, *p_pw2;
      tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_core->pending_recv_list, link)
	{
	  nm_pw_poll_recv(p_pw);
	}
    }
  /* poll pending out requests */
  if (!tbx_fast_list_empty(&p_core->pending_send_list))
    {
      NM_TRACEF("polling outbound requests");
      struct nm_pkt_wrap*p_pw, *p_pw2;
      tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_core->pending_send_list, link)
	{
	  nm_pw_poll_send(p_pw);
	}
    }

  nm_out_prefetch(p_core);
  
  nmad_unlock();
  
#ifdef DEBUG
  scheduling_in_progress = 0;
#endif /* DEBUG */

  return NM_ESUCCESS;
#else  /* NMAD_POLL */
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
  return 0;
#endif /* NMAD_POLL */
}


/** Add an event monitor to the list */
void nm_core_monitor_add(nm_core_t p_core, const struct nm_core_monitor_s*m)
{
  nm_core_monitor_vect_push_back(&p_core->monitors, m);
}

void nm_core_monitor_remove(nm_core_t p_core, const struct nm_core_monitor_s*m)
{
  nm_core_monitor_itor_t i = nm_core_monitor_vect_find(&p_core->monitors, m);
  if(i)
    {
      nm_core_monitor_vect_erase(&p_core->monitors, i);
    }
}

/** Load a newmad component from disk. The actual component is
 * NewMad_<entity>_<name> where <entity> is either 'driver' or 'strategy'.
 */
puk_component_t nm_core_component_load(const char*entity, const char*name)
{
  char component_name[1024];
  snprintf(component_name, 1024, "NewMad_%s_%s", entity, name);
  puk_component_t component = puk_adapter_resolve(component_name);
  if(component == NULL)
    {
      padico_fatal("nmad: failed to load component '%s'\n", component_name);
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
      tbx_init(argc, &argv);
    }

  FUT_DO_PROBE0(FUT_NMAD_INIT_CORE);

  /* allocate core object and init lists */

  struct nm_core *p_core = TBX_MALLOC(sizeof(struct nm_core));
  memset(p_core, 0, sizeof(struct nm_core));

  TBX_INIT_FAST_LIST_HEAD(&p_core->gate_list);
  TBX_INIT_FAST_LIST_HEAD(&p_core->driver_list);
  p_core->nb_drivers = 0;

  /* Initialize "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_init(p_core);

  nm_core_monitor_vect_init(&p_core->monitors);

  /* unpacks */
  TBX_INIT_FAST_LIST_HEAD(&p_core->unpacks);

  /* unexpected */
  TBX_INIT_FAST_LIST_HEAD(&p_core->unexpected);
  
  /* pending packs */
  TBX_INIT_FAST_LIST_HEAD(&p_core->pending_packs);

#ifdef NMAD_POLL
  TBX_INIT_FAST_LIST_HEAD(&p_core->pending_recv_list);
  TBX_INIT_FAST_LIST_HEAD(&p_core->pending_send_list);
#endif /* NMAD_POLL*/

  p_core->strategy_adapter = NULL;

#ifdef PIOMAN
  pioman_init(argc, argv);
  nmad_lock_init(p_core);
  nm_lock_interface_init(p_core);
  nm_lock_status_init(p_core);  
#if(defined(PIOMAN_POLL))
  nm_ltask_set_policy();
#endif	/* PIOM_POLL */
#endif /* PIOMAN */

  *pp_core = p_core;

  return err;
}

int nm_core_set_strategy(nm_core_t p_core, puk_component_t strategy)
{
  puk_facet_t strat_facet = puk_component_get_facet_NewMad_Strategy(strategy, NULL);
  if(strat_facet == NULL)
    {
      fprintf(stderr, "# nmad: component %s given as strategy has no interface 'NewMad_Strategy'\n", strategy->name);
      abort();
    }
  p_core->strategy_adapter = strategy;
  NM_DISPF("# nmad: strategy = %s\n", strategy->name);
  return NM_ESUCCESS;
}

/** Shutdown the core struct and the main scheduler.
 */
int nm_core_exit(nm_core_t p_core)
{
  nmad_lock();

  /* flush strategies */
  int strat_done = 0;
  while(!strat_done)
    {
      int todo = 0;
      struct nm_gate*p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
	  if(r->driver->todo && r->driver->todo(r->_status, p_gate)) 
	    {
	      todo = 1;
	    }
	}
      if(todo)
	nm_schedule(p_core);
      strat_done = !todo;
    }

  nm_core_driver_exit(p_core);

  /* purge receive requests not posted yet to the driver */
  struct nm_drv*p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_trk_id_t trk;
      for(trk = 0; trk < NM_SO_MAX_TRACKS; trk++)
	{
	  struct nm_pkt_wrap*p_pw, *p_pw2;
	  tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_drv->post_recv_list[trk], link)
	    {
	      NM_TRACEF("extracting pw from post_recv_list\n");
	      nm_so_pw_free(p_pw);
	    }
	}
    }

#ifdef NMAD_POLL
  /* Sanity check- everything is supposed to be empty here */
  struct nm_pkt_wrap*p_pw, *p_pw2;
  tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_core->pending_recv_list, link)
    {
      NM_WARN("purging pw from pending_recv_list\n");
      nm_so_pw_free(p_pw);
    }
#endif /* NMAD_POLL */
  
  /* Remove any remaining unexpected chunk */
  nm_unexpected_clean(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  nm_core_monitor_vect_destroy(&p_core->monitors);

  nmad_unlock();

  nm_ns_exit(p_core);
  TBX_FREE(p_core);

  return NM_ESUCCESS;
}

