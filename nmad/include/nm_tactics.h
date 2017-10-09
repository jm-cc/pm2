/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
  assert(p_pw->max_len != NM_LEN_UNDEFINED);
  assert(p_pw->length <= p_pw->max_len);
  return p_pw->max_len - p_pw->length;
}

/** checks whether req_chunk is a SHORT chunk for optimized protocol */
static inline int nm_tactic_req_is_short(const struct nm_req_chunk_s*p_req_chunk)
{
  return ( (p_req_chunk->chunk_offset == 0) &&
	   (p_req_chunk->chunk_len < 255) &&
	   (p_req_chunk->chunk_len == p_req_chunk->p_req->pack.len) &&
	   (p_req_chunk->proto_flags == NM_PROTO_FLAG_LASTCHUNK) );
}

/** returns size in pw for the given short req_chunk */
static inline nm_len_t nm_tactic_req_short_size(const struct nm_req_chunk_s*p_req_chunk)
{
  assert(nm_tactic_req_is_short(p_req_chunk));
  return (NM_HEADER_SHORT_DATA_SIZE + p_req_chunk->chunk_len);
}

/** returns max size in pw for given data req_chunk-
 * value is a upper bound, actual size may be lower */
static inline nm_len_t nm_tactic_req_data_max_size(const struct nm_req_chunk_s*p_req_chunk)
{
  const nm_len_t chunk_len = p_req_chunk->chunk_len;
  const struct nm_req_s*p_pack = p_req_chunk->p_req;
  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
  const nm_len_t max_blocks = (p_props->blocks > chunk_len) ? chunk_len : p_props->blocks;
  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + max_blocks * sizeof(struct nm_header_pkt_data_chunk_s) + NM_ALIGN_FRONTIER;
  return (max_header_len + chunk_len);
}

/** compute average block size */
static inline nm_len_t nm_tactic_req_data_density(const struct nm_req_chunk_s*p_req_chunk)
{
  const struct nm_req_s*p_pack = p_req_chunk->p_req;
  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
  const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : 0;
  return density;
}

/** Try to pack a control chunk in a pw.
 * @return NM_ESUCCESS for success, -NM_EBUSY if pw full
 * @note function takes ownership of p_ctrl_chunk
 */
static inline int nm_tactic_pack_ctrl(nm_gate_t p_gate, nm_drv_t p_drv,
				      struct nm_ctrl_chunk_s*p_ctrl_chunk,
				      struct nm_pkt_wrap_s*p_pw)
{
  if(NM_HEADER_CTRL_SIZE < nm_pw_remaining_buf(p_pw))
    {
      nm_pw_add_control(p_pw, &p_ctrl_chunk->ctrl);
      nm_ctrl_chunk_list_remove(&p_gate->ctrl_chunk_list, p_ctrl_chunk);
      nm_ctrl_chunk_free(&p_gate->p_core->ctrl_chunk_allocator, p_ctrl_chunk);
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EBUSY;
    }
}

static inline int nm_tactic_pack_rdv(nm_gate_t p_gate, nm_drv_t p_drv,
				     struct nm_req_chunk_list_s*p_req_chunk_list,
				     struct nm_req_chunk_s*p_req_chunk,
				     struct nm_pkt_wrap_s*p_pw)
{
  if(NM_HEADER_CTRL_SIZE < nm_pw_remaining_buf(p_pw))
    {
      if(p_req_chunk_list)
	nm_req_chunk_list_remove(p_req_chunk_list, p_req_chunk);
      struct nm_req_s*p_pack = p_req_chunk->p_req;
      const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
      const nm_len_t density = p_props->size / p_props->blocks;
      const int is_lastchunk = (p_req_chunk->chunk_offset + p_req_chunk->chunk_len == p_pack->pack.len);
      nm_pw_flag_t flags = NM_PW_FLAG_NONE;
      if((!p_props->is_contig) && (density < NM_LARGE_MIN_DENSITY) && (p_pack->data.ops.p_generator == NULL))
	{
	  flags |= NM_REQ_CHUNK_FLAG_USE_COPY;
	}
      struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader(p_gate->p_core);
      nm_pw_add_req_chunk(p_large_pw, p_req_chunk, flags);
      nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_large_pw);
      union nm_header_ctrl_generic_s rdv;
      nm_header_init_rdv(&rdv, p_pack, p_req_chunk->chunk_len, p_req_chunk->chunk_offset, is_lastchunk ? NM_PROTO_FLAG_LASTCHUNK : 0);
      nm_pw_add_control(p_pw, &rdv);
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EBUSY;
    }
}

/** a chunk of rdv data. 
 * Use one for regular rendezvous; use nb_driver chunks for multi-rail.
 */
struct nm_rdv_chunk
{
  nm_trk_id_t trk_id; /**< trk_id of track to send data on */
  nm_len_t len;       /**< length of data chunk */
};

/** Packs a series of RTR for multiple chunks of a pw, and post corresponding recv
*/
static inline void nm_tactic_rtr_pack(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw, int nb_chunks, const struct nm_rdv_chunk*chunks)
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
	  p_pw2 = nm_pw_alloc_noheader(p_gate->p_core);
	  p_pw2->p_drv    = p_pw->p_drv;
	  p_pw2->trk_id   = p_pw->trk_id;
	  p_pw2->p_gate   = p_pw->p_gate;
	  p_pw2->p_trk    = p_pw->p_trk;
	  p_pw2->p_unpack = p_pw->p_unpack;
	  /* populate p_pw2 iovec */
	  nm_pw_split_data(p_pw, p_pw2, chunks[i].len);
	  assert(p_pw->length == chunks[i].len);
	}
      if(chunks[i].trk_id == NM_TRK_LARGE)
	{
	  struct nm_trk_s*p_trk = &p_gate->trks[chunks[i].trk_id];
	  if(p_pw->p_data != NULL && !p_trk->p_drv->props.capabilities.supports_data)
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
	  nm_core_post_recv(p_pw, p_pw->p_gate, chunks[i].trk_id, NULL);
	}
      else
	{
	  nm_pw_free(p_core, p_pw);
	}
      nm_core_post_rtr(p_gate, tag, seq, chunks[i].trk_id, chunk_offset, chunks[i].len);
      chunk_offset += chunks[i].len;
      p_pw = p_pw2;
      p_pw2 = NULL;
    }
}

#endif /* NM_TACTICS_H */

