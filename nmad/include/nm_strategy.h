/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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


/* Driver for 'NewMad_Strategy' component interface
 */
struct nm_strategy_iface_s
{
  /** submit a chunk of data to the strategy */
  void (*pack_chunk)(void*_status, struct nm_pack_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);

  /** submit a chunk of control data */
  int (*pack_ctrl)(void*_status, struct nm_gate*p_gate, const union nm_header_ctrl_generic_s*p_ctrl);

  /** submit a pack with iterator-based data description */
  void (*pack_data)(void*_status, struct nm_pack_s*p_pack);
  
  /** Compute and apply the best possible packet rearrangement, then
      return next packet to send */
  int (*try_and_commit)(void*_status, struct nm_gate*p_gate);

  /** Emit RTR series for received RDV requests. */
  void (*rdv_accept)(void*_status, struct nm_gate*p_gate);

  /** flush pending packs */
  int (*flush)(void*_status, struct nm_gate*p_gate);

  /** Returns 1 if there are packets to send */
  int (*todo)(void* _status, struct nm_gate*p_gate);

  /** process strat private protocol */
  void (*proto)(void*_status, struct nm_gate*p_gate, struct nm_pkt_wrap*p_pw, const void*ptr, nm_len_t len);

};

PUK_IFACE_TYPE(NewMad_Strategy, struct nm_strategy_iface_s);

#endif /* NM_SO_STRATEGIES_H */
