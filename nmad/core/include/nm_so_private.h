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


#ifndef NM_SO_PRIVATE_H
#define NM_SO_PRIVATE_H

#include <stdint.h>

/* ** SchedOpt status and flags, used in pack/unpack requests and events */

/** sending operation has completed */
#define NM_SO_STATUS_PACK_COMPLETED           ((nm_so_status_t)0x0001)
/** unpack operation has completed */
#define NM_SO_STATUS_UNPACK_COMPLETED         ((nm_so_status_t)0x0002)
/** data or rdv has arrived, with no matching unpack */
#define NM_SO_STATUS_UNEXPECTED               ((nm_so_status_t)0x0004)
/** unpack operation has been cancelled */
#define NM_SO_STATUS_UNPACK_CANCELLED         ((nm_so_status_t)0x0008)
/** ack received for the given pack */
#define NM_SO_STATUS_ACK_RECEIVED             ((nm_so_status_t)0x0010)

/** flag: unpack is contiguous */
#define NM_UNPACK_TYPE_CONTIGUOUS    ((nm_so_flag_t)0x0100)
/** flag: unpack is an iovec */
#define NM_UNPACK_TYPE_IOV           ((nm_so_flag_t)0x0200)
/** flag: unpack is a datatype */
#define NM_UNPACK_TYPE_DATATYPE      ((nm_so_flag_t)0x0400)
/* flag: unpack datatype through a temporary buffer */
#define NM_UNPACK_TYPE_COPY_DATATYPE ((nm_so_flag_t)0x0800)

#define NM_PACK_TYPE_CONTIGUOUS ((nm_so_flag_t)0x0100)
#define NM_PACK_TYPE_IOV        ((nm_so_flag_t)0x0200)
#define NM_PACK_TYPE_DATATYPE   ((nm_so_flag_t)0x0400)
/** flag pack as synchronous (i.e. request the receiver to send an ack) */
#define NM_PACK_SYNCHRONOUS     ((nm_so_flag_t)0x1000)


/** an event notifier, fired upon status transition */
typedef struct nm_so_event_s* nm_so_event_t;
typedef void (*nm_event_notifier_t)(const struct nm_so_event_s* const event);
/** monitor for status transitions */
struct nm_so_monitor_s
{
  nm_event_notifier_t notifier;
  nm_so_status_t mask;
};
PUK_VECT_TYPE(nm_so_monitor, const struct nm_so_monitor_s*);


/** a chunk of unexpected message to be stored */
struct nm_unexpected_s
{
  void*header;
  struct nm_pkt_wrap*p_pw;
  nm_gate_t p_gate;
  nm_seq_t seq;
  nm_tag_t tag;
  struct tbx_fast_list_head link;
};

/** fast allocator for struct nm_unexpected_s */
extern p_tbx_memory_t nm_so_unexpected_mem;

#define NM_UNEXPECTED_PREALLOC (NM_TAGS_PREALLOC * 256)

#define nm_l2chunk(l)  tbx_fast_list_entry(l, struct nm_unexpected_s, link)

/** tag-indexed type for known-src requests 
 */
struct nm_so_tag_s
{
  /* ** receiving-related fields */
  nm_seq_t recv_seq_number; /**< next sequence number for recv */

  /* ** sending-related fields */
  nm_seq_t send_seq_number;                  /**< next sequence number for send */
};
static inline void nm_so_tag_ctor(struct nm_so_tag_s*so_tag, nm_tag_t tag)
{
  so_tag->recv_seq_number = 0;
  so_tag->send_seq_number = 0;
}
static inline void nm_so_tag_dtor(struct nm_so_tag_s*so_tag)
{
}
NM_TAG_TABLE_TYPE(nm_so_tag, struct nm_so_tag_s);

/** SchedOpt internal status */
struct nm_so_sched
{
  /** Post-scheduler outgoing lists, to be posted to thre driver. */
  p_tbx_slist_t post_sched_out_list[NM_DRV_MAX][NM_SO_MAX_TRACKS];

  /** recv requests submited to nmad, to be posted to the driver. */
  p_tbx_slist_t	post_sched_in_list[NM_DRV_MAX][NM_SO_MAX_TRACKS];
  
#ifdef PIOMAN
  /** Lock used to access post_sched_out_list */
  piom_spinlock_t post_sched_out_lock[NM_DRV_MAX];

  /** Lock used to access post_recv_list */
  piom_spinlock_t post_sched_in_lock[NM_DRV_MAX];
#endif
#ifdef NMAD_POLL
  /* We don't use these lists since PIOMan already manage a 
     list of requests to poll */

  /** Outgoing active requests. */
  p_tbx_slist_t pending_send_list[NM_DRV_MAX];

  /** recv requests posted to the driver. */
  p_tbx_slist_t pending_recv_list[NM_DRV_MAX];

#ifdef PIOMAN
  /** Lock used to access pending_send_list */
  piom_spinlock_t pending_send_lock[NM_DRV_MAX];

  /** Lock used to access pending_recv_list */
  piom_spinlock_t pending_recv_lock[NM_DRV_MAX];
#endif /* PIOMAN */
#endif /* NMAD_POLL */

  /** selected strategy */
  puk_component_t strategy_adapter;

  /** monitors for upper layers to track events in nmad scheduler */
  struct nm_so_monitor_vect_s monitors;

  /** list of posted unpacks */
  struct tbx_fast_list_head unpacks;

  /** list of unexpected chunks */
  struct tbx_fast_list_head unexpected;

  /** list of pending packs waiting for an ack */
  struct tbx_fast_list_head pending_packs;
};

int nm_so_pack(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_tag_t tag, nm_gate_t p_gate,
	       const void*data, uint32_t len, nm_so_flag_t pack_type);

int __nm_so_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_tag_t tag,
		   nm_so_flag_t flag, void *data_description, uint32_t len);

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack);

int nm_so_iprobe(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_gate**pp_out_gate, nm_tag_t tag);

static inline int iov_len(const struct iovec *iov, int nb_entries)
{
  uint32_t len = 0;
  int i;
  for(i = 0; i < nb_entries; i++)
    {
      len += iov[i].iov_len;
    }
  return len;
}

static inline int datatype_size(const struct DLOOP_Segment *segp)
{
  DLOOP_Handle handle = segp->handle;
  int data_sz;
  CCSI_datadesc_get_size_macro(handle, data_sz); // * count?
  return data_sz;
}


#endif /* NM_SO_PRIVATE_H */

