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

#include <nm_predictions.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_split_balance.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_tracks.h"
#include "nm_so_parameters.h"
#include "nm_log.h"

#define DATATYPE_DENSITY 2*1024

struct nm_so_strat_split_balance_gate {
  /* list of raw outgoing packets */
  struct list_head out_list;
  unsigned nb_packets;
};

static int
pack_extended_ctrl(struct nm_gate *p_gate,
                   uint32_t cumulated_header_len,
                   union nm_so_generic_ctrl_header *p_ctrl,
                   struct nm_so_pkt_wrap **pp_so_pw){

  struct nm_so_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;
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
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &p_so_sa_gate->out_list);
  p_so_sa_gate->nb_packets++;

  return NM_ESUCCESS;
}

/* Add a new control "header" to the flow of outgoing packets */
static int
pack_ctrl(struct nm_gate *p_gate,
          union nm_so_generic_ctrl_header *p_ctrl){
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;
  int err;

  if(!list_empty(&p_so_sa_gate->out_list)) {
    /* Inspect only the head of the list */
    p_so_pw = nm_l2so(p_so_sa_gate->out_list.next);

    /* If the paquet is reasonably small, we can form an aggregate */
    if(NM_SO_CTRL_HEADER_SIZE <= nm_so_pw_remaining_header_area(p_so_pw)){

      struct nm_so_pkt_wrap TBX_UNUSED dummy_p_so_pw;
      FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET, &dummy_p_so_pw, 0, 0, 0);
      FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, &dummy_p_so_pw);
      FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG, p_gate->id, 0, 0, &dummy_p_so_pw, p_so_pw);

      err = nm_so_pw_add_control(p_so_pw, p_ctrl);
      goto out;
    }
  }

  /* Otherwise, simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* TODO tag should be filled correctly */
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET, p_so_pw, 0, 0, p_so_pw->pw.length);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 0, p_so_pw);

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &p_so_sa_gate->out_list);
  p_so_sa_gate->nb_packets++;

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

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                          data, len, chunk_offset, is_last_chunk,
                                          NM_SO_DATA_DONT_USE_HEADER,
                                          &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;


  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 1, p_so_pw);

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(p_so_gate->pending_large_send[tag]));

  /* Signal we're waiting for an ACK */
  p_so_gate->pending_unpacks++;

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, chunk_offset, is_last_chunk);

    NM_SO_TRACE("RDV pack_ctrl\n");
    err = pack_ctrl(p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }

  /* Check if we should post a new recv packet: we're waiting for an ACK! */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;
 out:
  return err;
}

static int
try_to_agregate_small(struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;
  int flags = 0;
  int err;

  /* We aggregate ONLY if data are very small OR if there are
     already two ready packets */
  if(len <= 512 || p_so_sa_gate->nb_packets >= 2) {

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
      
      struct nm_so_pkt_wrap TBX_UNUSED dummy_p_so_pw;
      FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, &dummy_p_so_pw, tag, seq, len);
      FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, &dummy_p_so_pw);
      FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG, p_gate->id, 0, 0, &dummy_p_so_pw, p_so_pw);

      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, chunk_offset, is_last_chunk, flags);
      goto out;

    next:
      ;
    }
  }

  flags = NM_SO_DATA_USE_COPY;

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk, flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;


  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 0, p_so_pw);

  list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);
  p_so_sa_gate->nb_packets++;

  err = NM_ESUCCESS;
 out:
  return err;
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int
pack(struct nm_gate *p_gate,
     uint8_t tag, uint8_t seq,
     void *data, uint32_t len){
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  int err;

  p_so_gate->send[tag][seq] = len;

  if(len <= NM_SO_MAX_SMALL) {
    NM_SO_TRACE("PACK of a small one - tag = %u, seq = %u, len = %u\n", tag, seq, len);

    /* Small packet */
    err = try_to_agregate_small(p_gate, tag, seq, data, len, 0, 1);

  } else {
    NM_SO_TRACE("PACK of a large one - tag = %u, seq = %u, len = %u\n", tag, seq, len);

    /* Large packets are splited in 2 chunks. */
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
      NM_SO_TRACE("PACK of a small iov entry - tag = %u, seq = %u, len = %u, offset = %u, is_last_chunk = %u\n", tag, seq, iov[i].iov_len, offset, last_chunk);
      /* Small packet */
      try_to_agregate_small(p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);

    } else {
      NM_SO_TRACE("PACK of a large iov entry - tag = %u, seq = %u, nb_entries = %u\n", tag, seq, nb_entries);
      /* Large packets are splited in 2 chunks. */
      launch_large_chunk(p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);
    }
    offset += iov[i].iov_len;
  }
  p_so_gate->send[tag][seq] = offset;

  return NM_ESUCCESS;
}

static int
agregate_datatype(struct nm_gate *p_gate,
                  uint8_t tag, uint8_t seq,
                  uint32_t len, struct DLOOP_Segment *segp){

  struct nm_so_pkt_wrap *p_so_pw;
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;
  int err;

  // Look for a wrapper to fullfill
  if(len <= 512 || p_so_sa_gate->nb_packets >= 2) {

    /* We first try to find an existing packet to form an aggregate */
    list_for_each_entry(p_so_pw, &p_so_sa_gate->out_list, link) {
      uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
      uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

      if(size > h_rlen)
        /* There's not enough room to add our data to this paquet */
        goto next;

      // add the datatype. Actually, we add the header and copy the data just after
      err = nm_so_pw_add_datatype(p_so_pw, tag + 128, seq, len, segp);

      goto out;

    next:
      ;
    }
  }
  // We don't find any free wrapper so we build a new one
  int flags = NM_SO_DATA_USE_COPY;

  err = nm_so_pw_alloc(flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_datatype(p_so_pw, tag + 128, seq, len, segp);

  list_add_tail(&p_so_pw->link, &p_so_sa_gate->out_list);
  p_so_sa_gate->nb_packets++;

  err = NM_ESUCCESS;
 out:
  return err;
}

static int
launch_large_datatype(struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      uint32_t len, struct DLOOP_Segment *segp){
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  nm_so_pw_store_datatype(p_so_pw, tag + 128, seq, len, segp);

  p_so_gate->status[tag][seq] |= NM_SO_STATUS_IS_DATATYPE;

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(p_so_gate->pending_large_send[tag]));

  /* Signal we're waiting for an ACK */
  p_so_gate->pending_unpacks++;

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

    NM_SO_TRACE("RDV pack_ctrl\n");
    err = pack_ctrl(p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }

  /* Check if we should post a new recv packet: we're waiting for an ACK! */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;
 out:
  return err;
}



// Si c'est un petit, on copie systématiquement (pour le moment)
// Si c'est un long, on passe par un iov qu'on va recharger au nieavu du driver
// s'il n'y a pas assez d'entrées. Les données sont (pour l'instant) systématiquement reçus en contigu
static int
pack_datatype(struct nm_gate *p_gate,
              uint8_t tag, uint8_t seq,
              struct DLOOP_Segment *segp){
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;

  DLOOP_Handle handle;
  int data_sz;

  handle = segp->handle;
  CCSI_datadesc_get_size_macro(handle, data_sz); // * count?
#warning a revoir dans ccs
  //data_sz *= segp->handle.ref_count;

  p_so_gate->send[tag][seq] = data_sz;

  NM_SO_TRACE("Send a datatype on gate %d and tag %d with length %d\n", p_gate->id, tag, data_sz);

  if(data_sz <= NM_SO_MAX_SMALL) {
    NM_SO_TRACE("Short datatype : try to aggregate it\n");
    agregate_datatype(p_gate, tag, seq, data_sz, segp);

  } else {
    NM_SO_TRACE("Large datatype : send a rdv\n");
    launch_large_datatype(p_gate, tag, seq, data_sz, segp);
  }

  p_so_gate->status[tag][seq] = NM_SO_STATUS_IS_DATATYPE;

  return NM_ESUCCESS;
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int
try_and_commit(struct nm_gate *p_gate){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_split_balance_gate *p_so_sa_gate
    = (struct nm_so_strat_split_balance_gate *)p_so_gate->strat_priv;
  struct list_head *out_list = &p_so_sa_gate->out_list;
  struct nm_so_pkt_wrap *p_so_pw;
  int nb_drivers = p_gate->p_sched->p_core->nb_drivers;
  uint8_t *drv_ids = NULL;
  int drv_id, n = 0;

 start:
  if(list_empty(out_list))
    /* We're done */
    goto out;

  if(nb_drivers == 1){
    drv_id = 0;
    if (p_so_gate->active_send[drv_id][TRK_SMALL] +
	p_so_gate->active_send[drv_id][TRK_LARGE] == 0)
      /* We found an idle NIC */
      goto next;
  }

  nm_ns_inc_lats(p_gate->p_core, &drv_ids);
  while(n < nb_drivers){
    if (p_so_gate->active_send[drv_ids[n]][TRK_SMALL] +
        p_so_gate->active_send[drv_ids[n]][TRK_LARGE] == 0){
      /* We found an idle NIC */
      drv_id = drv_ids[n];
      n++;
      goto next;
    }
    n++;
  }

  /* We didn't found any idle NIC, so we're done*/
  goto out;

 next:
  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);
  p_so_sa_gate->nb_packets--;

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  /* Post packet on track 0 */
  _nm_so_post_send(p_gate, p_so_pw, TRK_SMALL, drv_id);

  if(nb_drivers == 1)
    goto out;

  /* Loop and see if we can feed another NIC with a ready paquet */
  goto start;

 out:
    return NM_ESUCCESS;
}

/* Initialization */
static int
init(void){
  NM_SO_TRACE("[loading strategy: <split_balance>]\n");
  return NM_ESUCCESS;
}

/* Warning: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
static int
rdv_accept(struct nm_gate *p_gate,
           uint8_t *drv_id,
           uint8_t *trk_id){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int nb_drivers = p_gate->p_sched->p_core->nb_drivers;
  int n;

  /* Is there any large data track available? */
  for(n = 0; n < nb_drivers; n++) {

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


//#warning ajouter le n° de la track a utiliser pour chaque driver
static int
extended_rdv_accept(struct nm_gate *p_gate,
                    uint32_t len_to_send,
                    int * nb_drv,
                    uint8_t *drv_ids,
                    uint32_t *chunk_lens){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int nb_drivers = p_gate->p_core->nb_drivers;
  uint8_t *ordered_drv_id_by_bw = NULL;
  int cur_drv_idx = 0;
  uint8_t trk_id = TRK_LARGE;
  int err;
  int i;

  nm_ns_dec_bws(p_gate->p_core, &ordered_drv_id_by_bw);

  for(i = 0; i < nb_drivers; i++){
    if(p_so_gate->active_recv[ordered_drv_id_by_bw[i]][trk_id] == 0) {
      drv_ids[cur_drv_idx] = i;
      cur_drv_idx++;
    }
  }

  *nb_drv = cur_drv_idx;

  if(cur_drv_idx == 0){
    /* We're forced to postpone the acknowledgement. */
    err = -NM_EAGAIN;
  } else if(cur_drv_idx == 1){
    err = NM_ESUCCESS;
  } else {
    int final_nb_drv = 0;

    nm_ns_multiple_split_ratio(len_to_send,
                               p_gate->p_core,
                               cur_drv_idx, drv_ids,
                               chunk_lens,
                               &final_nb_drv);

    *nb_drv = final_nb_drv;

    err = NM_ESUCCESS;
  }

  return err;
}

static int
init_gate(struct nm_gate *p_gate){
  struct nm_so_strat_split_balance_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_split_balance_gate));

  /* we will fake some input list ... */
  FUT_DO_PROBE2(FUT_NMAD_GATE_NEW_INPUT_LIST, 0 /* index */, p_gate->id);

  INIT_LIST_HEAD(&priv->out_list);
  FUT_DO_PROBE2(FUT_NMAD_GATE_NEW_OUTPUT_LIST, 0 /* index */, p_gate->id);
  FUT_DO_PROBE2(FUT_NMAD_GATE_NEW_OUTPUT_LIST, 1 /* index */, p_gate->id);

  priv->nb_packets = 0;

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}

static int
exit_gate(struct nm_gate *p_gate){
  struct nm_so_strat_split_balance_gate *priv = ((struct nm_so_gate *)p_gate->sch_private)->strat_priv;
  TBX_FREE(priv);
  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = NULL;
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_split_balance = {
  .init = init,
  .exit = NULL,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .pack = pack,
  .packv = packv,
  .pack_datatype = pack_datatype,
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
  .extended_rdv_accept = extended_rdv_accept,
  .priv = NULL,
};
