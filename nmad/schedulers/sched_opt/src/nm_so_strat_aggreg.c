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
#include "nm_so_strategies/nm_so_strat_aggreg.h"
#include "nm_so_pkt_wrap.h"

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int pack(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len)
{
  struct nm_so_pkt_wrap *p_so_pw;
  struct list_head *out_list =
    &((struct nm_so_gate *)p_gate->sch_private)->out_list;
  int flags = 0;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, out_list, link) {
    uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
    uint32_t d_rlen = nm_so_pw_remaining_data(p_so_pw);
    uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

    if(size > d_rlen || NM_SO_DATA_HEADER_SIZE > h_rlen)
      /* There's not enough room to add our data to this paquet */
      goto next;

    if(len <= NM_SO_COPY_ON_SEND_THRESHOLD && size <= h_rlen)
      /* We can copy data into the header zone */
      flags = NM_SO_DATA_USE_COPY;
    else
      if(p_so_pw->pw.v_nb == NM_SO_PREALLOC_IOV_LEN)
	goto next;

    err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, flags);
    goto out;

  next:
    ;
  }

  if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
    flags = NM_SO_DATA_USE_COPY;

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					  data, len,
					  flags,
					  &p_so_pw);
  if(err != NM_ESUCCESS) {
    printf("nm_so_pw_alloc failed: err = %d\n", err);
    goto out;
  }

  list_add_tail(&p_so_pw->link, out_list);

  err = NM_ESUCCESS;
 out:
  return err;
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int try_and_commit(struct nm_gate *p_gate)
{
  struct list_head *out_list =
    &((struct nm_so_gate *)p_gate->sch_private)->out_list;
  p_tbx_slist_t post = p_gate->post_sched_out_list;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err = NM_ESUCCESS;

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
  tbx_slist_append(post, &p_so_pw->pw);

 out:
    return err;
}

/* Initialization */
static int init(void)
{
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_aggreg = {
  .init = init,
  .pack = pack,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .priv = NULL,
};
