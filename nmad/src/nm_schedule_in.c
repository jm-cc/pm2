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

PADICO_MODULE_HOOK(NewMad_Core);


/** Poll active incoming requests 
 */
int nm_pw_poll_recv(struct nm_pkt_wrap_s*p_pw)
{
  int err;
  struct nm_core*p_core = p_pw->p_drv->p_core;
  static struct timespec next_poll = { .tv_sec = 0, .tv_nsec = 0 };

  nm_core_nolock_assert(p_core);
  
  NM_TRACEF("polling inbound request: gate %p, drv %p, trk %d",
	    p_pw->p_gate,
	    p_pw->p_drv,
	    p_pw->trk_id);
  if(p_pw->p_drv->driver->capabilities.min_period > 0)
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
	return -NM_EAGAIN;
      t.tv_nsec += 100 * 1000;
      if(t.tv_nsec > 1000000000)
	{
	  t.tv_nsec -= 1000000000;
	  t.tv_sec += 1;
	}
      next_poll = t;
      NM_WARN("nm_pw_poll_recv()- non-zero min_period.\n");
    }
  if(p_pw->p_gate)
    {
      const struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      err = (*r->driver->poll_recv_iov)(r->_status, p_pw);
    }
  else
    {
      err = (*p_pw->p_drv->driver->poll_recv_iov)(NULL, p_pw);
    }
   
  if(err == NM_ESUCCESS)
    {
#ifdef PIOMAN
      piom_ltask_completed(&p_pw->ltask);
#endif /* PIOMAN */
      nm_core_lock(p_core);
      nm_pw_process_complete_recv(p_core, p_pw);
      nm_core_unlock(p_core);
    }
  else if(err == -NM_ECLOSED)
    {
      nm_gate_t p_gate = p_pw->p_gate;
      nm_core_lock(p_core);
      p_gate->status = NM_GATE_STATUS_DISCONNECTING;
      if(p_pw->p_gdrv)
	{
	  p_pw->p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	  if (p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
	    {
	      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
	    }
	}
#ifdef PIOMAN
      piom_ltask_completed(&p_pw->ltask);
#else
      nm_pkt_wrap_list_erase(&p_core->pending_recv_list, p_pw);
#endif /* PIOMAN */
      nm_pw_ref_dec(p_pw);
      nm_core_unlock(p_core);
    }
  else if (err != -NM_EAGAIN)
    {
      NM_WARN("drv->poll_recv returned %d", err);
    }
  return err;
}

/** Actually post a recv request to the driver
 */
static int nm_pw_post_recv(struct nm_pkt_wrap_s*p_pw)
{
  int err = NM_ESUCCESS;
  
  NM_TRACEF("posting new recv request: gate %p, drv %p, trk %d",
	    p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);

  if(p_pw->p_gate)
    {
      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      /* update incoming request field in gate track if
	 the request specifies a gate */
      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = p_pw;
      err = r->driver->post_recv_iov(r->_status, p_pw);
    } 
  else
    {
      err = p_pw->p_drv->driver->post_recv_iov(NULL, p_pw);
    }
  
#ifndef PIOMAN
  /* always put the request in the list of pending requests,
   * even if it completes immediately. It will be removed from
   * the list by nm_pw_process_complete_recv().
   */
  nm_pkt_wrap_list_push_back(&p_pw->p_drv->p_core->pending_recv_list, p_pw);
#endif /* !PIOMAN */
  
  if(err == NM_ESUCCESS)
    {
      /* immediate succes, process request completion */
      NM_TRACEF("request completed immediately");
      nm_pw_process_complete_recv(p_pw->p_drv->p_core, p_pw);
    }
  else if(err == -NM_EAGAIN)
    {
      NM_TRACEF("new recv request pending: gate %p, drv %p, trk %d",
		p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
#ifdef PIOMAN
      nm_ltask_submit_poll_recv(p_pw);      
#endif /* PIOMAN */
    }
  else if(err != -NM_ECLOSED)
    {
      NM_FATAL("nm_pw_post_recv failed- err = %d", err);
    }
  
  return err;
}

void nm_drv_refill_recv(nm_drv_t p_drv)
{
  struct nm_core *p_core = p_drv->p_core;

  if(p_drv->driver->capabilities.has_recv_any)
    {
      /* recv any available- single pw with no gate */
      if(p_drv->p_in_rq == NULL)
	{
	  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_buffer();
	  nm_core_post_recv(p_pw, NM_GATE_NONE, NM_TRK_SMALL, p_drv);
	}
    }
  else
    {
      /* no recv any- post a pw per gate */
      nm_gate_t p_gate = NULL;
      NM_FOR_EACH_GATE(p_gate, p_core)
	{
	  /* Make sure the gate is not being connected
	   * This may happen when using multiple threads
	   */
	  if(p_gate->status == NM_GATE_STATUS_CONNECTED) 
	    {
	      struct nm_gate_drv *p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	      if(p_gdrv != NULL && !p_gdrv->active_recv[NM_TRK_SMALL])
		{
		  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_buffer();
		  nm_core_post_recv(p_pw, p_gate, NM_TRK_SMALL, p_drv);
		}
	    }
	}
    }
}


void nm_drv_post_recv(nm_drv_t p_drv)
{
  struct nm_pkt_wrap_s*p_pw = NULL;
  do
    {
      p_pw = nm_pw_post_lfqueue_dequeue(&p_drv->post_recv);
      if(p_pw)
	{
  	  if(!(p_pw->p_gate && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id]))
	    {		    
	      NM_TRACEF("posting inbound request");
	      nm_pw_post_recv(p_pw);
	    }
	  else
	    {
	      nm_pw_post_lfqueue_enqueue(&p_drv->post_recv, p_pw);
	      /* the driver is busy, so don't insist */
	      break;
	    }
	}
    }
  while(p_pw);
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
