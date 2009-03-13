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

/* ** SchedOpt status flags */

typedef uint16_t nm_so_status_t;
/* for now, flags are included in status */
typedef nm_so_status_t nm_so_flag_t;

/** sending operation has completed */
#define NM_SO_STATUS_PACK_COMPLETED           ((nm_so_status_t)0x0001)
/** unpack operation has completed */
#define NM_SO_STATUS_UNPACK_COMPLETED         ((nm_so_status_t)0x0002)
/** data has arrived */
#define NM_SO_STATUS_PACKET_HERE              ((nm_so_status_t)0x0004)
/** recv is posted */
#define NM_SO_STATUS_UNPACK_HERE              ((nm_so_status_t)0x0008)
/** rendez-vous has arrived */
#define NM_SO_STATUS_RDV_HERE                 ((nm_so_status_t)0x0010)
/** rendez-vous is in progress */
#define NM_SO_STATUS_RDV_IN_PROGRESS          ((nm_so_status_t)0x0020)
/** ACK for rendez-vous has arrived */
#define NM_SO_STATUS_ACK_HERE                 ((nm_so_status_t)0x0040)
/** unpack operation has been cancelled */
#define NM_SO_STATUS_UNPACK_CANCELLED         ((nm_so_status_t)0x0080)
/** flag: unpack is an iovec */
#define NM_SO_STATUS_UNPACK_IOV               ((nm_so_flag_t)0x0100)
/** flag: unpack is a datatype */
#define NM_SO_STATUS_IS_DATATYPE              ((nm_so_flag_t)0x0200)
/* flag: unpack datatype through a temporary buffer */
#define NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE ((nm_so_flag_t)0x0400)

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


/** chunk of unexpected message to be stored */
struct nm_so_chunk 
{
  void *header;
  struct nm_pkt_wrap *p_pw;
  struct list_head link;
};

/** fast allocator for struct nm_so_chunk */
extern p_tbx_memory_t nm_so_chunk_mem;

#define nm_l2chunk(l) \
        ((struct nm_so_chunk *)((char *)(l) -\
         (unsigned long)(&((struct nm_so_chunk *)0)->link)))


/** tag-indexed type for 'any_src' requests */
struct nm_so_any_src_s
{
  nm_so_status_t status;
  void *data;
  int32_t cumulated_len;
  int32_t expected_len;
  nm_gate_t p_gate;
  uint8_t seq;
};
static inline void nm_so_any_src_ctor(struct nm_so_any_src_s*any_src, nm_tag_t tag)
{
  memset(any_src, 0, sizeof(struct nm_so_any_src_s));
}
static inline void nm_so_any_src_dtor(struct nm_so_any_src_s*any_src)
{
}
NM_TAG_TABLE_TYPE(nm_so_any_src, struct nm_so_any_src_s);

/** tag-indexed type for known-src requests 
 * @todo compress arrays of size NM_SO_PENDING_PACKS_WINDOW.
 */
struct nm_so_tag_s
{

  /* ** common fields 
   *  WARNING: false sharing between send and recv!
   */
  
  /** send and recv status (filled with NM_SO_STATUS_* bitmasks) */
  nm_so_status_t status[NM_SO_PENDING_PACKS_WINDOW];

  /** context for the interface */
  struct nm_sr_tag_s*p_sr_tag;

  /* ** receiving-related fields */
  uint8_t recv_seq_number;
  struct
  {
    struct nm_so_chunk pkt_here; /**< unexpected packet */
    struct
    { /* expected packet (i.e. recv posted) */
      void *data;
      int32_t cumulated_len;
      int32_t expected_len;
    } unpack_here;
  } recv[NM_SO_PENDING_PACKS_WINDOW];

  /* ** sending-related fields */

  uint8_t send_seq_number;
  /** pending len to send */
  int32_t send[NM_SO_PENDING_PACKS_WINDOW];
  /** large messages waiting for ACKs */
  struct list_head pending_large_send;
};
static inline void nm_so_tag_ctor(struct nm_so_tag_s*so_tag, nm_tag_t tag)
{
  memset(&so_tag->status, 0, sizeof(so_tag->status));
  so_tag->recv_seq_number = 0;
  memset(&so_tag->recv, 0, sizeof(so_tag->recv));
  so_tag->send_seq_number = 0;
  memset(&so_tag->send , 0, sizeof(so_tag->send));
  INIT_LIST_HEAD(&so_tag->pending_large_send);
  so_tag->p_sr_tag = NULL;
}
static inline void nm_so_tag_dtor(struct nm_so_tag_s*so_tag)
{
  if(so_tag->p_sr_tag)
    TBX_FREE(so_tag->p_sr_tag);
}
NM_TAG_TABLE_TYPE(nm_so_tag, struct nm_so_tag_s);

/** SchedOpt internal status */
struct nm_so_sched
{
  /** Post-scheduler outgoing lists, to be posted to thre driver. */
  p_tbx_slist_t post_sched_out_list;

#ifndef PIOMAN
  /* We don't use these lists since PIOMan already manage a 
     list of requests to poll */

  /** Outgoing active requests. */
  p_tbx_slist_t out_req_list;

  /** recv requests posted to the driver. */
  p_tbx_slist_t	 pending_recv_req;
#endif

  /** recv requests submited to nmad, to be posted to the driver. */
  p_tbx_slist_t	post_recv_req;
  
  /** selected strategy */
  puk_component_t strategy_adapter;

  /** monitors for upper layers to track events in nmad scheduler */
  struct nm_so_monitor_vect_s monitors;

  /* "any source" messages management */
  struct nm_so_any_src_table_s any_src;
  /** next gate_id to poll for any_src */
  int next_gate_id;

  unsigned pending_any_src_unpacks;
};


struct nm_so_gate
{
  struct nm_so_sched *p_so_sched;

  /* Actually counts the number of expected small messages, including
     RdV requests for large messages */
  unsigned pending_unpacks;

  unsigned active_recv[NM_DRV_MAX][NM_SO_MAX_TRACKS];
  unsigned active_send[NM_DRV_MAX][NM_SO_MAX_TRACKS];

  /** table of tag status */
  struct nm_so_tag_table_s tags;

  /* For large messages waiting for Track 1 (or 2) to be free */
  struct list_head pending_large_recv;

  /* Strategy components elements */
  struct puk_receptacle_NewMad_Strategy_s strategy_receptacle;
  puk_instance_t strategy_instance;
};

int _nm_so_copy_data_in_iov(struct iovec *iov, uint32_t chunk_offset, const void *data, uint32_t len);

int __nm_so_unpack(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
		   nm_so_flag_t flag, void *data_description, uint32_t len);

int __nm_so_unpack_any_src(struct nm_core *p_core, nm_tag_t tag, nm_so_flag_t flag,
			   void *data_description, uint32_t len);

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq);


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

static inline int nm_so_unpack(struct nm_gate *p_gate,
			       nm_tag_t tag, uint8_t seq,
			       void *data, uint32_t len)
{
  /* Nothing special to flag for the contiguous reception */
  const nm_so_flag_t flag = 0;
  return __nm_so_unpack(p_gate, tag, seq, flag, data, len);
}

static inline int nm_so_unpackv(struct nm_gate *p_gate,
				nm_tag_t tag, uint8_t seq,
				struct iovec *iov, int nb_entries)
{
  /* Data will be receive in an iovec tab */
  const nm_so_flag_t flag = NM_SO_STATUS_UNPACK_IOV;
  return __nm_so_unpack(p_gate, tag, seq, flag, iov, iov_len(iov, nb_entries));
}

static inline int nm_so_unpack_datatype(struct nm_gate *p_gate,
					nm_tag_t tag, uint8_t seq,
					struct DLOOP_Segment *segp)
{
  /* Data will be receive through a datatype */
  const nm_so_flag_t flag = NM_SO_STATUS_IS_DATATYPE;
  return __nm_so_unpack(p_gate, tag, seq, flag, segp, datatype_size(segp));
}

static inline int nm_so_unpack_any_src(struct nm_core *p_core, nm_tag_t tag, void *data, uint32_t len)
{
    /* Nothing special to flag for the contiguous reception */
  const nm_so_flag_t flag = 0;
  return __nm_so_unpack_any_src(p_core, tag, flag, data, len);
}


static inline int nm_so_unpackv_any_src(struct nm_core *p_core, nm_tag_t tag, struct iovec *iov, int nb_entries)
{
  /* Data will be receive in an iovec tab */
  const nm_so_flag_t flag = NM_SO_STATUS_UNPACK_IOV;
  return __nm_so_unpack_any_src(p_core, tag, flag, iov, iov_len(iov, nb_entries));
}

static inline int nm_so_unpack_datatype_any_src(struct nm_core *p_core, nm_tag_t tag, struct DLOOP_Segment *segp){

  /* Data will be receive through a datatype */
  const nm_so_flag_t flag = NM_SO_STATUS_IS_DATATYPE;
  return __nm_so_unpack_any_src(p_core, tag, flag, segp, datatype_size(segp));
}

static inline int nm_so_pack(nm_gate_t p_gate, nm_tag_t tag, int seq, const void*data, int len)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  return (*r->driver->pack)(r->_status, p_gate, tag, seq, data, len);
}

static inline int nm_so_packv(nm_gate_t p_gate, nm_tag_t tag, int seq, const struct iovec*iov, int len)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  return (*r->driver->packv)(r->_status, p_gate, tag, seq, iov, len);
}

static inline int nm_so_pack_datatype(nm_gate_t p_gate, nm_tag_t tag, int seq, const struct DLOOP_Segment*segp)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  return (*r->driver->pack_datatype)(r->_status, p_gate, tag, seq, segp);
}

static inline int nm_so_pack_extended(nm_gate_t p_gate, nm_tag_t tag, int seq, const void*data, uint32_t len, tbx_bool_t is_completed)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  if(r->driver->pack_extended)
    {
      return (*r->driver->pack_extended)(r->_status, p_gate, tag, seq, data, len, is_completed);
    }
  else
    {
      NM_DISPF("The current strategy does not provide an extended pack");
      return (*r->driver->pack)(r->_status, p_gate, tag, seq, data, len);
    }
}

static inline int nm_so_flush(nm_gate_t p_gate)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  if(tbx_unlikely(r->driver->flush))
    {
      return (*r->driver->flush)(r->_status, p_gate);
    }
  else
    return -NM_ENOTIMPL;
}


#endif /* NM_SO_PRIVATE_H */

