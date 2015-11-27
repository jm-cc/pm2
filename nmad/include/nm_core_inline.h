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
  if((r->driver->todo == NULL) ||
     (r->driver->todo && r->driver->todo(r->_status, p_gate)))
    {
      err = r->driver->try_and_commit(r->_status, p_gate);
    }
#endif
  return err;
}

/** Post a ready-to-receive
 */
static inline void nm_so_post_rtr(struct nm_gate*p_gate,  nm_core_tag_t tag, nm_seq_t seq,
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
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_gate, &h);
}

/** Post an ACK
 */
static inline void nm_so_post_ack(struct nm_gate*p_gate, nm_core_tag_t tag, nm_seq_t seq)
{
  nm_header_ctrl_generic_t h;
  nm_header_init_ack(&h, tag, seq);
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
