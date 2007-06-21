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
#include "nm_so_tracks.h"

/** Gate storage for strat default.
 */
struct nm_so_strat_default_gate {
  /** List of raw outgoing packets. */
  struct list_head out_list;
};


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param p_gate a pointer to the gate object.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int pack_ctrl(struct nm_gate *p_gate,
		     union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int err;

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add_tail(&p_so_pw->link,
                &((struct nm_so_strat_default_gate *)p_so_gate->strat_priv)->out_list);

 out:
  return err;
}

/** Handle a new packet submitted by the user code.
 *
 *  @note The strategy may already apply some optimizations at this point.
 *  @param p_gate a pointer to the gate object.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @return The NM status.
 */
static int pack(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len)
{
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int flags = 0;
  int err;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */

    if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
      flags = NM_SO_DATA_USE_COPY;

    /* Simply form a new packet wrapper and add it to the out_list */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
					    0, flags, &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link,
		  &((struct nm_so_strat_default_gate *)p_so_gate->strat_priv)->out_list);

  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                            0, NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    /* Then place it into the appropriate list of large pending
       "sends". */
    list_add_tail(&p_so_pw->link,
                  &(p_so_gate->pending_large_send[tag]));

    /* Signal we're waiting for an ACK */
    p_so_gate->pending_unpacks++;

    /* Finally, generate a RdV request */
    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0);

      err = pack_ctrl(p_gate, &ctrl);
      if(err != NM_ESUCCESS)
	goto out;
    }

    /* Check if we should post a new recv packet: we're waiting for an
       ACK! */
    nm_so_refill_regular_recv(p_gate);

  }

  err = NM_ESUCCESS;
 out:
  return err;
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int try_and_commit(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct list_head *out_list =
    &((struct nm_so_strat_default_gate *)p_so_gate->strat_priv)->out_list;
  struct nm_so_pkt_wrap *p_so_pw;

  if(p_so_gate->active_send[NM_SO_DEFAULT_NET][TRK_SMALL] ==
     NM_SO_MAX_ACTIVE_SEND_PER_TRACK)
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

  /* Post packet on track 0 */
  _nm_so_post_send(p_gate, p_so_pw, TRK_SMALL, NM_SO_DEFAULT_NET);

 out:
    return NM_ESUCCESS;
}

/** Initialization. */
static int init(void)
{
  NM_LOGF("[loading strategy: <default>]");
  return NM_ESUCCESS;
}

/** Accept or refuse a RDV on the suggested (driver/track/gate).
 *
 *  @warning @p drv_id and @p trk_id are IN/OUT parameters. They initially
 *  hold values "suggested" by the caller.
 *  @param p_gate a pointer to the gate object.
 *  @param drv_id the suggested driver id.
 *  @param trk_id the suggested track id.
 *  @return The NM status.
 */
static int rdv_accept(struct nm_gate *p_gate,
		      unsigned long *drv_id,
		      unsigned long *trk_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  *drv_id = 0;

  if(p_so_gate->active_recv[*drv_id][*trk_id] == 0)
    /* Cool! The suggested track is available! */
    return NM_ESUCCESS;
  else
    /* We decide to postpone the acknowledgement. */
    return -NM_EAGAIN;
}

/** Initialize the gate storage for default strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int init_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_default_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_default_gate));

  INIT_LIST_HEAD(&priv->out_list);

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}


/** Cleanup the gate storage for default strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int exit_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_default_gate *priv = ((struct nm_so_gate *)p_gate->sch_private)->strat_priv;
  TBX_FREE(priv);
  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = NULL;

  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_default = {
  .init = init,
  .exit = NULL,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .pack = pack,
  .pack_extended = NULL,
  .pack_ctrl = pack_ctrl,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .rdv_accept = rdv_accept,
  .pack_extended_ctrl = NULL,
  .pack_ctrl_chunk = NULL,
  .pack_extended_ctrl_end = NULL,
  .extended_rdv_accept = NULL,
  .priv = NULL,
};

