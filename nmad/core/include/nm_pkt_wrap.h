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

/** IO vector entry meta-data.
 */
struct nm_iovec {

    /** Flags. */
    uint32_t		 priv_flags;

};

/** Io vector iterator.
 */
struct nm_iovec_iter {

        /** Flags.
         */
        uint32_t		 priv_flags;

        /** Current entry num.
         */
        uint32_t		 v_cur;

        /** Current ptr and len copy since the io vector must be modified in-place.
         */
        struct iovec		 cur_copy;

        /** Current size.
         */
        uint32_t		 v_cur_size;
};

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
  uint8_t			 seq;
  
  /* Implementation dependent data.				*/
  
  /** Driver implementation data.  */
  void			*drv_priv;
  
  /** Gate data. */
  void			*gate_priv;
  
  
  /* Packet related fields.					*/
  
  /** Packet internal flags. */
  uint32_t		 pkt_priv_flags;
  
  /** Cumulated amount of data (everything included) referenced by this wrap. */
  uint32_t		 length;

  /** Io vector setting.
   * bit 0:
   * 0 - no main/iovec header
   * 1 - reference main and iovec header in iovec
   *
   * bit 1:
   * 0 - no length vector header
   * 1 - reference length vector header in iovec
   *
   * bit 2: block type
   * 0 - headerless data
   * 1 - packet
   * - wrapped packet types may only be data, packet data or multipacket
   *
   * bit 3-7: reserved, must be 0
   */
  uint8_t 		 iov_flags;
  
  /** Body.
      - shortcut to pkt first data block vector entry (if relevant),
      or NULL
      - this field is to be used for convenience when needed, it does
      not imply that every pkt_wrap iov should be organized as a
      header+body msg
  */
  void			*data;
  
  /** Length vector for iov headers (if needed).
      - shortcut and ptr to length vector buffer
      - each entry must by 8-byte long, low to high weight bytes
  */
  uint8_t			*len_v;
  
  
  /** IO vector internal flags. */
  uint32_t		 iov_priv_flags;
  
  /** IO vector size. */
  uint32_t		 v_size;
  
  /** First used entry in io vector, not necessarily 0, to allow for prepend.
      - first unused entry before iov contents is v[v_first - 1]
  */
  uint32_t		 v_first;
  
  /** Number of used entries in io vector.
      - first unused entry after iov  contents is v[v_first + v_nb]
  */
  uint32_t		 v_nb;
  
  /** IO vector. */
  struct iovec		*v;
  
  /** IO vector per-entry meta-data, if needed, or NULL. */
  struct nm_iovec		*nm_v;
  
  tbx_tick_t start_transfer_time;
  
  struct DLOOP_Segment *segp;
  /** Current position in the datatype */
  DLOOP_Offset datatype_offset;

  int                header_ref_count;

  /* Used on the sending side */
  int                pending_skips;

  /* Used on the receiving side */
  struct list_head   link;

  /** pre-allcoated iovec */
  struct iovec       prealloc_v[NM_SO_PREALLOC_IOV_LEN];

#ifdef _NM_SO_HANDLE_DYNAMIC_IOVEC_ENTRIES
  struct nm_iovec    prealloc_nm_v[NM_SO_PREALLOC_IOV_LEN];
#endif

  uint32_t chunk_offset;
  tbx_bool_t is_completed;

  tbx_bool_t datatype_copied_buf;

  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];

};

#endif /* NM_PKT_WRAP_H */

