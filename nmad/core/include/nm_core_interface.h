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

/* ** This is the interface of the NewMad core. All other
 * include files in the core are internal and shouldn't be
 * included outside of the core.
 *
 * End-users are not supposed to use directly this core
 * interface. They should use instead a higher level interface
 * such as 'sendrecv', 'pack', or 'mpi' for messaging, and 'session'
 * or 'launcher' for init and connection establishment.
 *
 */


/* don't include pm2_common.h or tbx.h here.
 * They are not needed and not ISO C compliant.
 */
#include <nm_public.h>
#include <Padico/Puk.h>
#include <tbx_fast_list.h>

#include <sys/uio.h>


/* ** Core init ******************************************** */

typedef struct nm_core*nm_core_t;

puk_component_t nm_core_component_load(const char*entity, const char*name);

int nm_core_init(int*argc, char*argv[], nm_core_t *pp_core);

int nm_core_set_strategy(nm_core_t p_core, puk_component_t strategy);

int nm_core_exit(nm_core_t p_core);


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

int nm_core_driver_load(nm_core_t p_core, puk_component_t driver, nm_drv_t*pp_drv);

int nm_core_driver_init(nm_core_t p_core, nm_drv_t p_drv, const char**p_url);

int nm_core_driver_query(nm_core_t p_core, nm_drv_t p_drv, struct nm_driver_query_param *param);


int nm_core_driver_load_init(nm_core_t p_core, puk_component_t driver,
			     struct nm_driver_query_param*param,
			     nm_drv_t *pp_drv, const char**p_url);


/* ** Gates ************************************************ */

int nm_core_gate_init(nm_core_t p_core, nm_gate_t *pp_gate);

int nm_core_gate_accept(nm_core_t p_core, nm_gate_t p_gate, nm_drv_t p_drv, const char *url);

int nm_core_gate_connect(nm_core_t p_core, nm_gate_t gate, nm_drv_t  p_drv, const char *url);


/* ** Packs/unpacks **************************************** */

/** status of a pack/unpack request */
typedef uint16_t nm_status_t;

/** pack/unpack flags
 * @note for now, flags are included in status */
typedef nm_status_t nm_so_flag_t;

/* ** status and flags, used in pack/unpack requests and events */

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
#else
/** An internal tag */
typedef nm_tag_t nm_core_tag_t;
#define NM_CORE_TAG_MASK_FULL NM_TAG_MASK_FULL
#define NM_CORE_TAG_NONE ((nm_tag_t)0)
#define NM_CORE_TAG_INIT_NONE(tag) { tag = 0; }
#define NM_CORE_TAG_INIT_MASK_FULL(tag) { tag = NM_TAG_MASK_FULL; }

#endif /* NM_TAGS_AS_INDIRECT_HASH */

/** An unpack request */
struct nm_unpack_s
{
  nm_status_t status;
  void *data;
  int expected_len;
  int cumulated_len;
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
  nm_core_tag_t tag_mask;
};

/** A pack request */
struct nm_pack_s
{
  nm_status_t status;
  void*data; /**< actually, char*, struct iovec*, or DLOOP_Segment* depending on pack type (see status) */
  int len;       /**< cumulated data length */
  int scheduled; /**< cumulated length of data scheduled for sending */
  int done;      /**< cumulated length of data sent so far */
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
};

static inline void nm_core_pack_data(nm_core_t p_core, struct nm_pack_s*p_pack,
				     const void*data, uint32_t len)
{
  p_pack->status = NM_PACK_TYPE_CONTIGUOUS;
  p_pack->data   = (void*)data;
  p_pack->len    = len;
  p_pack->done   = 0;
  p_pack->scheduled = 0;
}

void nm_core_pack_iov(nm_core_t p_core, struct nm_pack_s*p_pack, const struct iovec*iov, int num_entries);

void nm_core_pack_datatype(nm_core_t p_core, struct nm_pack_s*p_pack, const struct CCSI_Segment *segp);

int nm_core_pack_send(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate, nm_so_flag_t flags);

static inline void nm_core_unpack_data(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
				       void *data, uint32_t len)
{
  p_unpack->status = NM_UNPACK_TYPE_CONTIGUOUS;
  p_unpack->data = data;
  p_unpack->cumulated_len = 0;
  p_unpack->expected_len = len;
}

void nm_core_unpack_iov(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct iovec*iov, int num_entries);

void nm_core_unpack_datatype(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct CCSI_Segment*segp);

int nm_core_unpack_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask);

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack);

int nm_so_iprobe(struct nm_core*p_core, struct nm_gate*p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask, struct nm_gate**pp_out_gate);



/* ** Event notification *********************************** */

/** An event, generated by the NewMad core. */
struct nm_so_event_s
{
  nm_status_t status;
  nm_gate_t p_gate;
  nm_core_tag_t tag;
  nm_seq_t seq;
  uint32_t len;
  struct nm_unpack_s*p_unpack;
  struct nm_pack_s*p_pack;
};

/** an event notifier, fired upon status transition */
typedef void (*nm_event_notifier_t)(const struct nm_so_event_s* const event);
/** monitor for status transitions */
struct nm_so_monitor_s
{
  nm_event_notifier_t notifier;
  nm_status_t mask;
};
/** Add an event monitor. */
void nm_so_monitor_add(nm_core_t p_core, const struct nm_so_monitor_s*m);


/* ** Progression ****************************************** */

int nm_schedule(nm_core_t p_core);

#if(!defined(PIOMAN) || defined(PIOM_POLLING_DISABLED))
/* use nmad progression */
#define NMAD_POLL 1
#else
/* use pioman as a progression engine */
#define PIOMAN_POLL 1
#endif


/* The following functions are deprecated- don't use! */

#ifdef PIOMAN_POLL
int nm_core_disable_progression(struct nm_core*p_core);

int nm_core_enable_progression(struct nm_core*p_core);

#else /* PIOMAN_POLL */
static inline int nm_core_disable_progression(struct nm_core*p_core)
{
  return 0;
}
static inline int nm_core_enable_progression(struct nm_core*p_core)
{
  return 0;
}
#endif /* PIOMAN_POLL */



#endif /* NM_CORE_INTERFACE_H */

