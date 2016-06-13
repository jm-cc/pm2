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
static inline nm_len_t nm_so_pw_remaining_buf(struct nm_pkt_wrap *p_pw)
{
  assert(((p_pw->v[0].iov_base + p_pw->v[0].iov_len) - (void*)p_pw->buf) <= p_pw->length);
  return NM_SO_MAX_UNEXPECTED - p_pw->length;
}

/** Pack a generic control header as a new packet wrapper on track #0.
 */
static inline void nm_tactic_pack_ctrl(const union nm_header_ctrl_generic_s*p_ctrl,
				       struct tbx_fast_list_head*out_list)
{
  struct nm_pkt_wrap*p_pw = nm_pw_alloc_global_header();
  nm_so_pw_add_control(p_pw, p_ctrl);
  tbx_fast_list_add_tail(&p_pw->link, out_list);
}

/** Pack small data into an existing packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_into_pw(struct nm_req_s*p_pack, const char*data, nm_len_t len, nm_len_t offset,
						nm_len_t copy_threshold, struct nm_pkt_wrap*p_pw)
{
  if(len < copy_threshold)
    nm_so_pw_add_data_chunk(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER | NM_PW_DATA_USE_COPY);
  else
    nm_so_pw_add_data_chunk(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER);
}

/** Pack small data into a new packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_new_pw(struct nm_req_s*p_pack, const char*data, int len, int offset,
					       int copy_threshold, struct tbx_fast_list_head*out_list)
{ 
  struct nm_pkt_wrap*p_pw = nm_pw_alloc_global_header();
  nm_tactic_pack_small_into_pw(p_pack, data, len, offset, copy_threshold, p_pw);
  tbx_fast_list_add_tail(&p_pw->link, out_list);
}

/** Pack large data into a new packet wrapper stored as pending large,
 * and pack a rdv for this data.
 */
static inline void nm_tactic_pack_rdv(struct nm_req_s*p_pack, const char*data, nm_len_t len, nm_len_t offset)
{
  struct nm_pkt_wrap*p_pw = nm_pw_alloc_noheader();
  nm_so_pw_add_data_chunk(p_pw, p_pack, data, len, offset, NM_PW_NOHEADER);
  tbx_fast_list_add_tail(&p_pw->link, &p_pack->p_gate->pending_large_send);
  union nm_header_ctrl_generic_s ctrl;
  nm_header_init_rdv(&ctrl, p_pack, len, offset, (p_pack->pack.scheduled == p_pack->pack.len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pack->p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_pack->p_gate, &ctrl);
}

static inline void nm_tactic_pack_data_rdv(struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset)
{
  struct nm_pkt_wrap*p_pw = nm_pw_alloc_noheader();
  nm_so_pw_add_data_chunk(p_pw, p_pack, p_pack->p_data, chunk_len, chunk_offset, NM_PW_NOHEADER | NM_PW_DATA_ITERATOR);
  tbx_fast_list_add_tail(&p_pw->link, &p_pack->p_gate->pending_large_send);
  union nm_header_ctrl_generic_s ctrl;
  nm_header_init_rdv(&ctrl, p_pack, chunk_len, chunk_offset,
		     (p_pack->pack.scheduled == p_pack->pack.len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pack->p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_pack->p_gate, &ctrl);
}

/** Find in the given outlist a packet wrapper with
 * at least 'header_len' available as header and
 * 'data_len' available as data.
 * return NULL if none found.
 */
static inline struct nm_pkt_wrap*nm_tactic_try_to_aggregate(struct tbx_fast_list_head*out_list,
							    int header_len, int data_len)
{
  struct nm_pkt_wrap*p_pw = NULL;
  tbx_fast_list_for_each_entry(p_pw, out_list, link)
    {
      const nm_len_t rlen = nm_so_pw_remaining_buf(p_pw);
      if(header_len + data_len + NM_SO_ALIGN_FRONTIER < rlen)
	{
	  return p_pw;
	}
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
static inline void nm_tactic_rtr_pack(struct nm_pkt_wrap*p_pw, int nb_chunks, const struct nm_rdv_chunk*chunks)
{
  int i;
  nm_len_t chunk_offset = p_pw->chunk_offset;
  const nm_seq_t seq = p_pw->p_unpack->seq;
  const nm_core_tag_t tag = p_pw->p_unpack->tag;
  nm_gate_t p_gate = p_pw->p_gate;
  for(i = 0; i < nb_chunks; i++)
    {
      struct nm_pkt_wrap*p_pw2 = NULL;
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
	  nm_so_pw_split_data(p_pw, p_pw2, chunks[i].len);
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
		  vec->iov_base = p_props->base_ptr + p_pw->chunk_offset;
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
      nm_so_post_rtr(p_gate, tag, seq, chunks[i].p_drv, chunks[i].trk_id, chunk_offset, chunks[i].len);
      chunk_offset += chunks[i].len;
      p_pw = p_pw2;
      p_pw2 = NULL;
    }
}

#endif /* NM_TACTICS_H */

