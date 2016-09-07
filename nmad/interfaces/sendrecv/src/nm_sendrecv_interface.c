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
  struct nm_core_monitor_vect_s core_monitors;   /**< core monitors for the session; sr_monitor stored in ref */
};

/* ** Events *********************************************** */

static void nm_sr_event_req_handler(const struct nm_core_event_s*const event, void*_ref);
static void nm_sr_event_handler(const struct nm_core_event_s*const event, void*_ref);

/* User interface */

int nm_sr_init(nm_session_t p_session)
{
  if(p_session->ref != NULL)
    {
      fprintf(stderr, "nmad: FATAL- sendrecv cannot use session %p; ref is non-empty.\n", p_session);
      abort();
    }
  struct nm_sr_session_s*p_sr_session = malloc(sizeof(struct nm_sr_session_s));
  nm_sr_request_lfqueue_init(&p_sr_session->completed_rreq);
  nm_sr_request_lfqueue_init(&p_sr_session->completed_sreq);
  nm_core_monitor_vect_init(&p_sr_session->core_monitors);
  p_session->ref = p_sr_session;
  return NM_ESUCCESS;
}

int nm_sr_exit(nm_session_t p_session)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  assert(p_sr_session != NULL);
  while(!nm_core_monitor_vect_empty(&p_sr_session->core_monitors))
    {
      const struct nm_core_monitor_s*p_core_monitor = *nm_core_monitor_vect_begin(&p_sr_session->core_monitors);
      const struct nm_sr_monitor_s*p_sr_monitor = p_core_monitor->monitor.ref;
      nm_sr_session_monitor_remove(p_session, p_sr_monitor);
    }
  nm_core_monitor_vect_destroy(&p_sr_session->core_monitors);
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
  nm_gate_t p_gate = NULL;
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

int nm_sr_swait(nm_session_t p_session, nm_sr_request_t*p_request)
{
  nm_core_t p_core = p_session->p_core;
#ifdef DEBUG
  if(!nm_status_test(&p_request->req, NM_STATUS_PACK_POSTED))
    NM_FATAL("nm_sr_swait- req=%p no send posted!\n", p_request);
  if(p_request->monitor.mask & NM_SR_EVENT_FINALIZED)
    NM_FATAL("nm_sr_swait- req=%p has monitor for EVENT_FINALIZED; cannot swait.\n", p_request);
#endif /* DEBUG */
  const nm_status_t status = (p_request->req.flags & NM_FLAG_PACK_SYNCHRONOUS) ?
    (NM_STATUS_PACK_COMPLETED | NM_STATUS_ACK_RECEIVED) : (NM_STATUS_PACK_COMPLETED);
  if(!nm_status_testall(&p_request->req, status | NM_STATUS_FINALIZED))
    {
      nm_sr_flush(p_core);
      if(p_request->req.flags & NM_FLAG_PACK_SYNCHRONOUS)
	{
	  nm_status_wait(&p_request->req, NM_STATUS_ACK_RECEIVED, p_core);
	  while(!nm_status_test(&p_request->req, NM_STATUS_PACK_COMPLETED))
	    {
	      nm_sr_progress(p_session);
	    }
	}
      else
	{
	  nm_status_wait(&p_request->req, NM_STATUS_PACK_COMPLETED, p_core);
	}
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
    NM_FATAL("nm_sr_rwait- req=%p no recv posted!\n", p_request);
  if(p_request->monitor.mask & NM_SR_EVENT_FINALIZED)
    NM_FATAL("nm_sr_rwait- req=%p has monitor for EVENT_FINALIZED; cannot rwait.\n", p_request);
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

int nm_sr_session_monitor_set(nm_session_t p_session, const struct nm_sr_monitor_s*p_sr_monitor)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  struct nm_core_monitor_s*p_core_monitor = malloc(sizeof(struct nm_core_monitor_s));
  p_core_monitor->monitor = (struct nm_monitor_s)
    {
      .notifier = &nm_sr_event_handler,
      .mask     = p_sr_monitor->event_mask,
      .ref      = (void*)p_sr_monitor
    };
  p_core_monitor->matching = (struct nm_core_event_matching_s)
    {
      .p_gate   = p_sr_monitor->p_gate,
      .tag      = nm_tag_build(p_session->hash_code, p_sr_monitor->tag),
      .tag_mask = nm_tag_build(NM_CORE_TAG_HASH_FULL, p_sr_monitor->tag_mask)
    };
  nmad_lock();
  nm_core_monitor_vect_push_back(&p_sr_session->core_monitors, p_core_monitor);
  nm_core_monitor_add(p_session->p_core, p_core_monitor);
  nmad_unlock();
  return NM_ESUCCESS;
}

int nm_sr_session_monitor_remove(nm_session_t p_session, const struct nm_sr_monitor_s*p_sr_monitor)
{
  struct nm_sr_session_s*p_sr_session = p_session->ref;
  const struct nm_core_monitor_s*p_core_monitor = NULL;
  nm_core_monitor_vect_itor_t i;
  puk_vect_foreach(i, nm_core_monitor, &p_sr_session->core_monitors)
    {
      if((*i)->monitor.ref == p_sr_monitor)
	{
	  p_core_monitor = *i;
	  break;
	}
    }
  if(p_core_monitor == NULL)
    {
      fprintf(stderr, "# nmad: cannot remove session monitor %p; not found.\n", p_sr_monitor);
      abort();
    }
  nmad_lock();
  nm_core_monitor_vect_erase(&p_sr_session->core_monitors, i);
  nm_core_monitor_remove(p_session->p_core, p_core_monitor);
  nmad_unlock();
  free((void*)p_core_monitor);
  return NM_ESUCCESS;
}


int nm_sr_request_monitor(nm_session_t p_session, nm_sr_request_t *p_request,
			  nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  if(p_request->monitor.notifier != NULL)
    {
      fprintf(stderr, "# nmad: WARNING- duplicate request monitor.\n");
      return -NM_EINVAL;
    }
  assert(!(mask & NM_SR_EVENT_RECV_UNEXPECTED));
  if(mask & NM_STATUS_PACK_COMPLETED)
    mask |=  NM_STATUS_ACK_RECEIVED;
  nmad_lock();
  p_request->monitor.mask = mask;
  p_request->monitor.notifier = notifier;
  const struct nm_monitor_s monitor = 
    {
      .notifier = &nm_sr_event_req_handler,
      .mask     = mask
    };
  nm_core_req_monitor(&p_request->req, monitor);
  nmad_unlock();
  return NM_ESUCCESS;
}

static void nm_sr_completion_enqueue(nm_sr_event_t event, const nm_sr_event_info_t*event_info, void*_ref)
{
  nm_sr_request_t*p_request = event_info->req.p_request;
  struct nm_sr_session_s*p_sr_session = p_request->p_session->ref;
  if(event & NM_SR_EVENT_RECV_COMPLETED)
    {
      int rc = nm_sr_request_lfqueue_enqueue(&p_sr_session->completed_rreq, p_request);
      if(rc != 0)
	abort();
    }
  else if(event & NM_SR_EVENT_SEND_COMPLETED)
    {
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
 *  -NM_ECANCELED   receipt was already canceled
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

static void nm_sr_event_req_handler(const struct nm_core_event_s*const p_event, void*_ref)
{
  struct nm_req_s*p_req = p_event->p_req;
  struct nm_sr_request_s*p_request = tbx_container_of(p_req, struct nm_sr_request_s, req);
  assert(p_req != NULL);
  assert(p_request != NULL);
  assert(p_request->monitor.notifier);
  const nm_status_t masked_status = p_event->status & p_request->monitor.mask;
  const nm_sr_event_info_t info = { .req.p_request = p_request };
  if( (masked_status & NM_STATUS_FINALIZED) ||
      (masked_status & NM_STATUS_UNPACK_COMPLETED) ||
      ( (masked_status & NM_STATUS_PACK_COMPLETED) &&
	(((p_req->flags & NM_FLAG_PACK_SYNCHRONOUS) && (p_event->status & NM_STATUS_ACK_RECEIVED)) ||
	((!(p_req->flags & NM_FLAG_PACK_SYNCHRONOUS)) && (p_event->status & NM_STATUS_PACK_COMPLETED))
	 ))
      )
    {
      nmad_unlock();
      (*p_request->monitor.notifier)(masked_status, &info, p_request->ref);
      nmad_lock();
    }
}

static void nm_sr_event_handler(const struct nm_core_event_s*const p_event, void*_ref)
{
  struct nm_sr_monitor_s*p_monitor = _ref;
  const uint32_t hashcode = nm_tag_get_hashcode(p_event->tag);
  nm_session_t p_session = nm_session_lookup(hashcode);
  assert(p_session != NULL);
  const nm_tag_t sr_tag = nm_tag_get(p_event->tag);
  const nm_sr_event_info_t info =
    { 
      .recv_unexpected.p_gate    = p_event->p_gate,
      .recv_unexpected.tag       = sr_tag,
      .recv_unexpected.len       = p_event->len,
      .recv_unexpected.p_session = p_session,
      .recv_unexpected.p_core_event = p_event
    };
  nmad_unlock();
  (*p_monitor->p_notifier)(NM_SR_EVENT_RECV_UNEXPECTED, &info, p_monitor->ref);
  nmad_lock();
}

int nm_sr_progress(nm_session_t p_session)
{
  /* either explicit schedule, or PIOMan makes communications progress */
  nm_schedule(p_session->p_core);
  return NM_ESUCCESS;
}
