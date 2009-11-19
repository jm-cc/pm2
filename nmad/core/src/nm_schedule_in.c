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

/** Poll active incoming requests in poll_slist.
 */
__inline__ int nm_poll_recv(struct nm_pkt_wrap*p_pw)
{
  int err;

  NM_TRACEF("polling inbound request: gate %d, drv %d, trk %d, proto %d",
	    p_pw->p_gate?p_pw->p_gate->id:-1,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id);
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
      err = nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
    }
  else if(err == -NM_ECLOSED)
    {
      struct nm_gate*p_gate = p_pw->p_gate;
      p_gate->status = NM_GATE_STATUS_DISCONNECTING;
      p_gate->active_recv[p_pw->p_drv->id][NM_TRK_SMALL] = 0;
      if (p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
	{
	  p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
	}
      nm_so_pw_free(p_pw);
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
      
      NM_TRACEF("posting new recv request: gate %d, drv %d, trk %d, proto %d",
		p_pw->p_gate?p_pw->p_gate->id:-1,
		p_pw->p_drv->id,
		p_pw->trk_id,
		p_pw->proto_id);
#if(defined(PIOMAN_POLL) && !defined(PIOM_ENABLE_LTASKS))
      piom_req_init(&p_pw->inst);
      p_pw->inst.server = &p_pw->p_drv->server;
      p_pw->which = RECV;
#endif /* PIOMAN_POLL */
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
	  NM_TRACEF("new recv request pending: gate %d, drv %d, trk %d, proto %d",
		    p_pw->p_gate?p_pw->p_gate->id:-1,
		    p_pw->p_drv->id,
		    p_pw->trk_id,
		    p_pw->proto_id);
#ifdef PIOM_ENABLE_LTASKS
	  nm_submit_poll_recv_ltask(p_pw);
#else

#ifdef NMAD_POLL
	  /* put the request in the list of pending requests */
	  nm_poll_lock_in(&p_pw->p_gate->p_core->so_sched, p_pw->p_drv->id);
	  tbx_slist_append(p_pw->p_gate->p_core->so_sched.pending_recv_list[p_pw->p_drv->id], p_pw);
	  nm_poll_unlock_in(&p_pw->p_gate->p_core->so_sched, p_pw->p_drv->id);

#else /* NMAD_POLL */
	  p_pw->inst.state |= PIOM_STATE_DONT_POLL_FIRST | PIOM_STATE_ONE_SHOT;
/* TODO : implementer les syscall */
#ifdef PIOM_BLOCKING_CALLS
	  if (! ((r->driver->get_capabilities(p_pw->p_drv))->is_exportable))
		  p_pw->inst.func_to_use = PIOM_FUNC_POLLING;
#endif /* PIOM_BLOCKING_CALLS */
	  piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);
#endif /* NMAD_POLL */
	  
#endif /* PIOM_ENABLE_LTASKS */
	}
      else if(err == NM_ESUCCESS)
	{
	  /* immediate succes, process request completion */
	  NM_TRACEF("request completed immediately");
	  nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
	}
      else if(err != -NM_ECLOSED)
	{
	  TBX_FAILUREF("nm_post_recv failed- err = %d", err);
	}
    }
  
  return err;
}

/** Main scheduler func for incoming requests.
   - this function must be called on a regular basis
 */
int nm_sched_in(struct nm_core *p_core)
{
  /* refill input requests */
  const int nb_drivers = p_core->nb_drivers;
  const int nb_gates = p_core->nb_gates;
  nm_drv_id_t drv;
  int i;
  int err;

  for(drv = 0; drv < nb_drivers; drv++)
    {
      for(i = 0; i < nb_gates; i++)
	{
	  struct nm_gate*p_gate = &p_core->gate_array[i];
	  if(p_gate->status == NM_GATE_STATUS_CONNECTED && !p_gate->active_recv[drv][NM_TRK_SMALL])
	    {
	      struct nm_pkt_wrap *p_pw;
	      nm_so_pw_alloc(NM_PW_BUFFER, &p_pw);
	      nm_core_post_recv(p_pw, p_gate, NM_TRK_SMALL, drv);
	    }
	}
    }

#ifdef NMAD_POLL
  /* poll pending requests */
  for(i=0; i<NM_DRV_MAX; i++) {
    if (tbx_slist_is_nil(p_core->so_sched.pending_recv_list[i]))
      continue;
    nm_poll_lock_in(&p_core->so_sched, i);
    if (!tbx_slist_is_nil(p_core->so_sched.pending_recv_list[i]))
    {
      NM_TRACEF("polling inbound requests");
      struct nm_pkt_wrap* first_pw = NULL;
      do {
  	  struct nm_pkt_wrap*p_pw = tbx_slist_remove_from_head(p_core->so_sched.pending_recv_list[i]);

	  nm_poll_unlock_in(&p_core->so_sched, i);
	  err = nm_poll_recv(p_pw);
	  nm_poll_lock_in(&p_core->so_sched, i);

	  if(err != NM_ESUCCESS && err != -NM_ECLOSED)
	    {
	      if(!first_pw)
	        first_pw=p_pw;
	      tbx_slist_add_at_tail(p_core->so_sched.pending_recv_list[i], p_pw);
	    }
	}
      while(!tbx_slist_is_nil(p_core->so_sched.pending_recv_list[i])
	      &&tbx_slist_peek_from_head(p_core->so_sched.pending_recv_list[i])!= first_pw);
    }
    nm_poll_unlock_in(&p_core->so_sched, i);
  }
#endif /* NMAD_POLL */

  /* post new requests */
  FOR_EACH_DRIVER(i, p_core)
  {
    int trk;
    for(trk=0;trk<NM_SO_MAX_TRACKS;trk++){
      if(tbx_slist_is_nil(p_core->so_sched.post_recv_list[i][trk]))
        goto next;

      nm_so_lock_in(&p_core->so_sched, i);  
      
      if (!tbx_slist_is_nil(p_core->so_sched.post_recv_list[i][trk]))
      {
        NM_TRACEF("posting inbound requests");
	struct nm_pkt_wrap* first_pw = NULL;
	do
	{
	  struct nm_pkt_wrap*p_pw = tbx_slist_remove_from_head(p_core->so_sched.post_recv_list[i][trk]);

	  nm_so_unlock_in(&p_core->so_sched, i);
	  const int err = nm_post_recv(p_pw);
	  nm_so_lock_in(&p_core->so_sched, i);

	  if(err == -NM_EBUSY)
	  {
	    tbx_slist_add_at_tail(p_core->so_sched.post_recv_list[i][trk], p_pw);
	    /* the driver is busy, so don't insist */
	    goto out;
	  }
	}
	while(!tbx_slist_is_nil(p_core->so_sched.post_recv_list[i][trk])
	      && tbx_slist_peek_from_head(p_core->so_sched.post_recv_list[i][trk])!= first_pw);
      }
    out:
      nm_so_unlock_in(&p_core->so_sched, i);
    next:
      err = NM_ESUCCESS;
    }
  }
    
  return err;
}

#ifdef PIOM_BLOCKING_CALLS

int nm_piom_block_recv(struct nm_pkt_wrap  *p_pw)
{
  NM_TRACEF("waiting inbound request: gate %d, drv %d, trk %d, proto %d",
	    p_pw->p_gate?p_pw->p_gate->id:-1,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id);
  
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do {
	  err = r->driver->wait_recv_iov(r->_status, p_pw);
  }while(err == -NM_EAGAIN);

  nmad_lock();

  if (err != NM_ESUCCESS) {
    NM_LOGF("drv->wait_recv returned %d", err);
  }
#ifdef PIOM_ENABLE_LTASKS
  piom_ltask_completed(&p_pw->ltask);
#else
  piom_req_success(&p_pw->inst);
#endif
  /* process complete request */
  err = nm_so_process_complete_recv(p_pw->p_gate->p_core, p_pw);
  
  return err;
}
#endif /* PIOM_BLOCKING_CALLS */
