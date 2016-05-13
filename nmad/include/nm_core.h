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


#ifndef NM_CORE_H
#define NM_CORE_H

PUK_VECT_TYPE(nm_core_monitor, const struct nm_core_monitor_s*);

/** a chunk of unexpected message to be stored */
struct nm_unexpected_s
{
  PUK_LIST_LINK(nm_unexpected);
  void*header;
  struct nm_pkt_wrap*p_pw;
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
  /** List of gates. */
  struct tbx_fast_list_head gate_list;

  /** List of drivers. */
  struct nm_drv_list_s driver_list;

  /** Number of drivers currently loaded. */
  int nb_drivers;

#ifdef NMAD_POLL
  /* if PIOMan is enabled, it already manages a 
     list of requests to poll */

  /** Outgoing active pw. */
  struct tbx_fast_list_head pending_send_list;

  /** recv pw */
  struct tbx_fast_list_head pending_recv_list;
#endif /* NMAD_POLL */

  /** whether schedopt is enabled atop drivers */
  int enable_schedopt;
 
  struct nm_req_list_s unpacks;            /**< List of posted unpacks */
  struct nm_unexpected_list_s unexpected;  /**< List of unexpected chunks */
  struct nm_req_list_s pending_packs;      /**< List of pending packs waiting for an ack */
  
  /** Monitors for upper layers to track events in nmad core */
  struct nm_core_monitor_vect_s monitors;
  
  /** Selected strategy */
  puk_component_t strategy_component;
  
};

#endif /* NM_CORE_H */
