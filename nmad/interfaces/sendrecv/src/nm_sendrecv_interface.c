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
#include "nm_sendrecv_interface.h"


/* ** Tags ************************************************* */

struct nm_sr_tag_s
{
  nm_sr_request_t *rreqs[NM_SO_PENDING_PACKS_WINDOW],
    *sreqs[NM_SO_PENDING_PACKS_WINDOW];
};
/** Create a new sendrecv tag entry, if not already allocated.
 */
static inline struct nm_sr_tag_s*nm_sr_tag_init(struct nm_so_tag_s*p_so_tag)
{
  struct nm_sr_tag_s*p_sr_tag = p_so_tag->p_sr_tag;
  if(tbx_unlikely(p_sr_tag == NULL))
    {
      p_sr_tag = TBX_MALLOC(sizeof(struct nm_sr_tag_s));
      int j;
      for(j = 0; j < NM_SO_PENDING_PACKS_WINDOW; j++)
	{
	  p_sr_tag->rreqs[j] = NULL;
	  p_sr_tag->sreqs[j] = NULL;
	}
      p_so_tag->p_sr_tag = p_sr_tag;
    }
  return p_sr_tag;
}
static inline void nm_sr_tag_destroy(struct nm_sr_tag_s*p_sr_tag)
{
}


/** Status for any source receive requests.
 */
struct nm_sr_anysrc_s
{
  nm_sr_request_t *rreq;
  nm_gate_t p_gate;
};
static inline void nm_sr_anysrc_ctor(struct nm_sr_anysrc_s*p_sr_anysrc, nm_tag_t tag)
{
  p_sr_anysrc->rreq = NULL;
  p_sr_anysrc->p_gate = NM_ANY_GATE;
}
static inline void nm_sr_anysrc_dtor(struct nm_sr_anysrc_s*p_sr_anysrc)
{
}
NM_TAG_TABLE_TYPE(nm_sr_anysrc, struct nm_sr_anysrc_s);


/** Structure that contains all sendrecv-related static variables.
 */
static struct
{
  /** Tag-indexed table of any_src_status structs. One entry per tag. */
  struct nm_sr_anysrc_table_s any_src;
  
  struct list_head completed_rreq;
  struct list_head completed_sreq;

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

#ifdef PIOMAN
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
#else /* PIOMAN */
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
      }
    }
}
static inline void nm_so_post_all(nm_core_t p_core)
{ /* do nothing */ }
static inline void nm_so_post_all_force(nm_core_t p_core)
{ /* do nothing */ }
#endif /* PIOMAN */

#ifdef PIOMAN
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
#endif /* PIOMAN */

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
    .mask = NM_SO_STATUS_PACK_COMPLETED
  };

static const struct nm_so_monitor_s nm_sr_monitor_unpack_completed = 
  {
    .notifier = &nm_sr_event_unpack_completed,
    .mask = NM_SO_STATUS_UNPACK_COMPLETED
  };

static const struct nm_so_monitor_s nm_sr_monitor_unexpected = 
  {
    .notifier = &nm_sr_event_unexpected,
    .mask = NM_SO_STATUS_PACKET_HERE 
  };

/* User interface */

int nm_sr_init(struct nm_core *p_core)
{
  NM_SO_SR_LOG_IN();

  if(!nm_sr_data.init_done)
    {
      /* Fill-in scheduler callbacks */
      nm_so_monitor_add(p_core, &nm_sr_monitor_unexpected);
      nm_so_monitor_add(p_core, &nm_sr_monitor_unpack_completed);
      nm_so_monitor_add(p_core, &nm_sr_monitor_pack_completed);
      
      INIT_LIST_HEAD(&nm_sr_data.completed_rreq);
      INIT_LIST_HEAD(&nm_sr_data.completed_sreq);
      
      nm_sr_anysrc_table_init(&nm_sr_data.any_src);

      nm_sr_event_monitor_vect_init(&nm_sr_data.monitors);

      nm_sr_data.init_done = 1;
    }
  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_exit(struct nm_core *p_core)
{
  NM_SO_SR_LOG_IN();

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

/* Send operations */
int nm_sr_isend_generic(struct nm_core *p_core,
			nm_gate_t p_gate, nm_tag_t tag,
			nm_sr_transfer_type_t sending_type,
			const void *data, uint32_t len,
			nm_sr_request_t *p_request,
			void*ref)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  struct nm_sr_tag_s*p_sr_tag = nm_sr_tag_init(p_so_tag);
  nm_seq_t seq;
  int ret;

  NM_SO_SR_LOG_IN();
  nmad_lock();

  NM_SO_SR_TRACE("tag=%d; data=%p; len=%d; req=%p\n", tag, data, len, p_request);

  seq = p_so_tag->send_seq_number++;

  nm_sr_status_init(&p_request->status, NM_SR_STATUS_SEND_POSTED);
  p_request->seq    = seq;
  p_request->p_gate = p_gate;
  p_request->tag    = tag;
  p_request->ref    = ref;
  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
  if(p_sr_tag->sreqs[seq])
    {
      nm_sr_swait(p_core, p_sr_tag->sreqs[seq]);
    }
  p_sr_tag->sreqs[seq] = p_request;
  
  switch(sending_type)
    {
    case nm_sr_contiguous_transfer:
      ret = nm_so_pack(p_gate, tag, seq, data, len);
      break;

    case nm_sr_iov_transfer:
      ret = nm_so_packv(p_gate, tag, seq, data, len);
      break;
      
    case nm_sr_datatype_transfer:
      ret = nm_so_pack_datatype(p_gate, tag, seq, data);
      break;

    default:
      TBX_FAILURE("Unkown sending type.");
      break;
    }
  
  nm_so_post_all(p_core);
  nmad_unlock();
  NM_SO_SR_TRACE("req=%p; rc=%d\n", p_request, ret);
  NM_SO_SR_LOG_OUT();
  return ret;
}

int nm_sr_rsend(struct nm_core *p_core,
		   nm_gate_t p_gate, nm_tag_t tag,
		   const void *data, uint32_t len,
		   nm_sr_request_t *p_request)
{
  const nm_sr_transfer_type_t tt = nm_sr_contiguous_transfer;
  
  NM_SO_SR_LOG_IN();
  
  int ret = nm_sr_isend_generic(p_core, p_gate, tag, tt, data, len, p_request, NULL);

  NM_SO_SR_LOG_OUT();
  return ret;
}


/** Test for the completion of a non blocking send request.
 *  @param p_so_interface a pointer to the NM/SchedOpt interface.
 *  @param request the request to check.
 *  @return The NM status.
 */
int nm_sr_stest(struct nm_core *p_core, nm_sr_request_t *p_request)
{
  int rc = NM_ESUCCESS;
  NM_SO_SR_LOG_IN();

#ifdef NMAD_DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_POSTED))
    TBX_FAILUREF("nm_sr_stest- req=%p no send posted!\n", p_request);
#endif /* NMAD_DEBUG */

  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    goto exit;

#ifdef PIOMAN
  /* Useless ? */
  __piom_check_polling(PIOM_POLL_AT_IDLE);
#else
  nm_schedule(p_core);
#endif

  rc = (nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED)) ?
    NM_ESUCCESS : -NM_EAGAIN;

 exit:
  NM_SO_SR_TRACE("req=%p; rc=%d\n", p_request, rc);
  NM_SO_SR_LOG_OUT();
  return rc;
}

static inline int nm_sr_flush(struct nm_core *p_core)
{
  int ret = NM_EAGAIN;
  int i;

  for(i = 0 ; i < p_core->nb_gates ; i++)
    {
      struct nm_gate *p_gate = &p_core->gate_array[i];
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  ret = nm_so_flush(p_gate);
	}
    } 
  return ret;
}

int nm_sr_swait(struct nm_core *p_core, nm_sr_request_t *p_request)
{
  NM_SO_SR_LOG_IN();

  nm_sr_flush(p_core);

  if(! nm_sr_status_test(&p_request->status, NM_SR_STATUS_SEND_COMPLETED))
    {
      nmad_lock();
      nm_so_post_all_force(p_core);
      nmad_unlock();
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_SEND_COMPLETED, p_core);
    }

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}

int nm_sr_scancel(struct nm_core *p_core, nm_sr_request_t *p_request) 
{

  return NM_ENOTIMPL;
}

static struct
{
  nm_sr_request_t*request;
  void*data;
  uint32_t len;
  int missed;
} nm_sr_irecv_event_info =
  {
    .request = NULL,
    .data = NULL,
    .len = 0,
    .missed = 0
  };

extern int nm_sr_irecv_event(nm_core_t p_core, nm_tag_t tag,
			     void*data, uint32_t len,
			     nm_sr_request_t *p_request)
{
  if(nm_sr_irecv_event_info.missed)
    {
      nm_sr_irecv_event_info.missed--;
      return nm_sr_irecv(p_core, NM_ANY_GATE, tag, data, len, p_request);
    }

  nmad_lock();

  nm_sr_status_init(&p_request->status, NM_SR_STATUS_RECV_POSTED);
  p_request->p_gate  = NM_GATE_NONE;
  p_request->ref     = NULL;
  p_request->tag     = tag;
  p_request->seq     = -1;
  p_request->monitor = NM_SR_EVENT_MONITOR_NULL;

  if(nm_sr_irecv_event_info.request != NULL)
    {
      TBX_FAILURE("sendrecv: concurrent nm_sr_irecv_event detected\n");
    }

  nm_sr_irecv_event_info.request = p_request;
  nm_sr_irecv_event_info.data = data;
  nm_sr_irecv_event_info.len = len;

  nmad_unlock();

  return NM_ESUCCESS;
}

/* Receive operations */
extern int nm_sr_irecv_generic(nm_core_t p_core,
			       nm_gate_t p_gate, nm_tag_t tag,
			       nm_sr_transfer_type_t reception_type,
			       void *data_description, uint32_t len,
			       nm_sr_request_t *p_request,
			       void *ref)
{
  int ret;

  NM_SO_SR_LOG_IN();
  NM_SO_SR_TRACE("tag=%d; data=%p; len=%d; req=%p\n", tag, data_description, len, p_request);

  if(p_gate == NM_ANY_GATE)
    {
      struct nm_sr_anysrc_s*p_sr_anysrc = nm_sr_anysrc_get(&nm_sr_data.any_src, tag);
      if(p_sr_anysrc->rreq)
	{
	  /* ensure that there is no other pending any_src request. */
	  NM_SO_SR_TRACE("Irecv not completed for ANY_SRC tag=%d\n", tag);
	  nm_sr_rwait(p_core, p_sr_anysrc->rreq);
	}
      nmad_lock();
      nm_sr_status_init(&p_request->status, NM_SR_STATUS_RECV_POSTED);
      p_request->p_gate  = NM_GATE_NONE;
      p_request->ref     = ref;
      p_request->tag     = tag;
      p_request->seq     = -1;
      p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
      p_sr_anysrc->rreq = p_request;

      NM_SO_SR_TRACE_LEVEL(3, "IRECV ANY_SRC: tag = %d, request = %p\n", tag, p_sr_anysrc->rreq);
      
      switch(reception_type)
	{
	case nm_sr_datatype_transfer:
	  {
	    struct DLOOP_Segment *segp = data_description;
	    ret = nm_so_unpack_datatype_any_src(p_core, tag, segp);
	    break;
	  }
	case nm_sr_iov_transfer:
	  {
	    struct iovec *iov = data_description;
	    ret = nm_so_unpackv_any_src(p_core, tag, iov, len);
	  }
	  break;
	case nm_sr_contiguous_transfer:
	  {
	    ret = nm_so_unpack_any_src(p_core, tag, data_description, len);
	  }
	  break;
	default:
	  TBX_FAILURE("Unknown reception type.");
	}
      
      NM_SO_SR_TRACE_LEVEL(3, "IRECV ANY_SRC completed : tag = %d, request = %p\n",
			   tag, p_sr_anysrc->rreq);
    }
  else
    {
      nmad_lock();
      nm_seq_t seq;
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
      struct nm_sr_tag_s*p_sr_tag = nm_sr_tag_init(p_so_tag);
  
      nm_sr_flush(p_core);
      
      seq = p_so_tag->recv_seq_number++;
      
      nm_sr_status_init(&p_request->status, NM_SR_STATUS_RECV_POSTED);
      p_request->seq     = seq;
      p_request->p_gate  = p_gate;
      p_request->ref     = ref;
      p_request->tag     = tag;
      p_request->monitor = NM_SR_EVENT_MONITOR_NULL;
      if(p_sr_tag->rreqs[seq])
	{
	  nm_sr_rwait(p_core, p_sr_tag->rreqs[seq]);
	}
      p_sr_tag->rreqs[seq] = p_request;

      NM_SO_SR_TRACE_LEVEL(3, "IRECV: tag = %d, seq = %d, gate = %p request %p\n", tag, seq, p_gate, p_sr_tag->rreqs[seq]);
      switch(reception_type)
	{
	case nm_sr_datatype_transfer:
	  {
	    struct DLOOP_Segment *segp = data_description;
	    ret = nm_so_unpack_datatype(p_gate, tag, seq, segp);
	  }
	  break;
	case nm_sr_iov_transfer:
	  {
	    struct iovec *iov = data_description;
	    ret = nm_so_unpackv(p_gate, tag, seq, iov, len);
	  }
	  break;
	case nm_sr_contiguous_transfer:
	  {
	    ret = nm_so_unpack(p_gate, tag, seq, data_description, len);
	  }
	  break;
	default:
	  TBX_FAILURE("Unknown reception type.");
	}
    }
  nm_so_post_all(p_core);
  nmad_unlock();
  NM_SO_SR_LOG_OUT();
  return ret;
}

/** Test for the completion of a non blocking receive request.
 *  @param p_core a pointer to the NewMad core object.
 *  @param request the request to check.
 *  @return The NM status.
 */
int nm_sr_rtest(struct nm_core *p_core, nm_sr_request_t *p_request) 
{

  int rc = NM_ESUCCESS;
  NM_SO_SR_LOG_IN();

#ifdef NMAD_DEBUG
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_POSTED))
    TBX_FAILUREF("nm_sr_rtest- req=%p no recv posted!\n", p_request);
#endif /* NMAD_DEBUG */

  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
#ifdef PIOMAN
      /* Useless ? */
      __piom_check_polling(PIOM_POLL_AT_IDLE);
#else
      nm_schedule(p_core);
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

int nm_sr_rwait(struct nm_core *p_core, nm_sr_request_t *p_request)
{
  NM_SO_SR_LOG_IN();

  nm_sr_flush(p_core);

  NM_SO_SR_TRACE("request %p completion = %d\n", p_request,
		 nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED));
  if(!nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED)) 
    {
      nmad_lock();
      nm_so_post_all_force(p_core);
      nmad_unlock();
      nm_sr_status_wait(&p_request->status, NM_SR_STATUS_RECV_COMPLETED, p_core);
    }

  NM_SO_SR_TRACE("request %p completed\n", p_request);
  NM_SO_SR_LOG_OUT();
  return nm_sr_rtest(p_core, p_request);
}

int nm_sr_get_size(struct nm_core *p_core,
		   nm_sr_request_t *p_request,
		   size_t *size)
{
  const nm_tag_t tag = p_request->tag;

  if (p_request->p_gate == NM_ANY_GATE)
    {
      *size = nm_so_any_src_get(&p_core->so_sched.any_src, tag)->expected_len;
    }
  else 
    {
      *size = nm_so_tag_get(&p_request->p_gate->tags, tag)->recv[p_request->seq].unpack_here.expected_len;
    }

  NM_SO_SR_TRACE("SIZE: tag = %d, seq = %d, gate = %p --> %zd\n", tag, p_request->seq, p_request->p_gate, *size);

  return NM_ESUCCESS;
}

int nm_sr_recv_source(struct nm_core *p_core, nm_sr_request_t *p_request, nm_gate_t *pp_gate)
{
  const nm_tag_t tag = p_request->tag;
  struct nm_sr_anysrc_s*p_sr_anysrc = nm_sr_anysrc_get(&nm_sr_data.any_src, tag);

  NM_SO_SR_LOG_IN();

  if(pp_gate)
    *pp_gate = p_sr_anysrc->p_gate;

  NM_SO_SR_LOG_OUT();
  return NM_ESUCCESS;
}


int nm_sr_probe(struct nm_core *p_core,
		nm_gate_t p_gate, nm_gate_t *pp_out_gate, nm_tag_t tag)
{
  int i;
  NM_SO_SR_LOG_IN();

  if (p_gate == NM_ANY_GATE)
    {
      for(i = 0; i < p_core->nb_gates; i++)
	{
	  p_gate = &p_core->gate_array[i];
	  if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	    {
	      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
	      nm_seq_t seq = p_so_tag->recv_seq_number;
	      volatile nm_so_status_t *status = &(p_so_tag->status[seq]);
	      if ((*status & NM_SO_STATUS_PACKET_HERE) || (*status & NM_SO_STATUS_RDV_HERE)) {
		*pp_out_gate = p_gate;
		NM_SO_SR_LOG_OUT();
		return NM_ESUCCESS;
	      }
	    }
	}
    }
  else
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
	  nm_seq_t seq = p_so_tag->recv_seq_number;
	  volatile nm_so_status_t *status = &(p_so_tag->status[seq]);
	  if ((*status & NM_SO_STATUS_PACKET_HERE) || (*status & NM_SO_STATUS_RDV_HERE)) {
	    *pp_out_gate = p_gate;
	    NM_SO_SR_LOG_OUT();
	    return NM_ESUCCESS;
	  }
	}
    }

#ifndef PIOMAN
  nm_schedule(p_core);
#endif
  *pp_out_gate = NM_ANY_GATE;
  NM_SO_SR_LOG_OUT();
  return -NM_EAGAIN;
}

int nm_sr_monitor(nm_core_t p_core, nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  nm_sr_event_monitor_t m = { .mask = mask, .notifier = notifier };
  nm_sr_event_monitor_vect_push_back(&nm_sr_data.monitors, m);
  return NM_ESUCCESS;
}


int nm_sr_request_monitor(nm_core_t p_core, nm_sr_request_t *p_request,
			  nm_sr_event_t mask, nm_sr_event_notifier_t notifier)
{
  assert(p_request->monitor.notifier == NULL);
  p_request->monitor.mask = mask;
  p_request->monitor.notifier = notifier;
  return NM_ESUCCESS;
}

int nm_sr_recv_success(struct nm_core *p_core, nm_sr_request_t **out_req)
{
  nm_schedule(p_core);
  if(!list_empty(&nm_sr_data.completed_rreq))
    {
      nm_sr_request_t *p_request = tbx_container_of(nm_sr_data.completed_rreq.next, struct nm_sr_request_s, _link);
      list_del(nm_sr_data.completed_rreq.next);
      *out_req = p_request;
      return NM_ESUCCESS;
    } 
  else 
    {
      *out_req = NULL;
      return -NM_EAGAIN;
    }
}

int nm_sr_send_success(struct nm_core *p_core, nm_sr_request_t **out_req)
{
  nm_schedule(p_core);
  if(!list_empty(&nm_sr_data.completed_sreq))
    {
      nm_sr_request_t *p_request = tbx_container_of(nm_sr_data.completed_sreq.next, struct nm_sr_request_s, _link);
      list_del(nm_sr_data.completed_sreq.next);
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
int nm_sr_rcancel(struct nm_core *p_core, nm_sr_request_t *p_request)
{
  int err = -NM_ENOTIMPL;
  nmad_lock();
  if(nm_sr_status_test(&p_request->status, NM_SR_STATUS_RECV_COMPLETED))
    {
      err = -NM_EALREADY;
    }
  else
    {
      err = nm_so_cancel_unpack(p_core, p_request->p_gate, p_request->tag, p_request->seq);
      if(err == NM_ESUCCESS && p_request->p_gate == NULL)
	{
	  struct nm_sr_anysrc_s*p_sr_anysrc = nm_sr_anysrc_get(&nm_sr_data.any_src, p_request->tag);
	  p_sr_anysrc->rreq = NULL;
	}
    }
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
  struct nm_so_tag_s*const p_so_tag = nm_so_tag_get(&event->p_gate->tags, event->tag);
  nm_sr_request_t**const sreq = &p_so_tag->p_sr_tag->sreqs[event->seq];
  nm_sr_request_t *const p_request = *sreq;

  NM_SO_SR_LOG_IN();
  NM_SO_SR_TRACE("data sent for request = %p - tag %d , seq %d\n", p_request , event->tag, event->seq);
  
  *sreq = NULL;
  if(p_request && p_request->ref)
    {
      list_add_tail(&p_request->_link, &nm_sr_data.completed_sreq);
    }
  const nm_sr_event_info_t info = { .send_completed.p_request = p_request };
  nm_sr_request_signal(p_request, NM_SR_STATUS_SEND_COMPLETED);
  nm_sr_monitor_notify(p_request, NM_SR_STATUS_SEND_COMPLETED, &info);

  NM_SO_SR_LOG_OUT();
}

static void nm_sr_event_unexpected(const struct nm_so_event_s*const event)
{
  if(nm_sr_irecv_event_info.request)
    {
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&event->p_gate->tags, event->tag);
      nm_seq_t seq = p_so_tag->recv_seq_number++;
      struct nm_sr_tag_s*p_sr_tag = nm_sr_tag_init(p_so_tag);
      p_sr_tag->rreqs[seq] = nm_sr_irecv_event_info.request;
      nm_sr_irecv_event_info.request = NULL;
      nm_so_unpack(event->p_gate, event->tag, seq, nm_sr_irecv_event_info.data, nm_sr_irecv_event_info.len);
    }
  else
    {
      nm_sr_irecv_event_info.missed++;
      const nm_sr_event_info_t info = { 
	.recv_unexpected.p_gate = event->p_gate,
	.recv_unexpected.tag = event->tag,
	.recv_unexpected.len = event->len
      };
      nm_sr_monitor_notify(NULL, NM_SR_EVENT_RECV_UNEXPECTED, &info);
    }
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
  struct nm_so_tag_s*const p_so_tag = nm_so_tag_get(&event->p_gate->tags, event->tag);
  struct nm_sr_tag_s*const p_sr_tag = nm_sr_tag_init(p_so_tag);
  nm_sr_request_t**const rreq = &p_sr_tag->rreqs[event->seq];
  nm_sr_request_t *p_request = NULL;

  NM_SO_SR_LOG_IN();
  NM_SO_SR_TRACE("data received for request = %p - tag %d, seq %d\n", rreq, event->tag, event->seq);

  if(event->any_src) 
    {
      struct nm_sr_anysrc_s*p_sr_anysrc = nm_sr_anysrc_get(&nm_sr_data.any_src, event->tag);
      p_sr_anysrc->p_gate = event->p_gate;
      p_request = p_sr_anysrc->rreq;
      p_sr_anysrc->rreq = NULL;
    } 
  else 
    {
      p_request = *rreq;
      *rreq = NULL;
    }
  nm_sr_status_t sr_event;
  if(event->status & NM_SO_STATUS_UNPACK_CANCELLED)
    {
      sr_event = NM_SR_STATUS_RECV_COMPLETED | NM_SR_STATUS_RECV_CANCELLED;
    }
  else
    {
      sr_event = NM_SR_STATUS_RECV_COMPLETED;
    }
  if(p_request && p_request->ref)
    {
      list_add_tail(&p_request->_link, &nm_sr_data.completed_rreq);
    }
  const nm_sr_event_info_t info = { 
    .recv_completed.p_request = p_request,
    .recv_completed.p_gate = event->p_gate
  };
  nm_sr_request_signal(p_request, sr_event);
  nm_sr_monitor_notify(p_request, sr_event, &info);

  NM_SO_SR_LOG_OUT();
}

int nm_sr_progress(struct nm_core *p_core)
{
  /* We assume that PIOMan makes the communications progress */
#ifndef PIOMAN
  NM_SO_SR_LOG_IN();
  nm_schedule(p_core);
  NM_SO_SR_LOG_OUT();
#endif

  return NM_ESUCCESS;
}
