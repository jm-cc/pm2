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
__inline__ int nm_pw_poll_recv(struct nm_pkt_wrap*p_pw)
{
  int err;

  static struct timespec next_poll = { .tv_sec = 0, .tv_nsec = 0 };

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
    }
  if(p_pw->p_gate)
    {
      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      err = r->driver->poll_recv_iov(r->_status, p_pw);
    }
  else
    {
      err = p_pw->p_drv->driver->poll_recv_iov(NULL, p_pw);
    }
   
  if(err == NM_ESUCCESS)
    {
#ifdef PIOMAN_POLL
      piom_ltask_completed(&p_pw->ltask);
#endif /* PIOMAN_POLL */
      err = nm_so_process_complete_recv(p_pw->p_drv->p_core, p_pw);
    }
  else if(err == -NM_ECLOSED)
    {
      struct nm_gate*p_gate = p_pw->p_gate;
      p_gate->status = NM_GATE_STATUS_DISCONNECTING;
      if(p_pw->p_gdrv)
	{
	  p_pw->p_gdrv->active_recv[NM_TRK_SMALL] = 0;
	  if (p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
	    {
	      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
	    }
	}
#ifdef NMAD_POLL
      tbx_fast_list_del(&p_pw->link);
#endif /* NMAD_POLL */
#ifdef PIOMAN_POLL
      piom_ltask_completed(&p_pw->ltask);
#endif /* PIOMAN_POLL */
     
      nm_pw_ref_dec(p_pw);
    }
  else if (err != -NM_EAGAIN)
    {
      NM_WARN("drv->poll_recv returned %d", err);
    }
  return err;
}

/** Actually post a recv request to the driver
 */
static int nm_pw_post_recv(struct nm_pkt_wrap*p_pw)
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
  
#ifdef NMAD_POLL
  /* always put the request in the list of pending requests,
   * even if it completes immediately. It will be removed from
   * the list by nm_so_process_complete_recv().
   */
  tbx_fast_list_add_tail(&p_pw->link, &p_pw->p_drv->p_core->pending_recv_list);
#endif /* NMAD_POLL */
  
  if(err == NM_ESUCCESS)
    {
      /* immediate succes, process request completion */
      NM_TRACEF("request completed immediately");
      nm_so_process_complete_recv(p_pw->p_drv->p_core, p_pw);
    }
  else if(err == -NM_EAGAIN)
    {
      NM_TRACEF("new recv request pending: gate %p, drv %p, trk %d",
		p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
#ifdef PIOMAN_POLL
      nm_ltask_submit_poll_recv(p_pw);      
#endif /* PIOMAN_POLL */
    }
  else if(err != -NM_ECLOSED)
    {
      TBX_FAILUREF("nm_pw_post_recv failed- err = %d", err);
    }
  
  return err;
}

void nm_drv_refill_recv(struct nm_drv*p_drv)
{
  struct nm_core *p_core = p_drv->p_core;

  if(p_drv->driver->capabilities.has_recv_any)
    {
      /* recv any available- single pw with no gate */
      struct nm_pkt_wrap *p_pw;
      if(p_drv->p_in_rq == NULL)
	{
	  nm_so_pw_alloc(NM_PW_BUFFER, &p_pw);
	  nm_core_post_recv(p_pw, NM_GATE_NONE, NM_TRK_SMALL, p_drv);
	}
    }
  else
    {
      /* no recv any- post a pw per gate */
      struct nm_gate*p_gate = NULL;
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
		  struct nm_pkt_wrap *p_pw;
		  nm_so_pw_alloc(NM_PW_BUFFER, &p_pw);
		  nm_core_post_recv(p_pw, p_gate, NM_TRK_SMALL, p_drv);
		}
	    }
	}
    }
}


void nm_drv_post_recv(struct nm_drv*p_drv)
{
  struct nm_pkt_wrap*p_pw = NULL;
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

int nm_piom_block_recv(struct nm_pkt_wrap  *p_pw)
{
  NM_TRACEF("waiting inbound request: gate %p, drv %p, trk %d, proto %d",
	    p_pw->p_gate,
	    p_pw->p_drv,
	    p_pw->trk_id,
	    p_pw->proto_id);
  
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do 
    {
      err = r->driver->wait_recv_iov(r->_status, p_pw);
    }
  while(err == -NM_EAGAIN);
  
  nmad_lock();

  if (err != NM_ESUCCESS) {
    NM_LOGF("drv->wait_recv returned %d", err);
  }

  piom_ltask_completed(&p_pw->ltask);

  /* process complete request */
  err = nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
  
  return err;
}
#endif /* PIOM_BLOCKING_CALLS */
