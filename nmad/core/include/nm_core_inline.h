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


/* ** Receive functions ************************************ */
/* ********************************************************* */

/** Places a packet in the receive requests list. 
 * The actual post_recv operation will be done on next 
 * nmad scheduling (immediately with vanilla PIOMan, 
 * maybe later with PIO-offloading, on next nm_schedule()
 * without PIOMan).
 */
static __tbx_inline__ void nm_core_post_recv(struct nm_pkt_wrap *p_pw, struct nm_gate *p_gate, 
					     nm_trk_id_t trk_id, nm_drv_id_t drv_id)
{
  struct nm_core*p_core = p_gate->p_core;
  nm_so_pw_assign(p_pw, trk_id, drv_id, p_gate);
  struct nm_drv*p_drv = p_pw->p_drv;
  nm_so_lock_in(p_core, p_drv);
  tbx_fast_list_add_tail(&p_pw->link, &p_drv->post_recv_list[trk_id]);
  struct nm_gate_drv*p_gdrv = p_pw->p_gdrv;
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
					     nm_trk_id_t trk_id, nm_drv_id_t drv_id)
{
  struct nm_core*p_core = p_gate->p_core;
  NM_SO_TRACE_LEVEL(3, "Packet posted on track %d\n", trk_id);
  FUT_DO_PROBE4(FUT_NMAD_NIC_OPS_GATE_TO_TRACK, p_gate->id, p_pw, drv_id, trk_id );
  /* Packet is assigned to given track, driver, and gate */
  nm_so_pw_assign(p_pw, trk_id, drv_id, p_gate);
  /* append pkt to scheduler post list */
  struct nm_drv*p_drv = p_pw->p_drv;
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
				  nm_drv_id_t drv_id, nm_trk_id_t trk_id, uint32_t chunk_offset, uint32_t chunk_len)
{
  nm_so_generic_ctrl_header_t h;
  nm_so_init_rtr(&h, tag, seq, drv_id, trk_id, chunk_offset, chunk_len);
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
