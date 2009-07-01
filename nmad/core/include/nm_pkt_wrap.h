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

#ifndef NM_PKT_WRAP_H
#define NM_PKT_WRAP_H

#include <sys/uio.h>

#include <ccs_public.h>


/** Internal packet wrapper.
 */
struct nm_pkt_wrap
{
#ifdef PIOMAN
  struct piom_req inst;
  enum {
    RECV,
    SEND,
    NONE,
  } which;
#ifdef PIO_OFFLOAD
  tbx_bool_t data_to_offload;
#endif /* PIO_OFFLOAD */
#endif /* PIOMAN */
  
  
  /* Scheduler fields.
   */
  
  /** Assignated driver.  */
  struct nm_drv	*p_drv;
  
  /** Assignated track ID.  */
  nm_trk_id_t trk_id;

  /** Assignated gate, if relevant. */
  struct nm_gate *p_gate;
  
  /** Assignated gate driver, if relevant. */
  struct nm_gate_drv *p_gdrv;

  /** Protocol id.
      - communication flow this packet belongs to
      - 0 means any
  */
  nm_tag_t proto_id;
  
  /** Sequence number for the given protocol id.
      - rank in the communication flow this packet belongs to
  */
  uint8_t seq;
  
  /** Driver implementation data.  */
  void			*drv_priv;
  
  
  /* Packet related fields.					*/
  
  /** Packet internal flags. */
  uint32_t		 pkt_priv_flags;
  
  /** Cumulated amount of data (everything included) referenced by this wrap. */
  uint32_t		 length;

  /** actual number of allocated entries in the iovec. */
  uint32_t		 v_size;
  
  /** Number of *used* entries in io vector.
      - first unused entry after iov  contents is v[v_nb]
  */
  uint32_t		 v_nb;
  
  /** IO vector. */
  struct iovec		*v;
  
  /** number of references pointing to the header (when storing unexpected packets) */
  int header_ref_count;

  /** number of pending 'skip' fields in data header to update before sending */
  int pending_skips;

  /* Used on the receiving side */
  struct list_head   link;

  /** pre-allcoated iovec */
  struct iovec       prealloc_v[NM_SO_PREALLOC_IOV_LEN];

  uint32_t chunk_offset;
  tbx_bool_t is_completed;

  tbx_tick_t start_transfer_time;
  
  struct DLOOP_Segment *segp;
  /** Current position in the datatype */
  DLOOP_Offset datatype_offset;

  tbx_bool_t datatype_copied_buf;

  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];

};

#endif /* NM_PKT_WRAP_H */

