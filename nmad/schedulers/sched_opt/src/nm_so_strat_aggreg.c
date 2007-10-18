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
#include "nm_so_tracks.h"

/** Gate storage for strat aggreg.
 */
struct nm_so_strat_aggreg_gate {
  /** List of raw outgoing packets. */
  struct list_head out_list;
};

static int nb_data_aggregation;
static int nb_ctrl_aggregation;

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
  struct nm_so_strat_aggreg_gate *p_so_sa_gate
    = (struct nm_so_strat_aggreg_gate *)p_so_gate->strat_priv;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {

    if(nm_so_pw_remaining_header_area(p_so_pw) < NM_SO_CTRL_HEADER_SIZE)
      /* There's not enough room to add our ctrl header to this paquet */
      goto next;

    err = nm_so_pw_add_control(p_so_pw, p_ctrl);

    goto out;

  next:
    ;
  }

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);

 out:
  return err;
}

static int
pack_extended_ctrl(struct nm_gate *p_gate,
                   uint32_t cumulated_header_len,
                   union nm_so_generic_ctrl_header *p_ctrl,
                   struct nm_so_pkt_wrap **pp_so_pw){

  struct nm_so_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_aggreg_gate *p_so_sa_gate
    = (struct nm_so_strat_aggreg_gate *)p_so_gate->strat_priv;
  uint32_t h_rlen;
  int err;

  if(!list_empty(&p_so_sa_gate->out_list)) {
    /* Inspect only the head of the list */
    p_so_pw = nm_l2so(p_so_sa_gate->out_list.next);

    h_rlen = nm_so_pw_remaining_header_area(p_so_pw);

    /* If the paquet is reasonably small, we can form an aggregate */
    if(cumulated_header_len <= h_rlen) {
      err = nm_so_pw_add_control(p_so_pw, p_ctrl);
      goto out;
    }
  }

  /* Otherwise, simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

 out:
  *pp_so_pw = p_so_pw;

  return err;
}

static int
pack_ctrl_chunk(struct nm_so_pkt_wrap *p_so_pw,
                union nm_so_generic_ctrl_header *p_ctrl){

  int err;

  err = nm_so_pw_add_control(p_so_pw, p_ctrl);

  return err;
}

static int
pack_extended_ctrl_end(struct nm_gate *p_gate,
                       struct nm_so_pkt_wrap *p_so_pw){

  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_aggreg_gate *p_so_sa_gate
    = (struct nm_so_strat_aggreg_gate *)p_so_gate->strat_priv;

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &p_so_sa_gate->out_list);

  return NM_ESUCCESS;
}

static int
try_to_agregate_small(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                      void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_aggreg_gate *p_so_sa_gate = (struct nm_so_strat_aggreg_gate *)p_so_gate->strat_priv;
  int flags = 0;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {
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

      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, chunk_offset, is_last_chunk, flags);

      nb_data_aggregation ++;
      goto out;

  next:
      ;
  }

  if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
    flags = NM_SO_DATA_USE_COPY;

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk, flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);
 out:
  return err;
}

static int
launch_large_chunk(struct nm_gate *p_gate,
                   uint8_t tag, uint8_t seq,
                   void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

  /* Large packets can not be sent immediately : we have to issue a RdV request. */

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk,
                                          NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(p_so_gate->pending_large_send[tag]));

  /* Signal we're waiting for an ACK */
  p_so_gate->pending_unpacks++;

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, chunk_offset, is_last_chunk);

    err = pack_ctrl(p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }

  /* Check if we should post a new recv packet: we're waiting for an
     ACK! */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

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
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  int err;

  p_so_gate->send[tag][seq] = len;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */
    err = try_to_agregate_small(p_gate, tag, seq, data, len, 0, 1);

  } else {
    /* Large packet */
    err = launch_large_chunk(p_gate, tag, seq, data, len, 0, 1);
  }

  return err;
}

static int
packv(struct nm_gate *p_gate,
      uint8_t tag, uint8_t seq,
      struct iovec *iov, int nb_entries){
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;

  uint32_t offset = 0;
  uint8_t last_chunk = 0;
  int i;

  p_so_gate->send[tag][seq] = 0;

  for(i = 0; i < nb_entries; i++){
    if(i == (nb_entries - 1)){
      last_chunk = 1;
    }

    if(iov[i].iov_len <= NM_SO_MAX_SMALL) {
      NM_SO_TRACE("PACK of a small one - tag = %u, seq = %u, len = %u, offset = %u, is_last_chunk = %u\n", tag, seq, iov[i].iov_len, offset, last_chunk);

      /* Small packet */
      try_to_agregate_small(p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);

    } else {
      NM_SO_TRACE("PACK of a large one - tag = %u, seq = %u, nb_entries = %u\n", tag, seq, nb_entries);

      /* Large packets are splited in 2 chunks. */
      launch_large_chunk(p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);
    }
    offset += iov[i].iov_len;
  }
  p_so_gate->send[tag][seq] = offset;

  return NM_ESUCCESS;
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
    &((struct nm_so_strat_aggreg_gate *)p_so_gate->strat_priv)->out_list;
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

/** Initialize the strategy module.
 *
 *  @return The NM status.
 */
static int init(void)
{
  NM_LOGF("[loading strategy: <aggreg>]");
  nb_data_aggregation = 0;
  nb_ctrl_aggregation = 0;
  return NM_ESUCCESS;
}

static int exit_strategy(void)
{
  NM_DISP_VAL("Aggregation data", nb_data_aggregation);
  NM_DISP_VAL("Aggregation control", nb_ctrl_aggregation);
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

  if(p_so_gate->active_recv[*drv_id][*trk_id] == 0)
    /* Cool! The suggested track is available! */
    return NM_ESUCCESS;
  else
    /* We decide to postpone the acknowledgement. */
    return -NM_EAGAIN;
}


/** Initialize the gate storage for aggreg strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int init_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_aggreg_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_aggreg_gate));

  INIT_LIST_HEAD(&priv->out_list);

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}

/** Cleanup the gate storage for aggreg strategy.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int exit_gate(struct nm_gate *p_gate)
{
  struct nm_so_strat_aggreg_gate *priv = ((struct nm_so_gate *)p_gate->sch_private)->strat_priv;
  TBX_FREE(priv);
  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = NULL;
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_aggreg = {
  .init = init,
  .exit = exit_strategy,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .pack = pack,
  .packv = packv,
  .pack_extended = NULL,
  .pack_ctrl = pack_ctrl,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .rdv_accept = rdv_accept,
  .pack_extended_ctrl = pack_extended_ctrl,
  .pack_ctrl_chunk = pack_ctrl_chunk,
  .pack_extended_ctrl_end = pack_extended_ctrl_end,
  .extended_rdv_accept = NULL,
#ifdef NMAD_QOS
  .ack_callback = NULL,
#endif /* NMAD_QOS */
  .priv = NULL,
  .flush = NULL,
};
