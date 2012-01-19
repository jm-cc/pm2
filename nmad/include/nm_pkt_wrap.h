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

struct nm_pw_contrib_s
{
  struct nm_pack_s*p_pack;
  uint32_t len; /**< length of the pack enclosed in the pw (a pw may contain a partial chunk of a pack) */
};

/** Internal packet wrapper.
 */
struct nm_pkt_wrap
{
#ifdef PIOMAN_POLL
  struct piom_ltask ltask;
#endif /* PIOMAN_POLL */

#ifdef PIO_OFFLOAD
  tbx_bool_t data_to_offload;
  struct piom_ltask offload_ltask;
#endif /* PIO_OFFLOAD */
  
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

  /** Driver implementation data.  */
  void*drv_priv;
  
  
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

  /** link to insert the pw into a tbx_fast_list. A pw may be store either in:
   * out_list in strategy, pending_large_send in sender, pending_large_recv in receiver,
   * pending_send_list, pending_recv_list in driver,
   * post_sched_out_list, post_recv_list in driver
   */
  struct tbx_fast_list_head link;

  /** pre-allcoated iovec */
  struct iovec       prealloc_v[NM_SO_PREALLOC_IOV_LEN];

  /* ** fields used when sending */

  /** list of contributions in this pw (sending) */
  struct nm_pw_contrib_s*contribs;
  /** number of contribs actually stored in this pw */
  int n_contribs;
  /** size of the allocated contribs array */
  int contribs_size;
  /** pre-allocated contribs */
  struct nm_pw_contrib_s prealloc_contribs[NM_SO_PREALLOC_IOV_LEN];

  /* ** fields used when receiving */

  /** offset of this chunk in the message */
  uint32_t chunk_offset;

  struct nm_unpack_s*p_unpack;

  tbx_tick_t start_transfer_time;
  
  /* The following field MUST be the LAST within the structure */
  NM_SO_ALIGN_TYPE   buf[1];

};


#define nm_l2so(l) tbx_fast_list_entry(l, struct nm_pkt_wrap, link)

int nm_so_pw_init(struct nm_core *p_core);

int nm_so_pw_exit(void);

void nm_so_pw_raz(struct nm_pkt_wrap *p_pw);

int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw);

int nm_so_pw_free(struct nm_pkt_wrap *p_pw);

int nm_so_pw_split_data(struct nm_pkt_wrap *p_pw,
			struct nm_pkt_wrap *pp_pw2,
			uint32_t offset);

void nm_so_pw_add_data(struct nm_pkt_wrap *p_pw, struct nm_pack_s*p_pack,
		       const void *data, uint32_t len, uint32_t offset, int flags);

int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw);


#endif /* NM_PKT_WRAP_H */

