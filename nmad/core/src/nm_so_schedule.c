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
 
  /* requests lists */
#ifndef PIOMAN
  p_core->so_sched.pending_recv_req    = tbx_slist_nil();
  p_core->so_sched.out_req_list        = tbx_slist_nil();
#endif
  p_core->so_sched.post_recv_req       = tbx_slist_nil();
  p_core->so_sched.post_sched_out_list = tbx_slist_nil();

  /* unpacks */
  INIT_LIST_HEAD(&p_core->so_sched.unpacks);

  /* unexpected */
  tbx_malloc_init(&nm_so_unexpected_mem, sizeof(struct nm_unexpected_s), NM_UNEXPECTED_PREALLOC, "nmad/core/chunks");
  INIT_LIST_HEAD(&p_core->so_sched.unexpected);

  /* which strategy is going to be used */
  const char*strategy_name = NULL;
#if defined(CONFIG_STRAT_SPLIT_BALANCE)
  strategy_name = "split_balance";
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

/** Shutdown the scheduler.
 */
int nm_so_schedule_exit (struct nm_core *p_core)
{

  nmad_lock();

  /* purge requests not posted yet to the driver */
  while (!tbx_slist_is_nil(p_core->so_sched.post_recv_req))
    {
      NM_SO_TRACE("extracting pw from post_recv_req\n");
      void *pw = tbx_slist_extract(p_core->so_sched.post_recv_req);
      nm_so_pw_free(pw);
    }

#ifndef PIOMAN
  /* Sanity check- everything is supposed to be empty here */
  {
    p_tbx_slist_t pending_slist = p_core->so_sched.pending_recv_req;
    while (!tbx_slist_is_nil(pending_slist))
      {
	struct nm_pkt_wrap *p_pw = tbx_slist_extract(pending_slist);
	NM_DISPF("extracting pw from pending_recv_req\n");
	nm_so_pw_free(p_pw);
      }
  }
#endif

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  nm_so_monitor_vect_destroy(&p_core->so_sched.monitors);

  tbx_malloc_clean(nm_so_unexpected_mem);

  /* free requests lists */
#ifndef PIOMAN
  tbx_slist_clear(p_core->so_sched.pending_recv_req);
  tbx_slist_clear(p_core->so_sched.out_req_list);
  tbx_slist_free(p_core->so_sched.pending_recv_req);
  tbx_slist_free(p_core->so_sched.out_req_list);
#endif

  tbx_slist_clear(p_core->so_sched.post_recv_req);
  tbx_slist_clear(p_core->so_sched.post_sched_out_list);
  tbx_slist_free(p_core->so_sched.post_recv_req);
  tbx_slist_free(p_core->so_sched.post_sched_out_list);

  nmad_unlock();
  return NM_ESUCCESS;
}

