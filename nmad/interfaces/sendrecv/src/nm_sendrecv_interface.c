/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_private.h>

PADICO_MODULE_HOOK(NewMad_Core);


PUK_LFQUEUE_TYPE(nm_sr_request, struct nm_sr_request_s*, NULL, 1024);

/** per-session status for sendrecv interface */
struct nm_sr_session_s
{
  struct nm_sr_request_lfqueue_s completed_rreq; /**< queue of completed recv requests */
  struct nm_sr_request_lfqueue_s completed_sreq; /**< queue of completed send requests */
  struct nm_core_monitor_s monitor;              /**< monitor unexpected messages */
  nm_sr_event_notifier_t unexpected;             /**< user callback for unexpected messages */
};

/* ** Events *********************************************** */

static void nm_sr_event_req_completed(const struct nm_core_event_s*const event);
static void nm_sr_event_unexpected(const struct nm_core_event_s*const event);

static const struct nm_core_monitor_s nm_sr_monitor_req_completed = 
  {
    .notifier = &nm_sr_event_req_completed,
    .mask     = NM_STATUS_PACK_COMPLETED | NM_STATUS_ACK_RECEIVED |
                NM_STATUS_UNPACK_COMPLETED | NM_STATUS_UNPACK_CANCELLED,
    .matching = { .p_gate = NM_ANY_GATE, .tag = { 0 }, .tag_mask = { 0 } }
  };

/* User interface */

int nm_sr_init(nm_session_t p_session)
{
  nm_core_t p_core = p_session->p_core;
  if(p_session->ref != NULL)
    {
      fprintf(stderr, "nmad: FATAL- sendrecv cannot use session %p; ref is non-empty.\n", p_session);
      abort();
    }
  struct nm_sr_session_s*p_sr_session = malloc(sizeof(struct nm_sr_session_s));
  nm_sr_request_lfqueue_init(&p_sr_session->completed_rreq);
  nm_sr_request_lfqueue_init(&p_sr_session->completed_sreq);
  p_sr_session->monitor = NM_CORE_MONITOR_NULL;
  p_sr_session->unexpected = NULL;
  p_session->ref = p_sr_session;
  return NM_ESUCCESS;
}

int nm_sr_exit(nm_session_t p_session)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  assert(p_sr_session != NULL);
  if(p_sr_session->monitor.notifier != NULL)
    {
      nm_core_monitor_remove(p_session->p_core, &p_sr_session->monitor);
    }
  free(p_sr_session);
  p_session->ref = NULL;
  return NM_ESUCCESS;
}

/** Test for the completion of a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
int nm_sr_stest(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
  int rc = NM_ESUCCESS;
#ifdef DEBUG
  if(!nm_status_test(&p_request->req, NM_STATUS_PACK_POSTED))
    {
      rc = -NM_ENOTPOSTED;
    }
  else
#endif
  if(nm_status_testall(&p_request->req, NM_STATUS_PACK_COMPLETED | NM_STATUS_FINALIZED))
    {
      rc = NM_ESUCCESS;
    }
  else
    {
      nm_sr_progress(p_session);
      rc = (nm_status_testall(&p_request->req, NM_STATUS_PACK_COMPLETED | NM_STATUS_FINALIZED)) ?
	NM_ESUCCESS : -NM_EAGAIN;
    }
  return rc;
}

extern int nm_sr_flush(struct nm_core *p_core)
{
  int ret = NM_EAGAIN;
  struct nm_gate*p_gate = NULL;
  nmad_lock();
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  ret = nm_core_flush(p_gate);
	}
    } 
  nmad_unlock();
  return ret;
}

int nm_sr_swait(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
#ifdef DEBUG
  if(!nm_status_test(&p_request->req, NM_STATUS_PACK_POSTED))
    TBX_FAILUREF("nm_sr_swait- req=%p no send posted!\n", p_request);
#endif /* DEBUG */
  if(!nm_status_testall(&p_request->req, NM_STATUS_PACK_COMPLETED | NM_STATUS_FINALIZED))
    {
      nm_sr_flush(p_core);
      nm_status_wait(&p_request->req, NM_STATUS_PACK_COMPLETED, p_core);
      nm_status_spinwait(&p_request->req, NM_STATUS_FINALIZED);
    }
  return NM_ESUCCESS;
}

int nm_sr_scancel(nm_session_t p_session, nm_sr_request_t *p_request) 
{
  return -NM_ENOTIMPL;
}

/* Receive operations */

/** Test for the completion of a non blocking receive request.
 *  @param p_core a pointer to the NewMad core object.
 *  @param request the request to check.
 *  @return The NM status.
 */
int nm_sr_rtest(nm_session_t p_session, nm_sr_request_t *p_request) 
{
  int rc = NM_ESUCCESS;
#ifdef DEBUG
  if(!nm_status_test(&p_request->req, NM_STATUS_UNPACK_POSTED))
    {
      rc = -NM_ENOTPOSTED;
    }
  else
#endif
  if(nm_status_testall(&p_request->req, NM_STATUS_UNPACK_COMPLETED | NM_STATUS_FINALIZED))
    {
      rc = NM_ESUCCESS;
    }
  else if(nm_status_test(&p_request->req, NM_STATUS_UNPACK_CANCELLED))
    {
      rc = -NM_ECANCELED;
    }
  else
    {
      nm_sr_progress(p_session);
      rc = -NM_EAGAIN;
    }
  return rc;
}

int nm_sr_rwait(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
#ifdef DEBUG
  if(!nm_status_test(&p_request->req, NM_STATUS_UNPACK_POSTED))
    TBX_FAILUREF("nm_sr_rwait- req=%p no recv posted!\n", p_request);
#endif /* DEBUG */
  int rc = nm_sr_rtest(p_session, p_request);
  if(rc != NM_ESUCCESS)
    {
      nm_sr_flush(p_core);
      nm_status_wait(&p_request->req, NM_STATUS_UNPACK_COMPLETED | NM_STATUS_UNPACK_CANCELLED, p_core);
      nm_status_spinwait(&p_request->req, NM_STATUS_FINALIZED);
      rc = nm_sr_rtest(p_session, p_request);
    }
  return rc;
}

int nm_sr_recv_source(nm_session_t p_session, nm_sr_request_t *p_request, nm_gate_t *pp_gate)
{
  if(pp_gate)
    *pp_gate = p_request->req.p_gate;
  return NM_ESUCCESS;
}

int nm_sr_probe(nm_session_t p_session,
		nm_gate_t p_gate, nm_gate_t *pp_out_gate,
		nm_tag_t tag, nm_tag_t mask, nm_tag_t*p_out_tag,
		nm_len_t*p_out_len)
{
  nm_core_t p_core = p_session->p_core;
  const nm_core_tag_t core_tag = nm_tag_build(p_session->hash_code, tag);
  const nm_core_tag_t core_mask = nm_tag_build(p_session->hash_code, mask);

  nm_lock_interface(p_core);
  nm_lock_status(p_core);
  nm_gate_t p_out_gate = NM_GATE_NONE;
  nm_core_tag_t out_core_tag = NM_CORE_TAG_NONE;
  nm_len_t out_size = 0;
  int err = nm_core_iprobe(p_core, p_gate, core_tag, core_mask, &p_out_gate, &out_core_tag, &out_size);
  nm_unlock_status(p_core);
  nm_unlock_interface(p_core);

  if(pp_out_gate)
    *pp_out_gate = p_out_gate;
  if(p_out_tag)
    *p_out_tag = nm_tag_get(out_core_tag);
  if(p_out_len)
    *p_out_len = out_size;

  if(err != NM_ESUCCESS)
    {
      nm_sr_progress(p_session);
    }
  return err;
}

int nm_sr_unexpected(nm_session_t p_session, nm_sr_event_notifier_t notifier)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  p_sr_session->monitor = (struct nm_core_monitor_s)
    {
      .notifier = &nm_sr_event_unexpected,
      .mask     = NM_STATUS_UNEXPECTED,
      .matching = { .p_gate   = NM_ANY_GATE,
		    .tag      = nm_tag_build(p_session->hash_code, 0),
		    .tag_mask = nm_tag_build(NM_CORE_TAG_HASH_FULL, 0)
      }
    };
  p_sr_session->unexpected = notifier;
  nmad_lock();
  nm_core_monitor_add(p_session->p_core, &p_sr_session->monitor);
  nmad_unlock();
  return NM_ESUCCESS;
}

int nm_sr_monitor(nm_session_t p_session, nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  p_sr_session->monitor = (struct nm_core_monitor_s)
    {
      .notifier = &nm_sr_event_unexpected,
      .mask     = mask,
      .matching = { .p_gate   = NM_ANY_GATE,
		    .tag      = nm_tag_build(p_session->hash_code, 0),
		    .tag_mask = nm_tag_build(NM_CORE_TAG_HASH_FULL, 0)
      }
    };
  p_sr_session->unexpected = notifier;
  /* sanity check */
  if(mask == NM_STATUS_UNEXPECTED)
    {
      int rc = nm_sr_probe(p_session, NM_GATE_NONE, NULL, 0, NM_TAG_MASK_NONE, NULL, NULL);
      if(rc == NM_ESUCCESS)
	{
	  fprintf(stderr, "# nmad: WARNING- set UNEXPECTED monitor with pending unexpected messages.\n");
	}
    }
  nmad_lock();
  nm_core_monitor_add(p_session->p_core, &p_sr_session->monitor);
  nmad_unlock();
  return NM_ESUCCESS;
}

int nm_sr_request_monitor(nm_session_t p_session, nm_sr_request_t *p_request,
			  nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  p_request->monitor.mask = mask;
  p_request->monitor.notifier = notifier;
  nm_core_req_monitor(&p_request->req, nm_sr_monitor_req_completed);
  return NM_ESUCCESS;
}

static void nm_sr_completion_enqueue(nm_sr_event_t event, const nm_sr_event_info_t*event_info)
{
  if(event & NM_SR_STATUS_RECV_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->recv_completed.p_request;
      struct nm_sr_session_s*p_sr_session = p_request->p_session->ref;
      int rc = nm_sr_request_lfqueue_enqueue(&p_sr_session->completed_rreq, p_request);
      if(rc != 0)
	abort();
    }
  else if(event & NM_SR_STATUS_SEND_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->send_completed.p_request;
      struct nm_sr_session_s*p_sr_session = p_request->p_session->ref;
      int rc = nm_sr_request_lfqueue_enqueue(&p_sr_session->completed_sreq, p_request);
      if(rc != 0)
	abort();
    }
}

int nm_sr_request_set_completion_queue(nm_session_t p_session, nm_sr_request_t*p_request)
{
  assert(p_request->monitor.notifier == NULL);
  nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_SEND_COMPLETED | NM_SR_EVENT_RECV_COMPLETED,
			&nm_sr_completion_enqueue);
  return NM_ESUCCESS;
}

int nm_sr_request_unset_completion_queue(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_lock_interface(p_session->p_core);
  nm_lock_status(p_session->p_core);
  nm_sr_request_monitor(p_session, p_request, 0, NULL);
  nm_unlock_status(p_session->p_core);
  nm_unlock_interface(p_session->p_core);
  return NM_ESUCCESS;
}


int nm_sr_recv_success(nm_session_t p_session, nm_sr_request_t**out_req)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  if(nm_sr_request_lfqueue_empty(&p_sr_session->completed_rreq))
    {
      nm_sr_progress(p_session);
    }
  nm_sr_request_t*p_request = nm_sr_request_lfqueue_dequeue(&p_sr_session->completed_rreq);
  *out_req = p_request;
  if(p_request)
    {
      nm_status_spinwait(&p_request->req, NM_STATUS_FINALIZED);
    }
  return (p_request != NULL) ? NM_ESUCCESS : -NM_EAGAIN;
}

int nm_sr_send_success(nm_session_t p_session, nm_sr_request_t**out_req)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  if(nm_sr_request_lfqueue_empty(&p_sr_session->completed_sreq))
    {
      nm_sr_progress(p_session);
    }
  nm_sr_request_t*p_request = nm_sr_request_lfqueue_dequeue(&p_sr_session->completed_sreq);
  *out_req = p_request;
  if(p_request)
    {
      nm_status_spinwait(&p_request->req, NM_STATUS_FINALIZED);
    }
  return (p_request != NULL) ? NM_ESUCCESS : -NM_EAGAIN;
}

/** cancel a receive request.
 * @return error code:
 *   NM_ESUCCESS    operaton successfully canceled
 *  -NM_ENOTIMPL    case where cancellation is not supported yet
 *  -NM_EINPROGRESS receipt is in progress, it is too late to cancel
 *  -NM_EALREADY    receipt is already completed
 *  -NM_CANCELED    receipt was already canceled
 */
int nm_sr_rcancel(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
  int err = -NM_ENOTIMPL;

  nmad_lock();
  nm_lock_interface(p_core);
  nm_lock_status(p_core);

  if(nm_status_test(&p_request->req, NM_STATUS_UNPACK_COMPLETED))
    {
      err = -NM_EALREADY;
    }
  else if(nm_status_test(&p_request->req, NM_STATUS_UNPACK_CANCELLED))
    {
      err = -NM_ECANCELED;
    }
  else
    {
      err = nm_core_unpack_cancel(p_core, &p_request->req);
    }
  nm_unlock_status(p_core);
  nm_unlock_interface(p_core);
  nmad_unlock();

  return err;
}

static void nm_sr_event_req_completed(const struct nm_core_event_s*const event)
{
  struct nm_req_s*p_req = event->p_req;
  struct nm_sr_request_s*p_request = tbx_container_of(p_req, struct nm_sr_request_s, req);
  if(p_req->flags & NM_FLAG_PACK)
    {
      if( ((p_req->flags & NM_FLAG_PACK_SYNCHRONOUS) && (event->status & NM_STATUS_ACK_RECEIVED)) ||
	  ((!(p_req->flags & NM_FLAG_PACK_SYNCHRONOUS)) && (event->status & NM_STATUS_PACK_COMPLETED)))
	{
	  const nm_sr_event_info_t info = { .send_completed.p_request = p_request };
	  if(p_request && (event->status & p_request->monitor.mask) && p_request->monitor.notifier)
	    {
	      nmad_unlock();
	      (*p_request->monitor.notifier)(NM_SR_STATUS_SEND_COMPLETED, &info);
	      nmad_lock();

	    }
	}
    }
  else if(p_req->flags & NM_FLAG_UNPACK)
    {
      const nm_sr_event_info_t info =
	{ 
	  .recv_completed.p_request = p_request,
	  .recv_completed.p_gate = p_req->p_gate
	};
      if(p_request && (event->status & p_request->monitor.mask) && p_request->monitor.notifier)
	{
	  nmad_unlock();
	  (*p_request->monitor.notifier)(NM_SR_STATUS_RECV_COMPLETED, &info);
	  nmad_lock();
	}
    }
  else
    {
      abort();
    }
}

static void nm_sr_event_unexpected(const struct nm_core_event_s*const event)
{
  const uint32_t hashcode = nm_tag_get_hashcode(event->tag);
  nm_session_t p_session = nm_session_lookup(hashcode);
  assert(p_session != NULL);
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  assert(p_sr_session != NULL);
  const nm_tag_t sr_tag = nm_tag_get(event->tag);
  const nm_sr_event_info_t info =
    { 
      .recv_unexpected.p_gate    = event->p_gate,
      .recv_unexpected.tag       = sr_tag,
      .recv_unexpected.len       = event->len,
      .recv_unexpected.p_session = p_session
    };
  nmad_unlock();
  (*p_sr_session->unexpected)(NM_SR_EVENT_RECV_UNEXPECTED, &info);
  nmad_lock();
}

int nm_sr_progress(nm_session_t p_session)
{
  /* either explicit schedule, or PIOMan makes communications progress */
  nm_schedule(p_session->p_core);
  return NM_ESUCCESS;
}
