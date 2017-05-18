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

#ifndef NM_TACTICS_H
#define NM_TACTICS_H

/** remaining space in pw buffer */
static inline nm_len_t nm_pw_remaining_buf(struct nm_pkt_wrap_s*p_pw)
{
  assert(((p_pw->v[0].iov_base + p_pw->v[0].iov_len) - (void*)p_pw->buf) <= p_pw->length);
  return NM_SO_MAX_UNEXPECTED - p_pw->length;
}

/** Try to pack a control chunk in a pw.
 * @return NM_ESUCCESS for success, -NM_EBUSY if pw full
 * @note function takes ownership of p_ctrl_chunk
 */
static inline int nm_tactic_pack_ctrl(nm_gate_t p_gate,
				      struct nm_ctrl_chunk_s*p_ctrl_chunk,
				      struct nm_pkt_wrap_s*p_pw)
{
  if(NM_HEADER_CTRL_SIZE + p_pw->length < nm_drv_max_small(p_gate->p_core))
    {
      nm_pw_add_control(p_pw, &p_ctrl_chunk->ctrl);
      nm_ctrl_chunk_list_erase(&p_gate->ctrl_chunk_list, p_ctrl_chunk);
      nm_ctrl_chunk_free(p_gate->p_core->ctrl_chunk_allocator, p_ctrl_chunk);
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EBUSY;
    }
}

/** Pack small data into an existing packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_into_pw(struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset,
						nm_len_t copy_threshold, struct nm_pkt_wrap_s*p_pw)
{
  if(chunk_len < copy_threshold)
    nm_pw_add_data_chunk(p_pw, p_pack, chunk_len, chunk_offset,
			    NM_PW_DATA_ITERATOR | NM_PW_DATA_USE_COPY);
  else
    nm_pw_add_data_chunk(p_pw, p_pack, chunk_len, chunk_offset, NM_PW_DATA_ITERATOR);
}

/** Pack small data into a new packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_new_pw(struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset,
					       struct nm_pkt_wrap_list_s*p_out_list)
{ 
  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
  nm_pw_add_data_chunk(p_pw, p_pack, chunk_len, chunk_offset, NM_PW_DATA_ITERATOR);
  assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
  nm_pkt_wrap_list_push_back(p_out_list, p_pw);
}

/** Pack large data into a new packet wrapper stored as pending large,
 * and pack a rdv for this data.
 */
static inline void nm_tactic_pack_data_rdv(struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset)
{
  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_noheader();
  nm_pw_add_data_chunk(p_pw, p_pack, chunk_len, chunk_offset, NM_PW_NOHEADER | NM_PW_DATA_ITERATOR);
  nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_pw);
  union nm_header_ctrl_generic_s ctrl;
  nm_header_init_rdv(&ctrl, p_pack, chunk_len, chunk_offset,
		     (p_pack->pack.scheduled == p_pack->pack.len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
  nm_strat_pack_ctrl(p_pack->p_gate, &ctrl);
}

/** Find in the given outlist a packet wrapper with
 * at least 'message_len' available, with a visibility window of length 'window'
 * return NULL if none found.
 */
static inline struct nm_pkt_wrap_s*nm_tactic_try_to_aggregate(struct nm_pkt_wrap_list_s*p_out_list,
							      nm_len_t message_len, int window)
{
  int i = 0;
  struct nm_pkt_wrap_s*p_pw = NULL;
  puk_list_foreach(p_pw, p_out_list)
    {
      const nm_len_t rlen = nm_pw_remaining_buf(p_pw);
      if(message_len + NM_ALIGN_FRONTIER < rlen)
	{
	  return p_pw;
	}
      i++;
      if(i > window)
	return NULL;
    }
  return NULL;
}

/** a chunk of rdv data. 
 * Use one for regular rendezvous; use nb_driver chunks for multi-rail.
 */
struct nm_rdv_chunk
{
  nm_drv_t p_drv;
  nm_trk_id_t trk_id;
  nm_len_t len;
};

/** Packs a series of RTR for multiple chunks of a pw, and post corresponding recv
*/
static inline void nm_tactic_rtr_pack(struct nm_pkt_wrap_s*p_pw, int nb_chunks, const struct nm_rdv_chunk*chunks)
{
  int i;
  nm_len_t chunk_offset = p_pw->chunk_offset;
  const nm_seq_t seq = p_pw->p_unpack->seq;
  const nm_core_tag_t tag = p_pw->p_unpack->tag;
  nm_gate_t p_gate = p_pw->p_gate;
  for(i = 0; i < nb_chunks; i++)
    {
      struct nm_pkt_wrap_s*p_pw2 = NULL;
      if(chunks[i].len < p_pw->length)
	{
	  /* create a new pw with the remaining data */
	  p_pw2 = nm_pw_alloc_noheader();
	  p_pw2->p_drv    = p_pw->p_drv;
	  p_pw2->trk_id   = p_pw->trk_id;
	  p_pw2->p_gate   = p_pw->p_gate;
	  p_pw2->p_gdrv   = p_pw->p_gdrv;
	  p_pw2->p_unpack = p_pw->p_unpack;
	  /* populate p_pw2 iovec */
	  nm_pw_split_data(p_pw, p_pw2, chunks[i].len);
	  assert(p_pw->length == chunks[i].len);
	}
      if(chunks[i].trk_id == NM_TRK_LARGE)
	{
	  if(p_pw->p_data != NULL && !chunks[i].p_drv->trk_caps[NM_TRK_LARGE].supports_data)
	    {
	      const struct nm_data_properties_s*p_props = nm_data_properties_get(p_pw->p_data);
	      struct iovec*vec = nm_pw_grow_iovec(p_pw);
	      if(p_props->is_contig)
		{
		  vec->iov_base = nm_data_baseptr_get(p_pw->p_data) + p_pw->chunk_offset;
		}
	      else
		{
		  void*buf = malloc(p_pw->length);
		  vec->iov_base = buf;
		  p_pw->flags |= NM_PW_DYNAMIC_V0;
		}
	      vec->iov_len = p_pw->length;
	    }
	  nm_core_post_recv(p_pw, p_pw->p_gate, chunks[i].trk_id, chunks[i].p_drv);
	}
      else
	{
	  nm_pw_free(p_pw);
	}
      nm_core_post_rtr(p_gate, tag, seq, chunks[i].p_drv, chunks[i].trk_id, chunk_offset, chunks[i].len);
      chunk_offset += chunks[i].len;
      p_pw = p_pw2;
      p_pw2 = NULL;
    }
}

#endif /* NM_TACTICS_H */

