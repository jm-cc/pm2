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

#ifndef NM_SO_PKT_WRAP_H
#define NM_SO_PKT_WRAP_H


#define nm_l2so(l) \
        ((struct nm_pkt_wrap *)((char *)(l) -\
         (unsigned long)(&((struct nm_pkt_wrap *)0)->link)))

static __inline__ uint32_t nm_so_pw_remaining_header_area(struct nm_pkt_wrap *p_pw)
{
  const struct iovec *vec = &p_pw->v[0];
  return NM_SO_MAX_UNEXPECTED - ((vec->iov_base + vec->iov_len) - (void *)p_pw->buf);
}

static __inline__ uint32_t nm_so_pw_remaining_data(struct nm_pkt_wrap *p_pw)
{
  return NM_SO_MAX_UNEXPECTED - p_pw->length;
}

static __inline__ void nm_so_pw_assign(struct nm_pkt_wrap*p_pw, nm_trk_id_t trk_id, nm_drv_id_t drv_id, nm_gate_t p_gate)
{
  p_pw->p_gate = p_gate;
   /* Packet is assigned to given driver */
  p_pw->p_drv = (p_pw->p_gdrv = nm_gate_drv_get(p_gate, drv_id))->p_drv;
  /* Packet is assigned to given track */
  p_pw->trk_id = trk_id;
}

/** Ensures p_pw->v contains at least n entries
 */
static inline void nm_pw_grow_n(struct nm_pkt_wrap*p_pw, int n)
{
  if(n >= p_pw->v_size)
    {
      while(n >= p_pw->v_size)
	p_pw->v_size *= 2;
      if(p_pw->v == p_pw->prealloc_v)
	p_pw->v = TBX_MALLOC(sizeof(struct iovec) * p_pw->v_size);
      else
	p_pw->v = TBX_REALLOC(p_pw->v, sizeof(struct iovec) * p_pw->v_size);
    }
}

static inline struct iovec*nm_pw_grow_iovec(struct nm_pkt_wrap*p_pw)
{
  nm_pw_grow_n(p_pw, p_pw->v_nb + 1);
  return &p_pw->v[p_pw->v_nb++];
}

int nm_so_pw_init(struct nm_core *p_core);

int nm_so_pw_exit(void);

int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw);

int nm_so_pw_free(struct nm_pkt_wrap *p_pw);

int nm_so_pw_split(struct nm_pkt_wrap *p_pw,
                   struct nm_pkt_wrap **pp_pw2,
                   uint32_t offset);

int nm_so_pw_add_data(struct nm_pkt_wrap *p_pw, struct nm_pack_s*p_pack,
		      const void *data, uint32_t len, uint32_t offset, uint8_t is_last_chunk, int flags);

int nm_so_pw_add_datatype(struct nm_pkt_wrap *p_pw, struct nm_pack_s*p_pack,
			  uint32_t len, const struct DLOOP_Segment *segp);

/** Add small data to pw, in header */
static inline void nm_so_pw_add_data_in_header(struct nm_pkt_wrap*p_pw, nm_tag_t tag, nm_seq_t seq,
					       const void*data, uint32_t len, uint32_t chunk_offset, uint8_t flags)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
  const uint32_t size = nm_so_aligned(len);
  nm_so_init_data(h, tag, seq, flags | NM_PROTO_FLAG_ALIGNED, 0, len, chunk_offset);
  if(len)
    {
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += size;
    }
  p_pw->length += NM_SO_DATA_HEADER_SIZE + size;

}

/** Add small data to pw, in iovec */
static inline void nm_so_pw_add_data_in_iovec(struct nm_pkt_wrap*p_pw, nm_tag_t tag, nm_seq_t seq,
					      const void*data, uint32_t len, uint32_t chunk_offset, uint8_t proto_flags)
{
  struct iovec*hvec = &p_pw->v[0];
  struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
  struct iovec *dvec = nm_pw_grow_iovec(p_pw);
  dvec->iov_base = (void*)data;
  dvec->iov_len = len;
  /* We don't know yet the gap between header and data, so we
     temporary store the iovec index as the 'skip' value */
  nm_so_init_data(h, tag, seq, proto_flags, p_pw->v_nb, len, chunk_offset);
  p_pw->length += NM_SO_DATA_HEADER_SIZE + len;
}

/** Add raw data to pw, without header */
static inline void nm_so_pw_add_raw(struct nm_pkt_wrap*p_pw, const void*data, uint32_t len, uint32_t chunk_offset)
{
  assert(p_pw->flags & NM_PW_NOHEADER);
  struct iovec*vec = nm_pw_grow_iovec(p_pw);
  vec->iov_base = (void*)data;
  vec->iov_len = len;
  p_pw->length += len;
  p_pw->chunk_offset = chunk_offset;
}

static inline int nm_so_pw_add_control(struct nm_pkt_wrap*p_pw, const union nm_so_generic_ctrl_header*p_ctrl)
{
  struct iovec*hvec = &p_pw->v[0];
  memcpy(hvec->iov_base + hvec->iov_len, p_ctrl, NM_SO_CTRL_HEADER_SIZE);
  hvec->iov_len += NM_SO_CTRL_HEADER_SIZE;
  p_pw->length += NM_SO_CTRL_HEADER_SIZE;
  return NM_ESUCCESS;
}


static __inline__ void 
nm_so_pw_alloc_and_fill_with_data(struct nm_pack_s*p_pack, const void*ptr, uint32_t chunk_len,
                                  uint32_t chunk_offset, tbx_bool_t is_last_chunk,
                                  int flags, struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw;
  nm_so_pw_alloc(flags, &p_pw);
  nm_so_pw_add_data(p_pw, p_pack, ptr, chunk_len, chunk_offset, is_last_chunk, flags);
  p_pw->chunk_offset = chunk_offset;
  *pp_pw = p_pw;
}

static __inline__
int
nm_so_pw_alloc_and_fill_with_control(const union nm_so_generic_ctrl_header *ctrl,
				     struct nm_pkt_wrap **pp_pw)
{
  int err;
  struct nm_pkt_wrap *p_pw;

  err = nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_control(p_pw, ctrl);
  if(err != NM_ESUCCESS)
    goto free;

  *pp_pw = p_pw;

 out:
    return err;
 free:
    nm_so_pw_free(p_pw);
    goto out;
}

static inline void nm_pw_add_contrib(struct nm_pkt_wrap*p_pw, struct nm_pack_s*p_pack, uint32_t len)
{
  if(p_pw->n_contribs > 0 && p_pw->contribs[p_pw->n_contribs - 1].p_pack == p_pack)
    {
      p_pw->contribs[p_pw->n_contribs - 1].len += len;
    }
  else
    {
      if(p_pw->n_contribs >= p_pw->contribs_size)
	{
	  p_pw->contribs_size *= 2;
	  if(p_pw->contribs == p_pw->prealloc_contribs)
	    {
	      p_pw->contribs = TBX_MALLOC(sizeof(struct nm_pw_contrib_s) * p_pw->contribs_size);
	      memcpy(p_pw->contribs, p_pw->prealloc_contribs, sizeof(struct nm_pw_contrib_s) * NM_SO_PREALLOC_IOV_LEN);
	    }
	  else
	    {
	      p_pw->contribs = TBX_REALLOC(p_pw->contribs, sizeof(struct nm_pw_contrib_s) * p_pw->contribs_size);
	    }
	}
      struct nm_pw_contrib_s*p_contrib = &p_pw->contribs[p_pw->n_contribs];
      p_contrib->len = len;
      p_contrib->p_pack = p_pack;
      p_pw->n_contribs++;
    }
}

int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw);


static __inline__ int nm_so_pw_dec_header_ref_count(struct nm_pkt_wrap *p_pw)
{
  if(!(--p_pw->header_ref_count))
    {
      nm_so_pw_free(p_pw);
    }
  return NM_ESUCCESS;
}

#endif /* NM_SO_PKT_WRAP_H */

