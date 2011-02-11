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

#ifndef NM_GATE_H
#define NM_GATE_H


/** tag-indexed type for known-src requests 
 */
struct nm_so_tag_s
{
  nm_seq_t recv_seq_number; /**< next sequence number for recv */
  nm_seq_t send_seq_number; /**< next sequence number for send */
};
static inline void nm_so_tag_ctor(struct nm_so_tag_s*so_tag, nm_core_tag_t tag)
{
  so_tag->recv_seq_number = NM_SEQ_FIRST;
  so_tag->send_seq_number = NM_SEQ_FIRST;
}
static inline void nm_so_tag_dtor(struct nm_so_tag_s*so_tag)
{
}

NM_TAG_TABLE_TYPE(nm_so_tag, struct nm_so_tag_s);


/** Per driver gate related data. */
struct nm_gate_drv
{
  /** NewMad driver reference . */
  struct nm_drv *p_drv;
  /** Receptacle for the current driver interface. */
  struct puk_receptacle_NewMad_Driver_s receptacle;
  /** Driver instance. */
  puk_instance_t instance;
  
  /** Array of reference to current incoming requests, (indexed by trk_id). */
  struct nm_pkt_wrap *p_in_rq_array[NM_SO_MAX_TRACKS];
  
  tbx_bool_t active_recv[NM_SO_MAX_TRACKS];
  tbx_bool_t active_send[NM_SO_MAX_TRACKS];
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
struct nm_gate
{
  /** link to store gates in the core gate_list */
  struct tbx_fast_list_head _link;

  /** current status of the gate (connected / not connected) */
  nm_gate_status_t status;

  /** table of tag status */
  struct nm_so_tag_table_s tags;

  /** large messages waiting for Track 1 (or 2) to be free- list of pw */
  struct tbx_fast_list_head pending_large_recv;
  /** large messages waiting for RTRs- list of pw, lookup by [gate,tag,seq,chunk_offset] */
  struct tbx_fast_list_head pending_large_send;       

  /* Strategy components elements */
  struct puk_receptacle_NewMad_Strategy_s strategy_receptacle;
  puk_instance_t strategy_instance;

  /** Gate data for each driver. */
  struct nm_gdrv_vect_s gdrv_array;

  /* user reference */
  void*ref;

  /** NM core object. */
  struct nm_core *p_core;
  
};


#define NM_FOR_EACH_GATE(P_GATE, P_CORE) \
  tbx_fast_list_for_each_entry(P_GATE, &(P_CORE)->gate_list, _link)

#endif /* NM_GATE_H */
