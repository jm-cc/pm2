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


/** Handle incoming requests that have been processed by the driver-dependent
   code.
   - requests may be successful or failed, and should be handled appropriately
    --> this function is responsible for the processing common to both cases
*/
__inline__ int nm_process_complete_recv_rq(struct nm_core     *p_core,
					   struct nm_pkt_wrap *p_pw,
					   int		      _err)
{
  int err;
  
  NM_TRACEF("recv request complete: gate %d, drv %d, trk %d, proto %d, seq %d",
	    p_pw->p_gate->id,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id,
	    p_pw->seq);
  
  /* clear the input request field in gate track */
  if (p_pw->p_gate && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw) {
    p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
  }

#ifdef PIOMAN
  piom_req_success(&p_pw->inst);
#endif

  if (_err == NM_ESUCCESS) {
    err	= nm_so_in_process_success_rq(p_core, p_pw);
    if (err != NM_ESUCCESS) {
      NM_DISPF("process_successful_recv_rq returned %d", err);
    }
  } else {
    err	= nm_so_in_process_failed_rq(p_core, p_pw, _err);
    if (err != NM_ESUCCESS) {
      NM_DISPF("process_failed_recv_rq returned %d", err);
    }
  }
  
  err = NM_ESUCCESS;
  
  return err;
}

/** Poll active incoming requests in poll_slist.
 */
__inline__ int nm_poll_recv(struct nm_pkt_wrap*p_pw)
{
  int err;

  NM_TRACEF("polling inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
	    p_pw->p_gate?p_pw->p_gate->id:-1,
	    p_pw->p_drv->id,
	    p_pw->p_trk->id,
	    p_pw->proto_id,
	    p_pw->seq);
  /* poll request 					*/
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if(p_pw->p_gate)
    {
      err = r->driver->poll_recv_iov(r->_status, p_pw);
    }
  else
    {
      err = r->driver->poll_recv_iov_all(p_pw);
    }
   
  if(err == NM_ESUCCESS)
    {
      err = nm_process_complete_recv_rq(p_pw->p_gate->p_core, p_pw, err);
    }
  else if (err != -NM_EAGAIN)
    {
      NM_DISPF("drv->poll_recv returned %d", err);
    }
  
  return err;
}

/** Transfer request from the post_slist to the pending_slist if the
   state of the drv/trk targeted by a request allows this request to
   be posted.
 */
static __inline__ int nm_post_recv(struct nm_pkt_wrap*p_pw)
{
  int err = NM_ESUCCESS;
  
  /* check track availability				*/
  if (p_pw->p_gate && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id])
    {
      /* a recv is already pending on this gate/track */
      err = -NM_EBUSY;
    }
  else
    {
      /* ready to post request				*/
      
      /* update incoming request field in gate track if
	 the request specifies a gate */
      if (p_pw->p_gate)
	{
	  p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = p_pw;
	}
      
      NM_TRACEF("posting new recv request: gate %d, drv %d, trk %d, proto %d, seq %d",
		p_pw->p_gate?p_pw->p_gate->id:-1,
		p_pw->p_drv->id,
		p_pw->p_trk->id,
		p_pw->proto_id,
		p_pw->seq);
#ifdef PIOMAN
      piom_req_init(&p_pw->inst);
      p_pw->inst.server = &p_pw->p_drv->server;
      p_pw->which = RECV;
#endif /* PIOMAN */
      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      /* post request */
      if(p_pw->p_gate)
	{
	  err = r->driver->post_recv_iov(r->_status, p_pw);
	} 
      else
	{
	  err = r->driver->post_recv_iov_all(p_pw);
	}
      
      /* process post command status				*/
      
      if (err == -NM_EAGAIN)
	{
	  NM_TRACEF("new recv request pending: gate %d, drv %d, trk %d, proto %d, seq %d",
		    p_pw->p_gate?p_pw->p_gate->id:-1,
		    p_pw->p_drv->id,
		    p_pw->p_trk->id,
		    p_pw->proto_id,
		    p_pw->seq);
#ifdef PIOMAN
	  p_pw->inst.state |= PIOM_STATE_DONT_POLL_FIRST | PIOM_STATE_ONE_SHOT;
      /* TODO : implementer les syscall */
#ifdef PIOM_BLOCKING_CALLS
	  if (! ((r->driver->get_capabilities(p_pw->p_drv))->is_exportable))
	    p_pw->inst.func_to_use = PIOM_FUNC_POLLING;
#endif
	  piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);
#else /* PIOMAN */
	  /* put the request in the list of pending requests */
	  tbx_slist_append(p_pw->p_gate->p_core->so_sched.pending_recv_req, p_pw);
#endif /* PIOMAN */
	}
      else
	{
	  /* immediate succes, process request completion */
	  NM_TRACEF("request completed immediately");
	  
	  if (err != NM_ESUCCESS) {
	    NM_DISPF("drv->post_recv returned %d", err);
	  }
	  nm_process_complete_recv_rq(p_pw->p_gate->p_core, p_pw, err);
	}
    }
  
  return err;
}

/** Main scheduler func for incoming requests.
   - this function must be called once for each gate on a regular basis
 */
int nm_sched_in(struct nm_core *p_core)
{
  int err;
  
#ifndef PIOMAN
  /* poll pending requests */
  if (!tbx_slist_is_nil(p_core->so_sched.pending_recv_req))
    {
      NM_TRACEF("polling inbound requests");
      tbx_slist_ref_to_head(p_core->so_sched.pending_recv_req);
      tbx_bool_t todo = tbx_true;
      do
	{
	  struct nm_pkt_wrap*p_pw = tbx_slist_ref_get(p_core->so_sched.pending_recv_req);
	  err = nm_poll_recv(p_pw);
	  if(err == NM_ESUCCESS)
	    {
	      todo = tbx_slist_ref_extract_and_forward(p_core->so_sched.pending_recv_req, NULL);
	    }
	  else
	    {
	      todo = tbx_slist_ref_forward(p_core->so_sched.pending_recv_req);
	    }
	}
      while(todo);
    }
#endif /* PIOMAN */

  /* post new requests */
  
  if (!tbx_slist_is_nil(p_core->so_sched.post_recv_req))
    {
      NM_TRACEF("posting inbound requests");
      tbx_slist_ref_to_head(p_core->so_sched.post_recv_req);
      tbx_bool_t todo = tbx_true;
      do
	{
	  struct nm_pkt_wrap*p_pw = tbx_slist_ref_get(p_core->so_sched.post_recv_req);
	  err = nm_post_recv(p_pw);
	  if(err == -NM_EBUSY)
	    {
	      todo = tbx_slist_ref_forward(p_core->so_sched.post_recv_req);
	    }
	  else
	    {
	      todo = tbx_slist_ref_extract_and_forward(p_core->so_sched.post_recv_req, NULL);
	    }
	}
      while(todo);
    }
  
  err = NM_ESUCCESS;
  
  return err;
}
