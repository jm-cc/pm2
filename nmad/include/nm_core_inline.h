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
static inline struct nm_drv*nm_drv_default(struct nm_gate*p_gate)
{
  assert(!nm_gdrv_vect_empty(&p_gate->gdrv_array));
  nm_gdrv_vect_itor_t i = nm_gdrv_vect_begin(&p_gate->gdrv_array);
  return (*i)->p_drv;
}

/** Get a driver given its id.
 */
static inline struct nm_drv*nm_drv_get_by_index(struct nm_gate*p_gate, int index)
{
  assert(!nm_gdrv_vect_empty(&p_gate->gdrv_array));
  assert(index < nm_gdrv_vect_size(&p_gate->gdrv_array));
  assert(index >= 0);
  struct nm_gate_drv*p_gdrv = nm_gdrv_vect_at(&p_gate->gdrv_array, index);
  return p_gdrv->p_drv;
}


/* ** Packet wrapper management **************************** */

static __inline__ nm_len_t nm_so_pw_remaining_header_area(struct nm_pkt_wrap *p_pw)
{
  const struct iovec *vec = &p_pw->v[0];
  return NM_SO_MAX_UNEXPECTED - ((vec->iov_base + vec->iov_len) - (void *)p_pw->buf);
}

static __inline__ nm_len_t nm_so_pw_remaining_data(struct nm_pkt_wrap *p_pw)
{
  return NM_SO_MAX_UNEXPECTED - p_pw->length;
}

/** assign packet to given driver, gate, and track */
static __inline__ void nm_so_pw_assign(struct nm_pkt_wrap*p_pw, nm_trk_id_t trk_id, nm_drv_t p_drv, nm_gate_t p_gate)
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
					   const void*data, nm_len_t len)
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
					       const void*data, nm_len_t len, nm_len_t chunk_offset, uint8_t flags)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
  nm_so_init_data(h, tag, seq, flags | NM_PROTO_FLAG_ALIGNED, 0, len, chunk_offset);
  hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
  p_pw->length  += NM_SO_DATA_HEADER_SIZE;
  if(len)
    {
      const nm_len_t size = nm_so_aligned(len);
      assert(len <= nm_so_pw_remaining_data(p_pw));
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += size;
      p_pw->length  += size;
    }
}

/** Add small data to pw, in iovec */
static inline void nm_so_pw_add_data_in_iovec(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
					      const void*data, nm_len_t len, nm_len_t chunk_offset, uint8_t proto_flags)
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
static inline void nm_so_pw_add_raw(struct nm_pkt_wrap*p_pw, const void*data, nm_len_t len, nm_len_t chunk_offset)
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


static inline void nm_so_pw_alloc_and_fill_with_data(struct nm_pack_s*p_pack, const void*ptr, nm_len_t chunk_len,
						     nm_len_t chunk_offset, tbx_bool_t is_last_chunk,
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

static inline void nm_pw_ref_inc(struct nm_pkt_wrap *p_pw)
{
  p_pw->ref_count++;
}
static inline void nm_pw_ref_dec(struct nm_pkt_wrap *p_pw)
{
  p_pw->ref_count--;
  assert(p_pw->ref_count >= 0);
  if(p_pw->ref_count == 0)
    {
      nm_so_pw_free(p_pw);
    }
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
  nm_so_pw_assign(p_pw, trk_id, p_drv, p_gate);
  nm_pw_post_lfqueue_enqueue(&p_drv->post_recv, p_pw);
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
  if(p_gdrv)
    {
      assert(p_gdrv->active_recv[trk_id] == 0);
      p_gdrv->active_recv[trk_id] = 1;
    }
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
  FUT_DO_PROBE4(FUT_NMAD_NIC_OPS_GATE_TO_TRACK, p_gate, p_pw, p_drv, trk_id );
  /* Packet is assigned to given track, driver, and gate */
  nm_so_pw_assign(p_pw, trk_id, p_drv, p_gate);
  /* append pkt to scheduler post list */
  nm_pw_post_lfqueue_enqueue(&p_drv->post_send, p_pw);
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
  p_gdrv->active_send[trk_id]++;
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
    if(nm_trylock_interface(p_gate->p_core))
      {
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
				  nm_drv_t p_drv, nm_trk_id_t trk_id, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  nm_so_generic_ctrl_header_t h;
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
  nm_so_init_rtr(&h, tag, seq, gdrv_index, trk_id, chunk_offset, chunk_len);
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

/** Fires an event
 */
static inline void nm_core_status_event(nm_core_t p_core, const struct nm_core_event_s*const event, nm_status_t*p_status)
{
  nm_core_monitor_vect_itor_t i;
  if(p_status)
    *p_status |= event->status;
  puk_vect_foreach(i, nm_core_monitor, &p_core->monitors)
    {
      if((*i)->mask & event->status)
	{
	  ((*i)->notifier)(event);
	}
    }
}


#endif /* NM_CORE_INLINE_H */
