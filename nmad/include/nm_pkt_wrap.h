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

/** @name Packet wrapper flags.
 * Flag constants for the flag field of pw.
 */
/*@{*/

typedef uint32_t nm_pw_flag_t;

/** Headerless pkt, if set.
 */
#define NM_PW_NOHEADER      (nm_pw_flag_t)0x0001

/** Pkt has been allocated with full (NM_SO_MAX_UNEXPECTED) contiguous buffer, if set.
 */
#define NM_PW_BUFFER        (nm_pw_flag_t)0x0002

/** Pkt has been allocated with a contiguous buffer and a global header has been prepared.
 */
#define NM_PW_GLOBAL_HEADER (nm_pw_flag_t)0x0004

/** v[0].iov_base has been dyanmically allocated
 */
#define NM_PW_DYNAMIC_V0    (nm_pw_flag_t)0x0008

/** Pkt has been finalized- ready to send on the wire.
 */
#define NM_PW_FINALIZED     (nm_pw_flag_t)0x0010

/** Pkt has been prefetched
 */
#define NM_PW_PREFETCHED    (nm_pw_flag_t)0x0020

/*@}*/

/* Data flags */
#define NM_SO_DATA_USE_COPY            0x0200

/** notification of pw send completion */
struct nm_pw_completion_s
{
  struct nm_pack_s*p_pack; /**< pack the pw contributes to */
  nm_len_t len; /**< length of the pack enclosed in the pw (a pw may contain a partial chunk of a pack) */
};

/** Internal packet wrapper.
 */
struct nm_pkt_wrap
{
  /** link to insert the pw into a tbx_fast_list. A pw may be stored either in:
   * out_list in strategy, pending_large_send in sender, pending_large_recv in receiver,
   * pending_send_list, pending_recv_list in driver,
   * post_sched_out_list, post_recv_list in driver
   */
  struct tbx_fast_list_head link;
  
  /* ** scheduler fields */
  
#ifdef PIOMAN_POLL
  struct piom_ltask ltask;   /**< ltask descriptor used by pioman for progression  */
#endif /* PIOMAN_POLL */
  struct nm_drv*p_drv;       /**< assignated driver.  */
  nm_trk_id_t trk_id;        /**< assignated track ID.  */
  struct nm_gate*p_gate;     /**< assignated gate, if relevant. */
  struct nm_gate_drv*p_gdrv; /**< assignated gate driver, if relevant. */
  void*drv_priv;             /**< driver implementation data.  */
    
  /* ** packet / data description fields. */

  nm_pw_flag_t flags;        /**< packet flags. */
  nm_len_t length;           /**< cumulated amount of data (everything included) referenced by this wrap. */
  struct iovec*v;            /**< IO vector. */
  int v_size;                /**< number of allocated entries in the iovec. */
  int v_nb;                  /**< number of used entries in the iovec */
  struct iovec prealloc_v[NM_SO_PREALLOC_IOV_LEN];  /**< pre-allcoated iovec */

  /* ** lifecycle */
  int ref_count;                                /**< number of references pointing to the header */
  void (*destructor)(struct nm_pkt_wrap*p_pw);  /**< destructor called uppon packet destroy */
  void*destructor_key;                          /**< key for destructor to store private data */

  /* ** fields used when sending */

  struct nm_pw_completion_s*completions;  /**< list of completion notifier for this pw (sending) */
  int n_completions;                      /**< number of completion notifiers actually registered in this pw */
  int completions_size;                   /**< size of the allocated completions array */
  struct nm_pw_completion_s prealloc_completions[NM_SO_PREALLOC_IOV_LEN]; /**< pre-allocated completions */

  /* ** fields used when receiving */

  struct nm_unpack_s*p_unpack;            /**< user-level unpack request (large message only) */
  nm_len_t chunk_offset;                  /**< offset of this chunk in the message (for large message) */

  /* ** obsolete */
  tbx_tick_t start_transfer_time;         /**< start time, when sampling is enabled */
#ifdef PIO_OFFLOAD
  tbx_bool_t data_to_offload;
  struct piom_ltask offload_ltask;
#endif /* PIO_OFFLOAD */

  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];

};


#define nm_l2so(l) tbx_fast_list_entry(l, struct nm_pkt_wrap, link)

int nm_so_pw_init(struct nm_core *p_core);

int nm_so_pw_exit(void);

int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw);

int nm_so_pw_free(struct nm_pkt_wrap *p_pw);

int nm_so_pw_split_data(struct nm_pkt_wrap *p_pw,
			struct nm_pkt_wrap *pp_pw2,
			nm_len_t offset);

void nm_so_pw_add_data(struct nm_pkt_wrap *p_pw, struct nm_pack_s*p_pack,
		       const void *data, nm_len_t len, nm_len_t offset, int flags);

int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw);

void nm_pw_completion_add(struct nm_pkt_wrap*p_pw, struct nm_pack_s*p_pack, nm_len_t len);

void nm_pw_completions_notify(struct nm_pkt_wrap*p_pw);


#endif /* NM_PKT_WRAP_H */

