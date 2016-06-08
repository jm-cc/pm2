/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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


void nm_core_pack_data(nm_core_t p_core, struct nm_req_s*p_pack, const struct nm_data_s*p_data)
{
  nm_status_init(p_pack, NM_STATUS_PACK_INIT);
  p_pack->flags     = NM_FLAG_PACK;
  p_pack->p_data    = p_data;
  p_pack->pack.len  = nm_data_size(p_data);
  p_pack->pack.done = 0;
  p_pack->pack.scheduled = 0;
  p_pack->monitor   = NM_MONITOR_NULL;
}

/** data iterator to call strategy pack on every chunk of data */
static void nm_core_pack_chunk(void*ptr, nm_len_t len, void*_context)
{
  struct nm_req_s*p_pack = _context;
  const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
  assert(r->driver->pack_chunk != NULL);
  (*r->driver->pack_chunk)(r->_status, p_pack, ptr, len, p_pack->pack.scheduled);
}

int nm_core_pack_send(struct nm_core*p_core, struct nm_req_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate,
		      nm_req_flag_t flags)
{
  int err = NM_ESUCCESS;
  nmad_lock();
  assert(p_gate != NULL);
  nm_status_assert(p_pack, NM_STATUS_PACK_INIT);
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t seq = nm_seq_next(p_so_tag->send_seq_number);
  p_so_tag->send_seq_number = seq;
  nm_status_add(p_pack, NM_STATUS_PACK_POSTED);
  p_pack->flags |= flags;
  p_pack->seq    = seq;
  p_pack->tag    = tag;
  p_pack->p_gate = p_gate;
  nmad_unlock();
  return err;
}

void nm_core_pack_header(struct nm_core*p_core, struct nm_req_s*p_pack, nm_len_t hlen)
{
  const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
  nmad_lock();
  if(r->driver->pack_data != NULL)
    {
      const nm_len_t size = nm_data_size(p_pack->p_data);
      assert(hlen <= size);
      if((hlen < size) && (size > NM_DATA_IOV_THRESHOLD))
	(*r->driver->pack_data)(r->_status, p_pack, hlen, p_pack->pack.scheduled);
    }
  else
    {
      fprintf(stderr, "# nmad: nm_core_pack_header not support with selected strategy (need pack_data).\n");
      abort();
    }
  nmad_unlock();
}

void nm_core_pack_submit(struct nm_core*p_core, struct nm_req_s*p_pack)
{
  const struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
  nmad_lock();
  nm_req_list_push_back(&p_core->pending_packs, p_pack);
  nm_core_polling_level(p_core);
  if(r->driver->pack_data != NULL)
    {
      const nm_len_t size = nm_data_size(p_pack->p_data) - p_pack->pack.scheduled;
      (*r->driver->pack_data)(r->_status, p_pack, size, p_pack->pack.scheduled);
    }
  else
    {
      nm_data_aggregator_traversal(p_pack->p_data, &nm_core_pack_chunk, p_pack);
    }
  nmad_unlock();
}


/** Process a complete successful outgoing request.
 */
int nm_so_process_complete_send(struct nm_core *p_core, struct nm_pkt_wrap *p_pw)
{
  nm_gate_t const p_gate = p_pw->p_gate;
  nmad_lock_assert();
  NM_TRACEF("send request complete: gate %p, drv %p, trk %d",
	    p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
  p_pw->p_gdrv->active_send[p_pw->trk_id]--;
  assert(p_pw->p_gdrv->active_send[p_pw->trk_id] == 0);
  int i;
  for(i = 0; i < p_pw->n_completions; i++)
    {
      const struct nm_pw_completion_s*p_completion = &p_pw->completions[i];
      struct nm_req_s*p_pack = p_completion->p_pack;
      p_pack->pack.done += p_completion->len;
      if(p_pack->pack.done == p_pack->pack.len)
	{
	  NM_TRACEF("all chunks sent for msg seq=%u len=%u!\n", p_pack->seq, p_pack->pack.len);
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_PACK_COMPLETED |
	      ( ((!(p_pack->flags & NM_FLAG_PACK_SYNCHRONOUS)) || nm_status_test(p_pack, NM_STATUS_ACK_RECEIVED)) ? NM_STATUS_FINALIZED : 0),
	      .p_req = p_pack
	    };
	  if(event.status & NM_STATUS_FINALIZED)
	    {
	      nm_req_list_erase(&p_core->pending_packs, p_pack);
	      nm_core_polling_level(p_core);
	    }
	  nm_core_status_event(p_pw->p_gate->p_core, &event, p_pack);
	}
      else if(p_pack->pack.done > p_pack->pack.len)
	{ 
	  TBX_FAILUREF("more bytes sent than posted (should have been = %lu; actually sent = %lu)\n",
		       p_pack->pack.len, p_pack->pack.done);
	}
    }
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

  if(p_pw->flags & NM_PW_GLOBAL_HEADER)
    nm_so_pw_finalize(p_pw);

  /* post request */
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  if((p_pw->p_data != NULL) &&
     ((p_pw->flags & NM_PW_DATA_USE_COPY) || !p_pw->p_drv->trk_caps[p_pw->trk_id].supports_data))
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
void nm_drv_post_send(nm_drv_t p_drv)
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
  nm_drv_t p_drv = NULL;
  if(!tbx_fast_list_empty(&p_core->pending_send_list))
    {
      return;
    }
  
  nm_gate_t p_gate = NULL;
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


