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

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_default.h"
#include "nm_so_pkt_wrap.h"

struct nm_so_strat_default_gate {
  /* list of raw outgoing packets */
  struct list_head out_list;
};


/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int pack(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len)
{
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int flags = 0;
  int err;

  if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
    flags = NM_SO_DATA_USE_COPY;

  /* Simply form a new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					  data, len,
					  flags,
					  &p_so_pw);
  if(err != NM_ESUCCESS) {
    printf("nm_so_pw_alloc failed: err = %d\n", err);
    goto out;
  }

  list_add_tail(&p_so_pw->link,
                &((struct nm_so_strat_default_gate *)p_so_gate->strat_priv)->out_list);

  p_so_gate->status[tag][seq] &= ~NM_SO_STATUS_SEND_COMPLETED;

  err = NM_ESUCCESS;
 out:
  return err;
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int try_and_commit(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct list_head *out_list =
    &((struct nm_so_strat_default_gate *)p_so_gate->strat_priv)->out_list;
  struct nm_so_pkt_wrap *p_so_pw;

  if(!tbx_slist_is_nil(p_gate->post_sched_out_list))
    /* We're done */
    goto out;

  if(list_empty(out_list))
    /* We're done */
    goto out;

  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  p_so_pw->pw.p_gate = p_gate;

  /* WARNING (HARDCODED): packet is assigned to track 0 */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[0])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk =
		       p_so_pw->pw.p_gdrv->p_gate_trk_array[0])->p_trk;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->post_sched_out_list, &p_so_pw->pw);

 out:
    return NM_ESUCCESS;
}

/* Initialization */
static int init(void)
{
  return NM_ESUCCESS;
}

static int init_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_default_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_default_gate));

  INIT_LIST_HEAD(&priv->out_list);

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_default = {
  .init = init,
  .init_gate = init_gate,
  .pack = pack,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .priv = NULL,
};
