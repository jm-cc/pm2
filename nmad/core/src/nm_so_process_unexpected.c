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


static int _nm_so_treat_chunk(tbx_bool_t is_any_src, void *dest_buffer, struct nm_so_chunk*chunk)
{
  struct nm_gate *p_gate = chunk->p_pw->p_gate;
  struct nm_so_sched *p_so_sched = &p_gate->p_core->so_sched;
  const void*header = chunk->header;
  const nm_tag_t proto_id = *(nm_tag_t *)header;

  if(proto_id >= NM_SO_PROTO_DATA_FIRST)
    {
      const struct nm_so_data_header *h = header;
      const nm_tag_t tag = h->proto_id - 128;
      const nm_seq_t seq = h->seq;
      const uint32_t len = h->len;
      assert(len <= NM_SO_MAX_UNEXPECTED);
      const void *ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;
      const uint32_t chunk_offset = h->chunk_offset;
      const uint8_t is_last_chunk = (h->flags & NM_SO_DATA_FLAG_LASTCHUNK);
      nm_so_status_t status = 0;

      NM_SO_TRACE("DATA recovered chunk: tag = %u, seq = %u, len = %u, chunk_offset = %u\n",
		  tag, seq, len, chunk_offset);
      
      if(is_any_src)
	{
	  struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);
	  if(is_last_chunk)
	    {
	      any_src->expected_len = chunk_offset + len;
	    }
	  any_src->cumulated_len += len;
	  status = any_src->status;
	}
      else 
	{
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
	  if(is_last_chunk)
	    {
	      p_so_tag->recv[seq].unpack_here.expected_len = chunk_offset + len;
	    }
	  p_so_tag->recv[seq].unpack_here.cumulated_len += len;
	  status = p_so_tag->status[seq];
	}
      /* Copy data to its final destination */
      if(status & NM_SO_STATUS_UNPACK_IOV)
	{
	  _nm_so_copy_data_in_iov(dest_buffer, chunk_offset, ptr, len);
	}
      else if(status & NM_SO_STATUS_IS_DATATYPE)
	{
	  struct DLOOP_Segment *segp = dest_buffer;
	  DLOOP_Offset last = chunk_offset + len;
	  CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
	}
      else
	{
	  memcpy(dest_buffer + chunk_offset, ptr, len);
	}
    }
  else if(proto_id == NM_SO_PROTO_RDV)
    {
      const struct nm_so_ctrl_rdv_header *rdv = header;
      const nm_tag_t tag = rdv->tag_id - 128;
      const nm_seq_t seq = rdv->seq;
      const uint32_t len = rdv->len;
      const uint32_t chunk_offset = rdv->chunk_offset;
      const uint8_t is_last_chunk = rdv->is_last_chunk;
      
      NM_SO_TRACE("RDV recovered chunk : tag = %u, seq = %u, len = %u, chunk_offset = %u\n",
		  tag, seq, len, chunk_offset);
      nm_so_rdv_success(is_any_src, p_gate, tag, seq, len, chunk_offset, is_last_chunk);
    }
  chunk->header = NULL;
  /* Decrement the packet wrapper reference counter. If no other
     chunks are still in use, the pw will be destroyed. */
  nm_so_pw_dec_header_ref_count(chunk->p_pw);

  return NM_ESUCCESS;
}

int nm_so_process_unexpected(tbx_bool_t is_any_src, struct nm_gate *p_gate,
			     nm_tag_t tag, nm_seq_t seq, uint32_t len, void *data)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  uint32_t expected_len = 0;
  uint32_t cumulated_len = 0;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag) : NULL;
  struct nm_so_chunk *chunk = &p_so_tag->recv[seq].pkt_here;

  p_so_tag->recv[seq].unpack_here.data = data;

  _nm_so_treat_chunk(is_any_src, data, chunk);

  /* copy of all the received chunks */
  struct list_head *chunks = &chunk->link;
  if(chunks)
    {
      while(!list_empty(chunks))
	{
	  chunk = nm_l2chunk(chunks->next);
	  _nm_so_treat_chunk(is_any_src, data, chunk);
	  list_del(chunks->next);
	  tbx_free(nm_so_chunk_mem, chunk);
	}
    }

  if(is_any_src)
    {
      expected_len  = any_src->expected_len;
      cumulated_len = any_src->cumulated_len;
    }
  else
    {
      expected_len  = p_so_tag->recv[seq].unpack_here.expected_len;
      cumulated_len = p_so_tag->recv[seq].unpack_here.cumulated_len;
    }
  if(expected_len == 0)
    {
      if(is_any_src)
	{
	  any_src->expected_len  = len;
	} 
      else 
	{
	  p_so_tag->recv[seq].unpack_here.expected_len  = len;
	}
    }

  NM_SO_TRACE("After retrieving data - expected_len = %d, cumulated_len = %d\n", expected_len, cumulated_len);

  /* Verify if the communication is done */
  if(cumulated_len >= expected_len)
    {
      NM_SO_TRACE("Wow! All data chunks were already in!\n");
      if(is_any_src)
	{
	  any_src->status = 0;
	  any_src->p_gate = NM_ANY_GATE;
	}
      else 
	{
	  p_so_tag->status[seq] = 0;
	}
      const struct nm_so_event_s event =
	{
	  .status = NM_SO_STATUS_UNPACK_COMPLETED,
	  .p_gate = p_gate,
	  .tag = tag,
	  .seq = seq,
	  .any_src = is_any_src
	};
      nm_so_status_event(p_gate->p_core, &event);
      return NM_ESUCCESS;
    }
  return NM_EINPROGRESS;
}
