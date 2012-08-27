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

#ifndef NM_SO_STRATEGIES_H
#define NM_SO_STRATEGIES_H

/** a chunk of rdv data. 
 * Use one for regular rendezvous; use nb_driver chunks for multi-rail.
 */
struct nm_rdv_chunk
{
  nm_drv_t p_drv;
  nm_trk_id_t trk_id;
  nm_len_t len;
};

typedef const union nm_so_generic_ctrl_header*nm_generic_header_t;

/* Driver for 'NewMad_Strategy' component interface
 */
struct nm_strategy_iface_s
{
  /** Handle the arrival of a new packet. The strategy may already apply
      some optimizations at this point */
  int (*pack)(void*_status, struct nm_pack_s*p_pack);

  int (*pack_ctrl)(void*_status,
                   struct nm_gate *p_gate,
		   nm_generic_header_t p_ctrl);
  
  /** Compute and apply the best possible packet rearrangement, then
      return next packet to send */
  int (*try_and_commit)(void*_status,
			struct nm_gate *p_gate);

  /** Allow (or not) the acknowledgement of a Rendez-Vous request.
      @warning drv_id and trk_id are IN/OUT parameters. They initially
      hold values "suggested" by the caller. */
  int (*rdv_accept)(void*_status,
		    struct nm_gate *p_gate,
		    nm_len_t len,
		    int*nb_chunks,
		    struct nm_rdv_chunk*chunks);

  /** flush pending packs */
  int (*flush)(void*_status, struct nm_gate *p_gate);

  /** Returns 1 if there are packets to send */
  int (*todo)(void* _status, struct nm_gate*p_gate);

  /** process strat private protocol */
  void (*proto)(void*_status, struct nm_gate*p_gate, struct nm_pkt_wrap*p_pw, const void*ptr, nm_len_t len);

};

PUK_IFACE_TYPE(NewMad_Strategy, struct nm_strategy_iface_s);

#endif
