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
#include "nm_so_strategies/nm_so_strat_balance.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_tracks.h"
#include "nm_so_parameters.h"


/** Gate storage for strat balance.
 */
struct nm_so_strat_balance_gate {
        /** List of raw outgoing packets. */
  struct list_head out_list;
  unsigned nb_paquets;
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
  struct nm_so_strat_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_balance_gate *)p_so_gate->strat_priv;
  int err;

  if(!list_empty(&p_so_sa_gate->out_list)) {
    /* Inspect only the head of the list */
    p_so_pw = nm_l2so(p_so_sa_gate->out_list.next);

    /* If the paquet is reasonably small, we can form an aggregate */
    if(p_so_pw->pw.length <= 32 - NM_SO_CTRL_HEADER_SIZE) {
      err = nm_so_pw_add_control(p_so_pw, p_ctrl);
      goto out;
    }
  }

  /* Otherwise, simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &p_so_sa_gate->out_list);
  p_so_sa_gate->nb_paquets++;

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
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_balance_gate *)p_so_gate->strat_priv;
  int flags = 0;
  int err;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */

    /* We aggregate ONLY if data are very small OR if there are
       already two ready paquets

       constant 512 is currently a hard-coded threshold which should
       eventually be updated with a real network value

       constant 2 is currently the hard-coded number of NICs in
       multi-rail --> we do not try to aggregate until there is at
       least as many available packets as the number of NICs (should
       eventually be updated with the actual number of NICs)
    */
    if(len <= 512 || p_so_sa_gate->nb_paquets >= 2) {

      /* We first try to find an existing packet to form an aggregate */
      list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {
	uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
	uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

	if(size <= h_rlen)
	  /* We can copy data into the header zone */
	  flags = NM_SO_DATA_USE_COPY;
	else
	  /* There's not enough room to add our data to this paquet */
	  goto next;

	err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, flags);
	goto out;

      next:
	;
      }
    }

    flags = NM_SO_DATA_USE_COPY;

    /* We didn't have a chance to form an aggregate, so simply form a
       new packet wrapper and add it to the out_list */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					    data, len,
					    flags,
					    &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);
    p_so_sa_gate->nb_paquets++;

  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                            data, len,
                                            NM_SO_DATA_DONT_USE_HEADER,
                                            &p_so_pw);
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

      nm_so_init_rdv(&ctrl, tag + 128, seq, len);

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
  struct nm_so_strat_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_balance_gate *)p_so_gate->strat_priv;
  struct list_head *out_list = &p_so_sa_gate->out_list;
  struct nm_so_pkt_wrap *p_so_pw;
  int n, drv_id;

 start:
  if(list_empty(out_list))
    /* We're done */
    goto out;

  for(n = 0; n < NM_SO_MAX_NETS; n++) {
    drv_id = nm_so_network_latency(n);
    if (p_so_gate->active_send[drv_id][TRK_SMALL] +
	p_so_gate->active_send[drv_id][TRK_LARGE] == 0)
    /* We found an idle NIC */
    goto next;
  }

  /* We didn't found any idle NIC, so we're done*/
  goto out;

 next:
  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);
  p_so_sa_gate->nb_paquets--;

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  /* Post packet on track 0 */
  _nm_so_post_send(p_gate, p_so_pw, TRK_SMALL, drv_id);

  /* Loop and see if we can feed another NIC with a ready paquet */
  goto start;

 out:
    return NM_ESUCCESS;
}

/** Initialize the strategy module.
 *
 *  @return The NM status.
 */
static int init(void)
{
  NM_LOGF("[loading strategy: <balance>]");
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
  int n;

  /* Is there any large data track available? */
  for(n = 0; n < NM_SO_MAX_NETS; n++) {

    /* We explore the drivers from the fastest to the slowest. */
    *drv_id = nm_so_network_bandwidth(n);

    if(p_so_gate->active_recv[*drv_id][*trk_id] == 0) {
      /* Cool! The suggested track is available! */
      return NM_ESUCCESS;
    }
  }

  /* We're forced to postpone the acknowledgement. */
  return -NM_EAGAIN;
}

/** Initialize the gate storage for balance strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int init_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_balance_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_balance_gate));

  INIT_LIST_HEAD(&priv->out_list);

  priv->nb_paquets = 0;

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}

/** Cleanup the gate storage for balance strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int exit_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_balance_gate *priv = ((struct nm_so_gate *)p_gate->sch_private)->strat_priv;
  TBX_FREE(priv);
  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = NULL;
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_balance = {
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
  .priv = NULL,
};
