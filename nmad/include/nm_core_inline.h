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

#ifndef NM_CORE_INLINE_H
#define NM_CORE_INLINE_H



/* ** Driver management ************************************ */

/** Get the driver-specific per-gate data */
static inline struct nm_gate_drv*nm_gate_drv_get(nm_gate_t p_gate, nm_drv_t p_drv)
{
  nm_gdrv_vect_itor_t i;
  assert(p_drv != NULL);
  puk_vect_foreach(i, nm_gdrv, &p_gate->gdrv_array)
    {
      if((*i)->p_drv == p_drv)
	return *i;
    }
  return NULL;
}

/** The default network to use when several networks are
 *  available, but the strategy does not support multi-rail.
 *  Currently: the first available network
 */
static inline nm_drv_t nm_drv_default(nm_gate_t p_gate)
{
  assert(!nm_gdrv_vect_empty(&p_gate->gdrv_array));
  nm_gdrv_vect_itor_t i = nm_gdrv_vect_begin(&p_gate->gdrv_array);
  return (*i)->p_drv;
}

/** Get a driver given its id.
 */
static inline nm_drv_t nm_drv_get_by_index(nm_gate_t p_gate, int index)
{
  assert(!nm_gdrv_vect_empty(&p_gate->gdrv_array));
  assert(index < nm_gdrv_vect_size(&p_gate->gdrv_array));
  assert(index >= 0);
  struct nm_gate_drv*p_gdrv = nm_gdrv_vect_at(&p_gate->gdrv_array, index);
  return p_gdrv->p_drv;
}

/** get maximum size for small messages accross all drivers */
static inline nm_len_t nm_drv_max_small(struct nm_core*p_core)
{
  static ssize_t nm_max_small = -1;
    /* lazy init */
  if(nm_max_small > 0)
    return nm_max_small;
  else
    {      
      nm_drv_t p_drv;
      NM_FOR_EACH_DRIVER(p_drv, p_core)
	{
	  if(nm_max_small <= 0 || (p_drv->driver->capabilities.max_unexpected > 0 && p_drv->driver->capabilities.max_unexpected < nm_max_small))
	    {
	      nm_max_small = p_drv->driver->capabilities.max_unexpected;
	    }
	}
      if(nm_max_small <= 0 || nm_max_small > (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE - NM_ALIGN_FRONTIER))
	{
	  nm_max_small = (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE - NM_ALIGN_FRONTIER);
	}
      NM_DISPF("# nmad: max_small = %lu\n", nm_max_small);
    }
  return nm_max_small;
}


/* ** Packet wrapper management **************************** */

/** assign packet to given driver, gate, and track */
static inline void nm_pw_assign(struct nm_pkt_wrap_s*p_pw, nm_trk_id_t trk_id, nm_drv_t p_drv, nm_gate_t p_gate)
{
  p_pw->p_drv = p_drv;
  p_pw->trk_id = trk_id;
  if(p_gate == NM_GATE_NONE)
    {
      assert(p_drv->p_in_rq == NULL);
      p_drv->p_in_rq = p_pw;
      p_pw->p_gdrv = NULL;
    }
  else
    {
      p_pw->p_gate = p_gate;
      p_pw->p_gdrv = nm_gate_drv_get(p_gate, p_drv);
    }
}

static inline void nm_pw_ref_inc(struct nm_pkt_wrap_s *p_pw)
{
  __sync_fetch_and_add(&p_pw->ref_count, 1);
}
static inline void nm_pw_ref_dec(struct nm_pkt_wrap_s *p_pw)
{
  const int count = __sync_sub_and_fetch(&p_pw->ref_count, 1);
  assert(count >= 0);
  if(count == 0)
    {
      nm_pw_free(p_pw);
    }
}


/** Schedule and post new outgoing buffers */
static inline void nm_strat_try_and_commit(nm_gate_t p_gate)
{
  nm_core_lock_assert(p_gate->p_core);
  if(!nm_pkt_wrap_list_empty(&p_gate->out_list))
    {
      struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
      nm_profile_inc(p_gate->p_core->profiling.n_try_and_commit);
      r->driver->try_and_commit(r->_status, p_gate);
    }
}
/** Schedule and post new large incoming buffers */
static inline void nm_strat_rdv_accept(nm_gate_t p_gate)
{
  nm_core_lock_assert(p_gate->p_core);
  if(!nm_pkt_wrap_list_empty(&p_gate->pending_large_recv))
    {
      const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
      strategy->driver->rdv_accept(strategy->_status, p_gate);
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
      struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_malloc(p_core->ctrl_chunk_allocator);
      p_ctrl_chunk->ctrl = *p_header;
      nm_ctrl_chunk_list_push_back(&p_gate->ctrl_chunk_list, p_ctrl_chunk);
    }
}

static inline void nm_req_chunk_submit(struct nm_core*p_core, struct nm_req_chunk_s*p_req_chunk)
{
  nm_core_nolock_assert(p_core);
  int rc;
  do
    {
      rc = nm_req_chunk_lfqueue_enqueue(&p_core->pack_submissions, p_req_chunk);
      if(rc)
	nm_core_flush(p_core);
    }
  while(rc);
}

static inline void nm_req_chunk_destroy(struct nm_core*p_core, struct nm_req_chunk_s*p_req_chunk)
{
  struct nm_req_s*p_pack = p_req_chunk->p_req;

  if(p_req_chunk != &p_pack->req_chunk)
    {
      nm_req_chunk_free(p_core->req_chunk_allocator, p_req_chunk);
    }
  else
    {
      p_pack->req_chunk.p_req = NULL;
    }
}

/** Post a ready-to-receive
 */
static inline void nm_core_post_rtr(nm_gate_t p_gate,  nm_core_tag_t tag, nm_seq_t seq,
				    nm_drv_t p_drv, nm_trk_id_t trk_id, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  nm_header_ctrl_generic_t h;
  int gdrv_index = -1, k = 0;
  nm_gdrv_vect_itor_t i;
  puk_vect_foreach(i, nm_gdrv, &p_gate->gdrv_array)
    {
      if((*i)->p_drv == p_drv)
	{
	  gdrv_index = k;
	  break;
	}
      k++;
    }
  assert(gdrv_index >= 0);
  nm_header_init_rtr(&h, tag, seq, gdrv_index, trk_id, chunk_offset, chunk_len);
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
  const int high = (!nm_req_list_empty(&p_core->unpacks)) || (!nm_req_list_empty(&p_core->pending_packs));
  piom_ltask_poll_level_set(high);
#endif /* PIOMAN */
}


#endif /* NM_CORE_INLINE_H */
