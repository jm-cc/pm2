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

#ifndef NM_SENDRECV_PRIVATE_H
#define NM_SENDRECV_PRIVATE_H

/** @file 
 * This file contains private bits of the sendrecv interface.
 * Though it is not part of the interface, it is included in 
 * nm_sendrecv_interface.h for the sake of inlining.
 */

#include <nm_core_interface.h>
#include <nm_session_private.h>
#ifdef PIOMAN
#include <pioman.h>
#endif
#ifdef MARCEL
#include <marcel.h>
#endif


/* ** Polling and locking ********************************** */

extern int nm_sr_flush(struct nm_session_s*p_session);


/* ** Events *********************************************** */

/** descriptor for an event monitor */
struct nm_sr_event_monitor_s
{
  nm_sr_event_t mask;               /**< event bitmask */
  nm_sr_event_notifier_t notifier;  /**< notification function to call uppon event*/
};
#define NM_SR_EVENT_MONITOR_NULL ((struct nm_sr_event_monitor_s){ .mask = 0, .notifier = NULL })

/* ** Request ********************************************** */

/** internal defintion of the sendrecv request */
struct nm_sr_request_s
{
  struct nm_req_s req;                  /**< inlined core pack/unpack request to avoid dynamic allocation */
  nm_session_t p_session;               /**< session this request belongs to */
  struct nm_sr_event_monitor_s monitor; /**< events triggered on status transitions */
  void*ref;                             /**< reference usable by end-user */
};

/* ** Requests inline ************************************** */

static inline int nm_sr_request_get_ref(nm_sr_request_t*p_request, void**ref)
{
  *ref = p_request->ref;
  return NM_ESUCCESS;
}

static inline int nm_sr_request_get_session(nm_sr_request_t*p_request, nm_session_t*pp_session)
{
  *pp_session = p_request->p_session;
  return NM_ESUCCESS;
}

static inline int nm_sr_request_get_tag(nm_sr_request_t*p_request, nm_tag_t*tag)
{
  *tag = nm_core_tag_get_tag(p_request->req.tag);
  return NM_ESUCCESS;
}

static inline int nm_sr_request_get_gate(nm_sr_request_t*p_request, nm_gate_t*pp_gate)
{
  *pp_gate = p_request->req.p_gate;
  return NM_ESUCCESS;
}

static inline int nm_sr_request_set_ref(nm_sr_request_t*p_request, void*ref)
{
  if(!p_request->ref)
    {
      p_request->ref = ref;
      return NM_ESUCCESS;
    }
  else
    return -NM_EALREADY;
}

static inline int nm_sr_request_get_size(nm_sr_request_t*p_request, nm_len_t*size)
{
  if(p_request->req.flags & NM_REQ_FLAG_PACK)
    {
      *size = p_request->req.pack.done;
      return NM_ESUCCESS;
    }
  else if(p_request->req.flags & NM_REQ_FLAG_UNPACK)
    {
      *size = p_request->req.unpack.cumulated_len;
      return NM_ESUCCESS;
    }
  else
    return -NM_EINVAL;
}

static inline void nm_sr_request_wait_all(nm_sr_request_t**p_requests, int n)
{
  assert(n >= 1);
  const nm_sr_request_t*p_req = NULL;
  const uintptr_t offset = (uintptr_t)&p_req->req - (uintptr_t)p_req;  /* offset of 'req' in nm_sr_request_t */
  nm_status_wait_multiple((void**)p_requests, n, offset, NM_STATUS_FINALIZED,
			  p_requests[0]->p_session->p_core);
}
static inline int nm_sr_request_test(nm_sr_request_t*p_request, nm_status_t status)
{
  return nm_status_test_allbits(&p_request->req, status);
}
static inline int nm_sr_rwait(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_sr_request_wait(p_request);
  return NM_ESUCCESS;
}
static inline int nm_sr_swait(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_sr_request_wait(p_request);
  return NM_ESUCCESS;
}

/* ** Send inline ****************************************** */

static inline void nm_sr_send_init(nm_session_t p_session, nm_sr_request_t*p_request)
{
  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
  p_request->ref = NULL;
  p_request->p_session = p_session;
}
static inline void nm_sr_send_pack_data(nm_session_t p_session, nm_sr_request_t*p_request, const struct nm_data_s*p_data)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_data(p_core, &p_request->req, p_data);
}
static inline void nm_sr_send_pack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
					      const void*ptr, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)ptr, len);
  nm_sr_send_pack_data(p_session, p_request, &data);
}
static inline void nm_sr_send_pack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
				       const struct iovec*v, int n)
{
  struct nm_data_s data;
  nm_data_iov_build(&data, v, n);
  nm_sr_send_pack_data(p_session, p_request, &data);
}

static inline int nm_sr_send_dest(nm_session_t p_session, nm_sr_request_t*p_request,
				  nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_core_tag_build(p_session->hash_code, tag);
  nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, 0);
  return NM_ESUCCESS;
}

static inline void nm_sr_send_set_priority(nm_session_t p_session, nm_sr_request_t*p_request, int priority)
{
  nm_core_pack_set_priority(p_session->p_core, &p_request->req, priority);
}

static inline int nm_sr_send_header(nm_session_t p_session, nm_sr_request_t*p_request, nm_len_t hlen)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_set_hlen(p_core, &p_request->req, hlen);
  return NM_ESUCCESS;
}
static inline int nm_sr_send_submit(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_pack_submit(p_core, &p_request->req);
  return NM_ESUCCESS;
}

static inline int nm_sr_send_isend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_core_tag_build(p_session->hash_code, tag);
  nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, 0);
  nm_core_pack_submit(p_core, &p_request->req);
  return NM_ESUCCESS;
}
static inline int nm_sr_send_issend(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_core_tag_build(p_session->hash_code, tag);
  nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, NM_REQ_FLAG_PACK_SYNCHRONOUS);
  nm_core_pack_submit(p_core, &p_request->req);
  return NM_ESUCCESS;
}
static inline int nm_sr_send_rsend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_core_tag_build(p_session->hash_code, tag);
  nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, 0);
  nm_core_pack_submit(p_core, &p_request->req);
  return NM_ESUCCESS;
}

/* ** Recv inline ****************************************** */

static inline void nm_sr_recv_init(nm_session_t p_session, nm_sr_request_t*p_request)
{ 
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_init(p_core, &p_request->req);
  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
  p_request->ref = NULL;
  p_request->p_session = p_session;
}

static inline void nm_sr_recv_unpack_data(nm_session_t p_session, nm_sr_request_t*p_request, const struct nm_data_s*p_data)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_data(p_core, &p_request->req, p_data);
}

static inline void nm_sr_recv_unpack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
						void*ptr, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_contiguous_build(&data, ptr, len);
  nm_sr_recv_unpack_data(p_session, p_request, &data);
}

static inline void nm_sr_recv_unpack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
					 const struct iovec*v, int n)
{
  struct nm_data_s data;
  nm_data_iov_build(&data, v, n);
  nm_sr_recv_unpack_data(p_session, p_request, &data);
}

static inline void nm_sr_recv_irecv(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag, nm_tag_t mask)
{
  nm_sr_recv_match(p_session, p_request, p_gate, tag, mask);
  nm_sr_recv_post(p_session, p_request);
}

static inline void nm_sr_recv_match(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag, nm_tag_t mask)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_core_tag_build(p_session->hash_code, tag);
  const nm_core_tag_t core_mask = nm_core_tag_build(NM_CORE_TAG_HASH_FULL, mask);
  nm_core_unpack_match_recv(p_core, &p_request->req, p_gate, core_tag, core_mask);
}

static inline void nm_sr_recv_match_event(nm_session_t p_session, nm_sr_request_t*p_request,
					  const nm_sr_event_info_t*p_event)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_match_event(p_core, &p_request->req, p_event->recv_unexpected.p_core_event);
}

static inline void nm_sr_recv_post(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_unpack_submit(p_core, &p_request->req, NM_REQ_FLAG_NONE);
}

static inline int nm_sr_recv_peek(nm_session_t p_session, nm_sr_request_t*p_request,
				  const struct nm_data_s*p_data)
{
  nm_core_t p_core = p_session->p_core;
  int err = nm_core_unpack_peek(p_core, &p_request->req, p_data);
  return err;
}

static inline int nm_sr_recv_iprobe(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_core_t p_core = p_session->p_core;
  int err = nm_core_unpack_iprobe(p_core, &p_request->req);
  return err;
}

#endif /* NM_SENDRECV_PRIVATE_H*/

