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

void nm_core_pack_iov(nm_core_t p_core, struct nm_pack_s*p_pack, const struct iovec*iov, int num_entries)
{
  p_pack->status = NM_PACK_TYPE_IOV;
  p_pack->data   = (void*)iov;
  p_pack->len    = nm_so_iov_len(iov, num_entries);
  p_pack->done   = 0;
}

void nm_core_pack_datatype(nm_core_t p_core, struct nm_pack_s*p_pack, const struct DLOOP_Segment *segp)
{
  p_pack->status = NM_PACK_TYPE_DATATYPE;
  p_pack->data   = (void*)segp;
  p_pack->len    = nm_so_datatype_size(segp);
  p_pack->done   = 0;
}

int nm_core_pack_send(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_tag_t tag, nm_gate_t p_gate,
		      nm_so_flag_t flags)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t seq = p_so_tag->send_seq_number++;
  p_pack->status |= flags | NM_STATUS_PACK_POSTED;
  p_pack->seq    = seq;
  p_pack->tag    = tag;
  p_pack->p_gate = p_gate;
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    {
#warning Paulette: lock
      tbx_fast_list_add_tail(&p_pack->_link, &p_core->pending_packs);
    }
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
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
  
#if(defined(PIOMAN_POLL) && !defined(PIOM_ENABLE_LTASKS))
  piom_req_success(&p_pw->inst);
#endif
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  
  p_pw->p_gdrv->active_send[p_pw->trk_id]--;

  int i;
  for(i = 0; i < p_pw->n_contribs; i++)
    {
      struct nm_pw_contrib_s*p_contrib = &p_pw->contribs[i];
      struct nm_pack_s*p_pack = p_contrib->p_pack;
      p_pack->done += p_contrib->len;
      if(p_pack->done == p_pack->len)
	{
	  NM_SO_TRACE("all chunks sent for msg seq=%u len=%u!\n", p_pack->seq, p_pack->len);
	  const struct nm_so_event_s event =
	    {
	      .status = NM_STATUS_PACK_COMPLETED,
	      .p_pack = p_pack
	    };
	  nm_core_status_event(p_core, &event, &p_pack->status);
	}
      else if(p_pack->done > p_pack->len)
	{ 
	  TBX_FAILUREF("more bytes sent than posted (should have been = %d; actually sent = %d)\n",
		       p_pack->len, p_pack->done);
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
void nm_post_send(struct nm_pkt_wrap*p_pw)
{
  /* ready to send					*/
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  NM_TRACEF("posting new send request: gate %d, drv %d, trk %d, proto %d",
	    p_pw->p_gate->id,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id);
#if(defined(PIOMAN_POLL) && !defined(PIOM_ENABLE_LTASKS))
    piom_req_init(&p_pw->inst);
    p_pw->inst.server = &p_pw->p_drv->server;
    p_pw->which = NM_PW_SEND;
#endif /* PIOMAN_POLL */
#ifdef PIO_OFFLOAD
    nm_so_pw_offloaded_finalize(p_pw);
#endif

  nm_so_pw_finalize(p_pw);

  /* post request */
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = r->driver->post_send_iov(r->_status, p_pw);
  
  /* process post command status				*/
  
  if (err == -NM_EAGAIN)
    {
      NM_TRACEF("new request pending: gate %d, drv %d, trk %d, proto %d",
		p_pw->p_gate->id,
		p_pw->p_drv->id,
		p_pw->trk_id,
		p_pw->proto_id);

#ifdef PIOM_ENABLE_LTASKS
      nm_submit_poll_send_ltask(p_pw);
#else
      /* todo: clean that mess ! */
#ifdef NMAD_POLL
      /* put the request in the list of pending requests */
      nm_poll_lock_out(p_pw->p_gate->p_core, p_pw->p_drv);
      tbx_fast_list_add_tail(&p_pw->link, &p_pw->p_drv->pending_send_list);
      nm_poll_unlock_out(p_pw->p_gate->p_core, p_pw->p_drv);
#else /* NMAD_POLL */
      /* Submit  the pkt_wrapper to Pioman */
      piom_req_init(&p_pw->inst);
#ifdef PIOM_BLOCKING_CALLS
      /* TODO : implementer les syscalls */
      if (! ((r->driver->get_capabilities(p_pw->p_drv))->is_exportable))
	p_pw->inst.func_to_use = PIOM_FUNC_POLLING;
#endif /* PIOM_BLOCKING_CALLS */
      p_pw->inst.state |= PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
      p_pw->which = NM_PW_SEND;
      piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);
#endif /* NMAD_POLL */

#endif
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
}

/** Main scheduler func for outgoing requests.
   - this function must be called once for each gate on a regular basis
 */
void nm_sched_out(struct nm_core *p_core)
{
  /* schedule new requests on all gates */
  struct nm_gate*p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  int err = nm_strat_try_and_commit(p_gate);
	  if (err < 0)
	    {
	      NM_DISPF("sched.schedule_out returned %d", err);
	    }
	}
    }
  
  /* post new requests	*/
  struct nm_drv*p_drv = NULL;
  NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core)
  {
    nm_trk_id_t trk;
    for(trk = 0; trk < NM_SO_MAX_TRACKS; trk++)
    {
      if(!tbx_fast_list_empty(&p_drv->post_sched_out_list[trk]))
	{
	  nm_so_lock_out(p_core, p_drv);
	  NM_TRACEF("posting outbound requests");
	  struct nm_pkt_wrap*p_pw, *p_pw2;
	  tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_drv->post_sched_out_list[trk], link)
	    {
	      tbx_fast_list_del(&p_pw->link);
	      nm_so_unlock_out(p_core, p_drv);
	      nm_post_send(p_pw);
	      nm_so_lock_out(p_core, p_drv);
	    }
	  nm_so_unlock_out(p_core, p_drv);
	}
    }
  }

#ifdef NMAD_POLL
  /* poll pending out requests	*/
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      if(!tbx_fast_list_empty(&p_drv->pending_send_list))
	{
	  nm_poll_lock_out(p_core, p_drv);
	  if (!tbx_fast_list_empty(&p_drv->pending_send_list))
	    {
	      NM_TRACEF("polling outbound requests");
	      struct nm_pkt_wrap*p_pw, *p_pw2;
	      tbx_fast_list_for_each_entry_safe(p_pw, p_pw2, &p_drv->pending_send_list, link)
		{
		  nm_poll_unlock_out(p_core, p_drv);
		  const int err = nm_poll_send(p_pw);
		  nm_poll_lock_out(p_core, p_drv);
		  if(err == NM_ESUCCESS)
		    {
		      tbx_fast_list_del(&p_pw->link);
		    }
		}
	    }
	  nm_poll_unlock_out(p_core, p_drv);
	}
    }
#endif /* NMAD_POLL */
  
}


#ifdef PIOM_BLOCKING_CALLS

int nm_piom_block_send(struct nm_pkt_wrap  *p_pw)
{
  nmad_unlock();
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err;
  do {
    err = r->driver->wait_send_iov(r->_status, p_pw);
  }while(err == -NM_EAGAIN);

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
