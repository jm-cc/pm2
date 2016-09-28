/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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


void nm_drv_post_all(nm_drv_t p_drv)
{ 
 /* schedule & post out requests */
  nm_drv_post_send(p_drv);
  
  /* post new receive requests */
  if(p_drv->p_core->enable_schedopt)
    nm_drv_refill_recv(p_drv);
  nm_drv_post_recv(p_drv);
}

/** apply strategy
 */
void nm_strat_apply(struct nm_core*p_core)
{
  nm_gate_t p_gate = NULL;
  nmad_lock_assert();
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
  if(!nm_core_event_queue_empty(&p_core->pending_events))
    {
      /* schedule front event */
      const struct nm_core_event_s event = nm_core_event_queue_retrieve(&p_core->pending_events); 
      fprintf(stderr, "# nmad: WARNING- invoking pending event... seq = %d; pending count = %d\n",
	      event.seq, nm_core_event_queue_size(&p_core->pending_events));
      assert(nm_core_event_queue_size(&p_core->pending_events) < 16);
      nm_core_status_event(p_core, &event, NULL);
    }
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
  
  nmad_unlock();
  
#ifdef DEBUG
  scheduling_in_progress = 0;
#endif /* DEBUG */

#else  /* NMAD_POLL */
  piom_polling_force();
#endif /* NMAD_POLL */
  return NM_ESUCCESS;
}

void nm_core_req_monitor(struct nm_req_s*p_req, struct nm_monitor_s monitor)
{
  nmad_lock_assert();
  assert(p_req->monitor.notifier == NULL);
  p_req->monitor = monitor;
  if(nm_status_test(p_req, monitor.mask))
    {
      /* immediate event */
      const struct nm_core_event_s event =
	{
	  .status = nm_status_test(p_req, monitor.mask),
	  .p_req  = p_req
	};
      (*p_req->monitor.notifier)(&event, p_req->monitor.ref);
    }
}

/** dispatch global events */
static inline void nm_core_event_notify(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_core_monitor_s*p_core_monitor)
{
  int locked = 0;
  if(nm_event_matches(p_core_monitor, p_event))
    {
      if(p_event->status & NM_STATUS_UNEXPECTED)
	{
	  locked = __sync_bool_compare_and_swap(&p_core_monitor->dispatching, 0, 1);
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
		  fprintf(stderr, "# nmad: WARNING- delaying event dispatch; got seq = %d; expected = %d; tag = %lx:%lx\n",
			  p_event->seq, next_seq, nm_core_tag_get_hashcode(p_event->tag), nm_core_tag_get_tag(p_event->tag));
		  nm_core_event_queue_append(&p_core->pending_events, *p_event);
		  sleep(1);
		  goto out;
		}
	    }
	  else
	    {
	      fprintf(stderr, "# nmad: WARNING- delaying event dispatch; already in progress\n");
	      nm_core_event_queue_append(&p_core->pending_events, *p_event);
	      sleep(1);
	      goto out;
	    }
	}	      
      (p_core_monitor->monitor.notifier)(p_event, p_core_monitor->monitor.ref);
    }
 out:
  if(locked)
    {
      __sync_fetch_and_sub(&p_core_monitor->dispatching, 1);
    }
}

/** Add an event monitor to the list */
void nm_core_monitor_add(nm_core_t p_core, struct nm_core_monitor_s*p_core_monitor)
{
  nmad_lock_assert();
  p_core_monitor->dispatching = 0;
  if(p_core_monitor->monitor.mask == NM_STATUS_UNEXPECTED)
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
	      nm_core_event_notify(p_core, &event, p_core_monitor);
	    }
	}
    }
  nm_core_monitor_vect_push_back(&p_core->monitors, p_core_monitor);
}

void nm_core_monitor_remove(nm_core_t p_core, struct nm_core_monitor_s*m)
{
  nmad_lock_assert();
  nm_core_monitor_vect_itor_t i = nm_core_monitor_vect_find(&p_core->monitors, m);
  if(i)
    {
      nm_core_monitor_vect_erase(&p_core->monitors, i);
    }
}

/** Fires an event
 */
void nm_core_status_event(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_req_s*p_req)
{
  if(p_req)
    {
      if(p_event->status & NM_STATUS_FINALIZED)
	{
	  /* signal event if finalized */
	  if(p_req->monitor.mask & NM_STATUS_FINALIZED)
	    {
	      /* a monitor is listening to FINALIZED events- signal without FINALIZED, then notify monitor */
	      nm_status_signal(p_req, p_event->status & ~NM_STATUS_FINALIZED);
	      (*p_req->monitor.notifier)(p_event, p_req->monitor.ref);
	    }
	  else
	    {
	      /* no monitor on FINALIZED- add bitmask, notify, then signal */
	      nm_status_add(p_req, p_event->status & ~NM_STATUS_FINALIZED);
	      if(p_req->monitor.mask & p_event->status)
		{
		  (*p_req->monitor.notifier)(p_event, p_req->monitor.ref);
		}
	      nm_status_signal(p_req, p_event->status);
	    }
	}
      else
	{
	  /* not finalized, only add bitmask */
	  nm_status_add(p_req, p_event->status);
	  if(p_req->monitor.mask & p_event->status)
	    {
	      (*p_req->monitor.notifier)(p_event, p_req->monitor.ref);
	    }
	}
    }
  else
    {
      /* fire global monitors */
      nm_core_monitor_vect_itor_t i;
      puk_vect_foreach(i, nm_core_monitor, &p_core->monitors)
	{
	  nm_core_event_notify(p_core, p_event, *i);
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
	"<puk:composite id=\"nm:shm\">"
	"  <puk:component id=\"0\" name=\"Minidriver_shm\"/>"
	"  <puk:component id=\"1\" name=\"Minidriver_CMA\"/>"
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

  struct nm_core *p_core = TBX_MALLOC(sizeof(struct nm_core));
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
  nm_core_event_queue_init(&p_core->pending_events, 8);
  
#ifdef NMAD_POLL
  nm_pkt_wrap_list_init(&p_core->pending_recv_list);
  nm_pkt_wrap_list_init(&p_core->pending_send_list);
#endif /* NMAD_POLL*/
  p_core->enable_schedopt = 1;

  p_core->strategy_component = NULL;

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
  p_core->strategy_component = strategy;
  NM_DISPF("# nmad: strategy = %s\n", strategy->name);
  return NM_ESUCCESS;
}

void nm_core_schedopt_disable(nm_core_t p_core)
{
  p_core->enable_schedopt = 0;
#ifdef NMAD_POLL
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
#else /* NMAD_POLL */
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
#endif /* NMAD_POLL */
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
      nm_gate_t p_gate = NULL;
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

#ifdef NMAD_POLL
  /* Sanity check- everything is supposed to be empty here */
  struct nm_pkt_wrap_s*p_pw, *p_pw2;
  puk_list_foreach_safe(p_pw, p_pw2, &p_core->pending_recv_list)
    {
      NM_WARN("purging pw from pending_recv_list\n");
      nm_pw_ref_dec(p_pw);
    }
#endif /* NMAD_POLL */
  
  /* Remove any remaining unexpected chunk */
  nm_unexpected_clean(p_core);

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_pw_alloc_exit();

  nm_core_monitor_vect_destroy(&p_core->monitors);

  nmad_unlock();


  nm_ns_exit(p_core);
  TBX_FREE(p_core);

  return NM_ESUCCESS;
}

