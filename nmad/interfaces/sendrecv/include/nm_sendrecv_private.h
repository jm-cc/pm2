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

#ifndef NM_SENDRECV_PRIVATE_H
#define NM_SENDRECV_PRIVATE_H

/** @file This file contains private bits of the sendrecv interface.
 * Though it is not part of the interface, it is included in 
 * nm_sendrecv_interface.h for the sake of inlining.
 */

#include <nm_core_interface.h>
#include <nm_session_private.h>
#ifdef PIOMAN
#include <pioman.h>
#endif

/* ** Polling and locking ********************************** */

#ifdef NMAD_POLL
/** a status with synchronization- PIOMan-less version */
typedef volatile nm_sr_status_t nm_sr_cond_t;
#else
/** a status with synchronization- PIOMan version */
typedef piom_cond_t nm_sr_cond_t;
#endif


/* ** Events *********************************************** */

typedef struct{
  nm_sr_event_t mask;
  nm_sr_event_notifier_t notifier;
} nm_sr_event_monitor_t;

#define NM_SR_EVENT_INIT_MONITOR_NULL(monitor) \
	do {								\
		monitor.mask = (nm_sr_event_t) 0;			\
		monitor.notifier= NULL;					\
	}while(0);							\

#define NM_SR_EVENT_MONITOR_NULL ((nm_sr_event_monitor_t){ .mask = 0, .notifier = NULL })

PUK_VECT_TYPE(nm_sr_event_monitor, nm_sr_event_monitor_t)

/* ** Request ********************************************** */

/** internal defintion of the sendrecv request */
struct nm_sr_request_s
{
  union /* inlined core pack/unpack request to avoid dynamic allocation */
  {
    struct nm_unpack_s unpack;
    struct nm_pack_s pack;
  } req;
  nm_sr_cond_t status;
  nm_sr_event_monitor_t monitor;
  void *ref;
  struct tbx_fast_list_head _link;
};

/* ** Locking inline *************************************** */

#ifdef PIOMAN_POLL
#define nm_sr_status_init(STATUS, BITMASK)       piom_cond_init((STATUS),   (BITMASK))
#define nm_sr_status_test(STATUS, BITMASK)       piom_cond_test((STATUS),   (BITMASK))
#define nm_sr_status_mask(STATUS, BITMASK)       piom_cond_mask((STATUS),   (BITMASK))
#define nm_sr_status_wait(STATUS, BITMASK, CORE) piom_cond_wait((STATUS),   (BITMASK))
static inline void nm_sr_request_signal(nm_sr_request_t*p_request, nm_sr_status_t mask)
{
  piom_cond_signal(&p_request->status, mask);
}
/** Post PIOMan poll requests (except when offloading PIO)
 */
static inline void nm_so_post_all(nm_core_t p_core)
{
}
/** Post PIOMan poll requests even when offloading PIO.
 * This avoids scheduling a tasklet on another lwp
 */
static inline void nm_so_post_all_force(nm_core_t p_core)
{
	/* todo: add a kind of ma_tasklet_schedule here */
//  nm_piom_post_all(p_core);
}
#else /* PIOMAN_POLL */
static inline void  nm_sr_status_init(nm_sr_cond_t*status, nm_sr_status_t bitmask)
{
  *status = bitmask;
}
static inline int  nm_sr_status_test(nm_sr_cond_t*status, nm_sr_status_t bitmask)
{
  return ((*status) & bitmask);
}
static inline void nm_sr_status_mask(nm_sr_cond_t*status, nm_sr_status_t bitmask)
{
  *status &= bitmask;
}
static inline void nm_sr_request_signal(nm_sr_request_t*p_request, nm_sr_status_t mask)
{
  p_request->status |= mask;
}
static inline void nm_sr_status_wait(nm_sr_cond_t*status, nm_sr_status_t bitmask, nm_core_t p_core)
{
  if(status != NULL)
    {
      while(!((*status) & bitmask)) {
	nm_schedule(p_core);
#if(!defined(FINE_GRAIN_LOCKING) && defined(MARCEL))
	if(!((*status) & bitmask)) {
	      marcel_yield();
      }
#endif 
      }
    }
}
static inline void nm_so_post_all(nm_core_t p_core)
{ /* do nothing */ }
static inline void nm_so_post_all_force(nm_core_t p_core)
{ /* do nothing */ }
#endif /* PIOMAN_POLL */


/* ** Requests inline ************************************** */

static inline int nm_sr_get_ref(nm_session_t p_session,
				nm_sr_request_t *p_request,
				void**ref)
{
  *ref = p_request->ref;
  return NM_ESUCCESS;
}

static inline int nm_sr_get_rtag(nm_session_t p_session,
				 nm_sr_request_t *p_request,
				 nm_tag_t*tag)
{
  *tag = p_request->req.unpack.tag;
  return NM_ESUCCESS;
}

static inline int nm_sr_get_stag(nm_session_t p_session,
				 nm_sr_request_t *p_request,
				 nm_tag_t*tag)
{
  *tag = p_request->req.pack.tag;
  return NM_ESUCCESS;
}

static inline int nm_sr_request_set_ref(nm_session_t p_session, nm_sr_request_t*p_request, void*ref)
{
  if(!p_request->ref)
    {
      p_request->ref = ref;
      return NM_ESUCCESS;
    }
  else
    return -NM_EALREADY;
}

static inline int nm_sr_get_size(nm_session_t p_session, nm_sr_request_t *p_request, size_t *size)
{
  *size = p_request->req.unpack.cumulated_len;
  return NM_ESUCCESS;
}

/* ** Send inline ****************************************** */

static inline void nm_sr_send_init(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_sr_status_init(&p_request->status, NM_SR_STATUS_SEND_POSTED);
  NM_SR_EVENT_INIT_MONITOR_NULL(p_request->monitor);
  p_request->ref = NULL;
}
static inline void nm_sr_send_pack_data(nm_session_t p_session, nm_sr_request_t*p_request, 
					const void*data, uint32_t len)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_data(p_core, &p_request->req.pack, data, len);
}
static inline void nm_sr_send_pack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
				       const struct iovec*iov, int num_entries)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_iov(p_core, &p_request->req.pack, iov, num_entries);
}
static inline void nm_sr_send_pack_datatype(nm_session_t p_session, nm_sr_request_t*p_request, 
					    const struct CCSI_Segment*segp)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_datatype(p_core, &p_request->req.pack, segp);
}

static inline int nm_sr_send_isend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const int err = nm_core_pack_send(p_core, &p_request->req.pack, tag, p_gate, 0);
  nm_so_post_all(p_core);
  return err;
}
static inline int nm_sr_send_issend(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const int err = nm_core_pack_send(p_core, &p_request->req.pack, tag, p_gate, NM_PACK_SYNCHRONOUS);
  nm_so_post_all(p_core);
  return err;
}
static inline int nm_sr_send_rsend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const int err = nm_core_pack_send(p_core, &p_request->req.pack, tag, p_gate, 0);
  nm_so_post_all(p_core);
  return err;
}

/* ** Recv inline ****************************************** */

static inline void nm_sr_recv_init(nm_session_t p_session, nm_sr_request_t*p_request)
{ 
  nm_sr_status_init(&p_request->status, NM_SR_STATUS_RECV_POSTED);
  NM_SR_EVENT_INIT_MONITOR_NULL(p_request->monitor);//  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
  p_request->ref = NULL;
}

static inline void nm_sr_recv_unpack_data(nm_session_t p_session, nm_sr_request_t*p_request, 
					  void*data, uint32_t len)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_data(p_core, &p_request->req.unpack, data, len);
}

static inline void nm_sr_recv_unpack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
					 struct iovec*iov, int num_entries)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_iov(p_core, &p_request->req.unpack, iov, num_entries);
}

static inline void nm_sr_recv_unpack_datatype(nm_session_t p_session, nm_sr_request_t*p_request, 
					      struct CCSI_Segment*segp)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_datatype(p_core, &p_request->req.unpack, segp);
}

static inline int  nm_sr_recv_irecv(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag, nm_tag_t mask)
{
  nm_core_t p_core = p_session->p_core;
  const int err = nm_core_unpack_recv(p_core, &p_request->req.unpack, p_gate, tag);
  return err;
}


#endif /* NM_SENDRECV_PRIVATE_H*/

