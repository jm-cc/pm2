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
  nm_so_pw_assign(p_pw, trk_id, drv_id, p_gate);
  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->p_core->so_sched.post_recv_req, p_pw);
  p_gate->active_recv[drv_id][trk_id] = 1;
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
  tbx_slist_append(p_core->so_sched.post_sched_out_list, p_pw);

  p_gate->active_send[drv_id][trk_id]++;
}


/** Schedule and post new outgoing buffers
 */
static inline int nm_strat_try_and_commit(struct nm_gate *p_gate)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  return r->driver->try_and_commit(r->_status, p_gate);
}

/** Post an ACK
 */
static inline void nm_so_post_rtr(struct nm_gate*p_gate,  nm_tag_t tag, nm_seq_t seq,
				  nm_drv_id_t drv_id, nm_trk_id_t trk_id, uint32_t chunk_offset, uint32_t chunk_len)
{
  nm_so_generic_ctrl_header_t h;
  nm_so_init_rtr(&h, tag, seq, drv_id, trk_id, chunk_offset, chunk_len);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_gate, &h);
}

/* ** Pack/unpack ****************************************** */

static inline int nm_so_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack, 
			       struct nm_gate *p_gate, nm_tag_t tag,
			       void *data, uint32_t len)
{
  /* Nothing special to flag for the contiguous reception */
  const nm_so_flag_t flag = NM_UNPACK_TYPE_CONTIGUOUS;
  return __nm_so_unpack(p_core, p_unpack, p_gate, tag, flag, data, len);
}

static inline int nm_so_unpackv(struct nm_core*p_core, struct nm_unpack_s*p_unpack, 
				struct nm_gate *p_gate, nm_tag_t tag,
				struct iovec *iov, int nb_entries)
{
  /* Data will be receive in an iovec tab */
  const nm_so_flag_t flag = NM_UNPACK_TYPE_IOV;
  return __nm_so_unpack(p_core, p_unpack, p_gate, tag, flag, iov, iov_len(iov, nb_entries));
}

static inline int nm_so_unpack_datatype(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
					struct nm_gate *p_gate, nm_tag_t tag,
					struct DLOOP_Segment *segp)
{
  /* Data will be receive through a datatype */
  const nm_so_flag_t flag = NM_UNPACK_TYPE_DATATYPE;
  return __nm_so_unpack(p_core, p_unpack, p_gate, tag, flag, segp, datatype_size(segp));
}

static inline int nm_so_pack(struct nm_pack_s*p_pack, nm_tag_t tag, nm_gate_t p_gate,
			     const void*data, uint32_t len, nm_so_flag_t pack_type)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t seq = p_so_tag->send_seq_number++;
  p_pack->status = pack_type;
  p_pack->data   = (void*)data;
  p_pack->len    = len;
  p_pack->done   = 0;
  p_pack->p_gate = p_gate;
  p_pack->tag    = tag;
  p_pack->seq    = seq;
  return (*r->driver->pack)(r->_status, p_pack);
}

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


#endif /* NM_CORE_INLINE_H */
