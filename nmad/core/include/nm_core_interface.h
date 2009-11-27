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


/* ** Core init ******************************************** */

typedef struct nm_core*nm_core_t;

puk_component_t nm_core_component_load(const char*entity, const char*name);

int nm_core_init(int*argc, char*argv[], nm_core_t *pp_core);

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

int nm_core_driver_query(nm_core_t p_core, nm_drv_t p_drv, struct nm_driver_query_param *params, int nparam);

int nm_core_driver_load_init_some_with_params(nm_core_t p_core,
					      int count,
					      puk_component_t*driver_array,
					      struct nm_driver_query_param **params_array,
					      int *nparam_array,
					      nm_drv_t *p_array,
					      const char **p_url_array);

static inline int nm_core_driver_load_init(nm_core_t p_core, puk_component_t driver,
					   nm_drv_t *pp_drv, const char**p_url)
{
  struct nm_driver_query_param * params_array[1] = { NULL };
  int nparam_array[1] = { 0 };
  return nm_core_driver_load_init_some_with_params(p_core, 1, &driver,
						   params_array, nparam_array, pp_drv, p_url);
}

/* ** Gates ************************************************ */

int nm_core_gate_init(nm_core_t p_core, nm_gate_t *pp_gate);

int nm_core_gate_accept(nm_core_t p_core, nm_gate_t p_gate, nm_drv_t p_drv, const char *url);

int nm_core_gate_connect(nm_core_t p_core, nm_gate_t gate, nm_drv_t  p_drv, const char *url);


/* ** Packs/unpacks **************************************** */

/** status of a pack/unpack request */
typedef uint16_t nm_so_status_t;

/** pack/unpack flags
 * @note for now, flags are included in status */
typedef nm_so_status_t nm_so_flag_t;

/* ** status and flags, used in pack/unpack requests and events */

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

/** Sequence number */
typedef uint16_t nm_seq_t;

/** An unpack request */
struct nm_unpack_s
{
  nm_so_status_t status;
  void *data;
  int expected_len;
  int cumulated_len;
  nm_gate_t p_gate;
  nm_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
};

/** A pack request */
struct nm_pack_s
{
  nm_so_status_t status;
  void*data; /**< actually, char*, struct iovec*, or DLOOP_Segment* depending on pack type (see status) */
  int len;   /**< cumulated data length */
  int done;
  nm_gate_t p_gate;
  nm_tag_t tag;
  nm_seq_t seq;
  struct tbx_fast_list_head _link;
};

int nm_so_pack(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_tag_t tag, nm_gate_t p_gate,
	       const void*data, uint32_t len, nm_so_flag_t pack_type);

int nm_so_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_tag_t tag,
		 nm_so_flag_t flag, void *data_description, uint32_t len);

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack);

int nm_so_iprobe(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_gate**pp_out_gate, nm_tag_t tag);


/* temporary hack- this stuff will move in nm_schedule_in/out.c */
#include <sys/uio.h>
#include <ccs_public.h>
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

/* ** Event notification *********************************** */

/** An event, generated by the NewMad core. */
struct nm_so_event_s
{
  nm_so_status_t status;
  nm_gate_t p_gate;
  nm_tag_t tag;
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
  nm_so_status_t mask;
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

