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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>
#include <ccs_public.h>
#include <segment.h>

#include "nm_so_process_large_transfer.h"

#include "nm_so_private.h"
#include "nm_so_tracks.h"

int nm_so_build_multi_ack(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq, uint32_t chunk_offset,
                          int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  union nm_so_generic_ctrl_header ctrl;
  struct nm_so_pkt_wrap *p_so_acks_pw = NULL;
  uint8_t trk_id = TRK_LARGE;
  int err, i;

  NM_SO_TRACE("Building of a multi-ack with %d chunks on tag %d and seq %d\n", nb_drv, tag, seq);

  nm_so_init_multi_ack(&ctrl, nb_drv, tag, seq, chunk_offset);
  err = cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * (nb_drv+1), &ctrl, &p_so_acks_pw);

  for(i = 0; i < nb_drv; i++){
    NM_SO_TRACE("NM_SO_PROTO_ACK_CHUNK - drv_id = %d, trk_id = %d, chunk_len =%u\n", drv_ids[i], trk_id, chunk_lens[i]);
    nm_so_add_ack_chunk(&ctrl, drv_ids[i] * NM_SO_MAX_TRACKS + trk_id, chunk_lens[i]);
    err = cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl);
  }

  err = cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

  return err;
}

/***************************************/

static int init_large_iov_recv(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                               struct iovec *iov, uint32_t len, uint32_t chunk_offset);
static int init_large_datatype_recv(tbx_bool_t is_any_src,
                                    struct nm_gate *p_gate,
                                    uint8_t tag, uint8_t seq,
                                    uint32_t len, uint32_t chunk_offset);
static int init_large_contiguous_recv(struct nm_gate *p_gate,
                                      uint8_t tag, uint8_t seq,
                                      void *data, uint32_t len,
                                      uint32_t chunk_offset);

int nm_so_rdv_success(tbx_bool_t is_any_src,
                      struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      uint32_t len,
                      uint32_t chunk_offset,
                      uint8_t is_last_chunk){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  int err;

  uint8_t *status = NULL;
  if(is_any_src){
    status = &(p_so_sched->any_src[tag].status);
  } else {
    status = &(p_so_gate->status[tag][seq]);
  }

  // Update the real size to receive
  if(is_last_chunk){
    if(is_any_src){
      p_so_sched->any_src[tag].expected_len = chunk_offset + len;
    } else {
      p_so_gate->recv[tag][seq].unpack_here.expected_len = chunk_offset + len;
    }
  }

  if(is_any_src){
    p_so_sched->any_src[tag].status |= NM_SO_STATUS_RDV_IN_PROGRESS;
  }

  if(is_any_src && p_so_sched->any_src[tag].gate_id == -1){
    p_so_gate->recv_seq_number[tag]++;

    p_so_sched->any_src[tag].gate_id = p_gate->id;
    p_so_sched->any_src[tag].seq = seq;
  }

  // 1) The final destination of the data is desribed by an iov
  if(*status & NM_SO_STATUS_UNPACK_IOV){
    struct iovec *iov = NULL;

    if(is_any_src){
      iov = p_so_sched->any_src[tag].data;
    } else {
      iov = p_so_gate->recv[tag][seq].unpack_here.data;
    }

    err = init_large_iov_recv(p_gate, tag, seq, iov, len, chunk_offset);


  // 2) The final destination of the data is desribed by a datatype
  } else if(*status & NM_SO_STATUS_IS_DATATYPE){
    err = init_large_datatype_recv(is_any_src, p_gate, tag, seq, len, chunk_offset);

  // 3) The final destination of the data is a contiguous buffer
  } else {
    void *data = NULL;
    NM_SO_TRACE("RDV_sucess - contiguous data with tag %d , seq %d, chunk_offset %d, any_src %d\n",
                tag, seq, chunk_offset, is_any_src);

    if(is_any_src){
      data = p_so_sched->any_src[tag].data + chunk_offset;
    } else {
      data = p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset;
    }

    err = init_large_contiguous_recv(p_gate, tag, seq, data, len, chunk_offset);
  }

  return err;
}






/*********************************************************************/
/***** Functions which ask for an available NIC      *****************/
/****  and send the data if there is at least one    *****************/
/****  or store them if not                          *****************/
/*********************************************************************/

//************ IOV ************/
static int store_iov_waiting_transfer(struct nm_gate *p_gate,
                                      uint8_t tag, uint8_t seq, uint32_t chunk_offset,
                                      struct iovec *iov, int nb_entries, uint32_t pending_len,
                                      uint32_t first_iov_offset);

/* fragment d'un message à envoyer qui s'étend sur plusieurs entrées de l'iov de réception*/
static int init_large_iov_recv(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                               struct iovec *iov, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  struct nm_so_pkt_wrap *p_so_acks_pw = NULL;
  uint8_t drv_id = NM_SO_DEFAULT_NET;
  uint8_t trk_id = TRK_LARGE;
  uint32_t offset = 0;
  int i = 0;

  /* Go to the right iovec entry */
  while(1){
    if(offset + iov[i].iov_len > chunk_offset)
      break;
    offset += iov[i].iov_len;
    i++;
  }

  if(offset + iov[i].iov_len >= chunk_offset + len){
    /* Data are contiguous */
    cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

    nm_so_post_large_recv(p_gate, drv_id,
                          tag+128, seq,
                          iov[i].iov_base + (chunk_offset - offset),
                          len);

    /* Launch the ACK */
    nm_so_build_multi_ack(p_gate, tag+128, seq, chunk_offset, 1, &drv_id, &len);

    goto out;
  }

  /* Data are on several entries of the iovec */
  union nm_so_generic_ctrl_header ctrl[NM_SO_PREALLOC_IOV_LEN];
  uint32_t chunk_len, pending_len = len;
  int nb_entries = 1;
  uint32_t cur_len = 0;
  int err;

#warning TODO:multirail
  chunk_len = iov[i].iov_len + (chunk_offset - offset);

  /* We ask the current strategy to find an available track for transfering this large data chunk.*/
  err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

  if(err != NM_ESUCCESS){
    /* No free track */
    err = store_iov_waiting_transfer(p_gate,
                                     tag + 128, seq, chunk_offset, // characteristic data
                                     &iov[i], nb_entries, pending_len,
                                     iov[i].iov_len + (chunk_offset - offset));
    goto out;
  }

  nm_so_post_large_recv(p_gate, drv_id,
                        tag+128, seq,
                        iov[i].iov_base + (chunk_offset - offset),
                        chunk_len);

  nm_so_add_ack_chunk(&ctrl[nb_entries], drv_id * NM_SO_MAX_TRACKS + trk_id, chunk_len);

  nb_entries++;
  pending_len -= chunk_len;
  offset += iov[i].iov_len;
  cur_len += chunk_len;
  i++;

  while(pending_len){

    /* We ask the current strategy to find an available track
       for transfering this large data chunk.*/
    err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

    if (err != NM_ESUCCESS){
      /* No free track */
      err = store_iov_waiting_transfer(p_gate,
                                       tag + 128, seq, chunk_offset+cur_len, // characteristic data
                                       &iov[i], nb_entries, pending_len, 0);
      goto ack_to_send;
    }

    if(offset + iov[i].iov_len >= chunk_offset + pending_len){
      nm_so_post_large_recv(p_gate, drv_id,
                            tag+128, seq,
                            iov[i].iov_base,
                            pending_len);

      nm_so_add_ack_chunk(&ctrl[nb_entries], drv_id * NM_SO_MAX_TRACKS + trk_id, pending_len);
      break;

    } else {
      chunk_len = iov[i].iov_len;

      nm_so_post_large_recv(p_gate, drv_id,
                            tag+128, seq,
                            iov[i].iov_base,
                            chunk_len);

      nm_so_add_ack_chunk(&ctrl[nb_entries], drv_id * NM_SO_MAX_TRACKS + trk_id, chunk_len);

      nb_entries++;
      pending_len -= chunk_len;
      offset += iov[i].iov_len;
      cur_len += chunk_len;
      i++;
    }
  }

 ack_to_send:
  /* Launch the acks */
  nm_so_init_multi_ack(&ctrl[0], nb_entries-1, tag+128, seq, chunk_offset);
  cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * nb_entries, &ctrl[0], &p_so_acks_pw);

  for(i = 1; i < nb_entries; i++){
    cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl[i]);
  }

  cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

 out:
  return NM_ESUCCESS;
}



//************ DATATYPE ************/
static int init_large_datatype_recv_to_tmpbuf(tbx_bool_t is_any_src,
                                              struct nm_gate *p_gate,
                                              uint8_t tag, uint8_t seq, uint32_t len,
                                              uint32_t chunk_offset,
                                              struct DLOOP_Segment *segp);
static int initiate_large_datatype_recv_with_multi_ack(struct nm_gate *p_gate,
                                                       uint8_t tag, uint8_t seq,
                                                       uint32_t first, uint32_t len,
                                                       uint32_t chunk_offset,
                                                       struct DLOOP_Segment *segp);
static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 uint8_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp);



static int init_large_datatype_recv(tbx_bool_t is_any_src,
                                    struct nm_gate *p_gate,
                                    uint8_t tag, uint8_t seq,
                                    uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct DLOOP_Segment *segp = NULL;
  int density = 0;
  int nb_blocks = 0;
  int last = len;

  NM_SO_TRACE("RDV reception for data described by a datatype\n");

  if(is_any_src){
    struct nm_so_sched *p_so_sched = p_gate->p_sched->sch_private;
    segp = p_so_sched->any_src[tag].data;

  } else {
    segp = p_so_gate->recv[tag][seq].unpack_here.data;
  }

  CCSI_Segment_count_contig_blocks(segp, 0, &last, &nb_blocks);
  density = len / nb_blocks; // taille moyenne des blocs
  if(density <= DATATYPE_DENSITY){
    NM_SO_TRACE("Reception in an intermediate buffer because there are lot of small blocks\n");
    init_large_datatype_recv_to_tmpbuf(is_any_src, p_gate, tag, seq, len, chunk_offset, segp);

  } else {
    NM_SO_TRACE("Reception of the data directly in their final location - use a multi-ack\n");
    initiate_large_datatype_recv_with_multi_ack(p_gate, tag, seq, 0, len, chunk_offset, segp);
  }
  return NM_ESUCCESS;
}


static int init_large_datatype_recv_to_tmpbuf(tbx_bool_t is_any_src,
                                              struct nm_gate *p_gate,
                                              uint8_t tag, uint8_t seq, uint32_t len,
                                              uint32_t chunk_offset,
                                              struct DLOOP_Segment *segp){

  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  uint8_t drv_id = NM_SO_DEFAULT_NET;
  uint8_t trk_id = TRK_LARGE;
  int err;

  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
#warning Multirail
  err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

  if(err == NM_ESUCCESS) {
    /* The strategy found an available transfer track */

    union nm_so_generic_ctrl_header ctrl;

    NM_SO_TRACE("Post the reception of data described by a datatype through a intermediate buffer\n");
    nm_so_post_large_datatype_recv_to_tmpbuf(is_any_src, p_gate, drv_id, tag+128, seq, len, segp);

    nm_so_init_ack(&ctrl, tag+128, seq, drv_id * NM_SO_MAX_TRACKS + trk_id, 0);
    err = cur_strat->pack_ctrl(p_gate, &ctrl);

  } else {
    /* No free track: postpone the ack */
    err = store_large_datatype_waiting_transfer(p_gate, tag + 128, seq, len, chunk_offset, segp);
  }

  return err;
}


int init_large_datatype_recv_with_multi_ack(struct nm_so_pkt_wrap *p_so_pw){
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  uint8_t drv_id = NM_SO_DEFAULT_NET;
  uint8_t trk_id = TRK_LARGE;

  uint8_t tag = p_so_pw->pw.proto_id;
  uint8_t seq = p_so_pw->pw.seq;
  uint32_t len = p_so_pw->pw.length;
  struct DLOOP_Segment *segp = p_so_pw->pw.segp;
  uint32_t first = p_so_pw->pw.datatype_offset;
  uint32_t last = 0;

  int err;

#warning Multi-rail?
  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
  err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

  if(err == NM_ESUCCESS) {
    int nb_entries = 1;
    last = len;

    CCSI_Segment_pack_vector(segp,
                             first, (DLOOP_Offset *)&last,
                             (DLOOP_VECTOR *)p_so_pw->pw.v,
                             &nb_entries);
    assert(nb_entries == 1);

    p_so_pw->pw.length = last - first;
    p_so_pw->pw.v_nb = nb_entries;

    // depot de la reception
    nm_so_direct_post_large_recv(p_gate, drv_id, p_so_pw);

    // init le multi_ack
    NM_SO_TRACE("Multi-ack building for a block with tag %d, length %do, chunk_offset %d\n",
                tag, last - first, first);
    nm_so_build_multi_ack(p_gate, tag, seq, first, 1, &drv_id, &p_so_pw->pw.length);

    if(last < len){
      goto store;
    } else {
      goto out;
    }

  store:
    //stockage du reste du segment
    nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);

    p_so_pw->pw.p_gate   = p_gate;
    p_so_pw->pw.proto_id = tag;
    p_so_pw->pw.seq      = seq;
    p_so_pw->pw.length   = len;
    p_so_pw->pw.v_nb     = 0;
    p_so_pw->pw.segp     = segp;
    p_so_pw->pw.datatype_offset += last - first;
    p_so_pw->chunk_offset = p_so_pw->pw.datatype_offset;
  }

  /* No free track: postpone the ack */
  list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);

 out:
  return err;
}

static int
initiate_large_datatype_recv_with_multi_ack(struct nm_gate *p_gate,
                                            uint8_t tag, uint8_t seq,
                                            uint32_t first, uint32_t len,
                                            uint32_t chunk_offset,
                                            struct DLOOP_Segment *segp){
  struct nm_so_pkt_wrap *p_so_pw =  NULL;

  nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);

  p_so_pw->pw.p_gate   = p_gate;
  p_so_pw->pw.proto_id = tag + 128;
  p_so_pw->pw.seq = seq;
  p_so_pw->pw.length = len;
  p_so_pw->pw.v_nb     = 0;
  p_so_pw->pw.segp     = segp;
  p_so_pw->pw.datatype_offset = 0;

  p_so_pw->chunk_offset = chunk_offset;

  return init_large_datatype_recv_with_multi_ack(p_so_pw);
}



//************ CONTIGUOUS ************/

static int single_rdv(struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      void *data, uint32_t len, uint32_t chunk_offset);
static int multiple_rdv(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                        void *data, uint32_t len, uint32_t chunk_offset);
static int store_large_waiting_transfer(struct nm_gate *p_gate,
                                        uint8_t tag, uint8_t seq,
                                        void *data, uint32_t len, uint32_t chunk_offset);

static int init_large_contiguous_recv(struct nm_gate *p_gate,
                                      uint8_t tag, uint8_t seq,
                                      void *data, uint32_t len,
                                      uint32_t chunk_offset){
  int nb_drivers = p_gate->p_sched->p_core->nb_drivers;
  int err;

  if (nb_drivers == 1){
    err = single_rdv(p_gate, tag, seq, data, len, chunk_offset);

  } else {
    err = multiple_rdv(p_gate, tag, seq, data, len, chunk_offset);
  }
  return err;
}

/** Handle a matching rendez-vous.

 rdv_success is called when a RdV request and an UNPACK operation
   match together. Basically, we just have to check if there is a
   track available for the large data transfer and to send an ACK if
   so. Otherwise, we simply postpone the acknowledgement.*/
static int single_rdv(struct nm_gate *p_gate,
                      uint8_t tag, uint8_t seq,
                      void *data, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  uint8_t drv_id = NM_SO_DEFAULT_NET;
  uint8_t trk_id = TRK_LARGE;
  int err;

  NM_SO_TRACE("-->single_rdv with tag %d, seq %d, len %d, chunk_offset %d\n", tag, seq, len, chunk_offset);

  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
  err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

  if(err == NM_ESUCCESS) {
    union nm_so_generic_ctrl_header ctrl;
    /* The strategy found an available transfer track */
    nm_so_post_large_recv(p_gate, drv_id, tag+128, seq, data, len);

    nm_so_init_ack(&ctrl, tag+128, seq, drv_id * NM_SO_MAX_TRACKS + trk_id, chunk_offset);
    err = cur_strat->pack_ctrl(p_gate, &ctrl);

  } else {
    /* No free track: postpone the ack */
    err = store_large_waiting_transfer(p_gate,
                                       tag + 128, seq, data, len, chunk_offset);
  }
  return err;
}

static int multiple_rdv(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                        void *data, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  uint8_t trk_id = TRK_LARGE;
  int nb_drv;
  uint8_t drv_ids[RAIL_MAX];
  uint32_t chunk_lens[RAIL_MAX];
  int err;

  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
  err = cur_strat->extended_rdv_accept(p_gate, len, &nb_drv, drv_ids, chunk_lens);

  if(err == NM_ESUCCESS){
    if(nb_drv == 1){
      union nm_so_generic_ctrl_header ctrl;

      nm_so_post_large_recv(p_gate, drv_ids[0], tag+128, seq, data, len);
      nm_so_init_ack(&ctrl, tag+128, seq, drv_ids[0] * NM_SO_MAX_TRACKS + trk_id, chunk_offset);
      err = cur_strat->pack_ctrl(p_gate, &ctrl);

    } else {
      err = nm_so_post_multiple_data_recv(p_gate, tag + 128, seq, data, nb_drv, drv_ids, chunk_lens);
      err = nm_so_build_multi_ack(p_gate, tag + 128, seq, chunk_offset, nb_drv, drv_ids, chunk_lens);
    }

  } else {
    NM_SO_TRACE("RDV_success - no available track for a large sent\n");

    /* The current strategy did'nt find any suitable track: we are
       forced to postpone the acknowledgement */
    err = store_large_waiting_transfer(p_gate, tag+128, seq, data, len, chunk_offset);
  }
  return err;
}


/*****************************************************************************************/
/******* Functions which allow to store the data while there is not any available NIC ****/
/*****************************************************************************************/
static int store_large_waiting_transfer(struct nm_gate *p_gate,
                                        uint8_t tag, uint8_t seq,
                                        void *data, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

  err = nm_so_pw_alloc_and_fill_with_data(tag, seq, data, len, chunk_offset, 0,
                                          NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);

 out:
  return err;
}

static int store_iov_waiting_transfer(struct nm_gate *p_gate,
                                      uint8_t tag, uint8_t seq, uint32_t chunk_offset,
                                      struct iovec *iov, int nb_entries, uint32_t pending_len,
                                      uint32_t first_iov_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  uint32_t cur_len = pending_len;
  int err, i;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  p_so_pw->pw.p_gate   = p_gate;
  p_so_pw->pw.proto_id = tag;
  p_so_pw->pw.seq      = seq;
  p_so_pw->pw.length   = pending_len;
  p_so_pw->pw.v_nb     = nb_entries;

  p_so_pw->chunk_offset = chunk_offset;

  p_so_pw->v[0].iov_len  = iov[0].iov_len - first_iov_offset;
  p_so_pw->v[0].iov_base = iov[0].iov_base + first_iov_offset;
  cur_len -= p_so_pw->v[0].iov_len;

  for(i = 1; i < nb_entries-1; i++){
    p_so_pw->v[i].iov_len  = iov[i].iov_len;
    p_so_pw->v[i].iov_base = iov[i].iov_base;
    cur_len -= p_so_pw->v[i].iov_len;
  }

  p_so_pw->v[i].iov_len  = cur_len;
  p_so_pw->v[i].iov_base = iov[i].iov_base;

  list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);

  err = NM_ESUCCESS;
 out:
  return err;
}

static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 uint8_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;

  nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);

  p_so_pw->pw.p_gate   = p_gate;
  p_so_pw->pw.proto_id = tag;
  p_so_pw->pw.seq      = seq;
  p_so_pw->pw.length   = len;
  p_so_pw->pw.v_nb     = 0;
  p_so_pw->pw.segp     = segp;
  p_so_pw->chunk_offset = chunk_offset;

  list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);

  return NM_ESUCCESS;
}

