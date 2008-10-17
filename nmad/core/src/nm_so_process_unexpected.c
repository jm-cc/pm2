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

#include <nm_private.h>

#include <ccs_public.h>
#include <segment.h>


static int _nm_so_treat_chunk(tbx_bool_t is_any_src,
			      void *dest_buffer,
			      void *header,
			      struct nm_pkt_wrap *p_so_pw)
{
  struct nm_gate *p_gate = p_so_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;

  nm_tag_t tag;
  uint8_t seq, is_last_chunk;
  uint32_t len, chunk_offset;
  int err;

  nm_tag_t proto_id = *(nm_tag_t *)header;

  if(proto_id == NM_SO_PROTO_RDV)
    {
      struct nm_so_ctrl_rdv_header *rdv = header;
      tag = rdv->tag_id - 128;
      seq = rdv->seq;
      len = rdv->len;
      chunk_offset = rdv->chunk_offset;
      is_last_chunk = rdv->is_last_chunk;
      
      NM_SO_TRACE("RDV recovered chunk : tag = %u, seq = %u, len = %u, chunk_offset = %u\n",
		  tag, seq, len, chunk_offset);
      err = nm_so_rdv_success(is_any_src, p_gate, tag, seq, len, chunk_offset, is_last_chunk);
    }
  else
    {
      struct nm_so_data_header *h = header;
      tag = h->proto_id - 128;
      seq = h->seq;
      len = h->len;
      void *ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;
      chunk_offset = h->chunk_offset;
      is_last_chunk = h->is_last_chunk;
      struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_so_sched->any_src, tag) : NULL;
      struct nm_so_tag_s*p_so_tag = (!is_any_src) ? nm_so_tag_get(&p_so_gate->tags, tag) : NULL;
      
      NM_SO_TRACE("DATA recovered chunk: tag = %u, seq = %u, len = %u, chunk_offset = %u\n",
		  tag, seq, len, chunk_offset);
      
      if(is_any_src){
	if(is_last_chunk){
	  any_src->expected_len = chunk_offset + len;
	}
	any_src->cumulated_len += len;
      }
      else 
	{
	  if(is_last_chunk){
	    p_so_tag->recv[seq].unpack_here.expected_len = chunk_offset + len;
	  }
	  p_so_tag->recv[seq].unpack_here.cumulated_len += len;
	}
      
      if((!is_any_src && (p_so_tag->status[seq] & NM_SO_STATUS_UNPACK_IOV))
	 || (is_any_src && (any_src->status & NM_SO_STATUS_UNPACK_IOV)))
	{
	  _nm_so_copy_data_in_iov(dest_buffer, chunk_offset, ptr, len);
	}
      else if((!is_any_src && (p_so_tag->status[seq] & NM_SO_STATUS_IS_DATATYPE))
              || (is_any_src && (any_src->status & NM_SO_STATUS_IS_DATATYPE)))
	{
	  struct DLOOP_Segment *segp = dest_buffer;
	  DLOOP_Offset last = chunk_offset + len;
	  CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
	}
      else
	{
	  /* Copy data to its final destination */
	  memcpy(dest_buffer + chunk_offset, ptr, len);
	}
    }
  
  /* Decrement the packet wrapper reference counter. If no other
     chunks are still in use, the pw will be destroyed. */
  nm_so_pw_dec_header_ref_count(p_so_pw);

  return NM_ESUCCESS;
}

int nm_so_process_unexpected(tbx_bool_t is_any_src, struct nm_gate *p_gate,
			     nm_tag_t tag, uint8_t seq, uint32_t len, void *data)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
  void *first_header = p_so_tag->recv[seq].pkt_here.header;
  struct nm_pkt_wrap *first_p_so_pw = p_so_tag->recv[seq].pkt_here.p_so_pw;
  struct list_head *chunks = p_so_tag->recv[seq].pkt_here.chunks;
  struct nm_so_chunk *chunk = NULL;
  uint32_t expected_len = 0;
  uint32_t cumulated_len = 0;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag) : NULL;

  if(is_any_src){
    any_src->expected_len = 0;
    any_src->cumulated_len = 0;
  } else {
    p_so_tag->recv[seq].unpack_here.expected_len = 0;
    p_so_tag->recv[seq].unpack_here.cumulated_len = 0;
  }

  p_so_tag->recv[seq].unpack_here.data = data;

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
    expected_len  = any_src->expected_len;
    cumulated_len = any_src->cumulated_len;
  } else {
    expected_len  = p_so_tag->recv[seq].unpack_here.expected_len;
    cumulated_len = p_so_tag->recv[seq].unpack_here.cumulated_len;
  }

  if(expected_len == 0){
    if(is_any_src){
      any_src->expected_len  = len;
    } else {
      p_so_tag->recv[seq].unpack_here.expected_len  = len;
    }
  }

  NM_SO_TRACE("After retrieving data - expected_len = %d, cumulated_len = %d\n", expected_len, cumulated_len);

  /* Verify if the communication is done */
  if(cumulated_len >= expected_len){
    NM_SO_TRACE("Wow! All data chunks were already in!\n");

    if(is_any_src){

      any_src->status = 0;
      any_src->p_gate = NM_ANY_GATE;

      p_gate->p_core->so_sched.pending_any_src_unpacks--;

    } else {
      p_so_tag->status[seq] = 0;
    }

    nm_so_status_event(p_gate->p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, is_any_src);

    return NM_ESUCCESS;
  }
  return NM_EINPROGRESS; // il manque encore des données
}
