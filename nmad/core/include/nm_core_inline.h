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
static inline struct nm_gate_drv*nm_gate_drv_get(struct nm_gate*p_gate, nm_drv_t p_drv)
{
  int i;
  assert(p_drv != NULL);
  for(i = 0; i < NM_DRV_MAX; i++)
    {
      if((p_gate->p_gate_drv_array[i] != NULL) && (p_gate->p_gate_drv_array[i]->p_drv == p_drv))
	return p_gate->p_gate_drv_array[i];
    }
  return NULL;
}

/** The default network to use when several networks are
 *  available, but the strategy does not support multi-rail.
 *  Currently: the first available network
 */
static inline struct nm_drv*nm_drv_default(struct nm_gate*p_gate)
{
  return p_gate->p_gate_drv_array[0]->p_drv;
}

/** Get a driver given its id.
 */
static inline struct nm_drv*nm_drv_get_by_id(struct nm_core*p_core, nm_drv_id_t id)
{
  struct nm_drv*p_drv = NULL;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      if(p_drv->id == id)
	return p_drv;
    }
  return NULL;
}


/* ** Packet wrapper management **************************** */

static __inline__ uint32_t nm_so_pw_remaining_header_area(struct nm_pkt_wrap *p_pw)
{
  const struct iovec *vec = &p_pw->v[0];
  return NM_SO_MAX_UNEXPECTED - ((vec->iov_base + vec->iov_len) - (void *)p_pw->buf);
}

static __inline__ uint32_t nm_so_pw_remaining_data(struct nm_pkt_wrap *p_pw)
{
  return NM_SO_MAX_UNEXPECTED - p_pw->length;
}

static __inline__ void nm_so_pw_assign(struct nm_pkt_wrap*p_pw, nm_trk_id_t trk_id, nm_drv_t p_drv, nm_gate_t p_gate)
{
  p_pw->p_gate = p_gate;
  /* Packet is assigned to given driver */
  p_pw->p_drv = p_drv;
  p_pw->p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  /* Packet is assigned to given track */
  p_pw->trk_id = trk_id;
}

/** Ensures p_pw->v contains at least n entries
 */
static inline void nm_pw_grow_n(struct nm_pkt_wrap*p_pw, int n)
{
  if(n >= p_pw->v_size)
    {
      while(n >= p_pw->v_size)
	p_pw->v_size *= 2;
      if(p_pw->v == p_pw->prealloc_v)
	p_pw->v = TBX_MALLOC(sizeof(struct iovec) * p_pw->v_size);
      else
	p_pw->v = TBX_REALLOC(p_pw->v, sizeof(struct iovec) * p_pw->v_size);
    }
}

static inline struct iovec*nm_pw_grow_iovec(struct nm_pkt_wrap*p_pw)
{
  nm_pw_grow_n(p_pw, p_pw->v_nb + 1);
  assert(p_pw->v_nb <= p_pw->v_size);
  return &p_pw->v[p_pw->v_nb++];
}

/** Add short data to pw, with compact header */
static inline void nm_so_pw_add_short_data(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
					   const void*data, uint32_t len)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_short_data_header *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_SO_SHORT_DATA_HEADER_SIZE;
  nm_so_init_short_data(h, tag, seq, len);
  if(len)
    {
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += len;
    }
  p_pw->length += NM_SO_SHORT_DATA_HEADER_SIZE + len;
}

/** Add small data to pw, in header */
static inline void nm_so_pw_add_data_in_header(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
					       const void*data, uint32_t len, uint32_t chunk_offset, uint8_t flags)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
  const uint32_t size = nm_so_aligned(len);
  nm_so_init_data(h, tag, seq, flags | NM_PROTO_FLAG_ALIGNED, 0, len, chunk_offset);
  if(len)
    {
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += size;
    }
  p_pw->length += NM_SO_DATA_HEADER_SIZE + size;
}

/** Add small data to pw, in iovec */
static inline void nm_so_pw_add_data_in_iovec(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
					      const void*data, uint32_t len, uint32_t chunk_offset, uint8_t proto_flags)
{
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
  struct iovec *dvec = nm_pw_grow_iovec(p_pw);
  dvec->iov_base = (void*)data;
  dvec->iov_len = len;
  /* We don't know yet the gap between header and data, so we
     temporary store the iovec index as the 'skip' value */
  nm_so_init_data(h, tag, seq, proto_flags, p_pw->v_nb, len, chunk_offset);
  p_pw->length += NM_SO_DATA_HEADER_SIZE + len;
}

/** Add raw data to pw, without header */
static inline void nm_so_pw_add_raw(struct nm_pkt_wrap*p_pw, const void*data, uint32_t len, uint32_t chunk_offset)
{
  assert(p_pw->flags & NM_PW_NOHEADER);
  struct iovec*vec = nm_pw_grow_iovec(p_pw);
  vec->iov_base = (void*)data;
  vec->iov_len = len;
  p_pw->length += len;
  p_pw->chunk_offset = chunk_offset;
}

static inline int nm_so_pw_add_control(struct nm_pkt_wrap*p_pw, const union nm_so_generic_ctrl_header*p_ctrl)
{
  struct iovec*hvec = &p_pw->v[0];
  memcpy(hvec->iov_base + hvec->iov_len, p_ctrl, NM_SO_CTRL_HEADER_SIZE);
  hvec->iov_len += NM_SO_CTRL_HEADER_SIZE;
  p_pw->length += NM_SO_CTRL_HEADER_SIZE;
  return NM_ESUCCESS;
}


static inline void nm_so_pw_alloc_and_fill_with_data(struct nm_pack_s*p_pack, const void*ptr, uint32_t chunk_len,
						     uint32_t chunk_offset, tbx_bool_t is_last_chunk,
						     int flags, struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw;
  nm_so_pw_alloc(flags, &p_pw);
  nm_so_pw_add_data(p_pw, p_pack, ptr, chunk_len, chunk_offset, flags);
  p_pw->chunk_offset = chunk_offset;
  *pp_pw = p_pw;
}

static inline int nm_so_pw_alloc_and_fill_with_control(const union nm_so_generic_ctrl_header *ctrl,
						       struct nm_pkt_wrap **pp_pw)
{
  int err;
  struct nm_pkt_wrap *p_pw;

  err = nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_control(p_pw, ctrl);
  if(err != NM_ESUCCESS)
    goto free;

  *pp_pw = p_pw;

 out:
    return err;
 free:
    nm_so_pw_free(p_pw);
    goto out;
}

static inline void nm_pw_add_contrib(struct nm_pkt_wrap*p_pw, struct nm_pack_s*p_pack, uint32_t len)
{
  if(p_pw->n_contribs > 0 && p_pw->contribs[p_pw->n_contribs - 1].p_pack == p_pack)
    {
      p_pw->contribs[p_pw->n_contribs - 1].len += len;
    }
  else
    {
      if(p_pw->n_contribs >= p_pw->contribs_size)
	{
	  p_pw->contribs_size *= 2;
	  if(p_pw->contribs == p_pw->prealloc_contribs)
	    {
	      p_pw->contribs = TBX_MALLOC(sizeof(struct nm_pw_contrib_s) * p_pw->contribs_size);
	      memcpy(p_pw->contribs, p_pw->prealloc_contribs, sizeof(struct nm_pw_contrib_s) * NM_SO_PREALLOC_IOV_LEN);
	    }
	  else
	    {
	      p_pw->contribs = TBX_REALLOC(p_pw->contribs, sizeof(struct nm_pw_contrib_s) * p_pw->contribs_size);
	    }
	}
      struct nm_pw_contrib_s*p_contrib = &p_pw->contribs[p_pw->n_contribs];
      p_contrib->len = len;
      p_contrib->p_pack = p_pack;
      p_pw->n_contribs++;
    }
  p_pack->scheduled += len;
}


static __inline__ int nm_so_pw_dec_header_ref_count(struct nm_pkt_wrap *p_pw)
{
  if(!(--p_pw->header_ref_count))
    {
      nm_so_pw_free(p_pw);
    }
  return NM_ESUCCESS;
}


/* ** Receive functions ************************************ */
/* ********************************************************* */

/** Places a packet in the receive requests list. 
 * The actual post_recv operation will be done on next 
 * nmad scheduling (immediately with vanilla PIOMan, 
 * maybe later with PIO-offloading, on next nm_schedule()
 * without PIOMan).
 */
static __tbx_inline__ void nm_core_post_recv(struct nm_pkt_wrap *p_pw, struct nm_gate *p_gate, 
					     nm_trk_id_t trk_id, struct nm_drv*p_drv)
{
  struct nm_core*p_core = p_gate->p_core;
  nm_so_pw_assign(p_pw, trk_id, p_drv, p_gate);
  nm_so_lock_in(p_core, p_drv);
  tbx_fast_list_add_tail(&p_pw->link, &p_drv->post_recv_list[trk_id]);
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
  assert(p_gdrv->active_recv[trk_id] == 0);
  p_gdrv->active_recv[trk_id] = 1;
  nm_so_unlock_in(p_core, p_drv);
}

/* ** Sending functions ************************************ */
/* ********************************************************* */

/** Places a packet in the send request list.
 * The actual post_send operation will be done on next
 * nmad scheduling (immediately with vanilla PIOMan, 
 * maybe later with PIO-offloading, on next nm_schedule()
 * without PIOMan).
 */
static __tbx_inline__ void nm_core_post_send(struct nm_gate *p_gate,
					     struct nm_pkt_wrap *p_pw,
					     nm_trk_id_t trk_id, struct nm_drv*p_drv)
{
  struct nm_core*p_core = p_gate->p_core;
  NM_SO_TRACE_LEVEL(3, "Packet posted on track %d\n", trk_id);
  FUT_DO_PROBE4(FUT_NMAD_NIC_OPS_GATE_TO_TRACK, p_gate, p_pw, p_drv, trk_id );
  /* Packet is assigned to given track, driver, and gate */
  nm_so_pw_assign(p_pw, trk_id, p_drv, p_gate);
  /* append pkt to scheduler post list */
  nm_so_lock_out(p_core, p_drv);
  tbx_fast_list_add_tail(&p_pw->link, &p_drv->post_sched_out_list[trk_id]);
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
  p_gdrv->active_send[trk_id]++;
  nm_so_unlock_out(p_core, p_drv);
}


/** Schedule and post new outgoing buffers
 */
static inline int nm_strat_try_and_commit(struct nm_gate *p_gate)
{
  int err;
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
#ifdef FINE_GRAIN_LOCKING
  if(r->driver->todo && 
     r->driver->todo(r->_status, p_gate)) 
  {
    if(nm_trylock_interface(p_gate->p_core)) {
      nm_lock_status(p_gate->p_core);
      err = r->driver->try_and_commit(r->_status, p_gate);
      nm_unlock_status(p_gate->p_core);
      nm_unlock_interface(p_gate->p_core);
      goto out;
    }
  }
  err = NM_EINPROGRESS;
out:
#else
  err = r->driver->try_and_commit(r->_status, p_gate);
#endif
  return err;
}

/** Post a ready-to-receive
 */
static inline void nm_so_post_rtr(struct nm_gate*p_gate,  nm_core_tag_t tag, nm_seq_t seq,
				  nm_drv_t p_drv, nm_trk_id_t trk_id, uint32_t chunk_offset, uint32_t chunk_len)
{
  nm_so_generic_ctrl_header_t h;
  nm_so_init_rtr(&h, tag, seq, p_drv->id, trk_id, chunk_offset, chunk_len);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_gate, &h);
}

/** Post an ACK
 */
static inline void nm_so_post_ack(struct nm_gate*p_gate, nm_core_tag_t tag, nm_seq_t seq)
{
  nm_so_generic_ctrl_header_t h;
  nm_so_init_ack(&h, tag, seq);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_gate, &h);
}

/** Flush the given gate.
 */
static inline int nm_so_flush(nm_gate_t p_gate)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  if(tbx_unlikely(r->driver->flush))
    {
      return (*r->driver->flush)(r->_status, p_gate);
    }
  else
    return -NM_ENOTIMPL;
}

/** Fires an event
 */
static inline void nm_core_status_event(nm_core_t p_core, const struct nm_so_event_s*const event, nm_status_t*p_status)
{
  nm_so_monitor_itor_t i;
  if(p_status)
    *p_status |= event->status;
  puk_vect_foreach(i, nm_so_monitor, &p_core->monitors)
    {
      if((*i)->mask & event->status)
	{
	  ((*i)->notifier)(event);
	}
    }
}


#endif /* NM_CORE_INLINE_H */
