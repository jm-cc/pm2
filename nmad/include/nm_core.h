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


/** Core NewMadeleine structure.
 */
struct nm_core
{
#ifdef PIOMAN
  piom_spinlock_t lock;                         /**< lock to protect req lists in core */
  struct piom_ltask ltask;                      /**< task used for main core progress */
#else
  int lock;
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
 
  struct nm_req_chunk_lfqueue_s pack_submissions;  /**< list of new pack reqs (lock-free submission list) */
  nm_req_chunk_allocator_t req_chunk_allocator;    /**< allocator for req_chunk elements */
  nm_ctrl_chunk_allocator_t ctrl_chunk_allocator;  /**< allocator for control chunks */
  uint64_t unpack_seq;                             /**< next sequence number for unpacks */
  struct nm_req_list_s unpacks;                    /**< list of wildcards unpacks; non-wildcard unpacks are in nm_gtag_s */
  struct nm_unexpected_core_list_s unexpected;     /**< global list of unexpected chunks */
  struct nm_req_list_s pending_packs;              /**< list of pack reqs in progress (or waiting for ACK) */
  struct nm_pkt_wrap_lfqueue_s completed_pws;      /**< queue of completed pw waiting for event dispatch */
  struct nm_core_monitor_vect_s monitors;          /**< monitors for upper layers to track events in nmad core */
  
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
