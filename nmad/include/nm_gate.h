/*
 * NewMadeleine
 * Copyright (C) 2006-2018 (see AUTHORS file)
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
  nm_seq_t recv_seq_number;     /**< next sequence number for recv */
  nm_seq_t send_seq_number;     /**< next sequence number for send */
  struct nm_req_list_s unpacks; /**< posted unpacks on this gate/tag */
  struct nm_req_list_s pending_packs;                 /**< pack reqs waiting for ACK */
  struct nm_unexpected_tag_list_s unexpected;         /**< unexpected chunks pending on this gate/tag; sorted by seq number */
  struct nm_core_pending_event_list_s pending_events; /**< events received out-of-order, waiting for dispatch */
};
static inline void nm_gtag_ctor(struct nm_gtag_s*p_so_tag, nm_core_tag_t tag)
{
  p_so_tag->recv_seq_number = NM_SEQ_FIRST;
  p_so_tag->send_seq_number = NM_SEQ_FIRST;
  nm_req_list_init(&p_so_tag->unpacks);
  nm_req_list_init(&p_so_tag->pending_packs);
  nm_unexpected_tag_list_init(&p_so_tag->unexpected);
  nm_core_pending_event_list_init(&p_so_tag->pending_events);
}
static inline void nm_gtag_dtor(struct nm_gtag_s*p_so_tag)
{
  assert(nm_core_pending_event_list_empty(&p_so_tag->pending_events));
}

NM_TAG_TABLE_TYPE(nm_gtag, struct nm_gtag_s);

/** a track on a given gate */
struct nm_trk_s
{
  struct nm_gate_s*p_gate;                          /**< gate this trk belong to */
  struct nm_drv_s*p_drv;                            /**< driver attached to the track */
  struct puk_receptacle_NewMad_minidriver_s receptacle; /**< receptacle for the driver */
  puk_instance_t instance;                          /**< driver instance */
  struct nm_pkt_wrap_s*p_pw_recv;                   /**< the active pw for recv on the given trk */
  struct nm_pkt_wrap_s*p_pw_send;                   /**< the active pw for send on the given trk */
  struct nm_pkt_wrap_list_s pending_pw_send;        /**< pw ready to post, waiting for track to be free */
  struct nm_data_s sdata, rdata;                    /**< nm_data for above pw, in case in needs to be flatten on the fly */
  enum nm_trk_kind_e
    {
      nm_trk_undefined,
      nm_trk_small, /**< small packets with headers & parsing */
      nm_trk_large  /**< large packets with rdv, no header */
    } kind;
};

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


PUK_LIST_DECLARE_TYPE(nm_gate);
PUK_LIST_DECLARE_TYPE2(nm_active_gate, struct nm_gate_s);

/** Connection to another process.
 */
struct nm_gate_s
{
  /** link to store gates in the core gate_list */
  PUK_LIST_LINK(nm_gate);
  PUK_LIST_LINK(nm_active_gate);
  
  /** current status of the gate (connected / not connected) */
  nm_gate_status_t status;

  /** Number of tracks opened on this driver. */
  int n_trks;

  /** Tracks opened for each driver. */
  struct nm_trk_s*trks;

  /** table of tag status */
  struct nm_gtag_table_s tags;

  /** large messages waiting for Track 1 (or 2) to be free- list of pw */
  struct nm_pkt_wrap_list_s pending_large_recv;
  /** large messages waiting for RTRs- list of pw, lookup by [gate,tag,seq,chunk_offset] */
  struct nm_pkt_wrap_list_s pending_large_send;

  /** Strategy components elements */
  struct puk_receptacle_NewMad_Strategy_s strategy_receptacle;
  puk_instance_t strategy_instance;
  int strat_todo; /**< strategy has work to do*/

  /** send reqs posted to the gate */
  struct nm_req_chunk_list_s req_chunk_list;

  /** control chunks posted to the gate */
  struct nm_ctrl_chunk_list_s ctrl_chunk_list;

  /** NM core object. */
  struct nm_core*p_core;
};

PUK_LIST_CREATE_FUNCS(nm_gate);
PUK_LIST_CREATE_FUNCS(nm_active_gate);

#define NM_FOR_EACH_GATE(P_GATE, P_CORE)                        \
  puk_list_foreach(nm_gate, P_GATE, &(P_CORE)->gate_list)

#endif /* NM_GATE_H */
