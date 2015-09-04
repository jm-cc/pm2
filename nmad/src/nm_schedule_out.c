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


void nm_core_pack_data(nm_core_t p_core, struct nm_pack_s*p_pack, const struct nm_data_s*p_data)
{
  p_pack->status    = NM_STATUS_NONE;
  p_pack->p_data    = p_data;
  p_pack->len       = nm_data_size(p_data);
  p_pack->done      = 0;
  p_pack->scheduled = 0;
}

/** data iterator to call strategy pack on every chunk of data */
static void nm_core_pack_chunk(void*ptr, nm_len_t len, void*_context)
{
  struct nm_pack_s*p_pack = _context;
  const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
  assert(r->driver->pack_chunk != NULL);
  (*r->driver->pack_chunk)(r->_status, p_pack, ptr, len, p_pack->scheduled);
}

int nm_core_pack_send(struct nm_core*p_core, struct nm_pack_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate,
		      nm_so_flag_t flags)
{
  int err = NM_ESUCCESS;
  nmad_lock();
  nm_lock_interface(p_core);
  assert(p_gate != NULL);
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t seq = nm_seq_next(p_so_tag->send_seq_number);
  p_so_tag->send_seq_number = seq;
  p_pack->status |= flags | NM_STATUS_PACK_POSTED;
  p_pack->seq    = seq;
  p_pack->tag    = tag;
  p_pack->p_gate = p_gate;
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    {
      tbx_fast_list_add_tail(&p_pack->_link, &p_core->pending_packs);
    }

  const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
  if(r->driver->pack_data != NULL)
    {
      (*r->driver->pack_data)(r->_status, p_pack, nm_data_size(p_pack->p_data), 0);
    }
  else
    {
      nm_data_aggregator_traversal(p_pack->p_data, &nm_core_pack_chunk, p_pack);
    }
  nm_unlock_interface(p_core);
  nmad_unlock();
  return err;
}


/** Process a complete successful outgoing request.
 */
int nm_so_process_complete_send(struct nm_core *p_core, struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  nmad_lock_assert();
  NM_TRACEF("send request complete: gate %p, drv %p, trk %d",
	    p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
  p_pw->p_gdrv->active_send[p_pw->trk_id]--;
  nm_pw_completions_notify(p_pw);
  nm_pw_ref_dec(p_pw);
  nm_strat_try_and_commit(p_gate);
  return NM_ESUCCESS;
}

/** Poll an active outgoing request.
 */
__inline__ void nm_pw_poll_send(struct nm_pkt_wrap *p_pw)
{
  nmad_lock_assert();
  assert(p_pw->flags & NM_PW_FINALIZED || p_pw->flags & NM_PW_NOHEADER);
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = (*r->driver->poll_send_iov)(r->_status, p_pw);
  if(err == NM_ESUCCESS)
    {
#ifdef PIOMAN_POLL
      piom_ltask_completed(&p_pw->ltask);
#endif /* PIOMAN_POLL */
#ifdef NMAD_POLL
      tbx_fast_list_del(&p_pw->link);
#endif /* NMAD_POLL */
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
    }
  else if(err != -NM_EAGAIN)
    {
      TBX_FAILUREF("poll_send failed- err = %d", err);
    }
}

/** Post a new outgoing request.
    - this function must handle immediately completed requests properly
*/
void nm_pw_post_send(struct nm_pkt_wrap*p_pw)
{
  nmad_lock_assert();

#ifdef PIO_OFFLOAD
  nm_so_pw_offloaded_finalize(p_pw);
#endif

  nm_so_pw_finalize(p_pw);

  /* post request */
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if((p_pw->p_data != NULL) && !p_pw->p_drv->trk_caps[p_pw->trk_id].supports_data)
    {
      void*buf = malloc(p_pw->length);
      nm_data_copy_from(p_pw->p_data, p_pw->chunk_offset, p_pw->length, buf);
      struct iovec*vec = nm_pw_grow_iovec(p_pw);
      vec->iov_base = (void*)buf;
      vec->iov_len = p_pw->length;
      p_pw->flags |= NM_PW_DYNAMIC_V0;
      p_pw->p_data = NULL;
    }
  int err = (*r->driver->post_send_iov)(r->_status, p_pw);
  
  /* process post command status				*/
  
  if (err == -NM_EAGAIN)
    {
#ifdef NMAD_POLL
      /* put the request in the list of pending requests */
      tbx_fast_list_add_tail(&p_pw->link, &p_pw->p_drv->p_core->pending_send_list);
#else /* NMAD_POLL */
      nm_ltask_submit_poll_send(p_pw);
#endif /* NMAD_POLL */
    } 
  else if(err == NM_ESUCCESS)
    {
      /* immediate succes, process request completion */
      nm_so_process_complete_send(p_pw->p_gate->p_core, p_pw);
    }
  else
    {
      TBX_FAILUREF("post_send failed- err = %d", err);
    }
}


/** Main scheduler func for outgoing requests on a specific driver.
   - this function must be called once for each driver on a regular basis
 */
void nm_drv_post_send(struct nm_drv *p_drv)
{
  /* post new requests	*/
  nmad_lock_assert();
  struct nm_pkt_wrap*p_pw = NULL;
  do
    {
      NM_TRACEF("posting outbound requests");
      p_pw = nm_pw_post_lfqueue_dequeue(&p_drv->post_send);
      if(p_pw)
	{
	  nm_pw_post_send(p_pw);
	}
    }
  while(p_pw);
}

int nm_core_flush(nm_gate_t p_gate)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  if(tbx_unlikely(r->driver->flush))
    {
      return (*r->driver->flush)(r->_status, p_gate);
    }
  else
    return -NM_ENOTIMPL;
}

#ifdef NMAD_POLL
void nm_out_prefetch(struct nm_core*p_core)
{
  /* check whether all drivers are idle */
  struct nm_drv*p_drv = NULL;
  if(!tbx_fast_list_empty(&p_core->pending_send_list))
    {
      return;
    }
  
  struct nm_gate*p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(!tbx_fast_list_empty(&p_gate->pending_large_send))
	{
	  struct nm_pkt_wrap *p_pw = nm_l2so(p_gate->pending_large_send.next);
	  if((p_pw->p_data == NULL) && !(p_pw->flags & NM_PW_PREFETCHED))
	    {
	      p_drv = nm_drv_default(p_gate);
	      struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	      if(p_gdrv->receptacle.driver->prefetch_send)
		{
		  (p_gdrv->receptacle.driver->prefetch_send)(p_gdrv->receptacle._status, p_pw);
		  p_pw->flags |= NM_PW_PREFETCHED;
		}
	    }
	}
    }  
}
#endif /* NMAD_POLL */


