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

#include <Padico/Puk.h>
#include "nm_so_headers.h"

/* Driver for 'NmStrategy' component interface
 */
struct nm_so_strategy_driver
{
  /** Handle the arrival of a new packet. The strategy may already apply
      some optimizations at this point */
  int (*pack)(void*_status,
	      struct nm_gate *p_gate,
	      uint8_t tag, uint8_t seq,
	      void *data, uint32_t len);

  int (*packv)(void*_status,
               struct nm_gate *p_gate,
               uint8_t tag, uint8_t seq,
               struct iovec *iov, int nb_entries);

  int (*pack_datatype)(void*_status, struct nm_gate *p_gate,
		       uint8_t tag, uint8_t seq,
		       struct DLOOP_Segment *segp);

  int (*pack_extended)(void*_status,
		       struct nm_gate *p_gate,
		       uint8_t tag, uint8_t seq,
		       void *data, uint32_t len,
		       tbx_bool_t is_completed);

  int (*pack_ctrl)(void*_status,
                   struct nm_gate *p_gate,
		   union nm_so_generic_ctrl_header *p_ctrl);

  int (*pack_ctrl_chunk)(void*_status,
                         struct nm_so_pkt_wrap *p_so_pw,
                         union nm_so_generic_ctrl_header *p_ctrl);

  int (*pack_extended_ctrl)(void*_status,
                            struct nm_gate *p_gate,
                            uint32_t cumulated_header_len,
                            union nm_so_generic_ctrl_header *p_ctrl,
                            struct nm_so_pkt_wrap **pp_so_pw);

  int (*pack_extended_ctrl_end)(void*_status,
                                struct nm_gate *p_gate,
                                struct nm_so_pkt_wrap *p_so_pw);

  /** Compute the best possible packet rearrangement with no side-effect
      on pre_list */
  int (*try)(void*_status,
	     struct nm_gate *p_gate,
	     unsigned *score);

  /** Apply the "already computed" strategy on pre_list and return next
      packet to send */
  int (*commit)(void*_status);

  /** Compute and apply the best possible packet rearrangement, then
      return next packet to send */
  int (*try_and_commit)(void*_status,
			struct nm_gate *p_gate);

  /** Forget the pre-computed stuff */
  int (*cancel)(void*_status);

  /** Allow (or not) the acknowledgement of a Rendez-Vous request.
      @warning drv_id and trk_id are IN/OUT parameters. They initially
      hold values "suggested" by the caller. */
  int (*rdv_accept)(void*_status,
		    struct nm_gate *p_gate,
		    uint8_t *drv_id,
		    uint8_t *trk_id);

  int (*extended_rdv_accept)(void*_status,
                             struct nm_gate *p_gate,
                             uint32_t len_to_send,
                             int * nb_drv,
                             uint8_t *drv_ids,
                             uint32_t *chunk_lens);

  int (*flush)(void*_status,
               struct nm_gate *p_gate);

#ifdef NMAD_QOS
  int (*ack_callback)(void *_status,
                      struct nm_so_pkt_wrap *p_so_pw,
                      uint8_t tag_id, uint8_t seq,
                      uint8_t track_id, uint8_t finished);
#endif /* NMAD_QOS */
};

PUK_IFACE_TYPE(NmStrategy, struct nm_so_strategy_driver);

#endif
