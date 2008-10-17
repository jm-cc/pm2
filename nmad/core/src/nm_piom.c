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
#ifdef PIOMAN

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>


int nm_piom_poll(piom_server_t            server,
		 piom_op_t                _op,
		 piom_req_t               req,
		 int                       nb_ev,
		 int                       option)
{
  nmad_lock();
  struct nm_pkt_wrap * pkt= struct_up(req, struct nm_pkt_wrap, inst);
  int err;
  
  if (pkt->which == SEND)
    err = nm_poll_send(pkt);
  else if(pkt->which == RECV)
    err = nm_poll_recv(pkt);
  else
    {
      err = nm_piom_post_all(pkt->p_drv->p_core);
    }
  nmad_unlock();
  PROF_EVENT(piom_polling_done);
  return err;
}

#if 0 
/* disabled for now- unused anyway */
static int nm_piom_poll_any(piom_server_t            server,
			    piom_op_t                _op,
			    piom_req_t               req,
			    int                       nb_ev,
			    int                       option)
{
  nmad_lock();
  struct nm_drv *p_drv = struct_up(server, struct nm_drv, server);
  int i;
  int j = 0;
  struct nm_pkt_wrap *p_pw;
  int err = p_drv->driver->poll_send_any_iov(NULL, &p_pw);
  /* process poll command status				*/
  if (err == -NM_EAGAIN) {
    err = p_drv->driver->poll_recv_any_iov(NULL, &p_pw);
    if(err == -NM_EAGAIN) {
      /* not complete, try again later
	 - leave the request in the list and go to next
      */
      nmad_unlock();
      return err;
    }
  }
  
  if (p_pw->which == SEND) {
    if (err != -NM_EAGAIN){
      const int req_nb = p_pw->p_gate->out_req_nb;
      
      struct nm_pkt_wrap *pkt2;
      /* met a jour la requete */
      piom_req_success(&p_pw->inst);
      nm_process_complete_send_rq(p_pw->p_gate, p_pw, err);
      
      for (i=0;i<req_nb;i++) { /* supprime la requete de la liste */
	pkt2 = p_pw->p_gate->out_req_list[i];
	if (pkt2->err == -NM_EAGAIN) {
	  pkt2->p_gate->out_req_list[j] = pkt2->p_gate->out_req_list[i];
	  j++;
	}
      }
    }
  } else {
    if (err != -NM_EAGAIN){
      piom_req_success(&p_pw->inst);
      /* supprimer la req de la liste des requetes pending */
      tbx_slist_search_and_extract(p_pw->slist,NULL,p_pw);
      /* process complete request */
      err = nm_process_complete_recv_rq(p_pw->p_gate->p_core, p_pw, err);
      
      if (err < 0) { // possible ?
	NM_LOGF("nm_process_complete_rq returned %d", err);
      }
    }
  }
  nm_piom_post_all(p_pw->p_gate->p_core);
  nmad_unlock();
  return err;
}
#endif 



/* Posting functions */
int nm_piom_post_all(struct nm_core	 *p_core)
{
  int err = -NM_EUNKNOWN;
  static int post_pending = 0;
  
  if (post_pending) {
    err = -NM_EAGAIN;
    goto out;
  }
  post_pending = 1;
  
  /* schedule & post out requests */
  nm_sched_out(p_core);
  
  /* post new receive requests */
  nm_sched_in(p_core);
  
  post_pending = 0;
 out:
  return err;
}



#ifdef PIOM_BLOCKING_CALLS

int nm_piom_block(piom_server_t server,
		  piom_op_t     _op,
		  piom_req_t    req,
		  int            nb_ev,
		  int            option)
{
  nmad_lock();
  struct nm_pkt_wrap * pkt = struct_up(req, struct nm_pkt_wrap, inst);
  int err;
  if (pkt->which == SEND)
    err = nm_piom_block_send(pkt);
  else
    err = nm_piom_block_recv(pkt);
  nmad_unlock();
  return err;
}

int nm_piom_block_send(struct nm_pkt_wrap  *p_pw)
{
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->poll_send_iov(r->_status, p_pw);
  nmad_lock();
  if (err != -NM_EAGAIN)
    {
      if (tbx_unlikely(err < 0))
	{
	  NM_DISPF("poll_send returned %d", err);
	}
      nm_process_complete_send_rq(p_pw->p_gate, p_pw, err);
    }
  return err; 
 }

int nm_piom_block_recv(struct nm_pkt_wrap  *p_pw)
{
  NM_TRACEF("waiting inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
	    p_pw->p_gate?p_pw->p_gate->id:-1,
	    p_pw->p_drv->id,
	    p_pw->p_trk->id,
	    p_pw->proto_id,
	    p_pw->seq);
  
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->wait_recv_iov(r->_status, p_pw);
  nmad_lock();
  
  /* process poll command status				*/
  if (err == -NM_EAGAIN) {
    /* not complete, try again later
       - leave the request in the list and go to next
    */
    return err;
  }
  
  if (err != NM_ESUCCESS) {
    NM_LOGF("drv->wait_recv returned %d", err);
  }
  piom_req_success(&p_pw->inst);
  /* process complete request */
  err = nm_process_complete_recv_rq(p_pw->p_gate->p_core, p_pw, err);
  
  return err;
}

int nm_piom_block_any(piom_server_t            server,
		     piom_op_t                _op,
		     piom_req_t               req,
		     int                       nb_ev,
		     int                       option)
{
  nmad_lock();
  int err;
  struct nm_drv *p_drv = struct_up(server, struct nm_drv, server);
  
  struct nm_pkt_wrap *p_pw;
  err = p_drv->driver->wait_send_any_iov(NULL, &p_pw);
  /* process poll command status				*/
  if (err == -NM_EAGAIN) {
    err = p_drv->driver->wait_recv_any_iov(NULL, &p_pw);
    if( err == -NM_EAGAIN) {
      /* not complete, try again later
	 - leave the request in the list and go to next
      */
      nmad_unlock();
      return err;
    }
  }
  
  if (p_pw->which == SEND)
    {
      if (err != -NM_EAGAIN)
	{
	  piom_req_success(&p_pw->inst);
	  nm_process_complete_send_rq(p_pw->p_gate, p_pw, err);
//	  tbx_slist_search_and_extract(p_pw->p_gate->p_core->so_sched.out_req_list, NULL, p_pw);
      
	}
    }
  else 
    {
      if (err != -NM_EAGAIN)
	{
	  piom_req_success(&p_pw->inst);
	  /* supprimer la req de la liste des requetes pending */
//	  tbx_slist_search_and_extract(p_pw->p_gate->p_core->so_sched.pending_recv_req, NULL, p_pw);
	  /* process complete request */
	  err = nm_process_complete_recv_rq(p_pw->p_gate->p_core, p_pw, err);
	  
	  if (err < 0) { // possible ?
	    NM_LOGF("nm_process_complete_rq returned %d", err);
	  }
	}
    }
  nmad_unlock();
  return err;
}

#endif

#endif /* PIOMAN */
