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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>

PADICO_MODULE_HOOK(NewMad_Core);

static void nm_pw_post_recv(struct nm_pkt_wrap_s*p_pw);
static void nm_pw_poll_recv(struct nm_pkt_wrap_s*p_pw);
static void nm_pw_poll_recv_any(struct nm_pkt_wrap_s*p_pw);

/** Post a pw for recv on a driver.
 * either p_gate or p_drv may be NULL, but not both at the same time.
 */
void nm_core_post_recv(struct nm_pkt_wrap_s*p_pw, nm_gate_t p_gate, 
		       nm_trk_id_t trk_id, nm_drv_t p_drv)
{
  nm_pw_assign(p_pw, trk_id, p_drv, p_gate);
  p_pw->flags |= NM_PW_RECV;
  if(p_gate)
    {
      struct nm_trk_s*p_trk = p_pw->p_trk;
      assert(p_trk != NULL);
      assert(p_trk->p_pw_recv == NULL);
      p_trk->p_pw_recv = p_pw;
    }
  else
    {
      p_pw->flags |= NM_PW_POLL_ANY;
    }
#ifdef PIOMAN
  nm_ltask_submit_pw_recv(p_pw);
#else /* PIOMAN */
  nm_pw_recv_progress(p_pw);
#endif /* PIOMAN */
}

/** Make progress on an active incoming request.
 */
void nm_pw_recv_progress(struct nm_pkt_wrap_s*p_pw)
{
  const nm_pw_flag_t flags = p_pw->flags;
  if(flags & NM_PW_POLL_ANY)
    {
      /* polling on any before posting */
      nm_pw_poll_recv_any(p_pw);
    }
  else if(flags & NM_PW_POSTED)
    {
      /* already posted- poll */
      nm_pw_poll_recv(p_pw);
    }
  else
    {
      /* not posted yet */
      nm_pw_post_recv(p_pw);
    }
}

static void nm_pw_poll_recv_any(struct nm_pkt_wrap_s*p_pw)
{
  struct nm_core*p_core = p_pw->p_drv->p_core;
  nm_core_nolock_assert(p_core);
  assert(p_pw->p_trk == NULL);
  const struct nm_minidriver_iface_s*p_driver = p_pw->p_drv->driver;
  assert(p_driver->recv_poll_any != NULL);
  void*_status = NULL;
  int rc = (*p_driver->recv_poll_any)(p_pw->p_drv->minidriver_context, &_status);
  if(rc == NM_ESUCCESS)
    {
      assert(_status != NULL);
      p_pw->flags &= ~NM_PW_POLL_ANY;
      p_pw->p_drv->p_pw_recv_any = NULL;
      /* reverse resolution status -> gate */
      struct nm_gate_s*p_gate;
      NM_FOR_EACH_GATE(p_gate, p_core)
        {
          struct nm_trk_s*p_trk = &p_gate->trks[p_pw->trk_id];
          const struct puk_receptacle_NewMad_minidriver_s*r = &p_trk->receptacle;
          if(_status == r->_status)
            {
              nm_pw_assign(p_pw, p_pw->trk_id, p_pw->p_drv, p_gate);
              break;
            }
        }
      assert(p_pw->p_gate != NULL);
      nm_pw_post_recv(p_pw);
    }
}

/** Poll active incoming requests 
 */
static void nm_pw_poll_recv(struct nm_pkt_wrap_s*p_pw)
{
  int err = NM_ESUCCESS;
  struct nm_core*p_core = p_pw->p_drv->p_core;
  static struct timespec next_poll = { .tv_sec = 0, .tv_nsec = 0 };

  nm_core_nolock_assert(p_core);
  
  NM_TRACEF("polling inbound request: gate %p, drv %p, trk %d",
	    p_pw->p_gate,
	    p_pw->p_drv,
	    p_pw->trk_id);
  if(p_pw->p_drv->props.capabilities.min_period > 0)
    {
      struct timespec t;
#ifdef CLOCK_MONOTONIC_RAW
      const clockid_t clock = CLOCK_MONOTONIC_RAW;
#else
      const clockid_t clock = CLOCK_MONOTONIC;
#endif
      clock_gettime(clock, &t);
      if(t.tv_sec < next_poll.tv_sec ||
	 (t.tv_sec == next_poll.tv_sec && t.tv_nsec < next_poll.tv_nsec))
	return;
      t.tv_nsec += 100 * 1000;
      if(t.tv_nsec > 1000000000)
	{
	  t.tv_nsec -= 1000000000;
	  t.tv_sec += 1;
	}
      next_poll = t;
      NM_WARN("nm_pw_poll_recv()- non-zero min_period.\n");
    }
  assert(p_pw->p_trk != NULL);
  assert(p_pw->p_trk == &p_pw->p_gate->trks[p_pw->trk_id]);
  const struct puk_receptacle_NewMad_minidriver_s*r = &p_pw->p_trk->receptacle;
  if(p_pw->flags & NM_PW_BUF_RECV)
    {
      void*buf = NULL;
      nm_len_t len = NM_LEN_UNDEFINED;
      err = (*r->driver->buf_recv_poll)(r->_status, &buf, &len);
      if(err == NM_ESUCCESS)
        {
          assert(len != NM_LEN_UNDEFINED);
          assert(len <= p_pw->max_len);
          p_pw->v[0].iov_base = buf;
          p_pw->v[0].iov_len = len;
          p_pw->length = len;
        }
    }
  else
    {
      err = (*r->driver->poll_one)(r->_status);
    }
#ifdef DEBUG
  if((err == NM_ESUCCESS) && (p_pw->p_data == NULL) && 
     (p_pw->p_drv->driver->recv_init == NULL) &&
     (p_pw->p_drv->driver->buf_recv_poll == NULL) &&
     (p_pw->p_drv->props.capabilities.supports_data))
    {
      struct nm_data_s*p_data = &p_pw->p_trk->rdata;
      assert(!nm_data_isnull(p_data));
      nm_data_null_build(p_data);
    }
#endif /* DEBUG */
   
  if(err == NM_ESUCCESS)
    {
#ifndef PIOMAN
      nm_pkt_wrap_list_remove(&p_core->pending_recv_list, p_pw);
#endif /* !PIOMAN */
      nm_pw_completed_enqueue(p_core, p_pw);
    }
  else if(err == -NM_ECLOSED)
    {
      p_pw->flags |= NM_PW_CLOSED;
#ifndef PIOMAN
      nm_pkt_wrap_list_remove(&p_core->pending_recv_list, p_pw);
#endif /* !PIOMAN */
      nm_pw_completed_enqueue(p_core, p_pw);
    }
  else if (err != -NM_EAGAIN)
    {
      NM_WARN("drv->poll_recv returned %d", err);
    }
}

/** Actually post a recv request to the driver
 */
static void nm_pw_post_recv(struct nm_pkt_wrap_s*p_pw)
{
  struct nm_core*p_core = p_pw->p_drv->p_core;
  /* no locck needed; only this ltask is allow to touch the pw */
  nm_core_nolock_assert(p_core);

  if(p_pw->p_gate)
    {
      assert(p_pw->p_trk == &p_pw->p_gate->trks[p_pw->trk_id]);
      struct puk_receptacle_NewMad_minidriver_s*r = &p_pw->p_trk->receptacle;
      if((p_pw->p_data != NULL) && (p_pw->v_nb == 0))
	{
	  /* pw content is only p_data */
	  assert(r->driver->recv_data != NULL);
	  (*r->driver->recv_data)(r->_status, p_pw->p_data, p_pw->chunk_offset, p_pw->length);
	}
      else
	{
	  /* no p_data, or data has been flattened */
	  if(r->driver->buf_recv_poll)
	    {
	      p_pw->flags |= NM_PW_BUF_RECV;
	    }
	  else if(r->driver->recv_init)
	    {
	      (*r->driver->recv_init)(r->_status, &p_pw->v[0], p_pw->v_nb);
	    }
	  else
	    {
	      assert(r->driver->recv_data);
	      struct nm_data_s*p_data = &p_pw->p_trk->rdata;
	      assert(nm_data_isnull(p_data));
	      nm_data_iov_set(p_data, (struct nm_data_iov_s){ .v = &p_pw->v[0], .n = p_pw->v_nb });
	      (*r->driver->recv_data)(r->_status, p_data, 0 /* chunk_offset */, p_pw->length);
	    }
	}
    } 
  else
    {
      NM_FATAL("nmad: FATAL- recv_any not implemented yet.\n");
    }
  
#ifndef PIOMAN
  /* always put the request in the list of pending requests,
   * even if it completes immediately. It will be removed from
   * the list by nm_pw_process_complete_recv().
   */
  nm_pkt_wrap_list_push_back(&p_core->pending_recv_list, p_pw);
#endif /* !PIOMAN */
  p_pw->flags |= NM_PW_POSTED;

  nm_pw_poll_recv(p_pw);
}

void nm_drv_refill_recv(nm_drv_t p_drv, nm_gate_t p_gate)
{
  struct nm_core*p_core = p_drv->p_core;
  nm_core_lock_assert(p_core);
  if(!p_core->enable_schedopt)
    return;
  if(p_drv->props.capabilities.has_recv_any)
    {
      /* recv any available- single pw with no gate */
      if(p_drv->p_pw_recv_any == NULL)
	{
	  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_buffer(p_core);
	  nm_core_post_recv(p_pw, NM_GATE_NONE, NM_TRK_SMALL, p_drv);
	}
    }
  else
    {
      /* no recv any- post a pw per gate */
      assert(p_gate != NULL);
      assert(p_gate->status == NM_GATE_STATUS_CONNECTED);
      struct nm_trk_s*p_trk = nm_gate_trk_get(p_gate, p_drv);
      assert(p_trk != NULL && !p_trk->p_pw_recv);
      assert(p_trk->kind == nm_trk_small);
      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_buffer(p_core);
      nm_core_post_recv(p_pw, p_gate, NM_TRK_SMALL, p_drv);
    }
}


#ifdef PIOM_BLOCKING_CALLS

int nm_piom_block_recv(struct nm_pkt_wrap_s  *p_pw)
{
  NM_TRACEF("waiting inbound request: gate %p, drv %p, trk %d, proto %d",
	    p_pw->p_gate,
	    p_pw->p_drv,
	    p_pw->trk_id,
	    p_pw->proto_id);
  
  nm_core_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do 
    {
      err = r->driver->wait_recv_iov(r->_status, p_pw);
    }
  while(err == -NM_EAGAIN);
  
  nm_core_lock();

  if (err != NM_ESUCCESS) {
    NM_WARN("drv->wait_recv returned %d", err);
  }

  piom_ltask_completed(&p_pw->ltask);

  /* process complete request */
  nm_pw_process_complete_recv(p_pw->p_gate->p_core, p_pw);
  
  return err;
}
#endif /* PIOM_BLOCKING_CALLS */
