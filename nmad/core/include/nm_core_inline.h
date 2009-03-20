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
					     nm_trk_id_t trk_id, int drv_id)
{
  p_pw->p_gate = p_gate;

  /* Packet is assigned to given driver */
  p_pw->p_drv = (p_pw->p_gdrv = nm_gate_drv_get(p_gate, drv_id))->p_drv;
  /* Packet is assigned to given track */
  p_pw->trk_id = trk_id;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->p_core->so_sched.post_recv_req, p_pw);

  p_gate->p_so_gate->active_recv[drv_id][trk_id] = 1;
}

static __tbx_inline__ int nm_so_post_regular_recv(struct nm_gate *p_gate, int drv_id)
{
  int err;
  struct nm_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER |
		       NM_SO_DATA_PREPARE_RECV,
		       &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  nm_core_post_recv(p_so_pw, p_gate, NM_TRK_SMALL, drv_id);

  err = NM_ESUCCESS;
 out:
  return err;
}

static __tbx_inline__ int nm_so_refill_regular_recv(struct nm_gate *p_gate)
{
  int err = NM_ESUCCESS;
  const int nb_drivers = p_gate->p_core->nb_drivers;
  int drv;

  for(drv = 0; drv < nb_drivers; drv++)
    if(!p_gate->p_so_gate->active_recv[drv][NM_TRK_SMALL])
      {
	err = nm_so_post_regular_recv(p_gate, drv);
      }
  
  return err;
}

static __tbx_inline__ int nm_so_direct_post_large_recv(struct nm_gate *p_gate, int drv_id,
						       struct nm_pkt_wrap *p_so_pw)
{
  int err;

  nm_core_post_recv(p_so_pw, p_gate, NM_TRK_LARGE, drv_id);

  err = NM_ESUCCESS;
  return err;
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
  p_pw->p_drv = (p_pw->p_gdrv = nm_gate_drv_get(p_gate, drv_id))->p_drv;
  p_pw->trk_id = trk_id;
  p_pw->p_gate = p_gate;

  /* append pkt to scheduler post list */
  //#warning vérifier le nb max de requetes concurrentes autorisées!!!!(dans nm_trk_cap.h -> max_pending_send_request)

  tbx_slist_append(p_core->so_sched.post_sched_out_list, p_pw);

  p_gate->p_so_gate->active_send[drv_id][trk_id]++;
}


#endif /* NM_CORE_INLINE_H */
