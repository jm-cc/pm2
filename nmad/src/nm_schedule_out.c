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
  p_pack->flags     = NM_REQ_FLAG_PACK;
  p_pack->data      = *p_data;
  p_pack->pack.len  = nm_data_size(&p_pack->data);
  p_pack->pack.done = 0;
  p_pack->pack.hlen = 0;
  p_pack->pack.priority = 0;
  p_pack->monitor   = NM_MONITOR_NULL;
#ifdef DEBUG
  if(p_core->enable_isend_csum)
    {
      p_pack->pack.checksum = nm_data_checksum(&p_pack->data);
    }
#endif /* DEBUG */
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
  p_pack->p_gtag = NULL;
  p_pack->req_chunk.p_req = NULL;
}

void nm_core_pack_submit(struct nm_core*p_core, struct nm_req_s*p_pack)
{
  const nm_len_t size = nm_data_size(&p_pack->data);
  if(p_pack->pack.hlen > 0)
    {
      assert(p_pack->pack.hlen <= size);
      if(size > NM_DATA_IOV_THRESHOLD)
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_alloc(p_core);
	  nm_req_chunk_init(p_req_chunk, p_pack, 0, p_pack->pack.hlen);
	  nm_req_chunk_init(&p_pack->req_chunk, p_pack, p_pack->pack.hlen, size - p_pack->pack.hlen);
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
  nm_profile_inc(p_core->profiling.n_packs);
  if(p_core->enable_auto_flush)
    {
      nm_core_flush(p_core);
      nm_schedule(p_core);
    }
}

/** Places a packet in the send request list.
 * to be called from a strategy
 */
void nm_core_post_send(struct nm_pkt_wrap_s*p_pw, nm_gate_t p_gate, nm_trk_id_t trk_id)
{
  nm_core_lock_assert(p_gate->p_core);
  /* Packet is assigned to given track, driver, and gate */
  nm_pw_assign(p_pw, trk_id, NULL, p_gate);
  p_pw->flags |= NM_PW_SEND;
  if(trk_id == NM_TRK_SMALL)
    {
      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
    }
  /* append pkt to scheduler post list */
  struct nm_trk_s*p_trk = p_pw->p_trk;
  if(p_trk->p_pw_send == NULL)
    {
      p_trk->p_pw_send = p_pw;
#ifdef PIOMAN
      nm_ltask_submit_pw_send(p_pw);
#else
      nm_pw_post_send(p_pw);
#endif
    }
  else
    {
      nm_pkt_wrap_list_push_back(&p_trk->pending_pw_send, p_pw);
    }
}

/** Process a complete successful outgoing request.
 */
void nm_pw_process_complete_send(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw)
{
  struct nm_trk_s*const p_trk = p_pw->p_trk;
  nm_gate_t const p_gate = p_pw->p_gate;
  nm_core_lock_assert(p_core);
  nm_profile_inc(p_core->profiling.n_pw_out);
  NM_TRACEF("send request complete: gate %p, drv %p, trk %d", p_pw->p_gate, p_pw->p_drv, p_pw->trk_id);
  assert(p_pw->p_trk->p_pw_send == p_pw);
  p_pw->p_trk->p_pw_send = NULL;
  while(!nm_req_chunk_list_empty(&p_pw->req_chunks))
    {
      struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_pw->req_chunks);
      struct nm_req_s*p_pack = p_req_chunk->p_req;
      const nm_len_t chunk_len = p_req_chunk->chunk_len;
      assert(nm_status_test(p_pack, NM_STATUS_PACK_POSTED));
      p_pack->pack.done += chunk_len;
      nm_req_chunk_destroy(p_core, p_req_chunk);
      if(p_pack->pack.done == p_pack->pack.len)
	{
	  NM_TRACEF("all chunks sent for msg seq=%u len=%u!\n", p_pack->seq, p_pack->pack.len);
#ifdef DEBUG
	  if(p_core->enable_isend_csum)
	    {
	      const uint32_t checksum = nm_data_checksum(&p_pack->data);
	      if(checksum != p_pack->pack.checksum)
		{
		  NM_FATAL("data corrupted between isend and completion.\n");
		}
	    }
#endif /* DEBUG */

	  const int is_sync = (p_pack->flags & NM_REQ_FLAG_PACK_SYNCHRONOUS);
	  const int acked = (is_sync && nm_status_test(p_pack, NM_STATUS_ACK_RECEIVED));
	  const int finalized = (acked || !is_sync);
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_PACK_COMPLETED | (finalized ? NM_STATUS_FINALIZED : 0),
	      .p_req = p_pack
	    };
	  if(finalized)
	    {
	      if(acked)
		{
		  nm_req_list_remove(&p_pack->p_gtag->pending_packs, p_pack);
		}
	      p_core->n_packs--;
              nm_trace_var(NM_TRACE_SCOPE_CORE, NM_TRACE_EVENT_VAR_N_PACKS, p_core->n_packs, p_pack->p_gate);
	      nm_core_polling_level(p_core);
	    }
	  nm_core_status_event(p_pw->p_gate->p_core, &event, p_pack);
	}
      else if(p_pack->pack.done > p_pack->pack.len)
	{
	  NM_FATAL("more bytes sent than posted (should have been = %lu; actually sent = %lu)\n",
		       p_pack->pack.len, p_pack->pack.done);
	}
    }
  nm_pw_ref_dec(p_pw);
  if(!nm_pkt_wrap_list_empty(&p_trk->pending_pw_send))
    {
      p_pw = nm_pkt_wrap_list_pop_front(&p_trk->pending_pw_send);
      p_trk->p_pw_send = p_pw;
#ifdef PIOMAN
      nm_ltask_submit_pw_send(p_pw);
#else
      nm_pw_post_send(p_pw);
#endif
    }
  nm_strat_try_and_commit(p_gate);
}

/** Poll an active outgoing request.
 */
void nm_pw_poll_send(struct nm_pkt_wrap_s*p_pw)
{
  struct nm_core*p_core = p_pw->p_gate->p_core;
  nm_core_nolock_assert(p_core);
  assert(p_pw->flags & NM_PW_FINALIZED || p_pw->flags & NM_PW_NOHEADER);
  struct puk_receptacle_NewMad_minidriver_s*r = &p_pw->p_trk->receptacle;
  int err = (*r->driver->send_poll)(r->_status);
#ifdef DEBUG
  if(err == NM_ESUCCESS)
    {
      struct nm_data_s*p_data = &p_pw->p_trk->sdata;
      nm_data_null_build(p_data);
    }
#endif /* DEBUG */
  if(err == NM_ESUCCESS)
    {
#ifndef PIOMAN
      nm_pkt_wrap_list_remove(&p_core->pending_send_list, p_pw);
#endif /* PIOMAN */
      nm_pw_completed_enqueue(p_core, p_pw);
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
  struct puk_receptacle_NewMad_minidriver_s*r = &p_pw->p_trk->receptacle;
  const struct nm_minidriver_capabilities_s*p_capabilities = &p_pw->p_trk->p_drv->props.capabilities;
  /* no lock needed; only this ltask is allowed to touch the pw */
  nm_core_nolock_assert(p_core);

#ifdef PIO_OFFLOAD
  nm_pw_offloaded_finalize(p_pw);
#endif

  if(p_pw->p_trk->kind == nm_trk_small)
    nm_pw_finalize(p_pw);

  if(p_pw->p_data)
    {
      /* ** pw contains nm_data; flatten if needed */
      if((p_pw->flags & NM_PW_DATA_COPY) || !p_capabilities->supports_data)
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
	  (*r->driver->send_post)(r->_status, p_pw->v, p_pw->v_nb);
	}
      else
	{
	  /* native nm_data support */
	  (*r->driver->send_data_post)(r->_status, p_pw->p_data, p_pw->chunk_offset, p_pw->length);
	}
    }
  else
    {
      /* ** pw contains iovec */
      if(p_pw->flags & NM_PW_BUF_SEND)
	{
	  assert(p_pw->length <= p_pw->max_len);
	  (*r->driver->send_buf_post)(r->_status, p_pw->length);
	}
      else if(r->driver->send_post)
	{
	  (*r->driver->send_post)(r->_status, &p_pw->v[0], p_pw->v_nb);
	}
      else
	{
	  assert(r->driver->send_data_post);
	  struct nm_data_s*p_data = &p_pw->p_trk->sdata;
	  assert(nm_data_isnull(p_data));
	  nm_data_iov_set(p_data, (struct nm_data_iov_s){ .v = &p_pw->v[0], .n = p_pw->v_nb });
	  (*r->driver->send_data_post)(r->_status, p_data, 0 /* chunk_offset */, p_pw->length);
	}
    }
  p_pw->flags |= NM_PW_POSTED;
#ifndef PIOMAN
  /* put the request in the list of pending requests; no lock needed since no thread without pioman */
  nm_pkt_wrap_list_push_back(&p_core->pending_send_list, p_pw);
#endif /* PIOMAN */

  nm_pw_poll_send(p_pw);

}

#if 0 /* #ifndef PIOMAN */
/* prefetch disabled for now */
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
