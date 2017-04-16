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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>
#include <sched.h>

#include <nm_private.h>

#include <Padico/Module.h>
PADICO_MODULE_BUILTIN(NewMad_Core, NULL, NULL, NULL);


/** Main scheduler func for a specific driver.
   - this function must be called once for each driver on a regular basis
 */
void nm_drv_post_all(nm_drv_t p_drv)
{ 
  /* schedule & post out requests */
  nm_core_lock_assert(p_drv->p_core);
#ifndef PIOMAN
  struct nm_pkt_wrap_s*p_pw = NULL;
  do
    {
      NM_TRACEF("posting outbound requests");
      p_pw = nm_pw_post_lfqueue_dequeue_single_reader(&p_drv->post_send);
      if(p_pw)
	{
	  nm_pw_post_send(p_pw);
	}
    }
  while(p_pw);
#endif /* PIOMAN */
  
  /* post new receive requests */
  if(p_drv->p_core->enable_schedopt)
    nm_drv_refill_recv(p_drv);
  nm_drv_post_recv(p_drv);
}


/** checks whether an event matches a given core monitor */
static inline int nm_core_event_matches(const struct nm_core_monitor_s*p_core_monitor,
					const struct nm_core_event_s*p_event)
{
  const nm_status_t status = p_event->status & ~NM_STATUS_FINALIZED;
  const int matches =
    ( (p_core_monitor->monitor.event_mask & status) &&
      (p_core_monitor->matching.p_gate == NM_GATE_NONE || p_core_monitor->matching.p_gate == p_event->p_gate) &&
      (nm_core_tag_match(p_event->tag, p_core_monitor->matching.tag, p_core_monitor->matching.tag_mask))
      );
  return matches;
}

/** try to schedule pending out-of-order events */
static void nm_core_pending_event_recover(nm_core_t p_core)
{
  nm_core_lock_assert(p_core);
  nm_core_pending_event_itor_t i;
  int matched = 0;
 restart:
  matched = 0;
  puk_list_foreach(i, &p_core->pending_events)
    {
      struct nm_core_pending_event_s*p_pending_event = i;
      /* only matching events are supposed to be enqueued; check anyway */
      assert(nm_core_event_matches(p_pending_event->p_core_monitor, &p_pending_event->event));
      assert(p_pending_event->event.status & NM_STATUS_UNEXPECTED);
      struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_pending_event->event.p_gate->tags, p_pending_event->event.tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(p_pending_event->event.seq == next_seq)
	{
#ifdef PIOMAN
	  int locked = !piom_mask_acquire(&p_pending_event->p_core_monitor->dispatching);
#else
	  int locked = 1;
#endif
	  if(locked)
	    {
	      matched = 1;
	      p_so_tag->recv_seq_number = next_seq;
	      nm_core_pending_event_list_erase(&p_core->pending_events, p_pending_event);
	      nm_core_unlock(p_core);
	      (p_pending_event->p_core_monitor->monitor.p_notifier)(&p_pending_event->event, p_pending_event->p_core_monitor->monitor.ref);
#ifdef PIOMAN
	      piom_mask_release(&p_pending_event->p_core_monitor->dispatching);
#endif
	      nm_core_lock(p_core);
	    }
	}
      if(matched)
	{
	  nm_core_pending_event_delete(p_pending_event);
	  p_pending_event = NULL;
	  goto restart; /* exit the foreach loop so as not to break the iterator, retry to find another matching event */
	}
    }
}

/** apply strategy
 */
void nm_strat_apply(struct nm_core*p_core)
{
  nm_gate_t p_gate = NULL;
  nm_core_lock_assert(p_core);
  nm_profile_inc(p_core->profiling.n_strat_apply);
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  /* schedule new requests on all gates */
	  nm_strat_try_and_commit(p_gate);
	  /* process postponed recv requests */
	  if(!nm_pkt_wrap_list_empty(&p_gate->pending_large_recv))
	    {
	      const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	      strategy->driver->rdv_accept(strategy->_status, p_gate);
	    }

	}
    }
  /* schedule pending out-of-order events */
  if(!nm_core_pending_event_list_empty(&p_core->pending_events))
     nm_core_pending_event_recover(p_core);
}

/** Main function of the core scheduler loop.
 *
 * This is the heart of NewMadeleine...
 */
int nm_schedule(struct nm_core*p_core)
{
  nm_profile_inc(p_core->profiling.n_schedule);
#ifdef PIOMAN
  piom_polling_force();
#else

#ifdef DEBUG
  static int scheduling_in_progress = 0;
  assert(!scheduling_in_progress);
  scheduling_in_progress = 1;  
#endif /* DEBUG */

  nm_strat_apply(p_core);

  /* post new requests	*/
  nm_drv_t p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
  {
    nm_drv_post_all(p_drv);
  }

  /* poll pending recv requests */
  if(!nm_pkt_wrap_list_empty(&p_core->pending_recv_list))
    {
      NM_TRACEF("polling inbound requests");
      struct nm_pkt_wrap_s*p_pw, *p_pw2;
      puk_list_foreach_safe(p_pw, p_pw2, &p_core->pending_recv_list)
	{
	  nm_pw_poll_recv(p_pw);
	}
    }
  /* poll pending out requests */
  if(!nm_pkt_wrap_list_empty(&p_core->pending_send_list))
    {
      NM_TRACEF("polling outbound requests");
      struct nm_pkt_wrap_s*p_pw, *p_pw2;
      puk_list_foreach_safe(p_pw, p_pw2, &p_core->pending_send_list)
	{
	  nm_pw_poll_send(p_pw);
	}
    }

  nm_out_prefetch(p_core);
  
#ifdef DEBUG
  scheduling_in_progress = 0;
#endif /* DEBUG */

#endif /* PIOMAN */
  return NM_ESUCCESS;
}

void nm_core_req_monitor(struct nm_core*p_core, struct nm_req_s*p_req, struct nm_monitor_s monitor)
{
  nm_core_lock(p_core); /* lock needed for atomicity of set monitor + tests status */
  assert(p_req->monitor.p_notifier == NULL);
  p_req->monitor = monitor;
  nm_status_t status = nm_status_test(p_req, monitor.event_mask);
  nm_core_unlock(p_core);
  if(status)
    {
      /* immediate event- unlocked */
      const struct nm_core_event_s event =
	{
	  .status = status,
	  .p_req  = p_req
	};
      (*p_req->monitor.p_notifier)(&event, p_req->monitor.ref);
    }
}

/** dispatch a global event that matches the given monitor */
static void nm_core_event_notify(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_core_monitor_s*p_core_monitor)
{
  int locked = 0;
  int pending = 0; /* mark event as pending */
  nm_core_lock_assert(p_core);
  assert(nm_core_event_matches(p_core_monitor, p_event));
  if(p_event->status & NM_STATUS_UNEXPECTED)
    {
#ifdef PIOMAN
      locked = !piom_mask_acquire(&p_core_monitor->dispatching);
#else
      locked = 1;
#endif
      if(locked)
	{
	  struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_event->p_gate->tags, p_event->tag);
	  const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
	  if(p_event->seq == next_seq)
	    {
	      p_so_tag->recv_seq_number = next_seq;
	    }
	  else
	    {
	      /*
	      fprintf(stderr, "# nmad: WARNING- delaying event dispatch; got seq = %d; expected = %d; tag = %lx:%lx; pending_events = %d\n",
		      p_event->seq, next_seq, nm_core_tag_get_hashcode(p_event->tag), nm_core_tag_get_tag(p_event->tag),
		      nm_core_pending_event_list_size(&p_core->pending_events));
	      */
	      pending = 1;
	      goto out;
	    }
	}
      else
	{
	  const int num_pending = nm_core_pending_event_list_size(&p_core->pending_events);
	  fprintf(stderr, "# nmad: WARNING- delaying event dispatch; already in progress (pending = %d)\n", num_pending);
	  pending = 1;
	  goto out;
	}
    }
  nm_core_unlock(p_core);
  (p_core_monitor->monitor.p_notifier)(p_event, p_core_monitor->monitor.ref);
  nm_core_lock(p_core);
 out:
  if(locked)
    {
#ifdef PIOMAN
      piom_mask_release(&p_core_monitor->dispatching);
#endif
    }
  if(pending)
    {
      struct nm_core_pending_event_s*p_pending_event = nm_core_pending_event_new();
      p_pending_event->event = *p_event;
      p_pending_event->p_core_monitor = p_core_monitor;
      nm_core_pending_event_list_push_back(&p_core->pending_events, p_pending_event);
      assert(nm_core_pending_event_list_size(&p_core->pending_events) < NM_SEQ_MAX - 1); /* seq number overflow */
      nm_core_pending_event_recover(p_core);
    }
}

/** Add an event monitor to the list */
void nm_core_monitor_add(nm_core_t p_core, struct nm_core_monitor_s*p_core_monitor)
{
  nm_core_lock(p_core);
#ifdef PIOMAN
  piom_mask_init(&p_core_monitor->dispatching);
#endif
  if(p_core_monitor->monitor.event_mask == NM_STATUS_UNEXPECTED)
    {
      struct nm_unexpected_s*p_chunk = NULL, *p_tmp;
      puk_list_foreach_safe(p_chunk, p_tmp, &p_core->unexpected) /* use 'safe' iterator since notifier is likely to post a recv */
	{
	  if(p_chunk->msg_len != NM_LEN_UNDEFINED)
	    {
	      const struct nm_core_event_s event =
		{
		  .status = NM_STATUS_UNEXPECTED,
		  .p_gate = p_chunk->p_gate,
		  .tag    = p_chunk->tag,
		  .seq    = p_chunk->seq,
		  .len    = p_chunk->msg_len
		};
	      if(nm_core_event_matches(p_core_monitor, &event))
		{
		  nm_core_event_notify(p_core, &event, p_core_monitor);
		}
	    }
	}
    }
  nm_core_monitor_vect_push_back(&p_core->monitors, p_core_monitor);
  nm_core_unlock(p_core);
}

void nm_core_monitor_remove(nm_core_t p_core, struct nm_core_monitor_s*m)
{
  nm_core_lock(p_core);
  nm_core_monitor_vect_itor_t i = nm_core_monitor_vect_find(&p_core->monitors, m);
  if(i)
    {
      nm_core_monitor_vect_erase(&p_core->monitors, i);
    }
  nm_core_unlock(p_core);
}

/** Fires an event
 */
void nm_core_status_event(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_req_s*p_req)
{
  if(p_req)
    {
      const nm_status_t notified_bits1 = (p_event->status &  p_req->monitor.event_mask) & ~NM_STATUS_FINALIZED;
      const nm_status_t notified_bits2 = (p_event->status &  p_req->monitor.event_mask) &  NM_STATUS_FINALIZED;
      const nm_status_t signaled_bits  =  p_event->status & ~p_req->monitor.event_mask;
      assert((notified_bits1 | notified_bits2 | signaled_bits) == p_event->status);
      if(notified_bits1)
	{
	  /* add bits (no signal) and notify, for bits with a monitor listening, 
	   * except FINALIZED (should be last) */
	  nm_status_add(p_req, notified_bits1);
	  nm_core_unlock(p_core);
	  (*p_req->monitor.p_notifier)(p_event, p_req->monitor.ref);
	  nm_core_lock(p_core);
	}
      if(signaled_bits)
	{
	  /* signal bits with no monitors */
	  nm_status_signal(p_req, p_event->status);
	}
      if(notified_bits2)
	{
	  /* add & notify FINALIZED _last_, if a monitor listening
	   * (if no monitor, FINALIZED is signaled) */
	  nm_status_add(p_req, notified_bits2);
	  nm_core_unlock(p_core);
	  (*p_req->monitor.p_notifier)(p_event, p_req->monitor.ref);
	  nm_core_lock(p_core);
	}
    }
  else
    {
      /* fire global monitors */
#ifdef DEBUG
      int matched = 0;
#endif /* DEBUG */
      nm_core_monitor_vect_itor_t i;
      puk_vect_foreach(i, nm_core_monitor, &p_core->monitors)
	{
	  struct nm_core_monitor_s*p_core_monitor = *i;
	  if(nm_core_event_matches(p_core_monitor, p_event))
	    {
#ifdef DEBUG
	      if(matched != 0)
		{
		  NM_FATAL("multiple monitors match the same core event.\n");
		}
	      matched++;
#endif /* DEBUG */
	      nm_core_event_notify(p_core, p_event, p_core_monitor);
	    }
	}
    }
}

/** Load a newmad component from disk. The actual component is
 * NewMad_<entity>_<name> where <entity> is either 'driver' or 'strategy'.
 */
puk_component_t nm_core_component_load(const char*entity, const char*name)
{
  puk_component_t component = NULL;
  if((strcmp(entity, "Driver") == 0) && (strcmp(name, "local") == 0))
    {
      static const char minidriver_local[] =
	"<puk:composite id=\"nsbe:nmminilocal\">"
	"  <puk:component id=\"0\" name=\"Minidriver_local\"/>"
	"  <puk:component id=\"1\" name=\"Minidriver_local\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      component = puk_component_parse(minidriver_local);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", minidriver_local);
	}
    }
  else if((strcmp(entity, "Driver") == 0) && (strcmp(name, "dcfa") == 0))
    {
      static const char dcfa[] = 
	"<puk:composite id=\"nm:dcfa\">"
	"  <puk:component id=\"0\" name=\"NewMad_dcfa_bycopy\"/>"
	"  <puk:component id=\"1\" name=\"NewMad_dcfa_lr2\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      component = puk_component_parse(dcfa);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", dcfa);
	}
    }
  else if((strcmp(entity, "Driver") == 0) && (strcmp(name, "shm") == 0))
    {
      static const char minidriver_shm[] = 
	"<puk:composite id=\"nm:shm-pipe\">"
	"  <puk:component id=\"0\" name=\"Minidriver_shm\"/>"
	"  <puk:component id=\"1\" name=\"Minidriver_largeshm\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      component = puk_component_parse(minidriver_shm);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", minidriver_shm);
	}
    }
  else if((strcmp(entity, "Driver") == 0) && (strcmp(name, "cma") == 0))
    {
      static const char minidriver_cma[] = 
	"<puk:composite id=\"nm:shm-cma\">"
	"  <puk:component id=\"0\" name=\"Minidriver_shm\"/>"
	"  <puk:component id=\"1\" name=\"Minidriver_CMA\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      component = puk_component_parse(minidriver_cma);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", minidriver_cma);
	}
    }
  else if((strcmp(entity, "Driver") == 0) && (strcmp(name, "ibverbs") == 0))
    {
      static const char ib_lr2[] = 
	"<puk:composite id=\"nm:ib-lr2\">"
	"  <puk:component id=\"0\" name=\"NewMad_ibverbs_bycopy\"/>"
	"  <puk:component id=\"1\" name=\"NewMad_ibverbs_lr2\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      static const char ib_lz4[] = 
	"<puk:composite id=\"nm:ib-lz4\">"
	"  <puk:component id=\"0\" name=\"NewMad_ibverbs_lz4\"/>"
	"  <puk:component id=\"1\" name=\"NewMad_ibverbs_lz4\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      static const char ib_rcache[] = 
	"<puk:composite id=\"nm:ib-rcache\">"
	"  <puk:component id=\"0\" name=\"NewMad_ibverbs_bycopy\"/>"
	"  <puk:component id=\"1\" name=\"NewMad_ibverbs_rcache\"/>"
	"  <puk:component id=\"2\" name=\"NewMad_Driver_minidriver\">"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk0\" provider-id=\"0\" />"
	"    <puk:uses iface=\"NewMad_minidriver\" port=\"trk1\" provider-id=\"1\" />"
	"  </puk:component>"
	"  <puk:entry-point iface=\"NewMad_Driver\" provider-id=\"2\" />"
	"</puk:composite>";
      static const char*ib_drv = NULL;
      if(ib_drv == NULL)
	{
	  if(getenv("NMAD_IBVERBS_RCACHE") != NULL)
	    {
	      ib_drv = ib_rcache;
	      NM_DISPF("# nmad ibverbs: rcache forced by environment.\n");
	    }
	  else if(getenv("NMAD_IBVERBS_LZ4") != NULL)
	    {
	      ib_drv = ib_lz4;
	      NM_DISPF("# nmad ibverbs: LZ4 forced by environment.\n");
	    }
	  else
	    {
	      ib_drv = ib_lr2;
	    }
	}
      component = puk_component_parse(ib_drv);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", ib_drv);
	}
    }
  else
    {
      char component_name[1024];
      snprintf(component_name, 1024, "NewMad_%s_%s", entity, name);
      component = puk_component_resolve(component_name);
      if(component == NULL)
	{
	  padico_fatal("nmad: failed to load component '%s'\n", component_name);
	}
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
#if defined(PM2_TOPOLOGY) && !defined(PIOMAN_ABT)
  if(topology == NULL)
    {
      tbx_topology_init(*argc, argv);
    }
#endif

  /* allocate core object and init lists */

  struct nm_core*p_core = malloc(sizeof(struct nm_core));
  memset(p_core, 0, sizeof(struct nm_core));

  nm_gate_list_init(&p_core->gate_list);
  nm_drv_list_init(&p_core->driver_list);
  p_core->nb_drivers = 0;

  /* Initialize "Lightning Fast" Packet Wrappers Manager */
  nm_pw_alloc_init(p_core);

  nm_core_monitor_vect_init(&p_core->monitors);

  nm_req_list_init(&p_core->unpacks);
  nm_req_list_init(&p_core->pending_packs);
  nm_unexpected_list_init(&p_core->unexpected);
  nm_core_pending_event_list_init(&p_core->pending_events);
  
  p_core->enable_schedopt = 1;
  p_core->strategy_component = NULL;

#ifdef PIOMAN
  pioman_init(argc, argv);
  nm_core_lock_init(p_core);
  nm_ltask_set_policy();
#else
  nm_pkt_wrap_list_init(&p_core->pending_recv_list);
  nm_pkt_wrap_list_init(&p_core->pending_send_list);
#endif /* PIOMAN */

#ifdef NMAD_PROFILE
  p_core->profiling.n_locks      = 0;
  p_core->profiling.n_schedule   = 0;
  p_core->profiling.n_packs      = 0;
  p_core->profiling.n_unpacks    = 0;
  p_core->profiling.n_unexpected = 0;
  p_core->profiling.n_pw_out     = 0;
  p_core->profiling.n_pw_in      = 0;
  p_core->profiling.n_try_and_commit = 0;
  p_core->profiling.n_strat_apply = 0;
#endif /* NMAD_PROFILE */
  
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
  p_core->strategy_component = strategy;
  NM_DISPF("# nmad: strategy = %s\n", strategy->name);
  return NM_ESUCCESS;
}

void nm_core_schedopt_disable(nm_core_t p_core)
{
  p_core->enable_schedopt = 0;
#ifndef PIOMAN
  /* flush pending recv requests posted by nm_drv_refill_recv() */
  while(!nm_pkt_wrap_list_empty(&p_core->pending_recv_list))
    {
      struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_pop_front(&p_core->pending_recv_list);
      if(p_pw->p_gdrv)
	{
	  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
	  assert(p_pw == p_gdrv->p_in_rq_array[NM_TRK_SMALL]);
	  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
	  (*r->driver->cancel_recv_iov)(r->_status, p_pw);
	  p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
	  p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	}
      else
	{
	  assert(p_pw->p_drv->p_in_rq == p_pw);
	  (*p_pw->p_drv->driver->cancel_recv_iov)(NULL, p_pw);
	  p_pw->p_drv->p_in_rq = NULL;
	}
    }
#else /* !PIOMAN */
  nm_drv_t p_drv;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      piom_ltask_cancel(&p_drv->p_ltask);
      if(p_drv->p_in_rq)
	{
	  struct nm_pkt_wrap_s*p_pw = p_drv->p_in_rq;
	  piom_ltask_cancel(&p_pw->ltask);
	  if(p_pw->p_drv->driver->cancel_recv_iov)
	    {
	      (*p_pw->p_drv->driver->cancel_recv_iov)(NULL, p_pw);
	      p_pw->p_drv->p_in_rq = NULL;
	    }
	}
    }
  nm_gate_t p_gate;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      nm_gdrv_vect_itor_t i;
      puk_vect_foreach(i, nm_gdrv, &p_gate->gdrv_array)
	{
	  struct nm_gate_drv*p_gdrv = *i;
	  struct nm_pkt_wrap_s*p_pw = p_gdrv->p_in_rq_array[NM_TRK_SMALL];
	  if(p_pw != NULL)
	    {
	      piom_ltask_cancel(&p_pw->ltask);
	      struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
	      if(r->driver->cancel_recv_iov)
		{
		  (*r->driver->cancel_recv_iov)(r->_status, p_pw);
		  p_gdrv->p_in_rq_array[NM_TRK_SMALL] = NULL;
		  p_gdrv->active_recv[NM_TRK_SMALL] = 0;
		}
	    }
	}
    }
#endif /* !PIOMAN */
}

/** Shutdown the core struct and the main scheduler.
 */
int nm_core_exit(nm_core_t p_core)
{
  nm_core_lock(p_core);

#ifdef NMAD_PROFILE
  fprintf(stderr, "# ## profiling stats___________\n");
  fprintf(stderr, "# ## n_lock           = %lld\n", p_core->profiling.n_locks);
  fprintf(stderr, "# ## n_schedule       = %lld\n", p_core->profiling.n_schedule);
  fprintf(stderr, "# ## n_packs          = %lld\n", p_core->profiling.n_packs);
  fprintf(stderr, "# ## n_unpacks        = %lld\n", p_core->profiling.n_unpacks);
  fprintf(stderr, "# ## n_unexpected     = %lld\n", p_core->profiling.n_unexpected);
  fprintf(stderr, "# ## n_try_and_commit = %lld\n", p_core->profiling.n_try_and_commit);
  fprintf(stderr, "# ## n_strat_apply    = %lld\n", p_core->profiling.n_strat_apply);
  fprintf(stderr, "# ## n_pw_in          = %lld\n", p_core->profiling.n_pw_in);
  fprintf(stderr, "# ## n_pw_out         = %lld\n", p_core->profiling.n_pw_out);
  
#endif /* NMAD_PROFILE */

  /* flush strategies */
  int strat_done = 0;
  while(!strat_done)
    {
      int todo = 0;
      nm_gate_t p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  todo += !nm_pkt_wrap_list_empty(&p_gate->out_list);
	}
      if(todo)
	nm_schedule(p_core);
      strat_done = !todo;
    }

  nm_core_driver_exit(p_core);

  /* purge receive requests not posted yet to the driver */
  nm_drv_t p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      struct nm_pkt_wrap_s*p_pw = nm_pw_post_lfqueue_dequeue(&p_drv->post_recv);
      while(p_pw != NULL)
	{
	  NM_TRACEF("extracting pw from post_recv_list\n");
	  nm_pw_ref_dec(p_pw);
	  p_pw = nm_pw_post_lfqueue_dequeue(&p_drv->post_recv);
	}
    }

#ifndef PIOMAN
  /* Sanity check- everything is supposed to be empty here */
  struct nm_pkt_wrap_s*p_pw, *p_pw2;
  puk_list_foreach_safe(p_pw, p_pw2, &p_core->pending_recv_list)
    {
      NM_WARN("purging pw from pending_recv_list\n");
      nm_pw_ref_dec(p_pw);
    }
#endif /* !PIOMAN */
  
  /* Remove any remaining unexpected chunk */
  nm_unexpected_clean(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_pw_alloc_exit();

  nm_core_monitor_vect_destroy(&p_core->monitors);

  nm_core_unlock(p_core);
  nm_core_lock_destroy(p_core);

  nm_ns_exit(p_core);
  TBX_FREE(p_core);

  return NM_ESUCCESS;
}

