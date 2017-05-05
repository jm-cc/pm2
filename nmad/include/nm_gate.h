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

#ifndef NM_GATE_H
#define NM_GATE_H

/** status of tags on each gate
 */
struct nm_gtag_s
{
  nm_seq_t recv_seq_number; /**< next sequence number for recv */
  nm_seq_t send_seq_number; /**< next sequence number for send */
  struct nm_core_pending_event_list_s pending_events; /**< events received out-of-order, waiting for dispatch */
};
static inline void nm_gtag_ctor(struct nm_gtag_s*p_so_tag, nm_core_tag_t tag)
{
  p_so_tag->recv_seq_number = NM_SEQ_FIRST;
  p_so_tag->send_seq_number = NM_SEQ_FIRST;
  nm_core_pending_event_list_init(&p_so_tag->pending_events);
}
static inline void nm_gtag_dtor(struct nm_gtag_s*p_so_tag)
{
  assert(nm_core_pending_event_list_empty(&p_so_tag->pending_events));
}

NM_TAG_TABLE_TYPE(nm_gtag, struct nm_gtag_s);


/** Per driver gate related data. */
struct nm_gate_drv
{
  nm_drv_t p_drv;                                   /**< driver reference */
  struct puk_receptacle_NewMad_Driver_s receptacle; /**< receptacle for the driver */
  puk_instance_t instance;                          /**< driver instance */
  /** Array of reference to current incoming requests, (indexed by trk_id). */
  struct nm_pkt_wrap_s*p_in_rq_array[NM_SO_MAX_TRACKS];
  int active_recv[NM_SO_MAX_TRACKS];                /**< whether a recv is active on the given trk */
  int active_send[NM_SO_MAX_TRACKS];                /**< whether a send is active on the given trk */
};

PUK_VECT_TYPE(nm_gdrv, struct nm_gate_drv*);

/** status of a gate, used for dynamic connections */
enum nm_gate_status_e
  {
    NM_GATE_STATUS_INIT,          /**< gate created, not connected */
    NM_GATE_STATUS_CONNECTING,    /**< connection establishment is in progress */
    NM_GATE_STATUS_CONNECTED,     /**< gate actually connected, may be used/polled */
    NM_GATE_STATUS_DISCONNECTING, /**< gate will be disconnected, do not post new request */
    NM_GATE_STATUS_DISCONNECTED   /**< gate has been disconnected, do not use */
  };
typedef enum nm_gate_status_e nm_gate_status_t;


/** Connection to another process.
 */
struct nm_gate_s
{
  /** link to store gates in the core gate_list */
  PUK_LIST_LINK(nm_gate);

  /** current status of the gate (connected / not connected) */
  nm_gate_status_t status;

  /** table of tag status */
  struct nm_gtag_table_s tags;

  /** large messages waiting for Track 1 (or 2) to be free- list of pw */
  struct nm_pkt_wrap_list_s pending_large_recv;
  /** large messages waiting for RTRs- list of pw, lookup by [gate,tag,seq,chunk_offset] */
  struct nm_pkt_wrap_list_s pending_large_send;       

  /** Strategy components elements */
  struct puk_receptacle_NewMad_Strategy_s strategy_receptacle;
  puk_instance_t strategy_instance;

  /** submission lock-free list of reqs */
  struct nm_req_chunk_lfqueue_s req_chunk_submission;
  
  /** send reqs posted to the gate */
  struct nm_req_chunk_list_s req_chunk_list;
  
  /** pw ready to be sent, formed by strategy */
  struct nm_pkt_wrap_list_s out_list;

  /** Gate data for each driver. */
  struct nm_gdrv_vect_s gdrv_array;

  /** NM core object. */
  struct nm_core *p_core;

#ifdef NMAD_TRACE
  int trace_connections_id;
#endif /* NMAD_TRACE */
};

PUK_LIST_DECLARE_TYPE(nm_gate);
PUK_LIST_CREATE_FUNCS(nm_gate);

#define NM_FOR_EACH_GATE(P_GATE, P_CORE) \
  puk_list_foreach(P_GATE, &(P_CORE)->gate_list)

#endif /* NM_GATE_H */
