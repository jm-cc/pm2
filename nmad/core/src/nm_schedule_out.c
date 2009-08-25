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


int nm_so_pack(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_tag_t tag, nm_gate_t p_gate,
	       const void*data, uint32_t len, nm_so_flag_t pack_type)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t seq = p_so_tag->send_seq_number++;
  p_pack->status = pack_type;
  p_pack->data   = (void*)data;
  p_pack->len    = len;
  p_pack->done   = 0;
  p_pack->p_gate = p_gate;
  p_pack->tag    = tag;
  p_pack->seq    = seq;
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    {
#warning Paulette: lock
      tbx_fast_list_add_tail(&p_pack->_link, &p_core->so_sched.pending_packs);
    }
  return (*r->driver->pack)(r->_status, p_pack);
}


/** Process a complete successful outgoing request.
 */
static int nm_so_process_complete_send(struct nm_core *p_core,
				       struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;

  NM_TRACEF("send request complete: gate %d, drv %d, trk %d",
	    p_pw->p_gate->id, p_pw->p_drv->id, p_pw->trk_id);
  
#ifdef PIOMAN
  piom_req_success(&p_pw->inst);
#endif
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  
  p_gate->active_send[p_pw->p_drv->id][p_pw->trk_id]--;

  int i;
  for(i = 0; i < p_pw->n_contribs; i++)
    {
      struct nm_pw_contrib_s*p_contrib = &p_pw->contribs[i];
      struct nm_pack_s*p_pack = p_contrib->p_pack;
      p_pack->done += p_contrib->len;
      if(p_pack->done == p_pack->len)
	{
	  NM_SO_TRACE("all chunks sent for msg tag=%u seq=%u len=%u!\n", p_pack->tag, p_pack->seq, p_pack->len);
	  const struct nm_so_event_s event =
	    {
	      .status = NM_SO_STATUS_PACK_COMPLETED,
	      .p_pack = p_pack
	    };
	  nm_so_status_event(p_core, &event, &p_pack->status);
	}
      else if(p_pack->done > p_pack->len)
	{ 
	  TBX_FAILUREF("more bytes sent than posted on tag %d (should have been = %d; actually sent = %d)\n",
		       p_pack->tag, p_pack->len, p_pack->done);
	}
    }

  nm_so_pw_free(p_pw);
  nm_strat_try_and_commit(p_gate);
  
  return NM_ESUCCESS;
}

/** Poll an active outgoing request.
 */
__inline__ int nm_poll_send(struct nm_pkt_wrap *p_pw)
{
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->poll_send_iov(r->_status, p_pw);
  if (err != -NM_EAGAIN)
    {
      if (tbx_unlikely(err < 0))
	{
	  TBX_FAILUREF("poll_send failed- err = %d", err);
	}
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
    }
  return err;
}

/** Post a new outgoing request.
    - this function must handle immediately completed requests properly
*/
static __inline__ int nm_post_send(struct nm_pkt_wrap*p_pw)
{
  int err;
  
  /* ready to send					*/
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  NM_TRACEF("posting new send request: gate %d, drv %d, trk %d, proto %d",
	    p_pw->p_gate->id,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id);
#ifdef PIOMAN
    piom_req_init(&p_pw->inst);
    p_pw->inst.server = &p_pw->p_drv->server;
    p_pw->which = SEND;
#endif /* PIOMAN */

  nm_so_pw_finalize(p_pw);

  /* post request */
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  err = r->driver->post_send_iov(r->_status, p_pw);
  
  /* process post command status				*/
  
  if (err == -NM_EAGAIN)
    {
      NM_TRACEF("new request pending: gate %d, drv %d, trk %d, proto %d",
		p_pw->p_gate->id,
		p_pw->p_drv->id,
		p_pw->trk_id,
		p_pw->proto_id);
#ifdef PIOMAN
      /* Submit  the pkt_wrapper to Pioman */
      piom_req_init(&p_pw->inst);
#ifdef PIOM_BLOCKING_CALLS
      /* TODO : implementer les syscalls */
      if (! ((r->driver->get_capabilities(p_pw->p_drv))->is_exportable))
	p_pw->inst.func_to_use = PIOM_FUNC_POLLING;
#endif /* PIOM_BLOCKING_CALLS */
      p_pw->inst.state |= PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
      p_pw->which = SEND;
      piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);
#else /* PIOMAN */
      /* put the request in the list of pending requests */
      tbx_slist_add_at_tail(p_pw->p_gate->p_core->so_sched.out_req_list, p_pw);
#endif /* PIOMAN */

    } 
  else if(err == NM_ESUCCESS)
    {
      /* immediate succes, process request completion */
      NM_TRACEF("request completed immediately");
      
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
    }
  else
    {
      TBX_FAILUREF("post_send failed- err = %d", err);
    }
  
    err = NM_ESUCCESS;
    
    return err;
}

/** Main scheduler func for outgoing requests.
   - this function must be called once for each gate on a regular basis
 */
int nm_sched_out(struct nm_core *p_core)
{
  int g;
  int err = NM_ESUCCESS;
  /* schedule new requests on all gates */
  for(g = 0; g < p_core->nb_gates; g++)
    {
      struct nm_gate*p_gate = &p_core->gate_array[g];
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  err = nm_strat_try_and_commit(p_gate);
	  if (err < 0)
	    {
	      NM_DISPF("sched.schedule_out returned %d", err);
	    }
	}
    }
  
  /* post new requests	*/
  while (!tbx_slist_is_nil(p_core->so_sched.post_sched_out_list))
    {
      NM_TRACEF("posting outbound requests");
      struct nm_pkt_wrap*p_pw = tbx_slist_remove_from_head(p_core->so_sched.post_sched_out_list);
      err = nm_post_send(p_pw);
      if (err < 0 && err != -NM_EAGAIN)
	{
	  NM_DISPF("post_send returned %d", err);
	}
    }
#ifndef PIOMAN
  /* poll pending out requests	*/
  if (!tbx_slist_is_nil(p_core->so_sched.out_req_list))
    {
      NM_TRACEF("polling outbound requests");
      tbx_slist_ref_to_head(p_core->so_sched.out_req_list);
      tbx_bool_t todo = tbx_true;
      do
	{
	  struct nm_pkt_wrap*p_pw = tbx_slist_ref_get(p_core->so_sched.out_req_list);
	  err = nm_poll_send(p_pw);
	  if(err == NM_ESUCCESS)
	    {
	      todo = tbx_slist_ref_extract_and_forward(p_core->so_sched.out_req_list, NULL);
	    }
	  else
	    {
	      todo = tbx_slist_ref_forward(p_core->so_sched.out_req_list);
	    }
	}
      while(todo);
  }
#endif /* PIOMAN */
  
  err = NM_ESUCCESS;
  
  return err;
}



#ifdef PIOM_BLOCKING_CALLS

int nm_piom_block_send(struct nm_pkt_wrap  *p_pw)
{
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->wait_send_iov(r->_status, p_pw);
  nmad_lock();
  if (err != -NM_EAGAIN)
    {
      if (tbx_unlikely(err < 0))
	{
	  NM_DISPF("poll_send returned %d", err);
	}
      nm_so_process_complete_send(p_pw->p_gate, p_pw);
    }
  return err; 
}

#endif /* PIOM_BLOCKING_CALLS */
