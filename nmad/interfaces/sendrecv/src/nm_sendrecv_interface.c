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

//#define DEBUG 1
/** Structure that contains all sendrecv-related static variables.
 */
static struct
{
  struct tbx_fast_list_head completed_rreq;
  struct tbx_fast_list_head completed_sreq;

  /** vector of sendrecv event monitors */
  struct nm_sr_event_monitor_vect_s monitors;
  /** flags whether sendrecv init has already been done */
  int init_done;
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
	  (*i->notifier)(event, info);
	}
    }
  if(p_request && (event & p_request->monitor.mask) && p_request->monitor.notifier)
    {
      (*p_request->monitor.notifier)(event, info);
    }
}

#ifdef PIOM_ENABLE_SHM
/** Attach a piom_sem_t to a request. This piom_sem_t is woken 
 *  up when the request is completed.
 *  @param p_request a pointer to a NM/SO request to be filled.
 *  @param p_sem a pointer to the piom_sem_t to attach.
 *  @return The NM status.
 */
extern int nm_sr_attach(nm_sr_request_t *p_request, piom_sh_sem_t *p_sem)
{
  return (piom_cond_attach_sem(&p_request->status , p_sem) ? 
	  NM_ESUCCESS: 
	  NM_EAGAIN);
}
#endif /* PIOM_ENABLE_SHM */

/* ** Debug ************************************************ */

static debug_type_t debug_nm_sr_trace TBX_UNUSED = NEW_DEBUG_TYPE("NM_SO_SR: ", "nm_sr_trace");
static debug_type_t debug_nm_sr_log TBX_UNUSED = NEW_DEBUG_TYPE("NM_SO_SR_LOG: ", "nm_sr_log");

#define NM_SO_SR_TRACE(fmt, args...) \
  debug_printf(&debug_nm_sr_trace,"[%s] " fmt ,__TBX_FUNCTION__  , ##args)

#define NM_SO_SR_TRACE_LEVEL(level, fmt, args...) \
  debug_printfl(&debug_nm_sr_trace, level, "[%s] " fmt ,__TBX_FUNCTION__  , ##args)

#define NM_SO_SR_LOG_IN() \
  debug_printf(&debug_nm_sr_log, "%s, : -->\n", __TBX_FUNCTION__)

#define NM_SO_SR_LOG_OUT() \
  debug_printf(&debug_nm_sr_log, "%s, : <--\n", __TBX_FUNCTION__)

void nm_sr_debug_init(int* argc TBX_UNUSED, char** argv TBX_UNUSED, int debug_flags TBX_UNUSED)
{
  pm2debug_register(&debug_nm_sr_trace);
  pm2debug_register(&debug_nm_sr_log);
  pm2debug_init_ext(argc, argv, debug_flags);
}



static void nm_sr_event_pack_completed(const struct nm_so_event_s*const event);
static void nm_sr_event_unpack_completed(const struct nm_so_event_s*const event);
static void nm_sr_event_unexpected(const struct nm_so_event_s*const event);

static const struct nm_so_monitor_s nm_sr_monitor_pack_completed = 
  {
    .notifier = &nm_sr_event_pack_completed,
    .mask = NM_STATUS_PACK_COMPLETED | NM_STATUS_ACK_RECEIVED
  };

static const struct nm_so_monitor_s nm_sr_monitor_unpack_completed = 
  {
    .notifier = &nm_sr_event_unpack_completed,
    .mask = NM_STATUS_UNPACK_COMPLETED
  };

static const struct nm_so_monitor_s nm_sr_monitor_unexpected = 
  {
    .notifier = &nm_sr_event_unexpected,
    .mask = NM_STATUS_UNEXPECTED
  };

/* User interface */

int nm_sr_init(nm_session_t p_session)
{
  nm_core_t p_core = p_session->p_core;
  NM_SO_SR_LOG_IN();

  if(!nm_sr_data.init_done)
    {
      /* Fill-in scheduler callbacks */
      nm_so_monitor_add(p_core, &nm_sr_monitor_unexpected);
      nm_so_monitor_add(p_core, &nm_sr_monitor_unpack_completed);
      nm_so_monitor_add(p_core, &nm_sr_monitor_pack_completed);
      
      TBX_INIT_FAST_LIST_HEAD(&nm_sr_data.completed_rreq);
      TBX_INIT_FAST_LIST_HEAD(&nm_sr_data.completed_sreq);
      
      nm_sr_event_monitor_vect_init(&nm_sr_data.monitors);

      nm_sr_data.init_done = 1;
    }
  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_exit(nm_session_t p_session)
{
  NM_SO_SR_LOG_IN();

  NM_SO_SR_LOG_OUT();
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
  NM_SO_SR_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_lock_interface(p_core);
  nm_lock_status(p_core);

#ifdef NMAD_DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_POSTED))
    TBX_FAILUREF("nm_sr_stest- req=%p no send posted!\n", p_request);
#endif /* NMAD_DEBUG */

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    goto exit;

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
  NM_SO_SR_TRACE("req=%p; rc=%d\n", p_request, rc);
  NM_SO_SR_LOG_OUT();
  return rc;
}

extern int nm_sr_flush(struct nm_core *p_core)
{
  int ret = NM_EAGAIN;

  struct nm_gate*p_gate = NULL;
  tbx_fast_list_for_each_entry(p_gate, &p_core->gate_list, _link)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  ret = nm_so_flush(p_gate);
	}
    } 
  return ret;
}

int nm_sr_swait(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
  NM_SO_SR_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_sr_flush(p_core);
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    {
      nm_so_post_all_force(p_core);
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_SEND_COMPLETED, p_core);
    }

  NM_SO_SR_LOG_OUT();
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
  NM_SO_SR_LOG_IN();
  assert(nm_sr_data.init_done);
#ifdef NMAD_DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_POSTED))
    TBX_FAILUREF("nm_sr_rtest- req=%p no recv posted!\n", p_request);
#endif /* NMAD_DEBUG */

  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
#ifdef NMAD_POLL
      nm_core_t p_core = p_session->p_core;
      nm_schedule(p_core);
#else
      piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif
    }

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
      if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_CANCELLED))
	{
	  rc = -NM_ECANCELED;
	}
      else
	{
	  rc = NM_ESUCCESS;
	}
    }
  else
    {
      rc = -NM_EAGAIN;
    }
  NM_SO_SR_TRACE("req=%p; rc=%d\n", p_request, rc);
  NM_SO_SR_LOG_OUT();
  return rc;
}

int nm_sr_rwait(nm_session_t p_session, nm_sr_request_t *p_request)
{
  nm_core_t p_core = p_session->p_core;
  NM_SO_SR_LOG_IN();
  assert(nm_sr_data.init_done);
  nm_sr_flush(p_core);
  NM_SO_SR_TRACE("request %p completion = %d\n", p_request,
		 nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED));
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED)) 
    {
      nm_so_post_all_force(p_core);
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_RECV_COMPLETED, p_core);
    }
  NM_SO_SR_TRACE("request %p completed\n", p_request);
  NM_SO_SR_LOG_OUT();
  return nm_sr_rtest(p_session, p_request);
}

int nm_sr_recv_source(nm_session_t p_session, nm_sr_request_t *p_request, nm_gate_t *pp_gate)
{
  NM_SO_SR_LOG_IN();
  if(pp_gate)
    *pp_gate = p_request->req.unpack.p_gate;
  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_probe(nm_session_t p_session,
		nm_gate_t p_gate, nm_gate_t *pp_out_gate, nm_tag_t tag, nm_tag_t mask)
{
  nm_core_t p_core = p_session->p_core;
  nm_core_tag_t core_tag = NM_CORE_TAG_NONE;
  nm_sr_tag_build(p_session, tag, &core_tag);
  nm_core_tag_t core_mask = NM_CORE_TAG_NONE;
  nm_sr_tag_build(p_session, mask, &core_mask);

  nm_lock_interface(p_core);
  nm_lock_status(p_core);
  int err = nm_so_iprobe(p_core, p_gate, core_tag, core_mask, pp_out_gate);
  nm_unlock_status(p_core);
  nm_unlock_interface(p_core);

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
  nm_sr_event_monitor_vect_push_back(&nm_sr_data.monitors, m);
  return NM_ESUCCESS;
}

int nm_sr_request_monitor(nm_session_t p_session, nm_sr_request_t *p_request,
			  nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  assert(p_request->monitor.notifier == NULL);
  p_request->monitor.mask = mask;
  p_request->monitor.notifier = notifier;
  return NM_ESUCCESS;
}

static void nm_sr_completion_enqueue(nm_sr_event_t event, const nm_sr_event_info_t*event_info)
{
  if(event & NM_SR_STATUS_RECV_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->recv_completed.p_request;
#warning Paulette: lock
      tbx_fast_list_add_tail(&p_request->_link, &nm_sr_data.completed_rreq);
    }
  else if(event & NM_SR_STATUS_SEND_COMPLETED)
    {
      nm_sr_request_t*p_request = event_info->send_completed.p_request;
#warning Paulette: lock
      tbx_fast_list_add_tail(&p_request->_link, &nm_sr_data.completed_sreq);
    }
}

int nm_sr_request_set_completion_queue(nm_session_t p_session, nm_sr_request_t*p_request)
{
  assert(p_request->monitor.notifier == NULL);
  nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_SEND_COMPLETED | NM_SR_EVENT_RECV_COMPLETED,
			&nm_sr_completion_enqueue);
  return NM_ESUCCESS;
}

int nm_sr_recv_success(nm_session_t p_session, nm_sr_request_t **out_req)
{
  nm_core_t p_core = p_session->p_core;
#ifdef NMAD_POLL
  nm_schedule(p_core);
#else
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif

  nm_lock_interface(p_core);
  nm_lock_status(p_core);

  if(!tbx_fast_list_empty(&nm_sr_data.completed_rreq))
    {
      nm_sr_request_t *p_request = tbx_container_of(nm_sr_data.completed_rreq.next, struct nm_sr_request_s, _link);
      tbx_fast_list_del(nm_sr_data.completed_rreq.next);
      *out_req = p_request;

      nm_unlock_status(p_core);
      nm_unlock_interface(p_core);
      return NM_ESUCCESS;
    } 
  else 
    {
      *out_req = NULL;

      nm_unlock_status(p_core);
      nm_unlock_interface(p_core);
      return -NM_EAGAIN;
    }
}

int nm_sr_send_success(nm_session_t p_session, nm_sr_request_t **out_req)
{
  nm_core_t p_core = p_session->p_core;
  nm_schedule(p_core);
  if(!tbx_fast_list_empty(&nm_sr_data.completed_sreq))
    {
      nm_sr_request_t *p_request = tbx_container_of(nm_sr_data.completed_sreq.next, struct nm_sr_request_s, _link);
      tbx_fast_list_del(nm_sr_data.completed_sreq.next);
      *out_req = p_request;
      return NM_ESUCCESS;
    } 
  else 
    {
      *out_req = NULL;
      return -NM_EAGAIN;
    }
}

/** cancel a receive request.
 * @return error code:
 *   NM_ESUCCESS    operaton successfully canceled
 *  -NM_ENOTIMPL    case where cancellation is not supported yet
 *  -NM_EINPROGRESS receipt is in progress, it is too late to cancel
 *  -NM_EALREADY    receipt is already completed
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
  else
    {
      err = nm_so_cancel_unpack(p_core, &p_request->req.unpack);
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
static void nm_sr_event_pack_completed(const struct nm_so_event_s*const event)
{
  struct nm_pack_s*p_pack = event->p_pack;
  struct nm_sr_request_s*p_request = tbx_container_of(p_pack, struct nm_sr_request_s, req.pack);
  NM_SO_SR_LOG_IN();
  const nm_status_t status = p_pack->status;
  if( (status & NM_STATUS_PACK_COMPLETED) &&
      ( (!(status & NM_PACK_SYNCHRONOUS)) || (status & NM_STATUS_ACK_RECEIVED)) )
    {
      const nm_sr_event_info_t info = { .send_completed.p_request = p_request };
      nm_sr_request_signal(p_request, NM_SR_STATUS_SEND_COMPLETED);
      nm_sr_monitor_notify(p_request, NM_SR_STATUS_SEND_COMPLETED, &info);
    }
  NM_SO_SR_LOG_OUT();
}

static void nm_sr_event_unexpected(const struct nm_so_event_s*const event)
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
static void nm_sr_event_unpack_completed(const struct nm_so_event_s*const event)
{
  struct nm_unpack_s*p_unpack = event->p_unpack;
  struct nm_sr_request_s*p_request = tbx_container_of(p_unpack, struct nm_sr_request_s, req.unpack);
  NM_SO_SR_LOG_IN();
  nm_sr_status_t sr_event;

  if(event->status & NM_STATUS_UNPACK_CANCELLED)
    {
      sr_event = NM_SR_STATUS_RECV_COMPLETED | NM_SR_STATUS_RECV_CANCELLED;
    }
  else
    {
      sr_event = NM_SR_STATUS_RECV_COMPLETED;
    }
  const nm_sr_event_info_t info = { 
    .recv_completed.p_request = p_request,
    .recv_completed.p_gate = event->p_gate
  };
  nm_sr_request_signal(p_request, sr_event);
  nm_sr_monitor_notify(p_request, sr_event, &info);

  NM_SO_SR_LOG_OUT();
}

int nm_sr_progress(nm_session_t p_session)
{
  /* We assume that PIOMan makes the communications progress */
#ifdef NMAD_POLL
  nm_core_t p_core = p_session->p_core;
  NM_SO_SR_LOG_IN();
  nm_schedule(p_core);
  NM_SO_SR_LOG_OUT();
#else
  piom_check_polling(PIOM_POLL_WHEN_FORCED);
#endif

  return NM_ESUCCESS;
}
