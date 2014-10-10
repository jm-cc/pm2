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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_private.h>

PADICO_MODULE_HOOK(NewMad_Core);


PUK_LFQUEUE_TYPE(nm_sr_request, struct nm_sr_request_s*, NULL, 1024);

/** Structure that contains all sendrecv-related static variables.
 */
static struct
{
  /** vector of sendrecv event monitors */
  struct nm_sr_event_monitor_vect_s monitors;
  /** flags whether sendrecv init has already been done */
  int init_done;
  /** queue of completed recv requests */
  struct nm_sr_request_lfqueue_s completed_rreq;
  /** queue of completed send requests */
  struct nm_sr_request_lfqueue_s completed_sreq;
} nm_sr_data = { .init_done = 0 };


/* ** Status *********************************************** */

static inline void nm_sr_monitor_notify(nm_sr_request_t*p_request, nm_sr_status_t event, const nm_sr_event_info_t*info)
{

  nm_sr_event_monitor_vect_itor_t i;
  for(i  = nm_sr_event_monitor_vect_begin(&nm_sr_data.monitors);
      i != nm_sr_event_monitor_vect_end(&nm_sr_data.monitors);
      i  = nm_sr_event_monitor_vect_next(i))
    {
      if(event & i->mask)
	{
	  nmad_unlock();
	  (*i->notifier)(event, info);
	  nmad_lock();
	}
    }
  if(p_request && (event & p_request->monitor.mask) && p_request->monitor.notifier)
    {
      nmad_unlock();
      (*p_request->monitor.notifier)(event, info);
      nmad_lock();
    }
}


/* ** Events *********************************************** */

static void nm_sr_event_pack_completed(const struct nm_core_event_s*const event);
static void nm_sr_event_unpack_completed(const struct nm_core_event_s*const event);
static void nm_sr_event_unexpected(const struct nm_core_event_s*const event);

static const struct nm_core_monitor_s nm_sr_monitor_pack_completed = 
  {
    .notifier = &nm_sr_event_pack_completed,
    .mask = NM_STATUS_PACK_COMPLETED | NM_STATUS_ACK_RECEIVED
  };

static const struct nm_core_monitor_s nm_sr_monitor_unpack_completed = 
  {
    .notifier = &nm_sr_event_unpack_completed,
    .mask = NM_STATUS_UNPACK_COMPLETED | NM_STATUS_UNPACK_CANCELLED
  };

static const struct nm_core_monitor_s nm_sr_monitor_unexpected = 
  {
    .notifier = &nm_sr_event_unexpected,
    .mask = NM_STATUS_UNEXPECTED
  };

/* User interface */

int nm_sr_init(nm_session_t p_session)
{
  nm_core_t p_core = p_session->p_core;
  NM_LOG_IN();

  if(!nm_sr_data.init_done)
    {
      /* Fill-in scheduler callbacks */
      nm_core_monitor_add(p_core, &nm_sr_monitor_unexpected);
      nm_core_monitor_add(p_core, &nm_sr_monitor_unpack_completed);
      nm_core_monitor_add(p_core, &nm_sr_monitor_pack_completed);
      
      nm_sr_request_lfqueue_init(&nm_sr_data.completed_rreq);
      nm_sr_request_lfqueue_init(&nm_sr_data.completed_sreq);
      
      nm_sr_event_monitor_vect_init(&nm_sr_data.monitors);

      nm_sr_data.init_done = 1;
    }
  NM_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_exit(nm_session_t p_session)
{
  nm_core_t p_core = p_session->p_core;
  NM_LOG_IN();
  if(nm_sr_data.init_done)
    {
      nm_core_monitor_remove(p_core, &nm_sr_monitor_unexpected);
      nm_core_monitor_remove(p_core, &nm_sr_monitor_unpack_completed);
      nm_core_monitor_remove(p_core, &nm_sr_monitor_pack_completed);
    }
  NM_LOG_OUT();
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
  NM_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_lock_interface(p_core);
  nm_lock_status(p_core);

  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_POSTED))
    {
      rc = -NM_ENOTPOSTED;
      goto exit;
    }

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    {
      rc = NM_ESUCCESS;
      goto exit;
    }

#ifdef NMAD_POLL
  nm_schedule(p_core);
#else
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif

  rc = (nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED)) ?
    NM_ESUCCESS : -NM_EAGAIN;

 exit:

  nm_unlock_status(p_core);
  nm_unlock_interface(p_core);
  NM_TRACEF("req=%p; rc=%d\n", p_request, rc);
  NM_LOG_OUT();
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
  NM_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_sr_flush(p_core);
#ifdef DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_POSTED))
    TBX_FAILUREF("nm_sr_swait- req=%p no send posted!\n", p_request);
#endif /* DEBUG */

  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    {
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_SEND_COMPLETED, p_core);
    }

  NM_LOG_OUT();
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
  NM_LOG_IN();
  assert(nm_sr_data.init_done);

  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_POSTED))
    {
      rc = -NM_ENOTPOSTED;
      goto exit;
    }
  if( !nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED | NM_SR_STATUS_RECV_CANCELLED))
    {
#ifdef NMAD_POLL
      nm_schedule(p_session->p_core);
#else /* NMAD_POLL */
      piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif /* NMAD_POLL */
    }

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
      rc = NM_ESUCCESS;
    }
  else if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_CANCELLED))
    {
      rc = -NM_ECANCELED;
    }
  else
    {
      rc = -NM_EAGAIN;
    }
 exit:
  NM_TRACEF("req=%p; rc=%d\n", p_request, rc);
  NM_LOG_OUT();
  return rc;
}

int nm_sr_rwait(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
  NM_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_sr_flush(p_core);
#ifdef DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_POSTED))
    TBX_FAILUREF("nm_sr_rwait- req=%p no recv posted!\n", p_request);
#endif /* DEBUG */

  NM_TRACEF("request %p completion = %d\n", p_request,
	    nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED));
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED | NM_SR_STATUS_RECV_CANCELLED)) 
    {
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_RECV_COMPLETED | NM_SR_STATUS_RECV_CANCELLED, p_core);
    }
  NM_TRACEF("request %p completed\n", p_request);
  NM_LOG_OUT();
  return nm_sr_rtest(p_session, p_request);
}

int nm_sr_recv_source(nm_session_t p_session, nm_sr_request_t *p_request, nm_gate_t *pp_gate)
{
  NM_LOG_IN();
  if(pp_gate)
    *pp_gate = p_request->req.unpack.p_gate;
  NM_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_probe(nm_session_t p_session,
		nm_gate_t p_gate, nm_gate_t *pp_out_gate,
		nm_tag_t tag, nm_tag_t mask, nm_tag_t*p_out_tag,
		nm_len_t*p_out_len)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_tag_t core_tag = NM_CORE_TAG_NONE;
  nm_sr_tag_build(p_session, tag, &core_tag);
  nm_core_tag_t core_mask = NM_CORE_TAG_NONE;
  nm_sr_tag_build(p_session, mask, &core_mask);

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
    *p_out_tag = nm_sr_tag_get(out_core_tag);
  if(p_out_len)
    *p_out_len = out_size;

#ifdef NMAD_POLL
  nm_schedule(p_core);
#else
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif

  return err;
}

int nm_sr_monitor(nm_session_t p_session, nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  nm_sr_event_monitor_t m = { .mask = mask, .notifier = notifier };
  if(mask == NM_SR_EVENT_RECV_UNEXPECTED)
    {
      if(!tbx_fast_list_empty(&p_session->p_core->gate_list))
	{
	  fprintf(stderr, "nmad: WARNING- nm_sr_monitor() set for event UNEXPECTED after connecting gate. Behavior is unspecified.\n");
	}
      int rc = nm_sr_probe(p_session, NM_GATE_NONE, NULL, 0, NM_TAG_MASK_NONE, NULL, NULL);
      if(rc == NM_ESUCCESS)
	{
	  TBX_FAILURE("nmad: FATAL- cannot set UNEXPECTED monitor: pending unexpected messages.\n");
	}
    }
  nm_sr_event_monitor_vect_push_back(&nm_sr_data.monitors, m);
  usleep(200*1000);
  return NM_ESUCCESS;
}

int nm_sr_request_monitor(nm_session_t p_session, nm_sr_request_t *p_request,
			  nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  p_request->monitor.mask = mask;
  p_request->monitor.notifier = notifier;
  return NM_ESUCCESS;
}

static void nm_sr_completion_enqueue(nm_sr_event_t event, const nm_sr_event_info_t*event_info)
{
  if(event & NM_SR_STATUS_RECV_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->recv_completed.p_request;
      nm_sr_request_lfqueue_enqueue(&nm_sr_data.completed_rreq, p_request);
    }
  else if(event & NM_SR_STATUS_SEND_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->send_completed.p_request;
      nm_sr_request_lfqueue_enqueue(&nm_sr_data.completed_sreq, p_request);
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


int nm_sr_recv_success(nm_session_t p_session, nm_sr_request_t **out_req)
{
  if(nm_sr_request_lfqueue_empty(&nm_sr_data.completed_rreq))
    {
      nm_schedule(p_session->p_core);
    }
  nm_sr_request_t*p_request = nm_sr_request_lfqueue_dequeue(&nm_sr_data.completed_rreq);
  *out_req = p_request;
  return (p_request != NULL) ? NM_ESUCCESS : -NM_EAGAIN;
}

int nm_sr_send_success(nm_session_t p_session, nm_sr_request_t **out_req)
{
  if(nm_sr_request_lfqueue_empty(&nm_sr_data.completed_sreq))
    {
      nm_schedule(p_session->p_core);
    }
  nm_sr_request_t*p_request = nm_sr_request_lfqueue_dequeue(&nm_sr_data.completed_sreq);
  *out_req = p_request;
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

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
      err = -NM_EALREADY;
    }
  else if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_CANCELLED))
    {
      err = -NM_ECANCELED;
    }
  else
    {
      err = nm_core_unpack_cancel(p_core, &p_request->req.unpack);
    }
  nm_unlock_status(p_core);
  nm_unlock_interface(p_core);
  nmad_unlock();

  return err;
}

/** Check the status for a send request (gate,tag,seq).
 *  @param p_gate the pointer to the gate object.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number.
 *  @return The NM status.
 */
static void nm_sr_event_pack_completed(const struct nm_core_event_s*const event)
{
  struct nm_pack_s*p_pack = event->p_pack;
  struct nm_sr_request_s*p_request = tbx_container_of(p_pack, struct nm_sr_request_s, req.pack);
  NM_LOG_IN();
  const nm_status_t status = p_pack->status;
  if( (status & NM_STATUS_PACK_COMPLETED) &&
      ( (!(status & NM_PACK_SYNCHRONOUS)) || (status & NM_STATUS_ACK_RECEIVED)) )
    {
      const nm_sr_event_info_t info = { .send_completed.p_request = p_request };
      nm_sr_monitor_notify(p_request, NM_SR_STATUS_SEND_COMPLETED, &info);
      nm_sr_request_signal(p_request, NM_SR_STATUS_SEND_COMPLETED);
    }
  NM_LOG_OUT();
}

static void nm_sr_event_unexpected(const struct nm_core_event_s*const event)
{
  nm_tag_t sr_tag = nm_sr_tag_get(event->tag);

  const nm_sr_event_info_t info = { 
    .recv_unexpected.p_gate = event->p_gate,
    .recv_unexpected.tag = sr_tag,
    .recv_unexpected.len = event->len
  };
  nm_sr_monitor_notify(NULL, NM_SR_EVENT_RECV_UNEXPECTED, &info);
}

/** Check the status for a receive request (gate/is_any_src,tag,seq).
 *  @param p_gate the pointer to the gate object or @c NULL if @p is_any_src is @c true.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number (ignored if @p is_any_src is @c true).
 *  @param is_any_src whether to check for a specific gate or for any gate.
 *  @return The NM status.
 */
static void nm_sr_event_unpack_completed(const struct nm_core_event_s*const event)
{
  struct nm_unpack_s*p_unpack = event->p_unpack;
  struct nm_sr_request_s*p_request = tbx_container_of(p_unpack, struct nm_sr_request_s, req.unpack);
  NM_LOG_IN();
  nm_sr_status_t sr_event;

  if(event->status & NM_STATUS_UNPACK_CANCELLED)
    {
      sr_event = NM_SR_STATUS_RECV_CANCELLED;
    }
  else
    {
      sr_event = NM_SR_STATUS_RECV_COMPLETED;
    }
  const nm_sr_event_info_t info = { 
    .recv_completed.p_request = p_request,
    .recv_completed.p_gate = p_unpack->p_gate
  };
  nm_sr_monitor_notify(p_request, sr_event, &info);
  nm_sr_request_signal(p_request, sr_event);

  NM_LOG_OUT();
}

int nm_sr_progress(nm_session_t p_session)
{
  NM_LOG_IN();
  /* We assume that PIOMan makes the communications progress */
#ifdef NMAD_POLL
  nm_schedule(p_session->p_core);
#else
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif
  NM_LOG_OUT();
  return NM_ESUCCESS;
}
