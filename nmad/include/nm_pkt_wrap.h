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

#ifndef NM_PKT_WRAP_H
#define NM_PKT_WRAP_H

#include <sys/uio.h>

/** @name Packet wrapper flags.
 * Flag constants for the flag field of pw.
 */
/*@{*/

/** flags to describe packet wrappers */
typedef uint32_t nm_pw_flag_t;

/** Headerless pkt, if set. */
#define NM_PW_NOHEADER      (nm_pw_flag_t)0x00000001

/** pw allocated with NM_SO_MAX_UNEXPECTED contiguous buffer.
 * Used to receive small pw on trk #0
 */
#define NM_PW_BUFFER        (nm_pw_flag_t)0x00000002

/** pw allocated with a contiguous buffer and a global header has been prepared.
 * Used to send small pw on trk #0
 */
#define NM_PW_GLOBAL_HEADER (nm_pw_flag_t)0x00000004

/** v[0] is dynamically allocated */
#define NM_PW_DYNAMIC_V0    (nm_pw_flag_t)0x00000008

/** Pkt has been finalized- ready to send on the wire. */
#define NM_PW_FINALIZED     (nm_pw_flag_t)0x00000010

/** Pkt has been prefetched */
#define NM_PW_PREFETCHED    (nm_pw_flag_t)0x00000020

/** Pkt has bee posted */
#define NM_PW_POSTED        (nm_pw_flag_t)0x00000040

/** send/recv of pw is completed */
#define NM_PW_COMPLETED     (nm_pw_flag_t)0x00000080

/** pw used for sending */
#define NM_PW_SEND          (nm_pw_flag_t)0x00000100

/** pw used for recv */
#define NM_PW_RECV          (nm_pw_flag_t)0x00000200

/** requests to copy data to contiguous block before sending */
#define NM_PW_DATA_COPY     (nm_pw_flag_t)0x00000400

/** use buffer-based driver for send */
#define NM_PW_BUF_SEND      (nm_pw_flag_t)0x00000800

/** use buffer-based driver for recv */
#define NM_PW_BUF_RECV      (nm_pw_flag_t)0x00001000

/** data received in driver buffer has been mirrored in pw buffer */
#define NM_PW_BUF_MIRROR    (nm_pw_flag_t)0x00002000

/*@}*/


/** Internal packet wrapper.
 */
struct nm_pkt_wrap_s
{
  /** link to insert the pw into a nm_pkt_wrap_list. A pw may be stored either in:
   * out_list in strategy, pending_large_send in sender, pending_large_recv in receiver,
   * pending_send_list, pending_recv_list in driver,
   * post_sched_out_list, post_recv_list in driver
   */
  PUK_LIST_LINK(nm_pkt_wrap);

  /* ** scheduler fields */
  
#ifdef PIOMAN
  struct piom_ltask ltask;   /**< ltask descriptor used by pioman for progression  */
#endif /* PIOMAN */
  nm_drv_t p_drv;            /**< assignated driver.  */
  nm_trk_id_t trk_id;        /**< assignated track ID.  */
  nm_gate_t p_gate;          /**< assignated gate, if relevant. */
  struct nm_trk_s*p_trk;     /**< assignated track, if relevant. */
    
  /* ** packet / data description fields. */

  nm_pw_flag_t flags;        /**< packet flags. */
  nm_len_t length;           /**< cumulated amount of data (everything included) referenced by this wrap. */
  nm_len_t max_len;          /**< maximum usable length */
  const struct nm_data_s*p_data; /**< data represented as datatype (if non-NULL, v must be empty) */
  struct iovec*v;            /**< IO vector. */
  int v_size;                /**< number of allocated entries in the iovec. */
  int v_nb;                  /**< number of used entries in the iovec */
  struct iovec prealloc_v[NM_SO_PREALLOC_IOV_LEN];  /**< pre-allcoated iovec */

  /* ** lifecycle */
  int ref_count;                                 /**< number of references pointing to the header */
  void (*destructor)(struct nm_pkt_wrap_s*p_pw); /**< destructor called uppon packet destroy */
  void*destructor_key;                           /**< key for destructor to store private data */

  /* ** fields used when sending */

  struct nm_req_chunk_list_s req_chunks;  /**< list of req chunks contained in this pw (sending) */

  /* ** fields used when receiving */

  struct nm_req_s*p_unpack;               /**< user-level unpack request (large message only) */
  nm_len_t chunk_offset;                  /**< offset of this chunk in the message (for large message) */

  /* ** obsolete */
  tbx_tick_t start_transfer_time;         /**< start time, when sampling is enabled */
#ifdef PIO_OFFLOAD
  tbx_bool_t data_to_offload;
  struct piom_ltask offload_ltask;
#endif /* PIO_OFFLOAD */

  /* The following field MUST be the LAST within the structure */
  NM_ALIGN_TYPE buf[1];
};

PUK_LIST_DECLARE_TYPE(nm_pkt_wrap);
PUK_LIST_CREATE_FUNCS(nm_pkt_wrap);

int nm_pw_alloc_init(struct nm_core*p_core);

int nm_pw_alloc_exit(void);

struct nm_pkt_wrap_s*nm_pw_alloc_buffer(void);

struct nm_pkt_wrap_s*nm_pw_alloc_noheader(void);

struct nm_pkt_wrap_s*nm_pw_alloc_global_header(void);

struct nm_pkt_wrap_s*nm_pw_alloc_driver_header(struct nm_trk_s*p_trk);

int nm_pw_free(struct nm_pkt_wrap_s*p_pw);

int nm_pw_split_data(struct nm_pkt_wrap_s*p_pw, struct nm_pkt_wrap_s*pp_pw2, nm_len_t offset);

void nm_pw_add_req_chunk(struct nm_pkt_wrap_s*p_pw, struct nm_req_chunk_s*p_req_chunk, nm_req_flag_t flags);

void nm_pw_add_data_in_header(struct nm_pkt_wrap_s*p_pw, nm_core_tag_t tag, nm_seq_t seq,
			      struct nm_data_s*p_data, nm_len_t len, nm_len_t chunk_offset, uint8_t flags);

void nm_pw_add_raw(struct nm_pkt_wrap_s*p_pw, const void*data, nm_len_t len, nm_len_t chunk_offset);

void nm_pw_set_data_raw(struct nm_pkt_wrap_s*p_pw, struct nm_data_s*p_data, nm_len_t chunk_len, nm_len_t chunk_offset);

struct iovec*nm_pw_grow_iovec(struct nm_pkt_wrap_s*p_pw);

/* forward declaration to resolve dependancy */
union nm_header_ctrl_generic_s;

int nm_pw_add_control(struct nm_pkt_wrap_s*p_pw, const union nm_header_ctrl_generic_s*p_ctrl);

int nm_pw_finalize(struct nm_pkt_wrap_s *p_pw);

void nm_core_post_send( struct nm_pkt_wrap_s*p_pw, nm_gate_t p_gate, nm_trk_id_t trk_id);

void nm_core_post_recv(struct nm_pkt_wrap_s*p_pw, nm_gate_t p_gate, nm_trk_id_t trk_id, nm_drv_t p_drv);


#endif /* NM_PKT_WRAP_H */

