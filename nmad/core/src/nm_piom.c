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

#ifdef PIOMAN_POLL

#ifndef PIOM_ENABLE_LTASKS
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
    err = nm_piom_post_all(pkt->p_drv->p_core);

  PROF_EVENT(piom_polling_done);
  nmad_unlock();
  return err;
}
#endif

/* Posting functions */
int nm_piom_post_all(struct nm_core	 *p_core)
{
  int err = -NM_EUNKNOWN;
  
  /* schedule & post out requests */
  nm_sched_out(p_core);
  
  /* post new receive requests */
  nm_sched_in(p_core);
  
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

#if 0
int nm_piom_block_any(piom_server_t            server,
		     piom_op_t                _op,
		     piom_req_t               req,
		     int                       nb_ev,
		     int                       option)
{
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
  return err;
}
#endif /* 0 */
#endif /* PIOM_BLOCKING_CALLS */

#endif /* PIOMAN_POLL */
