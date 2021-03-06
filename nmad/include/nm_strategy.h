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

#ifndef NM_STRATEGY_H
#define NM_STRATEGY_H

/** Driver for 'NewMad_Strategy' component interface
 */
struct nm_strategy_iface_s
{
  /** selects the drivers to actually use, among the list of available drivers for the given gate */
  nm_drv_vect_t (*connect)(void*_status, nm_gate_t p_gate, nm_drv_vect_t p_available_drvs);
  
  /** submit a pack with iterator-based data description */
  void (*pack_data)(void*_status, struct nm_req_s*p_pack, nm_len_t len, nm_len_t chunk_offset);

  /** submit a chunk of control data */
  void (*pack_ctrl)(void*_status, nm_gate_t p_gate, const union nm_header_ctrl_generic_s*p_ctrl);
  
  /** Compute and apply the best possible packet rearrangement, then
      return next packet to send */
  void (*try_and_commit)(void*_status, nm_gate_t p_gate);

  /** Emit RTR series for received RDV requests. */
  void (*rdv_accept)(void*_status, nm_gate_t p_gate);

  /** process strat private protocol */
  void (*proto)(void*_status, nm_gate_t p_gate, struct nm_pkt_wrap_s*p_pw, const void*ptr, nm_len_t len);

};

PUK_IFACE_TYPE(NewMad_Strategy, struct nm_strategy_iface_s);

#endif /* NM_STRATEGY_H */
