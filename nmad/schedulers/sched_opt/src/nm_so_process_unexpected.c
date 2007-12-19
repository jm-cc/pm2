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
#include "nm_gate.h"
#include "nm_sched.h"
#include "nm_trk.h"
#include "nm_pkt_wrap.h"
#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_debug.h"
#include "nm_log.h"
#include "nm_so_process_large_transfer.h"
#include "nm_so_process_unexpected.h"

#include <ccs_public.h>
#include <segment.h>

#include "nm_so_process_unexpected.h"

static int
_nm_so_treat_chunk(tbx_bool_t is_any_src,
                   void *dest_buffer,
                   void *header,
                   struct nm_so_pkt_wrap *p_so_pw){
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
    tag = rdv->tag_id - 128;
    seq = rdv->seq;
    len = rdv->len;
    chunk_offset = rdv->chunk_offset;
    is_last_chunk = rdv->is_last_chunk;

    NM_SO_TRACE("RDV recovered chunk : tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    err = nm_so_rdv_success(is_any_src, p_gate, tag, seq, len, chunk_offset, is_last_chunk);

  } else {
    h = header;
    tag = h->proto_id - 128;
    seq = h->seq;
    len = h->len;
    ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;

    chunk_offset = h->chunk_offset;
    is_last_chunk = h->is_last_chunk;

    NM_SO_TRACE("DATA recovered chunk: tag = %u, seq = %u, len = %u, chunk_offset = %u\n", tag, seq, len, chunk_offset);

    if(is_any_src){
      if(is_last_chunk){
        p_so_sched->any_src[tag].expected_len = chunk_offset + len;
      }
      p_so_sched->any_src[tag].cumulated_len += len;

    } else {
      if(is_last_chunk){
        p_so_gate->recv[tag][seq].unpack_here.expected_len = chunk_offset + len;
      }
      p_so_gate->recv[tag][seq].unpack_here.cumulated_len += len;
    }

    if((is_any_src && p_so_sched->any_src[tag].status & NM_SO_STATUS_UNPACK_IOV)
       || (!(is_any_src && p_so_gate->status[tag][seq] & NM_SO_STATUS_UNPACK_IOV))){

      _nm_so_copy_data_in_iov(dest_buffer, chunk_offset, ptr, len);

    } else if((!(is_any_src && p_so_gate->status[tag][seq] & NM_SO_STATUS_IS_DATATYPE))
              || (is_any_src && p_so_sched->any_src[tag].status & NM_SO_STATUS_IS_DATATYPE)){
      DLOOP_Offset last = chunk_offset + len;

      if(is_any_src){
        CCSI_Segment_unpack(p_so_sched->any_src[tag].data, chunk_offset, &last, ptr);
      } else {
        CCSI_Segment_unpack(p_so_gate->recv[tag][seq].unpack_here.segp, chunk_offset, &last, ptr);
      }

    } else {
      /* Copy data to its final destination */
      memcpy(dest_buffer + chunk_offset, ptr, len);
    }
  }

  /* Decrement the packet wrapper reference counter. If no other
     chunks are still in use, the pw will be destroyed. */
  nm_so_pw_dec_header_ref_count(p_so_pw);

  return NM_ESUCCESS;
}

int treat_unexpected(tbx_bool_t is_any_src, struct nm_gate *p_gate,
                     uint8_t tag, uint8_t seq, uint32_t len, void *data){
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_gate->p_sched->sch_private;
  struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;
  void *first_header = p_so_gate->recv[tag][seq].pkt_here.header;
  struct nm_so_pkt_wrap *first_p_so_pw = p_so_gate->recv[tag][seq].pkt_here.p_so_pw;
  struct list_head *chunks = p_so_gate->recv[tag][seq].pkt_here.chunks;
  struct nm_so_chunk *chunk = NULL;
  uint32_t expected_len = 0;
  uint32_t cumulated_len = 0;

  if(is_any_src){
    p_so_sched->any_src[tag].expected_len = 0;
    p_so_sched->any_src[tag].cumulated_len = 0;
  } else {
    p_so_gate->recv[tag][seq].unpack_here.expected_len = 0;
    p_so_gate->recv[tag][seq].unpack_here.cumulated_len = 0;
  }

  p_so_gate->recv[tag][seq].unpack_here.data = data;

  _nm_so_treat_chunk(is_any_src, data, first_header, first_p_so_pw);

  /* copy of all the received chunks */
  if(chunks != NULL){
    while(!list_empty(chunks)){
      chunk = nm_l2chunk(chunks->next);

      _nm_so_treat_chunk(is_any_src, data,
                         chunk->header, chunk->p_so_pw);

      // next
      list_del(chunks->next);
      tbx_free(nm_so_chunk_mem, chunk);
    }
  }

  if(is_any_src){
    expected_len  = p_so_sched->any_src[tag].expected_len;
    cumulated_len = p_so_sched->any_src[tag].cumulated_len;
  } else {
    expected_len  = p_so_gate->recv[tag][seq].unpack_here.expected_len;
    cumulated_len = p_so_gate->recv[tag][seq].unpack_here.cumulated_len;
  }

  if(expected_len == 0){
    if(is_any_src){
      p_so_sched->any_src[tag].expected_len  = len;
    } else {
      p_so_gate->recv[tag][seq].unpack_here.expected_len  = len;
    }
  }

  NM_SO_TRACE("After retrieving data - expected_len = %d, cumulated_len = %d\n", expected_len, cumulated_len);

  /* Verify if the communication is done */
  if(cumulated_len >= expected_len){
    NM_SO_TRACE("Wow! All data chunks were already in!\n");

    if(is_any_src){
      //p_so_gate->recv_seq_number[tag]++;

      p_so_sched->any_src[tag].status = 0;
      p_so_sched->any_src[tag].gate_id = -1;

      p_so_sched->pending_any_src_unpacks--;

    } else {
      p_so_gate->status[tag][seq] = 0;
    }

    interface->unpack_success(p_gate, tag, seq, is_any_src);

    return NM_ESUCCESS;
  }
  return NM_EINPROGRESS; // il manque encore des données
}
