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

typedef struct nm_so_strategy_struct nm_so_strategy;

// Initialization
typedef int (*nm_so_strategy_init_func)(nm_so_strategy *strat);

// Compute the best possible packet rearrangement
// with no side-effect on pre_list
typedef int (*nm_so_strategy_try_func)(nm_so_strategy *strat,
				       struct nm_gate *p_gate,
				       struct nm_drv *driver,
				       p_tbx_slist_t pre_list,
				       unsigned *score);

// Apply the "already computed" strategy on pre_list
// and return next packet to send
typedef int (*nm_so_strategy_commit_func)(nm_so_strategy *strat,
					  p_tbx_slist_t pre_list,
					  struct nm_so_pkt_wrap **pp_so_pw);

// Compute and apply the best possible packet rearrangement, 
// then return next packet to send
typedef int (*nm_so_strategy_try_and_commit_func)(nm_so_strategy *strat,
						  struct nm_gate *p_gate,
						  struct nm_drv *driver,
						  p_tbx_slist_t pre_list,
						  struct nm_so_pkt_wrap **pp_so_pw);

// Forget the pre-computed stuff
typedef int (*nm_so_strategy_cancel_func)(nm_so_strategy *strat);

struct nm_so_strategy_struct {
  nm_so_strategy_init_func init;
  nm_so_strategy_try_func try;
  nm_so_strategy_commit_func commit;
  nm_so_strategy_try_and_commit_func try_and_commit;
  nm_so_strategy_cancel_func cancel;
  void *priv;
};

#endif
