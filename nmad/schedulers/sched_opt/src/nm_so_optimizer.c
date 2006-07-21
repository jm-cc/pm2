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

#include "nm_so_optimizer.h"
#include "nm_so_strategies/nm_so_strat_exhaustive.h"
#include "nm_so_strategies/nm_so_strat_aggreg.h"

static nm_so_strategy *strategies[] = {
  &nm_so_strat_aggreg,
  &nm_so_strat_exhaustive,
  NULL
};

int
nm_so_optimizer_init(void)
{
  unsigned i = 0;
  int err = NM_ESUCCESS;

  for(i=0; strategies[i] != NULL; i++) {
    err = strategies[i]->init(strategies[i]);
    if(err != NM_ESUCCESS)
      goto out;
  }

out:
  return err;
}

int
nm_so_optimizer_schedule_out(struct nm_gate *p_gate,
			     struct nm_drv *driver,
			     p_tbx_slist_t pre_list,
			     struct nm_pkt_wrap **pp_pw)
{
  return strategies[0]->try_and_commit(strategies[0],
				       p_gate, driver, pre_list, pp_pw);
}

