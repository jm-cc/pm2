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


#ifndef NM_CORE_H
#define NM_CORE_H

#ifndef NM_PRIVATE_H
#  error "Cannot include this file directly. Please include <nm_private.h>"
#endif

PUK_VECT_TYPE(nm_core_monitor, struct nm_core_monitor_s*);

/** a pending event, not dispatched immediately because it was received out of order */
PUK_LIST_TYPE(nm_core_pending_event,
	      struct nm_core_event_s event;             /**< the event to dispatch */
	      struct nm_core_monitor_s*p_core_monitor;  /**< the monitor to fire (event matching already checked) */
	      );

/** an event ready for dispatch (matching already done) */
struct nm_core_dispatching_event_s
{
  struct nm_core_event_s event;
  struct nm_monitor_s*p_monitor;
};
PUK_LFQUEUE_TYPE(nm_core_dispatching_event, struct nm_core_dispatching_event_s*, NULL, 1024);
PUK_ALLOCATOR_TYPE(nm_core_dispatching_event, struct nm_core_dispatching_event_s);

/** a chunk of unexpected message to be stored */
struct nm_unexpected_s
{
  PUK_LIST_LINK(nm_unexpected);
  const struct nm_header_generic_s*p_header;
  struct nm_pkt_wrap_s*p_pw;
  nm_gate_t p_gate;
  nm_seq_t seq;
  nm_core_tag_t tag;
  nm_len_t msg_len; /**< length of full message on last chunk, NM_LEN_UNDEFINED if not last chunk */
};

PUK_LIST_DECLARE_TYPE(nm_unexpected);
PUK_LIST_CREATE_FUNCS(nm_unexpected);

/** Core NewMadeleine structure.
 */
struct nm_core
{
#ifdef PIOMAN
  piom_spinlock_t lock;                         /**< lock to protect req lists in core */
  struct piom_ltask ltask;                      /**< task used for main core progress */
#endif  /* PIOMAN */
  
  puk_component_t strategy_component;           /**< selected strategy */
  int enable_schedopt;                          /**< whether schedopt is enabled atop drivers */

  struct nm_gate_list_s gate_list;              /**< list of gates. */
  struct nm_drv_list_s driver_list;             /**< list of drivers. */
  int nb_drivers;                               /**< number of drivers in drivers list */

#ifndef PIOMAN
  /* if PIOMan is enabled, it already manages a list of pw to poll */
  struct nm_pkt_wrap_list_s pending_send_list;  /**< active pw for send to poll */
  struct nm_pkt_wrap_list_s pending_recv_list;  /**< active pw for recv to poll */
#endif /* !PIOMAN */
 
  struct nm_req_list_s unpacks;                 /**< list of posted unpacks */
  struct nm_unexpected_list_s unexpected;       /**< list of unexpected chunks */
  struct nm_req_list_s pending_packs;           /**< list of pack reqs in progress (or waiting for ACK) */
  struct nm_core_pending_event_list_s pending_events; /**< pending events not ready to dispatch */
  struct nm_core_monitor_vect_s monitors;       /**< monitors for upper layers to track events in nmad core */

  nm_core_dispatching_event_allocator_t dispatching_event_allocator;
  struct nm_core_dispatching_event_lfqueue_s dispatching_events;
#ifdef NMAD_PROFILE
  struct
  {
    long long n_locks;
    long long n_schedule;
    long long n_packs;
    long long n_unpacks;
    long long n_unexpected;
    long long n_pw_out;
    long long n_pw_in;
    long long n_try_and_commit;
    long long n_strat_apply;
    long long n_outoforder_event;
    long long n_event_queue_full;
  } profiling;
#endif /* NMAD_PROFILE */
};

#endif /* NM_CORE_H */
