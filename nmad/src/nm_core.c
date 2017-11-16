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

static inline void nm_core_pack_submissions_flush(struct nm_core*p_core)
{
  nm_core_lock_assert(p_core);
  /* flush submitted reqs */
  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_lfqueue_dequeue_single_reader(&p_core->pack_submissions);
  while(p_req_chunk != NULL)
    {
      struct nm_req_s*p_pack = p_req_chunk->p_req;
      assert(p_pack != NULL);
      if(p_pack->seq == NM_SEQ_NONE)
	{
	  /* first chunk in pack */
	  p_core->n_packs++;
	  p_pack->p_gtag = nm_gtag_get(&p_pack->p_gate->tags, p_pack->tag);
	  const nm_seq_t seq = nm_seq_next(p_pack->p_gtag->send_seq_number);
	  p_pack->p_gtag->send_seq_number = seq;
	  p_pack->seq = seq;
	}
      if(p_pack->flags & NM_REQ_FLAG_PACK_SYNCHRONOUS)
	{
	  nm_req_list_push_back(&p_pack->p_gtag->pending_packs, p_pack);
	}
      const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
      if(r->driver->pack_data != NULL)
	{
	  (*r->driver->pack_data)(r->_status, p_pack, p_req_chunk->chunk_len, p_req_chunk->chunk_offset);
	  nm_profile_inc(p_core->profiling.n_packs);
	  /* free req_chunk immediately */
	  nm_req_chunk_destroy(p_core, p_req_chunk);
	}
      else
	{
	  nm_req_chunk_list_push_back(&p_pack->p_gate->req_chunk_list, p_req_chunk);
	}
      nm_core_polling_level(p_core);
      p_req_chunk = nm_req_chunk_lfqueue_dequeue_single_reader(&p_core->pack_submissions);
    }
}

/** make progress on core pending operations.
 * all operations that need lock when multithreaded.
 */
void nm_core_progress(struct nm_core*p_core)
{
  nm_gate_t p_gate = NULL;
  nm_core_lock_assert(p_core);
  nm_profile_inc(p_core->profiling.n_strat_apply);
  nm_core_pack_submissions_flush(p_core);
  nm_core_pw_completions_flush(p_core);
  /* apply strategy on each gate */
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  /* schedule new requests on all gates */
	  nm_strat_try_and_commit(p_gate);
	  /* process postponed recv requests */
	  nm_strat_rdv_accept(p_gate);
	}
    }
}

void nm_core_flush(struct nm_core*p_core)
{
  nm_core_lock(p_core);
  nm_core_pack_submissions_flush(p_core);
  nm_core_pw_completions_flush(p_core);
  nm_core_unlock(p_core);
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

  nm_core_progress(p_core);

  /* poll pending recv requests */
  if(!nm_pkt_wrap_list_empty(&p_core->pending_recv_list))
    {
      NM_TRACEF("polling inbound requests");
      struct nm_pkt_wrap_s*p_pw, *p_pw2;
      puk_list_foreach_safe(nm_pkt_wrap, p_pw, p_pw2, &p_core->pending_recv_list)
	{
	  nm_pw_recv_progress(p_pw);
	}
    }
  /* poll pending out requests */
  if(!nm_pkt_wrap_list_empty(&p_core->pending_send_list))
    {
      NM_TRACEF("polling outbound requests");
      struct nm_pkt_wrap_s*p_pw, *p_pw2;
      puk_list_foreach_safe(nm_pkt_wrap, p_pw, p_pw2, &p_core->pending_send_list)
	{
	  nm_pw_poll_send(p_pw);
	}
    }
  /*
  nm_out_prefetch(p_core);
  */
#ifdef DEBUG
  scheduling_in_progress = 0;
#endif /* DEBUG */

#endif /* PIOMAN */
  nm_core_events_dispatch(p_core);
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

/** dispatch all ready events */
void nm_core_events_dispatch(struct nm_core*p_core)
{
  nm_core_nolock_assert(p_core);
  if(!nm_core_dispatching_event_lfqueue_empty(&p_core->dispatching_events))
    {
      struct nm_core_dispatching_event_s*p_dispatching_event =
	nm_core_dispatching_event_lfqueue_dequeue(&p_core->dispatching_events);
      while(p_dispatching_event != NULL)
	{
	  (*p_dispatching_event->p_monitor->p_notifier)(&p_dispatching_event->event, p_dispatching_event->p_monitor->ref);
	  nm_core_dispatching_event_free(&p_core->dispatching_event_allocator, p_dispatching_event);
	  p_dispatching_event =
	    nm_core_dispatching_event_lfqueue_dequeue(&p_core->dispatching_events);
	}
    }
}

static inline void nm_core_dispatching_event_enqueue(struct nm_core*p_core, const struct nm_core_event_s*const p_event,
                                                     struct nm_monitor_s*p_monitor)
{
  nm_core_lock_assert(p_core);
  struct nm_core_dispatching_event_s*p_dispatching_event =
    nm_core_dispatching_event_malloc(&p_core->dispatching_event_allocator);
  p_dispatching_event->event = *p_event;
  p_dispatching_event->p_monitor = p_monitor;
  int rc = 0;
  do
    {
      rc = nm_core_dispatching_event_lfqueue_enqueue_single_writer(&p_core->dispatching_events, p_dispatching_event);
      if(rc)
        {
          nm_profile_inc(p_core->profiling.n_event_queue_full);
          nm_core_unlock(p_core);
          nm_core_events_dispatch(p_core);
          nm_core_lock(p_core);
        }
    }
  while(rc);
}

/** dispatch a global event that matches the given monitor */
static void nm_core_event_notify(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_core_monitor_s*p_core_monitor)
{
  nm_core_lock_assert(p_core);
  assert(nm_core_event_matches(p_core_monitor, p_event));
  if(p_event->status & NM_STATUS_UNEXPECTED)
    {
      struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_event->p_gate->tags, p_event->tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(p_event->seq == next_seq)
	{
	  /* next packet in sequence; commit matching and enqueue for dispatch */
	  p_so_tag->recv_seq_number = next_seq;
          nm_core_dispatching_event_enqueue(p_core, p_event, &p_core_monitor->monitor);

        restart:
	  if(!nm_core_pending_event_list_empty(&p_so_tag->pending_events))
	    {
	      /* recover pending events */
	      struct nm_core_pending_event_s*p_pending_event = nm_core_pending_event_list_front(&p_so_tag->pending_events);
	      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
	      if(p_pending_event->event.seq == next_seq)
		{
		  assert(nm_core_event_matches(p_pending_event->p_core_monitor, &p_pending_event->event));
		  assert(p_pending_event->event.status & NM_STATUS_UNEXPECTED);
		  p_so_tag->recv_seq_number = next_seq;
		  nm_core_pending_event_list_pop_front(&p_so_tag->pending_events);
                  nm_core_dispatching_event_enqueue(p_core, &p_pending_event->event,
                                                    &p_pending_event->p_core_monitor->monitor);
		  nm_core_pending_event_delete(p_pending_event);
		  goto restart; /* retry to find another matching event */
		}
	    }
	}
      else
	{
	  /* out-of-order event; store as pending */
	  nm_profile_inc(p_core->profiling.n_outoforder_event);
	  struct nm_core_pending_event_s*p_pending_event = nm_core_pending_event_new();
	  p_pending_event->event = *p_event;
	  p_pending_event->p_core_monitor = p_core_monitor;
	  if(nm_core_pending_event_list_empty(&p_so_tag->pending_events))
	    {
	      nm_core_pending_event_list_push_back(&p_so_tag->pending_events, p_pending_event);
	    }
	  else
	    {
	      /* assume an out-of-order event comes last */
	      nm_core_pending_event_itor_t i = nm_core_pending_event_list_rbegin(&p_so_tag->pending_events);
	      while(i && (nm_seq_compare(p_so_tag->recv_seq_number, i->event.seq, p_pending_event->event.seq) > 0))
		{
		  i = nm_core_pending_event_list_rnext(i);
		}
	      if(i)
		{
		  nm_core_pending_event_list_insert_after(&p_so_tag->pending_events, i, p_pending_event);
		}
	      else
		{
		  nm_core_pending_event_list_push_front(&p_so_tag->pending_events, p_pending_event);
		}
	    }
	  assert(nm_core_pending_event_list_size(&p_so_tag->pending_events) < NM_SEQ_MAX - 1); /* seq number overflow */
	}
    }
}

/** Add an event monitor to the list */
void nm_core_monitor_add(nm_core_t p_core, struct nm_core_monitor_s*p_core_monitor)
{
  nm_core_lock(p_core);
  if(p_core_monitor->monitor.event_mask == NM_STATUS_UNEXPECTED)
    {
      struct nm_unexpected_s*p_chunk = NULL, *p_tmp;
      puk_list_foreach_safe(nm_unexpected_core, p_chunk, p_tmp, &p_core->unexpected) /* use 'safe' iterator since notifier is likely to post a recv */
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
      if(p_req->monitor.p_notifier)
	{
	  /* when a monitor is registered, add bits (no signal)
	   * and notify only if bitmask matches */
	  nm_status_add(p_req, p_event->status);
	  if(p_event->status & p_req->monitor.event_mask)
	    {
	      nm_core_unlock(p_core);
	      (*p_req->monitor.p_notifier)(p_event, p_req->monitor.ref);
	      nm_core_lock(p_core);
	    }
	}
      else
	{
	  /* no monitor: full signal */
	  nm_status_signal(p_req, p_event->status);
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
  char component_name[1024];
  snprintf(component_name, 1024, "NewMad_%s_%s", entity, name);
  component = puk_component_resolve(component_name);
  if(component == NULL)
    {
      NM_FATAL("failed to load component '%s'\n", component_name);
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
  nm_pw_nohd_allocator_init(&p_core->pw_nohd_allocator, INITIAL_PKT_NUM);
  nm_pw_buf_allocator_init(&p_core->pw_buf_allocator, INITIAL_PKT_NUM);

  nm_core_monitor_vect_init(&p_core->monitors);

  nm_req_list_init(&p_core->wildcard_unpacks);
  nm_unexpected_core_list_init(&p_core->unexpected);
  p_core->n_packs = 0;
  p_core->n_unpacks = 0;

  nm_req_chunk_lfqueue_init(&p_core->pack_submissions);
  nm_req_chunk_allocator_init(&p_core->req_chunk_allocator, NM_REQ_CHUNK_QUEUE_SIZE);
  nm_ctrl_chunk_allocator_init(&p_core->ctrl_chunk_allocator, NM_REQ_CHUNK_QUEUE_SIZE);

  nm_pkt_wrap_lfqueue_init(&p_core->completed_pws);

  nm_core_dispatching_event_allocator_init(&p_core->dispatching_event_allocator, 16);
  nm_core_dispatching_event_lfqueue_init(&p_core->dispatching_events);

  p_core->trk_table = puk_hashtable_new_ptr();

  p_core->enable_schedopt = 1;
  p_core->strategy_component = NULL;
  if(getenv("NMAD_AUTO_FLUSH"))
    {
      p_core->enable_auto_flush = 1;
      NM_DISPF("auto-flush on send enabled.\n");
    }
  else
    {
      p_core->enable_auto_flush = 0;
    }
  p_core->enable_isend_csum = 0;
  if(getenv("NMAD_ISEND_CHECK"))
    {
#ifdef DEBUG
      p_core->enable_isend_csum = 1;
      NM_DISPF("isend integrity check enabled.\n");
#else
      NM_WARN("isend integrity check available only in DEBUG mode.\n");
#endif
    }

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
  p_core->profiling.n_outoforder_event = 0;
  p_core->profiling.n_event_queue_full = 0;
#endif /* NMAD_PROFILE */

#ifdef PIOMAN
  pioman_init(argc, argv);
  nm_core_lock_init(p_core);
  nm_ltask_set_policy();
  nm_ltask_submit_core_progress(p_core);
#else
  nm_pkt_wrap_list_init(&p_core->pending_recv_list);
  nm_pkt_wrap_list_init(&p_core->pending_send_list);
#endif /* PIOMAN */

  *pp_core = p_core;

  return err;
}

int nm_core_set_strategy(nm_core_t p_core, puk_component_t strategy)
{
  puk_facet_t strat_facet = puk_component_get_facet_NewMad_Strategy(strategy, NULL);
  if(strat_facet == NULL)
    {
      NM_FATAL("component %s given as strategy has no interface 'NewMad_Strategy'\n", strategy->name);
    }
  p_core->strategy_component = strategy;
  NM_DISPF("loaded strategy '%s'\n", strategy->name);
  return NM_ESUCCESS;
}

void nm_core_schedopt_disable(nm_core_t p_core)
{
  nm_core_lock(p_core);
  p_core->enable_schedopt = 0;
  nm_core_driver_flush(p_core);
  nm_core_unlock(p_core);
}

/** Shutdown the core struct and the main scheduler.
 */
int nm_core_exit(nm_core_t p_core)
{
#ifdef PIOMAN
  /* stop posting new pw */
  piom_ltask_cancel(&p_core->ltask);
#endif
  nm_core_lock(p_core);

#warning TODO- flush dispatching events before shutdown

#ifdef NMAD_PROFILE
  fprintf(stderr, "# ## profiling stats___________\n");
  fprintf(stderr, "# ## n_lock             = %lld\n", p_core->profiling.n_locks);
  fprintf(stderr, "# ## n_schedule         = %lld\n", p_core->profiling.n_schedule);
  fprintf(stderr, "# ## n_packs            = %lld\n", p_core->profiling.n_packs);
  fprintf(stderr, "# ## n_unpacks          = %lld\n", p_core->profiling.n_unpacks);
  fprintf(stderr, "# ## n_unexpected       = %lld\n", p_core->profiling.n_unexpected);
  fprintf(stderr, "# ## n_try_and_commit   = %lld\n", p_core->profiling.n_try_and_commit);
  fprintf(stderr, "# ## n_strat_apply      = %lld\n", p_core->profiling.n_strat_apply);
  fprintf(stderr, "# ## n_pw_in            = %lld\n", p_core->profiling.n_pw_in);
  fprintf(stderr, "# ## n_pw_out           = %lld\n", p_core->profiling.n_pw_out);
  fprintf(stderr, "# ## n_outoforder_event = %lld\n", p_core->profiling.n_outoforder_event);
  fprintf(stderr, "# ## n_event_queue_full = %lld\n", p_core->profiling.n_event_queue_full);

#endif /* NMAD_PROFILE */

  /* flush pending requests */
  int flush_done = 0;
  while(!flush_done)
    {
      int todo = !nm_req_chunk_lfqueue_empty(&p_core->pack_submissions);
      nm_gate_t p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  todo += !nm_req_chunk_list_empty(&p_gate->req_chunk_list);
	  todo += !nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list);
	}
      if(todo)
	nm_schedule(p_core);
      flush_done = !todo;
    }

  nm_core_driver_flush(p_core);
  nm_core_driver_exit(p_core);

#if(defined(PIOMAN))
  pioman_exit();
#endif

#ifndef PIOMAN
  /* Sanity check- everything is supposed to be empty here */
  struct nm_pkt_wrap_s*p_pw, *p_pw2;
  puk_list_foreach_safe(nm_pkt_wrap, p_pw, p_pw2, &p_core->pending_recv_list)
    {
      NM_WARN("purging pw from pending_recv_list\n");
      nm_pw_ref_dec(p_pw);
    }
#endif /* !PIOMAN */

  /* Remove any remaining unexpected chunk */
  nm_unexpected_clean(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_pw_nohd_allocator_destroy(&p_core->pw_nohd_allocator);
  nm_pw_buf_allocator_destroy(&p_core->pw_buf_allocator);
  nm_req_chunk_allocator_destroy(&p_core->req_chunk_allocator);
  nm_ctrl_chunk_allocator_destroy(&p_core->ctrl_chunk_allocator);
  nm_core_dispatching_event_allocator_destroy(&p_core->dispatching_event_allocator);
  nm_core_monitor_vect_destroy(&p_core->monitors);

  nm_core_unlock(p_core);
  nm_core_lock_destroy(p_core);

  nm_ns_exit(p_core);
  free(p_core);

  return NM_ESUCCESS;
}
