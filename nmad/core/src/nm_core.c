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
#  ifdef FINE_GRAIN_LOCKING
piom_spinlock_t nm_tac_lock;
piom_spinlock_t nm_status_lock;
#  else
piom_spinlock_t piom_big_lock = PIOM_SPIN_LOCK_INITIALIZER;
#  endif /* FINE_GRAIN_LOCKING */
#endif  /* PIOMAN */


/** Main function of the core scheduler loop.
 *
 * This is the heart of NewMadeleine...
 */
int nm_schedule(struct nm_core *p_core)
{
#ifdef NMAD_POLL
  nmad_lock();  

#ifdef NMAD_DEBUG
  static int scheduling_in_progress = 0;
  assert(!scheduling_in_progress);
  scheduling_in_progress = 1;  
#endif /* NMAD_DEBUG */

  nm_sched_out(p_core);

  nm_sched_in(p_core);
  
  nmad_unlock();
  
#ifdef NMAD_DEBUG
  scheduling_in_progress = 0;
#endif /* NMAD_DEBUG */

  return NM_ESUCCESS;
#else  /* NMAD_POLL */
  __piom_check_polling(PIOM_POLL_WHEN_FORCED);
  return 0;
#endif /* NMAD_POLL */
}


#ifdef PIOMAN_POLL
int nm_core_disable_progression(struct nm_core*p_core)
{
  struct nm_drv*p_drv;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      piom_pause_server(&p_drv->server);
    }
  return 0;
}

int nm_core_enable_progression(struct nm_core *p_core)
{
  struct nm_drv*p_drv;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      piom_resume_server(&p_drv->server);
    }
  return 0;
}
#endif	/* PIOMAN_POLL */


/** Add an event monitor to the list */
void nm_so_monitor_add(nm_core_t p_core, const struct nm_so_monitor_s*m)
{
  nm_so_monitor_vect_push_back(&p_core->monitors, m);
}

/** Load a newmad component from disk. The actual path loaded is
 * ${PM2_CONF_DIR}/nmad/'entity'/'entity'_'driver'.xml
 */
puk_component_t nm_core_component_load(const char*entity, const char*name)
{
  char filename[1024];
  int rc = 0;
  const char*pm2_conf_dir = getenv("PM2_CONF_DIR");
  const char*home = getenv("HOME");
  if(pm2_conf_dir)
    {
      /* $PM2_CONF_DIR/nmad/<entity>/<entity>_<name>.xml */
      rc = snprintf(filename, 1024, "%s/nmad/%s/%s_%s.xml", pm2_conf_dir, entity, entity, name);
    }
  else if(home)
    {
      /* $HOME/.pm2/nmad/<entity>/<entity>_<name>.xml */
      rc = snprintf(filename, 1024, "%s/.pm2/nmad/%s/%s_%s.xml", home, entity, entity, name);
    }
  else
    {
      TBX_FAILURE("nmad: cannot compute PM2_CONF_DIR. Environment variable $HOME is not set.\n");
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

  /* allocate core object and init lists */

  struct nm_core *p_core = TBX_MALLOC(sizeof(struct nm_core));
  memset(p_core, 0, sizeof(struct nm_core));

  TBX_INIT_FAST_LIST_HEAD(&p_core->gate_list);
  TBX_INIT_FAST_LIST_HEAD(&p_core->driver_list);
  p_core->nb_drivers = 0;

  /* Initialize "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_init(p_core);

  nm_so_monitor_vect_init(&p_core->monitors);

  /* unpacks */
  TBX_INIT_FAST_LIST_HEAD(&p_core->unpacks);

  /* unexpected */
  TBX_INIT_FAST_LIST_HEAD(&p_core->unexpected);
  
  /* pending packs */
  TBX_INIT_FAST_LIST_HEAD(&p_core->pending_packs);

  p_core->strategy_adapter = NULL;

#ifdef PIOMAN
  nmad_lock_init(p_core);
  nm_lock_interface_init(p_core);
  nm_lock_status_init(p_core);  
#ifdef PIOM_ENABLE_LTASKS
  nm_ltask_set_policy();
#endif	/* PIOM_ENABLE_LTASKS */
#endif /* PIOMAN */

  *pp_core = p_core;

  return err;
}

int nm_core_set_strategy(nm_core_t p_core, puk_component_t strategy)
{
  puk_iface_t strat_iface = puk_iface_NewMad_Strategy();
  puk_facet_t strat_facet = puk_adapter_get_facet(strategy, strat_iface, NULL);
  if(strat_facet == NULL)
    {
      fprintf(stderr, "# nmad: component %s given as strategy has no interface 'NewMad_Strategy'\n", strategy->name);
      abort();
    }
  p_core->strategy_adapter = strategy;
  printf("# nmad: strategy = %s\n", strategy->name);
 return NM_ESUCCESS;
}

/** Shutdown the core struct and the main scheduler.
 */
int nm_core_exit(nm_core_t p_core)
{
  nmad_lock();

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
	      NM_SO_TRACE("extracting pw from post_recv_list\n");
	      nm_so_pw_free(p_pw);
	    }
	}
    }

#ifdef NMAD_POLL
  /* Sanity check- everything is supposed to be empty here */
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      struct nm_pkt_wrap*p_pw, *p_pw2;
      tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_drv->pending_recv_list, link)
	{
	  NM_DISPF("extracting pw from pending_recv_list\n");
	  nm_so_pw_free(p_pw);
	}
    }
#endif /* NMAD_POLL */
  
  /** Remove any remaining unexpected chunk */
  nm_unexpected_clean(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  nm_so_monitor_vect_destroy(&p_core->monitors);

  nmad_unlock();

  nm_ns_exit(p_core);
  TBX_FREE(p_core);

  return NM_ESUCCESS;
}

