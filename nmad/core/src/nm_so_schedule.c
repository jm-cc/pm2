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

#include <nm_private.h>


p_tbx_memory_t nm_so_unexpected_mem = NULL;

/** Initialize SchedOpt.
 */
int nm_so_schedule_init(struct nm_core *p_core)
{
  int err;
  struct nm_so_sched *p_so_sched = &p_core->so_sched;

  /* Initialize "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_init(p_core);

  nm_so_monitor_vect_init(&p_core->so_sched.monitors);

  /* unpacks */
  TBX_INIT_FAST_LIST_HEAD(&p_core->so_sched.unpacks);

  /* unexpected */
  tbx_malloc_extended_init(&nm_so_unexpected_mem, sizeof(struct nm_unexpected_s), NM_UNEXPECTED_PREALLOC, "nmad/core/unexpected", 1);

  TBX_INIT_FAST_LIST_HEAD(&p_core->so_sched.unexpected);
  
  /* pending packs */
  TBX_INIT_FAST_LIST_HEAD(&p_core->so_sched.pending_packs);

  /* which strategy is going to be used */
  const char*strategy_name = NULL;
#if defined(CONFIG_STRAT_SPLIT_BALANCE)
  strategy_name = "split_balance";
#elif defined(CONFIG_STRAT_SPLIT_ALL)
  strategy_name = "split_all";
#elif defined(CONFIG_STRAT_DEFAULT)
  strategy_name = "default";
#elif defined(CONFIG_STRAT_AGGREG)
  strategy_name = "aggreg";
#elif defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
  strategy_name = "aggreg_autoextended";
#elif defined(CONFIG_STRAT_QOS)
#  if defined(NMAD_QOS)
  strategy_name = "qos";
#  else
#    error Strategy Quality of service cannot be selected without enabling the quality of service
#  endif /* NMAD_QOS */
#else /*  defined(CONFIG_STRAT_CUSTOM) */
  strategy_name = "custom";
#endif

  p_so_sched->strategy_adapter = nm_core_component_load("strategy", strategy_name);
  printf("# nmad: strategy = %s\n", p_so_sched->strategy_adapter->name);

  err = NM_ESUCCESS;
  return err;
}

/** Remove any remaining unexpected chunk */
int nm_so_clean_unexpected(struct nm_core *p_core)
{  
  struct nm_unexpected_s*chunk, *tmp;
  int ret = 0;
  tbx_fast_list_for_each_entry_safe(chunk, tmp, &p_core->so_sched.unexpected, link)
  {
    if(chunk){
#ifdef NMAD_DEBUG
      fprintf(stderr, "nm_so_clean_unexpected: Chunk %p is still in use\n", chunk);
#endif
      ret++;
      if(chunk->p_pw)
        nm_so_pw_free(chunk->p_pw);
      tbx_fast_list_del(&chunk->link);
      tbx_free(nm_so_unexpected_mem, chunk);
    }
  }
  return ret;
}

/** Shutdown the scheduler.
 */
int nm_so_schedule_exit (struct nm_core *p_core)
{
  nmad_lock();

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

  nm_so_clean_unexpected(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  nm_so_monitor_vect_destroy(&p_core->so_sched.monitors);

  tbx_malloc_clean(nm_so_unexpected_mem);

  nmad_unlock();

  return NM_ESUCCESS;
}
