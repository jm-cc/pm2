/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
  nm_req_list_cell_init(p_pack);
  p_pack->flags     = NM_FLAG_PACK;
  p_pack->data      = *p_data;
  p_pack->pack.len  = nm_data_size(&p_pack->data);
  p_pack->pack.done = 0;
  p_pack->monitor   = NM_MONITOR_NULL;
}

void nm_core_pack_send(struct nm_core*p_core, struct nm_req_s*p_pack, nm_core_tag_t tag, nm_gate_t p_gate,
		       nm_req_flag_t flags)
{
  assert(p_gate != NULL);
  nm_status_assert(p_pack, NM_STATUS_PACK_INIT);
  nm_status_add(p_pack, NM_STATUS_PACK_POSTED);
  p_pack->flags |= flags;
  p_pack->seq    = NM_SEQ_NONE;
  p_pack->tag    = tag;
  p_pack->p_gate = p_gate;
  p_pack->req_chunk.p_req = NULL;
}

void nm_core_pack_submit(struct nm_core*p_core, struct nm_req_s*p_pack, nm_len_t hlen)
{
  const nm_len_t size = nm_data_size(&p_pack->data);
  if(hlen > 0)
    {
      assert(hlen <= size);
      if(size > NM_DATA_IOV_THRESHOLD)
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_alloc(p_core);
	  nm_req_chunk_init(p_req_chunk, p_pack, 0, hlen);
	  nm_req_chunk_init(&p_pack->req_chunk, p_pack, hlen, size - hlen);
	  nm_req_chunk_submit(p_core, p_req_chunk);
	  nm_req_chunk_submit(p_core, &p_pack->req_chunk);
	}
      else
	{
	  nm_req_chunk_init(&p_pack->req_chunk, p_pack, 0, size);
	  nm_req_chunk_submit(p_core, &p_pack->req_chunk);
	}
    }
  else
    {
      nm_req_chunk_init(&p_pack->req_chunk, p_pack, 0, size);
      nm_req_chunk_submit(p_core, &p_pack->req_chunk);
    }
}

/** Places a packet in the send request list.
 * to be called from a strategy
 */
void nm_core_post_send(nm_gate_t p_gate, struct nm_pkt_wrap_s*p_pw,
		       nm_trk_id_t trk_id, nm_drv_t p_drv)
{
  nm_core_lock_assert(p_drv->p_core);
  /* Packet is assigned to given track, driver, and gate */
  nm_pw_assign(p_pw, trk_id, p_drv, p_gate);
  if(trk_id == NM_TRK_SMALL)
    {
      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
    }
  /* append pkt to scheduler post list */
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
  p_gdrv->active_send[trk_id]++;
  assert(p_gdrv->active_send[trk_id] == 1);
#ifdef PIOMAN
  nm_ltask_submit_pw_send(p_pw);
#else
  nm_pw_post_send(p_pw);
#endif
}

/** Process a complete successful outgoing request.
 */
void nm_pw_process_complete_send(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw)
{
  nm_gate_t const p_gate = p_pw->p_gate;
  nm_core_lock_assert(p_core);
  nm_profile_inc(p_core->profiling.n_pw_out);
  NM_TRACEF("send request complete: gate %p, drv %p, trk %d",
	    p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
  p_pw->p_gdrv->active_send[p_pw->trk_id]--;
  assert(p_pw->p_gdrv->active_send[p_pw->trk_id] == 0);
  while(!nm_req_chunk_list_empty(&p_pw->req_chunks))
    {
      struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_pw->req_chunks);
      struct nm_req_s*p_pack = p_req_chunk->p_req;
      const nm_len_t chunk_len = p_req_chunk->chunk_len;
      assert(nm_status_test(p_pack, NM_STATUS_PACK_POSTED));
      p_pack->pack.done += chunk_len;
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
	  NM_FATAL("more bytes sent than posted (should have been = %lu; actually sent = %lu)\n",
		       p_pack->pack.len, p_pack->pack.done);
	}
      nm_req_chunk_destroy(p_core, p_req_chunk);
    }
  nm_pw_ref_dec(p_pw);
  nm_strat_try_and_commit(p_gate);
}

/** Poll an active outgoing request.
 */
void nm_pw_poll_send(struct nm_pkt_wrap_s*p_pw)
{
  struct nm_core*p_core = p_pw->p_gate->p_core;
  nm_core_nolock_assert(p_core);
  assert(p_pw->flags & NM_PW_FINALIZED || p_pw->flags & NM_PW_NOHEADER);
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = (*r->driver->poll_send_iov)(r->_status, p_pw);
  if(err == NM_ESUCCESS)
    {
#ifdef PIOMAN
      piom_ltask_completed(&p_pw->ltask);
#else
      nm_pkt_wrap_list_erase(&p_core->pending_send_list, p_pw);
#endif /* PIOMAN */
      nm_core_lock(p_core);
      nm_pw_process_complete_send(p_core, p_pw);
      nm_core_unlock(p_core);
    }
  else if(err != -NM_EAGAIN)
    {
      NM_FATAL("poll_send failed- err = %d", err);
    }
}

/** Post a new outgoing request.
    - this function must handle immediately completed requests properly
*/
void nm_pw_post_send(struct nm_pkt_wrap_s*p_pw)
{
  struct nm_core*p_core = p_pw->p_drv->p_core;
  /* no lock needed; only this ltask is allowed to touch the pw */
  nm_core_nolock_assert(p_core);

#ifdef PIO_OFFLOAD
  nm_pw_offloaded_finalize(p_pw);
#endif

  if(p_pw->flags & NM_PW_GLOBAL_HEADER)
    nm_pw_finalize(p_pw);

  /* flatten data if needed */
  if((p_pw->p_data != NULL) &&
     ((p_pw->flags & NM_PW_DATA_USE_COPY) || !p_pw->p_drv->trk_caps[p_pw->trk_id].supports_data))
    {
      const struct nm_data_properties_s*p_props = nm_data_properties_get(p_pw->p_data);
      void*buf = NULL;
      if(p_props->is_contig)
	{
	  buf = nm_data_baseptr_get(p_pw->p_data) + p_pw->chunk_offset;
	}
      else
	{
	  buf = malloc(p_pw->length);
	  if(buf == NULL)
	    {
	      NM_FATAL("out of memory.\n");
	    }
	  nm_data_copy_from(p_pw->p_data, p_pw->chunk_offset, p_pw->length, buf);
	  p_pw->flags |= NM_PW_DYNAMIC_V0;
	}
      struct iovec*vec = nm_pw_grow_iovec(p_pw);
      vec->iov_base = (void*)buf;
      vec->iov_len = p_pw->length;
      p_pw->p_data = NULL;
    }
  /* post request on driver */
  struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
  int err = (*r->driver->post_send_iov)(r->_status, p_pw);
  
  if (err == -NM_EAGAIN)
    {
      p_pw->flags |= NM_PW_POSTED;
#ifndef PIOMAN
      /* put the request in the list of pending requests; no lock needed since no thread without pioman */
      nm_pkt_wrap_list_push_back(&p_core->pending_send_list, p_pw);
#endif /* PIOMAN */
    } 
  else if(err == NM_ESUCCESS)
    {
      /* immediate succes, process request completion */
#ifdef PIOMAN
      piom_ltask_completed(&p_pw->ltask);
#endif
      nm_core_lock(p_core);
      nm_pw_process_complete_send(p_core, p_pw);
      nm_core_unlock(p_core);
    }
  else
    {
      NM_FATAL("post_send failed- err = %d", err);
    }
}

#ifndef PIOMAN
void nm_out_prefetch(struct nm_core*p_core)
{
  /* check whether all drivers are idle */
  nm_drv_t p_drv = NULL;
  if(!nm_pkt_wrap_list_empty(&p_core->pending_send_list))
    {
      return;
    }
  
  nm_gate_t p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_send);
      if((p_pw != NULL) && (p_pw->p_data == NULL) && !(p_pw->flags & NM_PW_PREFETCHED))
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
#endif /* PIOMAN */


