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

#ifndef NM_CORE_INLINE_H
#define NM_CORE_INLINE_H

#include <nm_private.h>

/* ** Driver management ************************************ */

/** Get the track per-gate data */
static inline struct nm_trk_s*nm_gate_trk_get(nm_gate_t p_gate, nm_drv_t p_drv)
{
  assert(p_drv != NULL);
  int i;
  for(i = 0; i < p_gate->n_trks; i++)
    {
      if(p_gate->trks[i].p_drv == p_drv)
	return &p_gate->trks[i];
    }
  return NULL;
}

/** Get a driver given its id.
 */
static inline struct nm_trk_s*nm_trk_get_by_index(nm_gate_t p_gate, int index)
{
  assert(p_gate->n_trks > 0);
  assert(index < p_gate->n_trks);
  assert(index >= 0);
  return &p_gate->trks[index];
}

/** get maximum size for small messages for the given driver */
static inline nm_len_t nm_drv_max_small(nm_drv_t p_drv)
{
  const nm_len_t max_small = (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE - NM_ALIGN_FRONTIER);
  return (p_drv->props.capabilities.max_msg_size > max_small) ? max_small : p_drv->props.capabilities.max_msg_size;
}


/* ** Packet wrapper management **************************** */

/** assign packet to given driver, gate, and track */
static inline void nm_pw_assign(struct nm_pkt_wrap_s*p_pw, nm_trk_id_t trk_id, struct nm_drv_s*p_drv, nm_gate_t p_gate)
{
  p_pw->trk_id = trk_id;
  if(p_gate == NM_GATE_NONE)
    {
      assert(p_drv->p_in_rq == NULL);
      p_drv->p_in_rq = p_pw;
      p_pw->p_trk = NULL;
      p_pw->p_drv = p_drv;
    }
  else
    {
      assert(trk_id >= 0);
      assert(trk_id < p_gate->n_trks);
      p_pw->p_gate = p_gate;
      p_pw->p_trk = &p_gate->trks[trk_id];
      p_pw->p_drv = p_pw->p_trk->p_drv;
    }
}

static inline void nm_pw_ref_inc(struct nm_pkt_wrap_s*p_pw)
{
  nm_atomic_inc(&p_pw->ref_count);
}
static inline void nm_pw_ref_dec(struct nm_pkt_wrap_s *p_pw)
{
  const int count = nm_atomic_dec(&p_pw->ref_count);
  assert(count >= 0);
  if(count == 0)
    {
      nm_pw_free(p_pw->p_gate->p_core, p_pw);
    }
}


/** Schedule and post new outgoing buffers */
static inline void nm_strat_try_and_commit(nm_gate_t p_gate)
{
  nm_core_lock_assert(p_gate->p_core);
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  nm_profile_inc(p_gate->p_core->profiling.n_try_and_commit);
  (*r->driver->try_and_commit)(r->_status, p_gate);
}
/** Schedule and post new large incoming buffers */
static inline void nm_strat_rdv_accept(nm_gate_t p_gate)
{
  nm_core_lock_assert(p_gate->p_core);
  if(!nm_pkt_wrap_list_empty(&p_gate->pending_large_recv))
    {
      const struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
      (*r->driver->rdv_accept)(r->_status, p_gate);
    }
}

static inline void nm_strat_pack_ctrl(nm_gate_t p_gate, nm_header_ctrl_generic_t*p_header)
{
  struct nm_core*p_core = p_gate->p_core;
  nm_core_lock_assert(p_core);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  if(strategy->driver->pack_ctrl != NULL)
    {
      (*strategy->driver->pack_ctrl)(strategy->_status, p_gate, p_header);
    }
  else
    {
      struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_malloc(&p_core->ctrl_chunk_allocator);
      p_ctrl_chunk->ctrl = *p_header;
      nm_ctrl_chunk_list_push_back(&p_gate->ctrl_chunk_list, p_ctrl_chunk);
    }
}

static inline void nm_core_pw_completions_flush(struct nm_core*p_core)
{
  nm_core_lock_assert(p_core);
  while(!nm_pkt_wrap_lfqueue_empty(&p_core->completed_pws))
    {
      struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_lfqueue_dequeue_single_reader(&p_core->completed_pws);
      assert(p_pw->flags & NM_PW_COMPLETED);
      if(p_pw->flags & NM_PW_SEND)
	nm_pw_process_complete_send(p_core, p_pw);
      else if(p_pw->flags & NM_PW_RECV)
	nm_pw_process_complete_recv(p_core, p_pw);
      else
	NM_FATAL("wrong state for completed pw.");
    }      
}

/** enqueue a pw completion, or process immediately if possible */
static inline void nm_pw_completed_enqueue(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw)
{
  nm_core_nolock_assert(p_core);
  assert(!(p_pw->flags & NM_PW_COMPLETED));
  p_pw->flags |= NM_PW_COMPLETED;
#ifdef PIOMAN
  piom_ltask_completed(&p_pw->ltask);
#endif
  if(nm_core_trylock(p_core))
    {
      /* got lock- flush pending completions & immediate completion */
      nm_core_pw_completions_flush(p_core);
      if(p_pw->flags & NM_PW_SEND)
	nm_pw_process_complete_send(p_core, p_pw);
      else if(p_pw->flags & NM_PW_RECV)
	nm_pw_process_complete_recv(p_core, p_pw);
      else
	NM_FATAL("wrong state for completed pw.");
      nm_core_unlock(p_core);
    }
  else
    {
      /* busy lock- don't wait, enqueue to process later */
      int rc;
      do
	{
	  rc = nm_pkt_wrap_lfqueue_enqueue(&p_core->completed_pws, p_pw);
	  if(tbx_unlikely(rc))
	    {
	      nm_core_flush(p_core);
	    }
	}
      while(rc);
    }
#ifndef PIOMAN
  nm_core_events_dispatch(p_core);
#endif
}

static inline void nm_req_chunk_submit(struct nm_core*p_core, struct nm_req_chunk_s*p_req_chunk)
{
  nm_core_nolock_assert(p_core);
  assert(p_req_chunk->p_req != NULL);
  int rc;
  do
    {
#ifdef PIOMAN_MULTITHREAD
      rc = nm_req_chunk_lfqueue_enqueue(&p_core->pack_submissions, p_req_chunk);
#else
      rc = nm_req_chunk_lfqueue_enqueue_single_writer(&p_core->pack_submissions, p_req_chunk);
#endif /* PIOMAN_MULTITHREAD */
      if(rc)
	nm_core_flush(p_core);
    }
  while(rc);
}

static inline void nm_req_chunk_destroy(struct nm_core*p_core, struct nm_req_chunk_s*p_req_chunk)
{
  struct nm_req_s*p_pack = p_req_chunk->p_req;
#ifdef DEBUG
  p_req_chunk->chunk_len = NM_LEN_UNDEFINED;
  p_req_chunk->chunk_offset = NM_LEN_UNDEFINED;
#endif
  if(p_req_chunk != &p_pack->req_chunk)
    {
      nm_req_chunk_free(&p_core->req_chunk_allocator, p_req_chunk);
    }
  else
    {
      p_pack->req_chunk.p_req = NULL;
    }
}

static inline struct nm_req_chunk_s*nm_req_chunk_alloc(struct nm_core*p_core)
{
  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_malloc(&p_core->req_chunk_allocator);
  nm_req_chunk_list_cell_init(p_req_chunk);
  return p_req_chunk;
}

static inline void nm_req_chunk_init(struct nm_req_chunk_s*p_req_chunk, struct nm_req_s*p_req,
				     nm_len_t chunk_offset, nm_len_t chunk_len)
{
  nm_req_chunk_list_cell_init(p_req_chunk);
  p_req_chunk->p_req        = p_req;
  p_req_chunk->chunk_offset = chunk_offset;
  p_req_chunk->chunk_len    = chunk_len;
  p_req_chunk->proto_flags  = 0;
  assert(chunk_offset + chunk_len <= p_req->pack.len);
  if(chunk_offset + chunk_len == p_req->pack.len)
    {
      p_req_chunk->proto_flags |= NM_PROTO_FLAG_LASTCHUNK;
    }
  if(p_req->flags & NM_REQ_FLAG_PACK_SYNCHRONOUS)
    {
      p_req_chunk->proto_flags |= NM_PROTO_FLAG_ACKREQ;
    }
}

/** Post a ready-to-receive to accept chunk on given trk_id
 */
static inline void nm_core_post_rtr(nm_gate_t p_gate,  nm_core_tag_t tag, nm_seq_t seq,
				    nm_trk_id_t trk_id, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  nm_header_ctrl_generic_t h;
  nm_header_init_rtr(&h, tag, seq, trk_id, chunk_offset, chunk_len);
  nm_strat_pack_ctrl(p_gate, &h);
}

/** Post an ACK
 */
static inline void nm_core_post_ack(nm_gate_t p_gate, nm_core_tag_t tag, nm_seq_t seq)
{
  nm_header_ctrl_generic_t h;
  nm_header_init_ack(&h, tag, seq);
  nm_strat_pack_ctrl(p_gate, &h);
}

/** dynamically adapt pioman polling frequency level depending on the number of pending requests */
static inline void nm_core_polling_level(struct nm_core*p_core)
{
#ifdef PIOMAN
  const int high = (!nm_req_list_empty(&p_core->wildcard_unpacks)) || (!nm_req_list_empty(&p_core->pending_packs));
  piom_ltask_poll_level_set(high);
#endif /* PIOMAN */
}


#endif /* NM_CORE_INLINE_H */
