/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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


#ifndef NM_CORE_INTERFACE_H
#define NM_CORE_INTERFACE_H

/* don't include tbx.h here.
 * They are not needed and not ISO C compliant.
 */
#include <nm_public.h>
#include <Padico/Puk.h>
#include <sys/uio.h>

#ifdef PIOMAN
#include <pioman.h>
#endif

/** @defgroup core_interface nmad core interface
 * This is the interface of the NewMad core. All other
 * include files in the core are internal and shouldn't be
 * included outside of the core.
 *
 * End-users are not supposed to use directly this core
 * interface. They should use instead a higher level interface
 * such as 'sendrecv', 'pack', or 'mpi' for messaging, and 'session'
 * or 'launcher' for init and connection establishment.
 * @{
 */

/* ** Core init ******************************************** */

typedef struct nm_core*nm_core_t;

puk_component_t nm_core_component_load(const char*entity, const char*name);

int nm_core_init(int*argc, char*argv[], nm_core_t *pp_core);

int nm_core_set_strategy(nm_core_t p_core, puk_component_t strategy);

int nm_core_exit(nm_core_t p_core);

/** disable schedopt for raw driver use */
void nm_core_schedopt_disable(nm_core_t p_core);


/* ** Drivers ********************************************** */

struct nm_driver_query_param
{
  enum {
    NM_DRIVER_QUERY_BY_NOTHING=0,
    NM_DRIVER_QUERY_BY_INDEX,
    /* NM_DRIVER_QUERY_BY_NUMA_NODE, */
    /* NM_DRIVER_QUERY_BY_SPEED, */
  } key;
  union {
    uint32_t index;
  } value;
};

int nm_core_driver_load_init(nm_core_t p_core, puk_component_t driver,
			     struct nm_driver_query_param*param,
			     nm_drv_t *pp_drv, const char**p_url);


/* ** Gates ************************************************ */

typedef int8_t nm_trk_id_t;

void nm_core_gate_init(nm_core_t p_core, nm_gate_t *pp_gate, nm_drv_vect_t*p_drvs);

void nm_core_gate_connect(nm_core_t p_core, nm_gate_t gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char *url);


/* ** Progression ****************************************** */

int nm_schedule(nm_core_t p_core);

/* ** Status *********************************************** */

#ifdef PIOMAN
/** status bits of pack/unpack requests */
typedef piom_cond_value_t nm_status_t;
/** status with synchronization (wait/signal) */
typedef piom_cond_t nm_cond_status_t;
#else /* PIOMAN */
/** status bits of pack/unpack requests */
typedef uint16_t nm_status_t;
/** status with synchronization (wait/signal) */
typedef nm_status_t nm_cond_status_t;
#endif /* PIOMAN */

/** pack/unpack flags */
typedef uint32_t nm_req_flag_t;

/** protocol flags- not part of the public API, bu needed for inline */
typedef uint8_t nm_proto_t;


/* ** status and flags, used in pack/unpack requests and events */

/** empty request */
#define NM_STATUS_NONE                     ((nm_status_t)0x0000)
/** request initialized, not sent yet */
#define NM_STATUS_PACK_INIT                ((nm_status_t)0x0001)
/** request initialized, not sent yet */
#define NM_STATUS_UNPACK_INIT              ((nm_status_t)0x0002)
/** sending operation has completed */
#define NM_STATUS_PACK_COMPLETED           ((nm_status_t)0x0004)
/** unpack operation has completed */
#define NM_STATUS_UNPACK_COMPLETED         ((nm_status_t)0x0008)
/** data or rdv has arrived, with no matching unpack */
#define NM_STATUS_UNEXPECTED               ((nm_status_t)0x0010)
/** unpack operation has been cancelled */
#define NM_STATUS_UNPACK_CANCELLED         ((nm_status_t)0x0020)
/** sending operation is in progress */
#define NM_STATUS_PACK_POSTED              ((nm_status_t)0x0040)
/** unpack operation is in progress */
#define NM_STATUS_UNPACK_POSTED            ((nm_status_t)0x0080)
/** ack received for the given pack */
#define NM_STATUS_ACK_RECEIVED             ((nm_status_t)0x0100)
/** request is finalized, may be freed */
#define NM_STATUS_FINALIZED                ((nm_status_t)0x0400)

/** mask to catch all bits of status */
#define NM_STATUS_MASK_FULL                ((nm_status_t)-1)

/** no flag set */
#define NM_REQ_FLAG_NONE                   ((nm_req_flag_t)0x00000000)
/** flag pack as synchronous (i.e. request the receiver to send an ack) */
#define NM_REQ_FLAG_PACK_SYNCHRONOUS       ((nm_req_flag_t)0x00001000)
/** flag request as a pack */
#define NM_REQ_FLAG_PACK                   ((nm_req_flag_t)0x00002000)
/** flag request as an unpack */
#define NM_REQ_FLAG_UNPACK                 ((nm_req_flag_t)0x00004000)
/** flag unpack request as matched */
#define NM_REQ_FLAG_UNPACK_MATCHED         ((nm_req_flag_t)0x00008000)

/** flag req_chunk as short */
#define NM_REQ_FLAG_SHORT_CHUNK            ((nm_req_flag_t)0x00010000)
/** use buf_send to send this request */
#define NM_REQ_FLAG_BUF_SEND               ((nm_req_flag_t)0x00020000)
/** flatten data as contiguous block before send */
#define NM_REQ_FLAG_USE_COPY               ((nm_req_flag_t)0x00040000)
/* use iterator-based data description in pw */
#define NM_REQ_FLAG_DATA_ITERATOR          ((nm_req_flag_t)0x00080000)
/** request submited in wildcard queue */
#define NM_REQ_FLAG_WILDCARD               ((nm_req_flag_t)0x00100000)

/** Sequence number */
typedef uint32_t nm_seq_t;

/* ** tags ************************************************* */

/** a session hashcode in tags, used to multiplex sessions */
typedef uint32_t nm_session_hash_t;

/** mask for all sessions */
#define NM_CORE_TAG_HASH_FULL ((nm_session_hash_t)0xFFFFFFFF)

#if defined(NM_TAGS_AS_INDIRECT_HASH)
/** An internal tag */
typedef struct
{
  nm_tag_t tag;               /**< the user-supplied tag */
  nm_session_hash_t hashcode; /**< the session hashcode */
} __attribute__((packed)) nm_core_tag_t;
#define NM_CORE_TAG_MASK_FULL ((nm_core_tag_t){ .tag = NM_TAG_MASK_FULL, .hashcode = NM_CORE_TAG_HASH_FULL })
#define NM_CORE_TAG_NONE      ((nm_core_tag_t){ .tag = 0, .hashcode = 0x0 })
static inline nm_core_tag_t nm_core_tag_build(nm_session_hash_t hashcode, nm_tag_t tag)
{
  const nm_core_tag_t core_tag = { .tag = tag, .hashcode = hashcode };
  return core_tag;
}
static inline nm_tag_t nm_core_tag_get_tag(nm_core_tag_t core_tag)
{
  return core_tag.tag;
}
static inline nm_session_hash_t nm_core_tag_get_hashcode(nm_core_tag_t core_tag)
{
  return core_tag.hashcode;
}
#else /* NM_TAGS_AS_INDIRECT_HASH */
/** An internal tag */
typedef nm_tag_t nm_core_tag_t;
#define NM_CORE_TAG_MASK_FULL NM_TAG_MASK_FULL
#define NM_CORE_TAG_NONE ((nm_tag_t)0)
static inline nm_core_tag_t nm_core_tag_build(nm_session_hash_t hashcode, nm_tag_t tag)
{
  const nm_core_tag_t core_tag = tag;
  return core_tag;
}
static inline nm_tag_t nm_core_tag_get_tag(nm_core_tag_t core_tag)
{
  return core_tag;
}
static inline nm_session_hash_t nm_core_tag_get_hashcode(nm_core_tag_t core_tag)
{
  return 0;
}
#endif /* NM_TAGS_AS_INDIRECT_HASH */


/* ** Event notification *********************************** */

/** An event, generated by the NewMad core. */
struct nm_core_event_s
{
  nm_status_t status; /**< status flags- describe the event */
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  nm_len_t len;
  struct nm_req_s*p_req; /**< the request that matched the event- NULL in case of unexpected packets */
};

/** an event notifier, fired upon status transition */
typedef void (*nm_core_event_notifier_t)(const struct nm_core_event_s*const event, void*ref);

/** matching info for global monitors */
struct nm_core_event_matching_s
{
  nm_gate_t p_gate;       /**< the gate to listen to, or NM_ANY_GATE for any */
  nm_core_tag_t tag;      /**< the tag to listen too */
  nm_core_tag_t tag_mask; /**< the mask to apply before comparing tags (only bits set in mask will be checked) */
};

/** generic monitor, used for requests and for global events (with matching) */
struct nm_monitor_s
{
  nm_core_event_notifier_t p_notifier; /**< notification function called to fire events */
  nm_status_t event_mask;              /**< mask applied to status to check whether to fire events */
  void*ref;                            /**< opaque user-supplied pointer passed to notifier */
};

/** global monitor for status transitions */
struct nm_core_monitor_s
{
  struct nm_monitor_s monitor;               /**< the monitor to fire upon matching event */
  struct nm_core_event_matching_s matching;  /**< packet matching information */
  int serialized;                            /**< whether dispatching should be serialized (order guaranteed) or may be multi-threaded */
};
/** Register an event monitor. */
void nm_core_monitor_add(nm_core_t p_core, struct nm_core_monitor_s*m);
/** Unregister an event monitor. */
void nm_core_monitor_remove(nm_core_t p_core, struct nm_core_monitor_s*m);
/** set a per-request monitor. Fire event immediately if pending */
void nm_core_req_monitor(struct nm_core*p_core, struct nm_req_s*p_req, struct nm_monitor_s monitor);


/** matches any event */
#define NM_EVENT_MATCHING_ANY ((struct nm_core_event_matching_s){ .p_gate = NM_ANY_GATE, .tag = NM_CORE_TAG_NONE, .tag_mask = NM_CORE_TAG_NONE })

#define NM_MONITOR_NULL ((struct nm_monitor_s){ .p_notifier = NULL, .event_mask = 0, .ref = NULL })

#define NM_CORE_MONITOR_NULL ((struct nm_core_monitor_s){ .monitor = NM_MONITOR_NULL, .matching = NM_EVENT_MATCHING_ANY })


/* ** Packs/unpacks **************************************** */

PUK_LIST_DECLARE_TYPE(nm_req_chunk);

/** a chunk of request */
struct nm_req_chunk_s
{
  PUK_LIST_LINK(nm_req_chunk);
  struct nm_req_s*p_req;  /**< the request this chunk belongs to */
  nm_len_t chunk_len;     /**< length of the chunk */
  nm_len_t chunk_offset;  /**< offset of the chunk relative to the full data in the req */
  nm_proto_t proto_flags; /**< pre-computed proto flags */
};

PUK_LIST_DECLARE_TYPE(nm_req);

/** a generic pack/unpack request */
struct nm_req_s
{
  PUK_LIST_LINK(nm_req);        /**< link to enqueue req in pending requests lists */
  nm_cond_status_t status;      /**< status, including status bits and synchronization */
  struct nm_data_s data;        /**< data descriptor to send/recv */
  struct nm_monitor_s monitor;  /**< monitor attached to this request (only 1) */
  nm_req_flag_t flags;          /**< flags given by user */
  nm_gate_t p_gate;             /**< dest/src gate; NULL if recv from any source */
  nm_core_tag_t tag;            /**< tag to send to/from (works in combination with tag_mask for recv) */
  nm_seq_t seq;                 /**< packet sequence number on the given tag */
  union
  {
    struct
    {
      nm_len_t len;             /**< cumulated data length */
      nm_len_t done;            /**< cumulated length of data sent so far */
      int priority;             /**< request priority level */
    } pack;
    struct
    {
      nm_len_t expected_len;  /**< length of posted recv (may be updated if matched packet is shorter) */
      nm_len_t cumulated_len; /**< amount of data unpacked so far */
      uint64_t req_seq;       /**< request sequence number used to interleave wildcard/non-wildcard requests */
      nm_core_tag_t tag_mask; /**< mask applied to tag for matching (only bits in mask need to match) */
    } unpack;
  };
  struct nm_req_chunk_s req_chunk; /**< preallocated chunk for the common case (single-chunk) */
};

/** build a pack request from data descriptor */
void nm_core_pack_data(nm_core_t p_core, struct nm_req_s*p_pack, const struct nm_data_s*p_data);

/** set tag/gate/flags for pack request */
void nm_core_pack_send(struct nm_core*p_core, struct nm_req_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate, nm_req_flag_t flags);

/** post a pack request */
void nm_core_pack_submit(struct nm_core*p_core, struct nm_req_s*p_pack, nm_len_t hlen);

/** set a priority for the given pack request */
static inline void nm_core_pack_set_priority(struct nm_core*p_core, struct nm_req_s*p_pack, int priority)
{
  p_pack->pack.priority = priority;
}

/** initializes an empty unpack request */
void nm_core_unpack_init(struct nm_core*p_core, struct nm_req_s*p_unpack);

/** build an unpack request from data descriptor */
void nm_core_unpack_data(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_data_s*p_data);

/** match an unpack request with given gate/tag, next sequence number assumed */
void nm_core_unpack_match_recv(struct nm_core*p_core, struct nm_req_s*p_unpack, nm_gate_t p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask);

/** match an unpack request with a packet that triggered an event */
void nm_core_unpack_match_event(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_core_event_s*p_event);

/** submit an unpack request */
void nm_core_unpack_submit(struct nm_core*p_core, struct nm_req_s*p_unpack, nm_req_flag_t flags);

/** peeks unexpected data without consumming it */
int nm_core_unpack_peek(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_data_s*p_data);

/** probe unexpected packet, using matching from unpack request */
int nm_core_unpack_iprobe(struct nm_core*p_core, struct nm_req_s*p_unpack);

/** cancel a pending unpack
 * @note cancel may fail if matching was already done.
 */
int nm_core_unpack_cancel(struct nm_core*p_core, struct nm_req_s*p_unpack);

/** probe unexpected packet, check matching for (packet_tag & tag_mask) == tag */
int nm_core_iprobe(struct nm_core*p_core,
		   nm_gate_t p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask,
		   nm_gate_t *pp_out_gate, nm_core_tag_t*p_out_tag, nm_len_t*p_out_size);

/** Flush pending packs (if supported by the strategy). */
void nm_core_flush(struct nm_core*p_core);

/* ** Status transition ************************************ */

/* not part of the public API; defined here for inline */

#if defined(PIOMAN)

/** @internal */
static inline void nm_cond_init(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  piom_cond_init(p_cond, bitmask);
}
/** @internal */
static inline nm_status_t nm_cond_test(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  if(bitmask & NM_STATUS_FINALIZED)
    return piom_cond_test_locked(p_cond, bitmask);
  return piom_cond_test(p_cond, bitmask);
}
/** @internal */
static inline void nm_cond_add(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  piom_cond_add(p_cond, bitmask);
}
/** @internal */
static inline void nm_cond_wait(nm_cond_status_t*p_cond, nm_status_t bitmask, nm_core_t p_core)
{
  piom_cond_wait(p_cond, bitmask);
}
/** @internal */
static inline void nm_cond_signal(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  piom_cond_signal(p_cond, bitmask);
}
/** @internal */
static inline void nm_cond_wait_multiple(void**pp_conds, int n, uintptr_t offset, nm_status_t bitmask, nm_core_t p_core)
{
  piom_cond_wait_all(pp_conds, n, offset, bitmask);
}
#else /* PIOMAN */
/** @internal */
static inline void nm_cond_init(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  *p_cond = bitmask;
}
/** @internal */
static inline nm_status_t nm_cond_test(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  return ((*p_cond) & bitmask);
}
/** @internal */
static inline void nm_cond_add(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  *p_cond |= bitmask;
}
/** @internal */
static inline void nm_cond_signal(nm_cond_status_t*p_cond, nm_status_t bitmask)
{
  *p_cond |= bitmask;
}
/** @internal */
static inline void nm_cond_wait(nm_cond_status_t*p_cond, nm_status_t bitmask, nm_core_t p_core)
{
  while(!nm_cond_test(p_cond, bitmask))
    {
      nm_schedule(p_core);
    }
}
/** @internal */
static inline void nm_cond_wait_multiple(void**pp_conds, int n, uintptr_t offset, nm_status_t bitmask, nm_core_t p_core)
{
  int i;
  for(i = 0; i < n; i++)
    {
      nm_cond_status_t*p_cond = (pp_conds[i] + offset);
      nm_cond_wait(p_cond, bitmask, p_core);
    }
}
#endif /* PIOMAN */

/* ** convenient frontends to deal with status in requests */

/** initialize cond status with given initial value */
static inline void nm_status_init(struct nm_req_s*p_req, nm_status_t bitmask)
{
  nm_cond_init(&p_req->status, bitmask);
}
/** query for given bits in req status; returns matched bits */
static inline nm_status_t nm_status_test(struct nm_req_s*p_req, nm_status_t bitmask)
{
  return nm_cond_test(&p_req->status, bitmask);
}
static inline void nm_status_add(struct nm_req_s*p_req, nm_status_t bitmask)
{
  nm_cond_add(&p_req->status, bitmask);
}
/** wait for _any bit_ matching in req status */
static inline void nm_status_wait(struct nm_req_s*p_req, nm_status_t bitmask, nm_core_t p_core)
{
  nm_cond_wait(&p_req->status, bitmask, p_core);
  assert(nm_status_test(p_req, bitmask) != 0);
}
static inline void nm_status_signal(struct nm_req_s*p_req, nm_status_t bitmask)
{
  nm_cond_signal(&p_req->status, bitmask);
}
/** wait for _all reqs_, _any bit_ in bitmask */
static inline void nm_status_wait_multiple(void**pp_reqs, int n, uintptr_t offset,
					   nm_status_t bitmask, nm_core_t p_core)
{
  const struct nm_req_s*p_req = NULL;
  const uintptr_t status_offset = (uintptr_t)&p_req->status - (uintptr_t)p_req; /* offset of 'status' in nm_req_s */
  nm_cond_wait_multiple(pp_reqs, n, offset + status_offset, bitmask, p_core);
}
static inline void nm_status_assert(struct nm_req_s*p_req, nm_status_t value)
{
  assert(nm_status_test(p_req, NM_STATUS_MASK_FULL) == value);
}

static inline void nm_status_spinwait(struct nm_req_s*p_req, nm_status_t status)
{
  while(!nm_status_test(p_req, status))
    { /* bust wait*/ }
}
/** tests for all given bits in status */
static inline int nm_status_test_allbits(struct nm_req_s*p_req, nm_status_t bitmask)
{
  return (nm_status_test(p_req, bitmask) == bitmask);
}


/** @} */

#endif /* NM_CORE_INTERFACE_H */

