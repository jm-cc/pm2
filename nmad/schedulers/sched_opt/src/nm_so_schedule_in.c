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

  err = nm_so_pw_alloc_and_fill_with_data(tag, seq, data, len, chunk_offset,
                                          NM_SO_DATA_DONT_USE_HEADER,
                                          &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);

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
    /* The strategy found an available transfer track, so let's
       acknowledge the Rendez-Vous request! */

    nm_so_post_large_recv(p_gate, drv_id, tag+128, seq, data, len);
    nm_so_init_ack(&ctrl, tag+128, seq, drv_id * NM_SO_MAX_TRACKS + trk_id, chunk_offset);
    err = cur_strat->pack_ctrl(p_gate, &ctrl);

  } else {
    /* The current strategy did'nt find any suitable track: we are
       forced to postpone the acknowledgement */

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

  if(nb_drivers == 1){
    err = single_rdv(p_gate, tag, seq, data, len, chunk_offset);

  } else {
    err = multiple_rdv(p_gate, tag, seq, data, len, chunk_offset);
  }
  return err;
}


static int
_mn_so_treat_chunk(uint32_t total_len,
                   void *dest_buffer,
                   void *header, struct nm_so_pkt_wrap *p_so_pw,
                   int *recovered_len){
  struct nm_gate *p_gate = NULL;
  struct nm_so_gate *p_so_gate = NULL;

  uint8_t proto_id;
  struct nm_so_ctrl_rdv_header *rdv = NULL;
  struct nm_so_data_header *h = NULL;

  uint8_t tag, seq;
  uint32_t len, chunk_offset;
  void *ptr = NULL;

  int err;

  p_gate = p_so_pw->pw.p_gate;
  p_so_gate = p_gate->sch_private;

  proto_id = *(uint8_t *)header;

  if(proto_id == NM_SO_PROTO_RDV){
    rdv = header;
    tag = rdv->tag_id;
    seq = rdv->seq;
    len = rdv->len;
    chunk_offset = rdv->chunk_offset;

    NM_SO_TRACE("RDV recovered chunk : tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    p_so_gate->p_so_sched->any_src[tag-128].status |= NM_SO_STATUS_RDV_IN_PROGRESS;

    err = rdv_success(p_gate, tag - 128, seq, dest_buffer + chunk_offset,
                      len, chunk_offset);

    len = 0;

  } else {
    h = header;
    tag = h->proto_id;
    seq = h->seq;
    len = h->len;
    ptr = h + NM_SO_DATA_HEADER_SIZE + h->skip;
    chunk_offset = h->chunk_offset;

    NM_SO_TRACE("DATA recovered chunk: tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    /* Copy data to its final destination */
    if(chunk_offset + len <= total_len){
      memcpy(dest_buffer + chunk_offset, ptr, len);

    } else { /* if it is the last chunk, do not copy the alignment bytes */
      len = total_len - chunk_offset;

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
_nm_so_data_chunk_recovery(uint32_t total_len,
                           void *first_header,
                           struct nm_so_pkt_wrap *first_p_so_pw,
                           struct list_head *chunks,
                           void *data,
                           uint32_t *cumulated_len){

  struct nm_so_chunk *chunk = NULL;
  int recovered_len = 0;

  _mn_so_treat_chunk(total_len,
                     data,
                     first_header, first_p_so_pw,
                     &recovered_len);

  *cumulated_len += recovered_len;

  /* copy of all the received chunks */
  while(!list_empty(chunks)){

    chunk = nm_l2chunk(chunks->next);

    _mn_so_treat_chunk(total_len,
                       data,
                       chunk->header, chunk->p_so_pw,
                       &recovered_len);

    *cumulated_len += recovered_len;

    // next
    list_del(chunks->next);

    tbx_free(nm_so_chunk_mem, chunk);
  }

  return NM_ESUCCESS;
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

  first = p_so_sched->next_gate_id;

  do {

    p_gate = p_core->gate_array + p_so_sched->next_gate_id;
    p_so_gate = p_gate->sch_private;

    seq = p_so_gate->recv_seq_number[tag];
    status = &p_so_gate->status[tag][seq];

    if(*status & NM_SO_STATUS_PACKET_HERE) {
      /* Wow! At least one data chunk already in! */
      uint32_t cumulated_len = 0;

      NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);

      *status = 0;
      p_so_gate->recv_seq_number[tag]++;

      _nm_so_data_chunk_recovery(len,
                                 p_so_gate->recv[tag][seq].pkt_here.header,
                                 p_so_gate->recv[tag][seq].pkt_here.p_so_pw,
                                 &p_so_gate->recv[tag][seq].pkt_here.chunks,
                                 data,
                                 &cumulated_len);

      p_so_gate->recv[tag][seq].pkt_here.header = NULL;
      p_so_gate->recv[tag][seq].pkt_here.p_so_pw = NULL;

      len -= cumulated_len;


      if(len == 0){
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
      p_so_sched->any_src[tag].len  = len;
      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;

      p_so_sched->pending_any_src_unpacks++;

      goto out;
    }

    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");

  p_so_sched->any_src[tag].status = NM_SO_STATUS_UNPACK_HERE;
  p_so_sched->any_src[tag].data = data;
  p_so_sched->any_src[tag].len = len;
  p_so_sched->any_src[tag].gate_id = -1;

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
        void * ptr = NULL;
        void *src = NULL;
        unsigned long skip = dh->skip;

        /* Retrieve data location */
        ptr = dh;
        ptr += NM_SO_DATA_HEADER_SIZE;

        src = ptr;

        if(dh->len) {
          uint32_t rlen;
          struct iovec *v = p_so_pw->pw.v;

          //** espace restant sur la ligne  **/
          rlen = (v->iov_base + v->iov_len) - src;
          if (skip < rlen)
            src += skip;
          else {
            do {
              skip -= rlen;
              v++;
              rlen = v->iov_len;
            } while (skip >= rlen);
            src = v->iov_base + skip;
          }
        }

        // As short data are not split, there up to 1 pending data block
        memcpy(data, src, len);

        /* Decrement the packet wrapper reference counter. If no other
           chunks are still in use, the pw will be destroyed. */
        nm_so_pw_dec_header_ref_count(p_so_gate->recv[tag][seq].pkt_here.p_so_pw);

        assert(len == dh->len);

        len = 0;

      } else {
        _nm_so_data_chunk_recovery(len,
                                   p_so_gate->recv[tag][seq].pkt_here.header,
                                   p_so_gate->recv[tag][seq].pkt_here.p_so_pw,
                                   &p_so_gate->recv[tag][seq].pkt_here.chunks,
                                   data,
                                   &cumulated_len);

        len -= cumulated_len;

      }
    }

    p_so_gate->recv[tag][seq].pkt_here.header = NULL;
    p_so_gate->recv[tag][seq].pkt_here.p_so_pw = NULL;

    /* if all the chunks were already received */
    if(len == 0){
      NM_SO_TRACE("Wow! All data chunks were already in!\n");

      *status = 0;
      interface->unpack_success(p_gate, tag, seq, tbx_false);

      goto out;
    }
  }

  *status = 0;
  *status = NM_SO_STATUS_UNPACK_HERE;

  p_so_gate->recv[tag][seq].unpack_here.data = data;
  p_so_gate->recv[tag][seq].unpack_here.len = len;

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
nm_so_in_schedule(struct nm_sched *p_sched)
{
  return NM_ESUCCESS;
}

/** Process a complete data request.
 */
static int data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
				    void *ptr,
                                    void *header, uint32_t len,
				    uint8_t proto_id, uint8_t seq,
                                    uint32_t chunk_offset)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  uint8_t tag = proto_id - 128;
  uint8_t *status = &(p_so_gate->status[tag][seq]);

  NM_SO_TRACE("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u\n", ptr, len, tag, seq, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    /* Cool! We already have a waiting unpack for this packet */

    if(len)
      /* Copy data to its final destination */
      memcpy(p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset, ptr, len);

    p_so_gate->recv[tag][seq].unpack_here.len -= len;

    if(p_so_gate->recv[tag][seq].unpack_here.len <= 0){
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

    if(p_so_sched->any_src[tag].gate_id == -1){
      p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;
    }

    /* Copy data to its final destination */
    memcpy(p_so_sched->any_src[tag].data + chunk_offset, ptr, len);

    p_so_sched->any_src[tag].len -= len;

    if(p_so_sched->any_src[tag].len <= 0){
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

    if(! p_so_gate->status[tag][seq] & NM_SO_STATUS_PACKET_HERE){

      p_so_gate->recv[tag][seq].pkt_here.header = header;
      p_so_gate->recv[tag][seq].pkt_here.p_so_pw = p_so_pw;

      INIT_LIST_HEAD(&p_so_gate->recv[tag][seq].pkt_here.chunks);

    } else {

      struct nm_so_chunk *chunk = tbx_malloc(nm_so_chunk_mem);
      if (!chunk) {
	TBX_FAILURE("chunk allocation - err = -NM_ENOMEM");
      }

      chunk->header = header;
      chunk->p_so_pw = p_so_pw;

      list_add_tail(&chunk->link, &p_so_gate->recv[tag][seq].pkt_here.chunks);
    }

    *status |= NM_SO_STATUS_PACKET_HERE;

    return NM_SO_HEADER_MARK_UNREAD;
  }
}

/** Process a complete rendez-vous request.
 */
static int rdv_callback(struct nm_so_pkt_wrap *p_so_pw,
                        void *rdv,
                        uint8_t tag_id, uint8_t seq,
                        uint32_t len, uint32_t chunk_offset)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  uint8_t tag = tag_id - 128;
  uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  NM_SO_TRACE("RDV completed for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    NM_SO_TRACE("RDV for a classic exchange on tag %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    //*** Ajout du merge : est-ce bon dans le cas du split?
    //***if (len < p_so_gate->recv[tag][seq].unpack_here.len) {
    //***  p_so_gate->recv[tag][seq].unpack_here.len = len;
    //***}

    /* Application is already ready! */
    err = rdv_success(p_gate, tag, seq,
		      p_so_gate->recv[tag][seq].unpack_here.data + chunk_offset,
		      len, chunk_offset);
    if(err != NM_ESUCCESS)
      TBX_FAILURE("PANIC!\n");

  } else if ((p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_HERE)
             &&
             ((p_so_sched->any_src[tag].gate_id == -1 && p_so_gate->recv_seq_number[tag] == seq)
              || (p_so_sched->any_src[tag].gate_id == p_gate->id && p_so_sched->any_src[tag].seq == seq))) {
    NM_SO_TRACE("RDV for an ANY_SRC exchange for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

    /* Application is already ready! */
    if(p_so_sched->any_src[tag].gate_id == -1){
      p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].gate_id = p_gate->id;
      p_so_sched->any_src[tag].seq = seq;
    }

    p_so_sched->any_src[tag].status |= NM_SO_STATUS_RDV_IN_PROGRESS;

    err = rdv_success(p_gate, tag, seq,
                      p_so_sched->any_src[tag].data + chunk_offset,
                      len, chunk_offset);

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


  NM_SO_TRACE("ACK_CHUNK completed for tag = %d, track_id = %u, seq = %u, chunk_len = %u\n", tag, track_id, seq, chunk_len);

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

         /* Completion of an ANY_SRC unpack: the unpack_success has to
            carry out this information (tbx_true) */
         p_so_gate->p_so_sched->any_src[tag].status = 0;

         interface->unpack_success(p_gate, tag, seq, tbx_true);

       } else {
         NM_SO_TRACE("Completion of a large message de tag %d et seq %d\n", tag, seq);

         p_so_gate->status[tag][seq] = 0;

         interface->unpack_success(p_gate, tag, seq, tbx_false);
       }
       p_so_gate->recv[tag][p_so_pw->pw.seq].unpack_here.len = p_pw->length;

    } else {
      if((p_so_sched->any_src[tag].status & NM_SO_STATUS_RDV_IN_PROGRESS)
         &&
         (p_so_sched->any_src[tag].gate_id == p_gate->id)
         &&
         (p_so_sched->any_src[tag].seq == seq)) {

        NM_SO_TRACE("Completion of a large ANY_SRC message with tag %d et seq %d\n", tag, seq);

        /* Completion of an ANY_SRC unpack: the unpack_success has to
           carry out this information (tbx_true) */
        p_so_sched->any_src[tag].len -= len;

        if(p_so_sched->any_src[tag].len <= 0){

          p_so_sched->any_src[tag].status = 0;
          p_so_sched->any_src[tag].gate_id = -1;

          p_so_sched->pending_any_src_unpacks--;
          interface->unpack_success(p_gate, tag, seq, tbx_true);
        }

      } else {
        NM_SO_TRACE("Completion of a large message - recv[%d][%d].unpack_here.len = %d \n", tag, seq, p_so_gate->recv[tag][seq].unpack_here.len);

        p_so_gate->recv[tag][seq].unpack_here.len -= len;

        if(p_so_gate->recv[tag][seq].unpack_here.len <= 0){
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

      struct nm_so_pkt_wrap *p_so_large_pw
        = nm_l2so(p_so_gate->pending_large_recv.next);

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

  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;

  return err;
}


/** Process a failed incoming request.
 */
int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		_err) {
  TBX_FAILURE("nm_so_in_process_failed_rq");
  return nm_so_in_process_success_rq(p_sched, p_pw);
}
