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


#ifndef NM_CORE_INTERFACE_H
#define NM_CORE_INTERFACE_H

/* don't include tbx.h here.
 * They are not needed and not ISO C compliant.
 */
#include <nm_public.h>
#include <Padico/Puk.h>
#include <tbx_fast_list.h>

#include <sys/uio.h>

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

typedef struct nm_drv*nm_drv_t;

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

int nm_core_gate_init(nm_core_t p_core, nm_gate_t *pp_gate);

int nm_core_gate_connect(nm_core_t p_core, nm_gate_t gate, nm_drv_t  p_drv, const char *url);


/* ** Packs/unpacks **************************************** */

/** status of a pack/unpack request */
typedef uint16_t nm_status_t;

/** pack/unpack flags
 * @note for now, flags are included in status */
typedef nm_status_t nm_so_flag_t;

/* ** status and flags, used in pack/unpack requests and events */

#define NM_STATUS_NONE                     ((nm_status_t)0x0000)
/** sending operation has completed */
#define NM_STATUS_PACK_COMPLETED           ((nm_status_t)0x0001)
/** unpack operation has completed */
#define NM_STATUS_UNPACK_COMPLETED         ((nm_status_t)0x0002)
/** data or rdv has arrived, with no matching unpack */
#define NM_STATUS_UNEXPECTED               ((nm_status_t)0x0004)
/** unpack operation has been cancelled */
#define NM_STATUS_UNPACK_CANCELLED         ((nm_status_t)0x0008)
/** sending operation is in progress */
#define NM_STATUS_PACK_POSTED              ((nm_status_t)0x0010)
/** unpack operation is in progress */
#define NM_STATUS_UNPACK_POSTED            ((nm_status_t)0x0020)
/** ack received for the given pack */
#define NM_STATUS_ACK_RECEIVED             ((nm_status_t)0x0080)

/** flag pack as synchronous (i.e. request the receiver to send an ack) */
#define NM_PACK_SYNCHRONOUS     ((nm_so_flag_t)0x1000)

#define _NM_DATA_CONTENT_SIZE 32

struct nm_data_s;
/** function apply to each data chunk upon traversal */
typedef void (*nm_data_apply_t)(void*ptr, nm_len_t len, void*_ref);

/** traversal function for data descriptors */
typedef void (*nm_data_traversal_t)(const void*_content, nm_data_apply_t apply, void*_context);

/** a data descriptor, used to pack/unpack data from app layout to/from contiguous buffers */
struct nm_data_s
{
  /** funtion to traverse data with app layout, i.e. map
   * @param p_data data descriptor
   * @param apply function to apply to all chunks
   * @param _context context pointer given to apply function
   */
  nm_data_traversal_t traversal;
  /** placeholder for type-dependant content */
  char _content[_NM_DATA_CONTENT_SIZE];
};
#define NM_DATA_TYPE(ENAME, TYPE, TRAVERSAL)				\
  static inline void nm_data_##ENAME##_set(struct nm_data_s*p_data, TYPE value) \
  {									\
    p_data->traversal = TRAVERSAL;					\
    assert(sizeof(TYPE) <= _NM_DATA_CONTENT_SIZE);			\
    TYPE*p_##ENAME = (TYPE*)&p_data->_content[0];			\
    *p_##ENAME = value;							\
  }

/** data descriptor for contiguous data 
 */
struct nm_data_contiguous_s
{
  void*ptr;
  nm_len_t len;
};
void nm_data_traversal_contiguous(const void*_content, nm_data_apply_t apply, void*_context);
NM_DATA_TYPE(contiguous, struct nm_data_contiguous_s, &nm_data_traversal_contiguous);
/** data descriptor for iov data
 */
struct nm_data_iov_s
{
  struct iovec*v;
  int n;
};
void nm_data_traversal_iov(const void*_content, nm_data_apply_t apply, void*_context);
NM_DATA_TYPE(iov, struct nm_data_iov_s, &nm_data_traversal_iov);

/** helper function to apply iterator to data
 */
static inline void nm_data_traversal_apply(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  (*p_data->traversal)((void*)p_data->_content, apply, _context);
}

void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context);

nm_len_t nm_data_size(const struct nm_data_s*p_data);

void nm_data_copy(const struct nm_data_s*p_data, nm_len_t chunk_offset, const void *ptr, nm_len_t len);

/** Sequence number */
typedef uint16_t nm_seq_t;

#if defined(NM_TAGS_AS_INDIRECT_HASH)
/* ** Note: only indirect hashing allows long/structured tags.
 */
#define NM_TAGS_STRUCT
/** An internal tag */
typedef struct
{
  nm_tag_t tag;       /**< the interface level tag */
  uint32_t hashcode;  /**< the session hashcode */
} __attribute__((packed)) nm_core_tag_t;
#define NM_CORE_TAG_MASK_FULL ((nm_core_tag_t){ .tag = NM_TAG_MASK_FULL, .hashcode = 0xFFFFFFFF })
#define NM_CORE_TAG_NONE      ((nm_core_tag_t){ .tag = 0, .hashcode = 0x0 })
#define NM_CORE_TAG_INIT_NONE(t) { t.tag = 0 ; t.hashcode = 0x0; }
#define NM_CORE_TAG_INIT_MASK_FULL(t) { t.tag = NM_TAG_MASK_FULL; t.hashcode = 0xFFFFFFFF; }
static inline nm_core_tag_t nm_tag_build(uint32_t hashcode, nm_tag_t tag)
{
  const nm_core_tag_t core_tag = { .tag = tag, .hashcode = hashcode };
  return core_tag;
}
#else /* NM_TAGS_AS_INDIRECT_HASH */
/** An internal tag */
typedef nm_tag_t nm_core_tag_t;
#define NM_CORE_TAG_MASK_FULL NM_TAG_MASK_FULL
#define NM_CORE_TAG_NONE ((nm_tag_t)0)
#define NM_CORE_TAG_INIT_NONE(tag) { tag = 0; }
#define NM_CORE_TAG_INIT_MASK_FULL(tag) { tag = NM_TAG_MASK_FULL; }
static inline nm_core_tag_t nm_tag_build(uint32_t hashcode, nm_tag_t tag)
{
  const nm_core_tag_t core_tag = tag;
  return core_tag;
}
#endif /* NM_TAGS_AS_INDIRECT_HASH */

/** An unpack request */
struct nm_unpack_s
{
  struct tbx_fast_list_head _link;
  nm_status_t status;
  const struct nm_data_s*p_data;
  nm_len_t expected_len;
  nm_len_t cumulated_len;
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  nm_core_tag_t tag_mask;
};

/** A pack request */
struct nm_pack_s
{
  struct tbx_fast_list_head _link;
  nm_status_t status;
  const struct nm_data_s*p_data;
  nm_len_t len;       /**< cumulated data length */
  nm_len_t scheduled; /**< cumulated length of data scheduled for sending */
  nm_len_t done;      /**< cumulated length of data sent so far */
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
};

/** build a pack request from data descriptor */
void nm_core_pack_data(nm_core_t p_core, struct nm_pack_s*p_pack, const struct nm_data_s*p_data);

/** post a pack request */
int nm_core_pack_send(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate, nm_so_flag_t flags);

/** build an unpack request from data descriptor */
void nm_core_unpack_data(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const struct nm_data_s*p_data);

/** post an unpack request */
int nm_core_unpack_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask);

/** cancel a pending unpack
 * @note cancel may fail if matching was already done.
 */
int nm_core_unpack_cancel(struct nm_core*p_core, struct nm_unpack_s*p_unpack);

/** probe unexpected packet, check matching for (packet_tag & tag_mask) == tag */
int nm_core_iprobe(struct nm_core*p_core,
		   struct nm_gate*p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask,
		   struct nm_gate**pp_out_gate, nm_core_tag_t*p_out_tag, nm_len_t*p_out_size);

/** Flush the given gate. */
int nm_core_flush(nm_gate_t p_gate);


/* ** Event notification *********************************** */

/** An event, generated by the NewMad core. */
struct nm_core_event_s
{
  nm_status_t status; /**< status flags- describe the event */
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  nm_len_t len;
  struct nm_unpack_s*p_unpack;
  struct nm_pack_s*p_pack;
};

/** an event notifier, fired upon status transition */
typedef void (*nm_event_notifier_t)(const struct nm_core_event_s* const event);

/** monitor for status transitions */
struct nm_core_monitor_s
{
  nm_event_notifier_t notifier;
  nm_status_t mask;
};
/** Register an event monitor. */
void nm_core_monitor_add(nm_core_t p_core, const struct nm_core_monitor_s*m);
/** Unregister an event monitor. */
void nm_core_monitor_remove(nm_core_t p_core, const struct nm_core_monitor_s*m);


/* ** Progression ****************************************** */

int nm_schedule(nm_core_t p_core);

#if(!defined(PIOMAN))
/* use nmad progression */
#define NMAD_POLL 1
#else
/* use pioman as a progression engine */
#define PIOMAN_POLL 1
#endif

/* @} */

#endif /* NM_CORE_INTERFACE_H */

