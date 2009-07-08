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

/** @name Packet wrapper flags.
 * Flag constants for the flag field of pw.
 */
/*@{*/

typedef uint32_t nm_pw_flag_t;

/** Headerless pkt, if set.
 */
#define NM_PW_NOHEADER      0x0001

/** Pkt has been allocated with full (NM_SO_MAX_UNEXPECTED) contiguous buffer, if set.
 */
#define NM_PW_BUFFER        0x0002

/** Pkt has been allocated with a contiguous buffer and a global header has been prepared.
 */
#define NM_PW_GLOBAL_HEADER 0x0004

/** v[0].iov_base has been dyanmically allocated
 */
#define NM_PW_DYNAMIC_V0    0x0008

/** Pkt has been finalized- ready to send on the wire.
 */
#define NM_PW_FINALIZED     0x0100
/*@}*/

/* Data flags */
#define NM_SO_DATA_USE_COPY            0x0100


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
  nm_seq_t seq;
  
  /** Driver implementation data.  */
  void			*drv_priv;
  
  
  /* Packet related fields.					*/
  
  /** Packet flags. */
  nm_pw_flag_t		 flags;
  
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

  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];

};

#endif /* NM_PKT_WRAP_H */

