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

#include "nm_so_headers.h"

typedef struct nm_so_strategy_struct nm_so_strategy;

/* Initialization */
typedef int (*nm_so_strategy_init_func)(void);

typedef int (*nm_so_strategy_init_gate)(struct nm_gate *p_gate);

/* Termination */
typedef int (*nm_so_strategy_exit_func)(void);

typedef int (*nm_so_strategy_exit_gate)(struct nm_gate *p_gate);

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
typedef int (*nm_so_strategy_pack_func)(struct nm_gate *p_gate,
					uint8_t tag, uint8_t seq,
					void *data, uint32_t len);

typedef int (*nm_so_strategy_pack_extended_func)(struct nm_gate *p_gate,
                                                 uint8_t tag, uint8_t seq,
                                                 void *data, uint32_t len,
                                                 tbx_bool_t is_completed);


typedef int (*nm_so_strategy_pack_ctrl_func)(struct nm_gate *gate,
					     union nm_so_generic_ctrl_header *p_ctrl);

/* Compute the best possible packet rearrangement with no side-effect
   on pre_list */
typedef int (*nm_so_strategy_try_func)(struct nm_gate *p_gate,
				       unsigned *score);

/* Apply the "already computed" strategy on pre_list and return next
   packet to send */
typedef int (*nm_so_strategy_commit_func)(void);

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
typedef int (*nm_so_strategy_try_and_commit_func)(struct nm_gate *p_gate);

typedef int (*nm_so_strategy_flush_func)(struct nm_gate *p_gate);

/* Forget the pre-computed stuff */
typedef int (*nm_so_strategy_cancel_func)(void);

/* Allow (or not) the acknowledgement of a Rendez-Vous request.
   WARNING: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
typedef int (*nm_so_strategy_rdv_accept_func)(struct nm_gate *p_gate,
					      unsigned long *drv_id,
					      unsigned long *trk_id);

#ifdef NMAD_QOS
typedef int (*nm_so_strategy_ack_callback_func)(struct nm_so_pkt_wrap *p_so_pw,
						uint8_t tag_id, uint8_t seq,
						uint8_t track_id, uint8_t finished);
#endif /* NMAD_QOS */

struct nm_so_strategy_struct {
  nm_so_strategy_init_func init;
  nm_so_strategy_exit_func exit;
  nm_so_strategy_init_gate init_gate;
  nm_so_strategy_exit_gate exit_gate;
  nm_so_strategy_pack_func pack;
  nm_so_strategy_pack_extended_func pack_extended;
  nm_so_strategy_pack_ctrl_func pack_ctrl;
  nm_so_strategy_try_func try;
  nm_so_strategy_commit_func commit;
  nm_so_strategy_try_and_commit_func try_and_commit;
  nm_so_strategy_flush_func flush;
  nm_so_strategy_cancel_func cancel;
  nm_so_strategy_rdv_accept_func rdv_accept;
#ifdef NMAD_QOS
  nm_so_strategy_ack_callback_func ack_callback;
#endif /* NMAD_QOS */
  void *priv;
};

#endif
