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
#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_tracks.h"
#include "nm_so_sendrecv_interface.h"
#include "nm_so_interfaces.h"
#include "nm_so_debug.h"

#define RAIL_MAX 8

static int post_multiple_data_recv(struct nm_gate *p_gate,
                                   uint8_t tag, uint8_t seq, void *data,
                                    int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens){
  uint32_t offset = 0;
  int i;

  for(i = 0; i < nb_drv; i++){
    nm_so_post_large_recv(p_gate, drv_ids[i], tag, seq, data + offset, chunk_lens[i]);
    offset += chunk_lens[i];
  }
  return NM_ESUCCESS;
}

static int post_multiple_pw_recv(struct nm_gate *p_gate,
                                 struct nm_so_pkt_wrap *p_so_pw,
                                  int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens){
  struct nm_so_pkt_wrap *p_so_pw2 = NULL;
  int i;

  for(i = 0; i < nb_drv; i++){
    nm_so_pw_split(p_so_pw, &p_so_pw2, chunk_lens[i]);

    nm_so_direct_post_large_recv(p_gate, drv_ids[i], p_so_pw);

    p_so_pw = p_so_pw2;
  }
  return NM_ESUCCESS;
}


static int init_multi_ack(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq, uint32_t chunk_offset,
                          int nb_drv, uint8_t *drv_ids, uint32_t *chunk_lens){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  union nm_so_generic_ctrl_header ctrl;
  struct nm_so_pkt_wrap *p_so_acks_pw = NULL;
  unsigned long trk_id = TRK_LARGE;
  int err, i;


  NM_SO_TRACE("Building of a multi-ack with %d chunks on tag %d and seq %d\n", nb_drv, tag, seq);

  nm_so_init_multi_ack(&ctrl, nb_drv, tag, seq, chunk_offset);
  err = cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * (nb_drv+1), &ctrl, &p_so_acks_pw);

  for(i = 0; i < nb_drv; i++){
    NM_SO_TRACE("NM_SO_PROTO_ACK_CHUNK - drv_id = %d, trk_id = %ld, chunk_len =%u\n", drv_ids[i], trk_id, chunk_lens[i]);
    nm_so_add_ack_chunk(&ctrl, drv_ids[i] * NM_SO_MAX_TRACKS + trk_id, chunk_lens[i]);
    err = cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl);
  }

  err = cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

  return err;
}

static int store_large_waiting_transfer(struct nm_gate *p_gate,
                                        uint8_t tag, uint8_t seq,
                                        void *data, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

  err = nm_so_pw_alloc_and_fill_with_data(tag, seq, data, len, chunk_offset, 0,
                                          NM_SO_DATA_DONT_USE_HEADER,
                                          &p_so_pw);
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
  unsigned long drv_id = NM_SO_DEFAULT_NET;
  unsigned long trk_id = TRK_LARGE;
  int err;

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
  unsigned long trk_id = TRK_LARGE;
  int nb_drv;
  uint8_t drv_ids[RAIL_MAX]; // ->il faut que 1 carte = 1 drv different
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
      err = post_multiple_data_recv(p_gate, tag + 128, seq, data, nb_drv, drv_ids, chunk_lens);
      err = init_multi_ack(p_gate, tag + 128, seq, chunk_offset, nb_drv, drv_ids, chunk_lens);
    }

  } else {
    NM_SO_TRACE("RDV_success - no available track for a large sent\n");

    /* The current strategy did'nt find any suitable track: we are
       forced to postpone the acknowledgement */
    err = store_large_waiting_transfer(p_gate, tag+128, seq, data, len, chunk_offset);
  }
  return err;
}

static int rdv_success(struct nm_gate *p_gate,
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


/* fragment d'un message à envoyer qui s'étend sur plusieurs entrées de l'iov de réception*/
static int large_on_several_entries(struct nm_gate *p_gate, uint8_t tag, uint8_t seq,
                                    struct iovec *iov, uint32_t len, uint32_t chunk_offset){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  struct nm_so_pkt_wrap *p_so_acks_pw = NULL;
  unsigned long drv_id = NM_SO_DEFAULT_NET;
  unsigned long trk_id = TRK_LARGE;
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
    union nm_so_generic_ctrl_header ctrl;

    /* Data are contiguous */
    cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

    nm_so_post_large_recv(p_gate, drv_id,
                          tag+128, seq,
                          iov[i].iov_base + (chunk_offset - offset),
                          len);

    /* Launch the ACK */
    nm_so_init_multi_ack(&ctrl, 1, tag+128, seq, chunk_offset);
    cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * 2, &ctrl, &p_so_acks_pw);

    nm_so_add_ack_chunk(&ctrl, drv_id * NM_SO_MAX_TRACKS + trk_id, len);
    cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl);

    cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

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

static int
_nm_so_copy_data_in_iov(struct iovec *iov, uint32_t chunk_offset, void *data, uint32_t len){
  uint32_t chunk_len, pending_len = len;
  int offset = 0;
  int i = 0;

  while(1){
    if(offset + iov[i].iov_len > chunk_offset)
      break;

    offset += iov[i].iov_len;
    i++;
  }

  if(offset + iov[i].iov_len >= chunk_offset + len){
    /* Data are contiguous */
    memcpy(iov[i].iov_base + (chunk_offset-offset), data, len);
    goto out;
  }

  /* Data are on several ioec entries */

  /* first entry: it can be a fragment of entry */
  chunk_len = iov[i].iov_len - (chunk_offset - offset);

  memcpy(iov[i].iov_base + (chunk_offset - offset), data, chunk_len);

  pending_len -= chunk_len;
  offset += iov[i].iov_len;
  i++;

  /* following entries: only full part */
  while(pending_len){

    if(offset + iov[i].iov_len >= chunk_offset + pending_len){
      /* it is the last line */
      memcpy(iov[i].iov_base, data+(len-pending_len), pending_len);
      goto out;
    }

    chunk_len = iov[i].iov_len;

    memcpy(iov[i].iov_base, data+(len-pending_len), chunk_len);

    pending_len -= chunk_len;
    offset += iov[i].iov_len;
    i++;
  }

 out:
  return NM_ESUCCESS;
}

static int
_nm_so_treat_chunk(tbx_bool_t is_any_src,
                   void *dest_buffer,
                   void *header,
                   struct nm_so_pkt_wrap *p_so_pw,
                   uint32_t *recovered_len, uint32_t *expected_len){
  struct nm_gate *p_gate = NULL;
  struct nm_so_gate *p_so_gate = NULL;
  struct nm_so_sched *p_so_sched = NULL;

  uint8_t proto_id;
  struct nm_so_ctrl_rdv_header *rdv = NULL;
  struct nm_so_data_header *h = NULL;

  uint8_t tag, seq, is_last_chunk;
  uint32_t len, chunk_offset;
  void *ptr = NULL;

  int err;

  p_gate = p_so_pw->pw.p_gate;
  p_so_gate = p_gate->sch_private;
  p_so_sched = p_so_gate->p_so_sched;

  proto_id = *(uint8_t *)header;

  if(proto_id == NM_SO_PROTO_RDV){
    rdv = header;
    tag = rdv->tag_id;
    seq = rdv->seq;
    len = rdv->len;
    chunk_offset = rdv->chunk_offset;
    is_last_chunk = rdv->is_last_chunk;

    NM_SO_TRACE("RDV recovered chunk : tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    if(is_any_src){
      p_so_gate->p_so_sched->any_src[tag-128].status |= NM_SO_STATUS_RDV_IN_PROGRESS;

      if(is_last_chunk){
        p_so_sched->any_src[tag-128].expected_len = chunk_offset+len;
      }

    } else {
      if(is_last_chunk){
        *expected_len = chunk_offset+len;
      }
    }

    if((is_any_src && p_so_sched->any_src[tag-128].status & NM_SO_STATUS_UNPACK_IOV)
       || (!is_any_src && p_so_gate->status[tag-128][seq] & NM_SO_STATUS_UNPACK_IOV)){
      err = large_on_several_entries(p_gate, tag-128, seq, dest_buffer, len, chunk_offset);

    } else {
      err = rdv_success(p_gate, tag - 128, seq, dest_buffer + chunk_offset, len, chunk_offset);
    }

    len = 0;

  } else {
    h = header;
    tag = h->proto_id;
    seq = h->seq;
    len = h->len;
    ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;

    chunk_offset = h->chunk_offset;
    is_last_chunk = h->is_last_chunk;

    NM_SO_TRACE("DATA recovered chunk: tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    if(is_last_chunk){
      if(is_any_src){
        p_so_sched->any_src[tag-128].expected_len = chunk_offset + len;

      } else {
        *expected_len = chunk_offset + len;
      }
    }

    if((is_any_src && p_so_sched->any_src[tag-128].status & NM_SO_STATUS_UNPACK_IOV)
       || (!is_any_src && p_so_gate->status[tag-128][seq] & NM_SO_STATUS_UNPACK_IOV)){

      _nm_so_copy_data_in_iov(dest_buffer, chunk_offset, ptr, len);

    } else {
      /* Copy data to its final destination */
      memcpy(dest_buffer + chunk_offset, ptr, len);
    }
  }


  /* Decrement the packet wrapper reference counter. If no other
     chunks are still in use, the pw will be destroyed. */
  nm_so_pw_dec_header_ref_count(p_so_pw);

  *recovered_len = len;

  return NM_ESUCCESS;
}

static int
_nm_so_data_chunk_recovery(tbx_bool_t is_any_src,
                           void *first_header,
                           struct nm_so_pkt_wrap *first_p_so_pw,
                           struct list_head *chunks,
                           void *data,
                           uint32_t *cumulated_len, uint32_t *expected_len){

  struct nm_so_chunk *chunk = NULL;
  uint32_t recovered_len = 0;

  _nm_so_treat_chunk(is_any_src,
                     data,
                     first_header, first_p_so_pw,
                     &recovered_len, expected_len);

  *cumulated_len += recovered_len;

  /* copy of all the received chunks */
  while(!list_empty(chunks)){

    chunk = nm_l2chunk(chunks->next);

    _nm_so_treat_chunk(is_any_src,
                       data,
                       chunk->header, chunk->p_so_pw,
                       &recovered_len, expected_len);

    *cumulated_len += recovered_len;

    // next
    list_del(chunks->next);

    tbx_free(nm_so_chunk_mem, chunk);
  }

  return NM_ESUCCESS;
}

static int
iov_len(struct iovec *iov, int nb_entries){
  uint32_t len = 0;
  int i;

  for(i = 0; i < nb_entries; i++){
    len += iov[i].iov_len;
  }

  return len;
}

/** Handle a source-less unpack.
 */
int
__nm_so_unpack_any_src(struct nm_core *p_core,
                       uint8_t tag, void *data, uint32_t len)
{

  struct nm_so_sched *p_so_sched = p_core->p_sched->sch_private;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  struct nm_gate *p_gate;
  struct nm_so_gate *p_so_gate;
  uint8_t *status;
  int seq, first, i, err = NM_ESUCCESS;

  NM_SO_TRACE("Unpack_ANY_src - tag = %u, len = %u\n", tag, len);

  if(p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
    TBX_FAILURE("Simultaneous any_src reception on the same tag");

  p_so_sched->any_src[tag].expected_len  = len;
  p_so_sched->any_src[tag].cumulated_len = 0;

  first = p_so_sched->next_gate_id;
  do {

    p_gate = p_core->gate_array + p_so_sched->next_gate_id;
    p_so_gate = p_gate->sch_private;

    seq = p_so_gate->recv_seq_number[tag];
    status = &p_so_gate->status[tag][seq];

    if(*status & NM_SO_STATUS_PACKET_HERE) {
      /* Wow! At least one data chunk already in! */
      uint32_t cumulated_len = 0;
      uint32_t expected_len = 0;

      NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);

      *status = 0;
      p_so_gate->recv_seq_number[tag]++;

      _nm_so_data_chunk_recovery(tbx_true,
                                 p_so_gate->recv[tag][seq].pkt_here.header,
                                 p_so_gate->recv[tag][seq].pkt_here.p_so_pw,
                                 &p_so_gate->recv[tag][seq].pkt_here.chunks,
                                 data,
                                 &cumulated_len,
                                 &expected_len);

      p_so_sched->any_src[tag].cumulated_len = cumulated_len;

      p_so_gate->recv[tag][seq].pkt_here.header = NULL;
      p_so_gate->recv[tag][seq].pkt_here.p_so_pw  = NULL;

      /* Verify if the communication is done */
      if(p_so_sched->any_src[tag].cumulated_len >=  p_so_sched->any_src[tag].expected_len){
        NM_SO_TRACE("Wow! All data chunks were already in!\n");

        p_so_gate->recv_seq_number[tag]++;

        p_so_sched->any_src[tag].status = 0;
        p_so_sched->any_src[tag].gate_id = -1;

        p_so_sched->pending_any_src_unpacks--;
        interface->unpack_success(p_gate, tag, seq, tbx_true);

        goto out;
      }

      p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
      p_so_sched->any_src[tag].data = data;
      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;

      p_so_sched->pending_any_src_unpacks++;

      goto out;
    }

    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");

  p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
  p_so_sched->any_src[tag].data = data;
  p_so_sched->any_src[tag].gate_id = -1;

  p_so_sched->pending_any_src_unpacks++;

  /* Make sure that each gate has a posted receive */
  for(i = 0; i < p_core->nb_gates; i++)
    nm_so_refill_regular_recv(&p_core->gate_array[i]);

 out:
  return err;
}

/** Handle a source-less unpack.
 */
int
__nm_so_unpackv_any_src(struct nm_core *p_core, uint8_t tag, struct iovec *iov, int nb_entries)
{

  struct nm_so_sched *p_so_sched = p_core->p_sched->sch_private;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  struct nm_gate *p_gate;
  struct nm_so_gate *p_so_gate;
  uint8_t *status;
  int seq, first, i, err = NM_ESUCCESS;

  NM_SO_TRACE("Unpackv_ANY_src - tag = %u, nb_entries = %u\n", tag, nb_entries);

  if(p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
    TBX_FAILURE("Simultaneous any_src reception on the same tag");

  first = p_so_sched->next_gate_id;
  do {

    p_gate = p_core->gate_array + p_so_sched->next_gate_id;
    p_so_gate = p_gate->sch_private;

    seq = p_so_gate->recv_seq_number[tag];
    status = &p_so_gate->status[tag][seq];

    if(*status & NM_SO_STATUS_PACKET_HERE) {
      /* Wow! At least one data chunk already in! */
      uint32_t cumulated_len = 0;
      uint32_t expected_len = 0;

      NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);

      *status = 0;
      p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_IOV;

      p_so_gate->recv_seq_number[tag]++;

      _nm_so_data_chunk_recovery(tbx_true,
                                 p_so_gate->recv[tag][seq].pkt_here.header,
                                 p_so_gate->recv[tag][seq].pkt_here.p_so_pw,
                                 &p_so_gate->recv[tag][seq].pkt_here.chunks,
                                 iov,
                                 &cumulated_len, &expected_len);

      p_so_sched->any_src[tag].cumulated_len = cumulated_len;
      p_so_sched->any_src[tag].expected_len  = expected_len;

      /* Verify if the communication is done */
      if(p_so_sched->any_src[tag].cumulated_len >=  p_so_sched->any_src[tag].expected_len){
        NM_SO_TRACE("Wow! All data chunks were already in!\n");

        p_so_gate->recv_seq_number[tag]++;

        p_so_sched->any_src[tag].status = 0;
        p_so_sched->any_src[tag].gate_id = -1;

        p_so_sched->pending_any_src_unpacks--;
        interface->unpack_success(p_gate, tag, seq, tbx_true);

        goto out;
      }

      p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
      p_so_sched->any_src[tag].data = iov;
      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;

      p_so_sched->pending_any_src_unpacks++;

      goto out;
    }

    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");

  p_so_sched->any_src[tag].status = 0;
  p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_HERE;
  p_so_sched->any_src[tag].status |= NM_SO_STATUS_UNPACK_IOV;

  p_so_sched->any_src[tag].data = iov;
  p_so_sched->any_src[tag].gate_id = -1;

  p_so_sched->any_src[tag].expected_len  = iov_len(iov, nb_entries);
  p_so_sched->any_src[tag].cumulated_len = 0;

  p_so_sched->pending_any_src_unpacks++;

  /* Make sure that each gate has a posted receive */
  for(i = 0; i < p_core->nb_gates; i++)
    nm_so_refill_regular_recv(&p_core->gate_array[i]);

 out:
  return err;
}


/** Handle a sourceful unpack.
 */
int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  uint32_t cumulated_len = 0;
  int err;

  NM_SO_TRACE("Unpack - gate_id = %u, tag = %u, seq = %u, len = %u\n", p_gate->id, tag, seq, len);

  if(*status & NM_SO_STATUS_PACKET_HERE) {
    /* Wow! At least one chunk already in! */
    NM_SO_TRACE("At least one data chunk already in!\n");

    if(len){

      if(len <= NM_SO_MAX_SMALL){
        struct nm_so_data_header *dh = p_so_gate->recv[tag][seq].pkt_here.header;
        struct nm_so_pkt_wrap *p_so_pw = p_so_gate->recv[tag][seq].pkt_here.p_so_pw;
        void *src = NULL;

        /* This is the only one chunk of this message
           -> not necessary to check if it is the last chunk*/
        p_so_gate->recv[tag][seq].unpack_here.expected_len = dh->chunk_offset + dh->len;
        p_so_gate->recv[tag][seq].unpack_here.cumulated_len = dh->len;

        /* Retrieve data location */
        src =  (void *)dh;
        src += NM_SO_DATA_HEADER_SIZE + dh->skip;

        // As short data are not split, there up to 1 pending data block
        memcpy(data, src, dh->len);

        /* Decrement the packet wrapper reference counter. If no other
           chunks are still in use, the pw will be destroyed. */
        nm_so_pw_dec_header_ref_count(p_so_pw);

      } else {
        void *first_header = p_so_gate->recv[tag][seq].pkt_here.header;
        struct nm_so_pkt_wrap *first_p_so_pw = p_so_gate->recv[tag][seq].pkt_here.p_so_pw;
        struct list_head *chunks = &p_so_gate->recv[tag][seq].pkt_here.chunks;
        uint32_t expected_len = 0;

        _nm_so_data_chunk_recovery(tbx_false,
                                   first_header, first_p_so_pw, chunks,
                                   data, &cumulated_len, &expected_len);

        p_so_gate->recv[tag][seq].unpack_here.cumulated_len = cumulated_len;

        p_so_gate->recv[tag][seq].unpack_here.expected_len  = expected_len;
        if(p_so_gate->recv[tag][seq].unpack_here.expected_len == 0)
          p_so_gate->recv[tag][seq].unpack_here.expected_len = len;
      }
    }

    /* Verify if the communication is done */
    if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len == p_so_gate->recv[tag][seq].unpack_here.expected_len){
      NM_SO_TRACE("Wow! All data chunks were already in!\n");

      *status = 0;
      interface->unpack_success(p_gate, tag, seq, tbx_false);

      goto out;
    }

  } else {
    p_so_gate->recv[tag][seq].unpack_here.expected_len = len;
    p_so_gate->recv[tag][seq].unpack_here.cumulated_len = 0;
  }


  *status = 0;
  *status = NM_SO_STATUS_UNPACK_HERE;

  p_so_gate->recv[tag][seq].unpack_here.data = data;

  p_so_gate->pending_unpacks++;

  /* Check if we should post a new recv packet */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

int
__nm_so_unpackv(struct nm_gate *p_gate,
                uint8_t tag, uint8_t seq,
                struct iovec *iov, int nb_entries)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  uint32_t cumulated_len = 0;
  int err;

  NM_SO_TRACE("Unpackv - gate_id = %u, tag = %u, seq = %u, nb_entries = %u\n", p_gate->id, tag, seq, nb_entries);

  if(*status & NM_SO_STATUS_PACKET_HERE) {
    /* Wow! At least one chunk already in! */
    NM_SO_TRACE("At least one data chunk already in!\n");

    void *first_header = p_so_gate->recv[tag][seq].pkt_here.header;
    struct nm_so_pkt_wrap *first_p_so_pw = p_so_gate->recv[tag][seq].pkt_here.p_so_pw;
    struct list_head *chunks = &p_so_gate->recv[tag][seq].pkt_here.chunks;
    uint32_t expected_len = 0;

    *status |= NM_SO_STATUS_UNPACK_IOV;

    _nm_so_data_chunk_recovery(tbx_false,
                               first_header, first_p_so_pw, chunks,
                               iov, &cumulated_len, &expected_len);

    p_so_gate->recv[tag][seq].pkt_here.header = NULL;
    p_so_gate->recv[tag][seq].pkt_here.p_so_pw = NULL;
    list_del(&p_so_gate->recv[tag][seq].pkt_here.chunks);

    p_so_gate->recv[tag][seq].unpack_here.expected_len = expected_len;
    if(p_so_gate->recv[tag][seq].unpack_here.expected_len == 0){
      p_so_gate->recv[tag][seq].unpack_here.expected_len = iov_len(iov, nb_entries);
    }
    p_so_gate->recv[tag][seq].unpack_here.cumulated_len = cumulated_len;


    /* Verify if the communication is done */
    if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len == p_so_gate->recv[tag][seq].unpack_here.expected_len){
      NM_SO_TRACE("Wow! All data chunks were already in!\n");

      *status = 0;
      interface->unpack_success(p_gate, tag, seq, tbx_false);

      goto out;
    }

  } else {
    p_so_gate->recv[tag][seq].unpack_here.expected_len = iov_len(iov, nb_entries);
    p_so_gate->recv[tag][seq].unpack_here.cumulated_len = 0;
  }

  *status = 0;
  *status |= NM_SO_STATUS_UNPACK_HERE;
  *status |= NM_SO_STATUS_UNPACK_IOV;

  p_so_gate->recv[tag][seq].unpack_here.data = iov;

  p_so_gate->pending_unpacks++;

  /* Check if we should post a new recv packet */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

/** Schedule and post new incoming buffers.
 */
int
nm_so_in_schedule(struct nm_sched *p_sched TBX_UNUSED)
{
  return NM_ESUCCESS;
}

/** Process a complete data request.
 */
static int data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
				    void *ptr,
                                    void *header, uint32_t len,
				    uint8_t proto_id, uint8_t seq,
                                    uint32_t chunk_offset,
                                    uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  uint8_t tag = proto_id - 128;
  uint8_t *status = &(p_so_gate->status[tag][seq]);

  NM_SO_TRACE("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u, is_last_chunk = %u\n", ptr, len, tag, seq, chunk_offset, is_last_chunk);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    /* Cool! We already have a waiting unpack for this packet */

    /* Save the real length to receive */
    if(is_last_chunk){
      p_so_gate->recv[tag][seq].unpack_here.expected_len = chunk_offset + len;
    }

    if(len){
      /* Copy data to its final destination */

      if(p_so_gate->status[tag][seq] & NM_SO_STATUS_UNPACK_IOV){
        /* Destination is organized in several non contiguous buffers */
        _nm_so_copy_data_in_iov(p_so_gate->recv[tag][seq].unpack_here.data, chunk_offset, ptr, len);

      } else {
        /* Data are contiguous */
        memcpy(p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset, ptr, len);
      }
    }

    p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;

    /* Verify if the communication is done */
    if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len >= p_so_gate->recv[tag][seq].unpack_here.expected_len){
      NM_SO_TRACE("Wow! All data chunks were already in!\n");
      p_so_gate->pending_unpacks--;

      /* Reset the status matrix*/
      p_so_gate->recv[tag][seq].pkt_here.header = NULL;

      *status = 0;
      interface->unpack_success(p_gate, tag, seq, tbx_false);

    }
    return NM_SO_HEADER_MARK_READ;


  } else if ((p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
             &&
             ((p_so_sched->any_src[tag].gate_id == -1 && p_so_gate->recv_seq_number[tag] == seq)
              || (p_so_sched->any_src[tag].gate_id == p_gate->id && p_so_sched->any_src[tag].seq == seq))) {

    NM_SO_TRACE("Pending any_src reception\n");

    if(is_last_chunk){
      p_so_sched->any_src[tag].expected_len = chunk_offset + len;
    }

    if(p_so_sched->any_src[tag].gate_id == -1){
      p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;
    }

    /* Copy data to its final destination */
    if(p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_IOV){
      /* Destination is organized in several non contiguous buffers */
      _nm_so_copy_data_in_iov(p_so_sched->any_src[tag].data, chunk_offset, ptr, len);

    } else {
      /* Data are contiguous */
      memcpy(p_so_sched->any_src[tag].data + chunk_offset, ptr, len);
    }

    p_so_sched->any_src[tag].cumulated_len += len;

    /* Verify if the communication is done */
    if(p_so_sched->any_src[tag].cumulated_len >= p_so_sched->any_src[tag].expected_len){
      NM_SO_TRACE("Wow! All data chunks in!\n");

      p_so_sched->any_src[tag].status = 0;
      p_so_sched->any_src[tag].gate_id = -1;

      p_so_sched->pending_any_src_unpacks--;

      interface->unpack_success(p_gate, tag, seq, tbx_true);
    }
    return NM_SO_HEADER_MARK_READ;

  } else {

    /* Receiver process is not ready, so store the information in the
       recv array and keep the p_so_pw packet alive */
    NM_SO_TRACE("Store the data chunk with tag = %u, seq = %u\n", tag, seq);

    if(!(*status & NM_SO_STATUS_PACKET_HERE)){

      p_so_gate->recv[tag][seq].pkt_here.header = header;
      p_so_gate->recv[tag][seq].pkt_here.p_so_pw = p_so_pw;

      INIT_LIST_HEAD(&p_so_gate->recv[tag][seq].pkt_here.chunks);

      *status |= NM_SO_STATUS_PACKET_HERE;

    } else {
      struct nm_so_chunk *chunk = tbx_malloc(nm_so_chunk_mem);
      if (!chunk) {
	TBX_FAILURE("chunk allocation - err = -NM_ENOMEM");
      }

      chunk->header = header;
      chunk->p_so_pw = p_so_pw;

      list_add_tail(&chunk->link, &p_so_gate->recv[tag][seq].pkt_here.chunks);
    }

    return NM_SO_HEADER_MARK_UNREAD;
  }
}

/** Process a complete rendez-vous request.
 */
static int rdv_callback(struct nm_so_pkt_wrap *p_so_pw,
                        void *rdv,
                        uint8_t tag_id, uint8_t seq,
                        uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  uint8_t tag = tag_id - 128;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  NM_SO_TRACE("RDV completed for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    NM_SO_TRACE("RDV for a classic exchange on tag %u, seq = %u, len = %u, chunk_offset = %u, is_last_chunk = %u\n", tag, seq, len, chunk_offset, is_last_chunk);

    if(is_last_chunk){
      p_so_gate->recv[tag][seq].unpack_here.expected_len = chunk_offset + len;
    }

    /* Application is already ready! */
    if(*status & NM_SO_STATUS_UNPACK_IOV){
      err = large_on_several_entries(p_gate, tag, seq, (struct iovec *)p_so_gate->recv[tag][seq].unpack_here.data, len, chunk_offset);

    } else {
      err = rdv_success(p_gate, tag, seq,
                        p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset,
                        len, chunk_offset);
    }

    if(err != NM_ESUCCESS)
      TBX_FAILURE("PANIC!\n");



  } else if ((p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
             &&
             ((p_so_sched->any_src[tag].gate_id == -1 && p_so_gate->recv_seq_number[tag] == seq)
              || (p_so_sched->any_src[tag].gate_id == p_gate->id && p_so_sched->any_src[tag].seq == seq))) {
    NM_SO_TRACE("RDV for an ANY_SRC exchange for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

    if(is_last_chunk){
      p_so_sched->any_src[tag].expected_len = chunk_offset + len;
    }

    /* Application is already ready! */
    if(p_so_sched->any_src[tag].gate_id == -1){
      p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;
    }

    p_so_sched->any_src[tag].status |= NM_SO_STATUS_RDV_IN_PROGRESS;

    if(p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_IOV){
      err = large_on_several_entries(p_gate, tag, seq, (struct iovec *)p_so_sched->any_src[tag].data, len, chunk_offset);

    } else {
      err = rdv_success(p_gate, tag, seq,
                        p_so_sched->any_src[tag].data + chunk_offset,
                        len, chunk_offset);
    }

    if(err != NM_ESUCCESS)
      TBX_FAILURE("PANIC!\n");

  } else {
    /* Store rdv request */
    NM_SO_TRACE("Store the RDV for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

    if(! p_so_gate->status[tag][seq] & NM_SO_STATUS_PACKET_HERE){
      p_so_gate->recv[tag][seq].pkt_here.header = rdv;
      p_so_gate->recv[tag][seq].pkt_here.p_so_pw = p_so_pw;

      INIT_LIST_HEAD(&p_so_gate->recv[tag][seq].pkt_here.chunks);

    } else {
      struct nm_so_chunk *chunk = tbx_malloc(nm_so_chunk_mem);
      if (!chunk) {
        TBX_FAILURE("chunk allocation - err = -NM_ENOMEM");
      }

      chunk->header = rdv;
      chunk->p_so_pw = p_so_pw;

      list_add_tail(&chunk->link, &p_so_gate->recv[tag][seq].pkt_here.chunks);
    }

    p_so_gate->status[tag][seq] |= NM_SO_STATUS_RDV_HERE;

    p_so_gate->status[tag][seq] |= NM_SO_STATUS_PACKET_HERE;

    return NM_SO_HEADER_MARK_UNREAD;
  }

  return NM_SO_HEADER_MARK_READ;
}

/** Process a complete rendez-vous acknowledgement request.
 */
static int ack_callback(struct nm_so_pkt_wrap *p_so_pw,
                        uint8_t tag_id, uint8_t seq,
                        uint8_t track_id, uint32_t chunk_offset)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_large_pw = NULL;
  uint8_t tag = tag_id - 128;

  NM_SO_TRACE("ACK completed for tag = %d, seq = %u, offset = %u\n", tag, seq, chunk_offset);

  p_so_gate->pending_unpacks--;
  p_so_gate->status[tag][seq] |= NM_SO_STATUS_ACK_HERE;

  list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link) {

    if(p_so_large_pw->pw.seq == seq
       && p_so_large_pw->chunk_offset == chunk_offset) {

      list_del(&p_so_large_pw->link);

      /* Send the data */
      _nm_so_post_send(p_gate, p_so_large_pw,
		       track_id % NM_SO_MAX_TRACKS,
		       track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }

  }
  TBX_FAILURE("PANIC!\n");
}


static int ack_chunk_callback(struct nm_so_pkt_wrap *p_so_pw,
                              uint8_t tag_id, uint8_t seq, uint32_t chunk_offset,
                              uint8_t track_id, uint32_t chunk_len){

  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_pkt_wrap *p_so_large_pw, *p_so_large_pw2;
  uint8_t tag = tag_id - 128;

  NM_SO_TRACE("ACK_CHUNK completed for tag = %d, track_id = %u, seq = %u, chunk_len = %u, chunk_offset = %u\n", tag, track_id, seq, chunk_len, chunk_offset);

  list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link) {

    if(p_so_large_pw->pw.seq == seq && p_so_large_pw->chunk_offset == chunk_offset) {

      list_del(&p_so_large_pw->link);

      if(chunk_len < p_so_large_pw->pw.length){
        NM_SO_TRACE("ack_chunk_callback - split pw\n");

        nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, chunk_len);

        list_add(&p_so_large_pw2->link, &p_so_gate->pending_large_send[tag]);
      } else {
        p_so_gate->pending_unpacks--;
      }

      /* Send the data */
      _nm_so_post_send(p_gate, p_so_large_pw,
                       track_id % NM_SO_MAX_TRACKS,
		       track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }
  }

  TBX_FAILURE("PANIC!\n");
}

/** Process a complete successful incoming request.
 */
int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw)
{
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  int err;

  NM_SO_TRACE("Packet %p received completely (on track %d, gate %d)!\n", p_so_pw, p_pw->p_trk->id, (int)p_gate->id);

  if(p_pw->p_trk->id == TRK_SMALL) {
    /* Track 0 */
    p_so_gate->active_recv[p_pw->p_drv->id][TRK_SMALL] = 0;

    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_completion_callback,
				  rdv_callback,
				  ack_callback,
                                  ack_chunk_callback);

    if(p_so_gate->pending_unpacks ||
       p_so_gate->p_so_sched->pending_any_src_unpacks) {

      NM_LOG_VAL("pending_unpacks", p_so_gate->pending_unpacks);
      NM_LOG_VAL("pending_any_src_unpacks", p_so_gate->p_so_sched->pending_any_src_unpacks);

      /* Check if we should post a new recv packet */
      nm_so_refill_regular_recv(p_gate);
    }
  } else if(p_pw->p_trk->id == TRK_LARGE) {

    /* This is the completion of a large message. */
    int drv_id = p_pw->p_drv->id;
    unsigned tag = p_so_pw->pw.proto_id - 128;
    int seq = p_so_pw->pw.seq;
    int len = p_pw->length;
    struct nm_so_interface_ops *interface= p_so_sched->current_interface;
    nm_so_strategy *cur_strat = p_so_sched->current_strategy;
    int nb_drivers = p_gate->p_sched->p_core->nb_drivers;

    if(nb_drivers == 1){

       if(p_so_gate->p_so_sched->any_src[tag].status & NM_SO_STATUS_RDV_IN_PROGRESS) {
         NM_SO_TRACE("Completion of a large any_src message with tag %d et seq %d\n", tag, seq);

         p_so_sched->any_src[tag].cumulated_len += len;

         if(p_so_sched->any_src[tag].cumulated_len >= p_so_sched->any_src[tag].expected_len){
           /* Completion of an ANY_SRC unpack */
           p_so_sched->any_src[tag].status = 0;
           p_so_sched->any_src[tag].gate_id = -1;

           p_so_sched->pending_any_src_unpacks--;
           interface->unpack_success(p_gate, tag, seq, tbx_true);
         }

       } else {
         NM_SO_TRACE("****Completion of a large message on tag %d, seq %d and len = %d\n", tag, seq, len);

         p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;

         /* Verify if the communication is done */
         if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len >= p_so_gate->recv[tag][seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_gate->recv[tag][seq].pkt_here.header = NULL;
          p_so_gate->status[tag][seq] = 0;

          interface->unpack_success(p_gate, tag, seq, tbx_false);

        }
       }

    } else {
      if((p_so_sched->any_src[tag].status & NM_SO_STATUS_RDV_IN_PROGRESS)
         &&
         (p_so_sched->any_src[tag].gate_id == p_gate->id)
         &&
         (p_so_sched->any_src[tag].seq == seq)) {

        NM_SO_TRACE("Completion of a large ANY_SRC message with tag %d et seq %d\n", tag, seq);

        p_so_sched->any_src[tag].cumulated_len += len;

        if(p_so_sched->any_src[tag].cumulated_len >= p_so_sched->any_src[tag].expected_len){
          /* Completion of an ANY_SRC unpack */
          p_so_sched->any_src[tag].status = 0;
          p_so_sched->any_src[tag].gate_id = -1;

          p_so_sched->pending_any_src_unpacks--;
          interface->unpack_success(p_gate, tag, seq, tbx_true);
        }

      } else {
        NM_SO_TRACE("Completion of a large message - tag %d, seq %d\n", tag, seq);

        p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;

        /* Verify if the communication is done */
        if(p_so_gate->recv[tag][seq].unpack_here.cumulated_len >= p_so_gate->recv[tag][seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_gate->recv[tag][seq].pkt_here.header = NULL;
          p_so_gate->status[tag][seq] = 0;

          interface->unpack_success(p_gate, tag, seq, tbx_false);
        }
      }
    }

    p_so_gate->active_recv[drv_id][TRK_LARGE] = 0;

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);

    /*--------------------------------------------*/
    /* Check if some recv requests were postponed */
    if(!list_empty(&p_so_gate->pending_large_recv)) {
      union nm_so_generic_ctrl_header ctrl;
      unsigned long trk_id = TRK_LARGE;

      struct nm_so_pkt_wrap *p_so_large_pw = nm_l2so(p_so_gate->pending_large_recv.next);

      if(p_so_gate->status[p_so_large_pw->pw.proto_id-128][p_so_large_pw->pw.seq]
         & NM_SO_STATUS_UNPACK_IOV){
        struct nm_so_pkt_wrap *p_so_acks_pw = NULL;

        /* Post next iov entry on driver drv_id */
        list_del(p_so_gate->pending_large_recv.next);

        if(p_so_large_pw->v[0].iov_len >= p_so_large_pw->pw.length){
          /* It stays only one entry in the iovec */
          nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

          /* Launch the ACK */
          nm_so_init_multi_ack(&ctrl, 1, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq, p_so_large_pw->chunk_offset);
          cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * 2, &ctrl, &p_so_acks_pw);

          nm_so_add_ack_chunk(&ctrl, drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->v[0].iov_len);
          cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl);

          cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

        } else {

          struct nm_so_pkt_wrap *p_so_acks_pw = NULL;
          struct nm_so_pkt_wrap *p_so_large_pw2 = NULL;

          /* Only post the first entry */
          nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, p_so_large_pw->v[0].iov_len);

          nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);


          /* Launch the ACK */
          nm_so_init_multi_ack(&ctrl, 1, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq, p_so_large_pw->chunk_offset);
          cur_strat->pack_extended_ctrl(p_gate, NM_SO_CTRL_HEADER_SIZE * 2, &ctrl, &p_so_acks_pw);

          nm_so_add_ack_chunk(&ctrl, drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->v[0].iov_len);
          cur_strat->pack_ctrl_chunk(p_so_acks_pw, &ctrl);

          cur_strat->pack_extended_ctrl_end(p_gate, p_so_acks_pw);

          list_add_tail(&p_so_large_pw2->link, &p_so_gate->pending_large_recv);
        }

      } else {
        list_del(p_so_gate->pending_large_recv.next);

        if(nb_drivers == 1){ // we do not search to have more NICs
          nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

          nm_so_init_ack(&ctrl, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                         drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
          err = cur_strat->pack_ctrl(p_gate, &ctrl);


        } else {
          int nb_drv;
          uint8_t drv_ids[RAIL_MAX]; // ->il faut que 1 carte = 1 drv different
          uint32_t chunk_lens[RAIL_MAX];

          /* We ask the current strategy to find other available tracks for
             transfering this large data chunk. */
          err = cur_strat->extended_rdv_accept(p_gate, p_so_large_pw->pw.length,
                                               &nb_drv, drv_ids, chunk_lens);

          if(err == NM_ESUCCESS){
            if(nb_drv == 1){ // le réseau qui vient de se libérer est le seul disponible
              nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

              nm_so_init_ack(&ctrl, p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                             drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
              err = cur_strat->pack_ctrl(p_gate, &ctrl);

            } else {

              err = post_multiple_pw_recv(p_gate, p_so_large_pw, nb_drv, drv_ids, chunk_lens);
              err = init_multi_ack(p_gate,
                                   p_so_large_pw->pw.proto_id, p_so_large_pw->pw.seq,
                                   p_so_large_pw->chunk_offset,
                                   nb_drv, drv_ids, chunk_lens);
            }
          }
        }
      }
    }
  }

  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;

  return err;
}


/** Process a failed incoming request.
 */
int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		_err TBX_UNUSED) {
  TBX_FAILURE("nm_so_in_process_failed_rq");
  return nm_so_in_process_success_rq(p_sched, p_pw);
}
