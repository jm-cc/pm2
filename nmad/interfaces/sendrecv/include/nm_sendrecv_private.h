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

/* ** Sessions inline ************************************** */

static inline nm_session_t nm_sr_session_open(const char*label)
{
  nm_session_t p_session = NULL;
  int rc = nm_session_open(&p_session, label);
  if(rc != NM_ESUCCESS)
    {
      fprintf(stderr, "# nmad: WARNING- cannot open session *%s*; already in use.\n", label);
      return NULL;
    }
  nm_sr_init(p_session);
  return p_session;
}

static inline void nm_sr_session_close(nm_session_t p_session)
{
  nm_sr_exit(p_session);
  nm_session_destroy(p_session);
}

/* ** Polling and locking ********************************** */

extern int nm_sr_flush(struct nm_core *p_core);


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
  struct nm_req_s req; /**< inlined core pack/unpack request to avoid dynamic allocation */
  struct nm_data_s data;
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
  *tag = nm_tag_get(p_request->req.tag);
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
  if(p_request->req.flags & NM_FLAG_PACK)
    {
      *size = p_request->req.pack.done;
      return NM_ESUCCESS;
    }
  else if(p_request->req.flags & NM_FLAG_UNPACK)
    {
      *size = p_request->req.unpack.cumulated_len;
      return NM_ESUCCESS;
    }
  else
    return -NM_EINVAL;
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
  p_request->data = *p_data;
  nm_core_pack_data(p_core, &p_request->req, &p_request->data);
}
static inline void nm_sr_send_pack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
					      const void*ptr, nm_len_t len)
{
  nm_core_t p_core = p_session->p_core;
  nm_data_contiguous_set(&p_request->data, (struct nm_data_contiguous_s){ .ptr = (void*)ptr, .len = len });
  nm_core_pack_data(p_core, &p_request->req, &p_request->data);
}
static inline void nm_sr_send_pack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
				       const struct iovec*iov, int num_entries)
{
  nm_core_t p_core = p_session->p_core;
  nm_data_iov_set(&p_request->data, (struct nm_data_iov_s){ .v = (struct iovec*)iov, .n = num_entries });
  nm_core_pack_data(p_core, &p_request->req, &p_request->data);
}

static inline int nm_sr_send_isend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_tag_build(p_session->hash_code, tag);
  const int err = nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, 0);
  return err;
}
static inline int nm_sr_send_issend(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_tag_build(p_session->hash_code, tag);
  const int err = nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, NM_FLAG_PACK_SYNCHRONOUS);
  return err;
}
static inline int nm_sr_send_rsend(nm_session_t p_session, nm_sr_request_t*p_request,
				   nm_gate_t p_gate, nm_tag_t tag)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_tag_build(p_session->hash_code, tag);
  const int err = nm_core_pack_send(p_core, &p_request->req, core_tag, p_gate, 0);
  return err;
}

/* ** Recv inline ****************************************** */

static inline void nm_sr_recv_init(nm_session_t p_session, nm_sr_request_t*p_request)
{ 
  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
  p_request->ref = NULL;
  p_request->p_session = p_session;
}

static inline void nm_sr_recv_unpack_contiguous(nm_session_t p_session, nm_sr_request_t*p_request, 
						void*data, nm_len_t len)
{
  nm_core_t p_core = p_session->p_core;
  nm_data_contiguous_set(&p_request->data, (struct nm_data_contiguous_s){ .ptr = (void*)data, .len = len });
  nm_core_unpack_data(p_core, &p_request->req, &p_request->data);
}

static inline void nm_sr_recv_unpack_iov(nm_session_t p_session, nm_sr_request_t*p_request,
					 struct iovec*iov, int num_entries)
{
  nm_core_t p_core = p_session->p_core;
  nm_data_iov_set(&p_request->data, (struct nm_data_iov_s){ .v = (struct iovec*)iov, .n = num_entries });
  nm_core_unpack_data(p_core, &p_request->req, &p_request->data);
}

static inline void nm_sr_recv_unpack_data(nm_session_t p_session, nm_sr_request_t*p_request, const struct nm_data_s*p_data)
{
  nm_core_t p_core = p_session->p_core;
  p_request->data = *p_data;
  nm_core_unpack_data(p_core, &p_request->req, &p_request->data);
}

static inline int  nm_sr_recv_irecv(nm_session_t p_session, nm_sr_request_t*p_request,
				    nm_gate_t p_gate, nm_tag_t tag, nm_tag_t mask)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_tag_build(p_session->hash_code, tag);
  const nm_core_tag_t core_mask = nm_tag_build(p_session->hash_code, mask);
  const int err = nm_core_unpack_recv(p_core, &p_request->req, p_gate, core_tag, core_mask);
  return err;
}


#endif /* NM_SENDRECV_PRIVATE_H*/

