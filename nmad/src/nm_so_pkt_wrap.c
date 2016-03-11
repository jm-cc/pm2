/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

PADICO_MODULE_HOOK(NewMad_Core);


/** Fast packet allocator constant for initial number of entries. */
#define INITIAL_PKT_NUM  8

/** Allocator for headerless pkt wrapper.
 */
PUK_ALLOCATOR_TYPE(nm_pw_nohd, struct nm_pkt_wrap);
static nm_pw_nohd_allocator_t nm_pw_nohd_allocator = NULL;

struct nm_pw_buf
{
  struct nm_pkt_wrap pw;
  char buf[NM_SO_MAX_UNEXPECTED];
};
/** Allocator for pkt wrapper with contiguous data block.
 */
PUK_ALLOCATOR_TYPE(nm_pw_buf, struct nm_pw_buf);
static nm_pw_buf_allocator_t nm_pw_buf_allocator = NULL;


/** Initialize the fast allocator structs for SO pkt wrapper.
 *
 *  @param p_core a pointer to the NM core object.
 *  @return The NM status.
 */
int nm_so_pw_init(struct nm_core *p_core TBX_UNUSED)
{
  nm_pw_nohd_allocator = nm_pw_nohd_allocator_new(INITIAL_PKT_NUM);
  nm_pw_buf_allocator = nm_pw_buf_allocator_new(INITIAL_PKT_NUM);
  return NM_ESUCCESS;
}

/** Cleanup the fast allocator structs for SO pkt wrapper.
 *
 *  @return The NM status.
 */
int nm_so_pw_exit(void)
{
  nm_pw_nohd_allocator_delete(nm_pw_nohd_allocator);
  nm_pw_buf_allocator_delete(nm_pw_buf_allocator);
  return NM_ESUCCESS;
}

/* ********************************************************* */

struct iovec*nm_pw_grow_iovec(struct nm_pkt_wrap*p_pw)
{
  if(p_pw->v_nb + 1 >= p_pw->v_size)
    {
      while(p_pw->v_nb + 1 >= p_pw->v_size)
	{
	  p_pw->v_size *= 2;
	}
      if(p_pw->v == p_pw->prealloc_v)
	{
	  p_pw->v = TBX_MALLOC(p_pw->v_size * sizeof(struct iovec));
	  memcpy(p_pw->v, p_pw->prealloc_v, NM_SO_PREALLOC_IOV_LEN * sizeof(struct iovec));
	}
      else
	{
	  p_pw->v = TBX_REALLOC(p_pw->v, p_pw->v_size * sizeof(struct iovec));
	}
    }
  assert(p_pw->v_nb <= p_pw->v_size);
  return &p_pw->v[p_pw->v_nb++];
}


/** Add short data to pw, with compact header */
void nm_so_pw_add_short_data(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
			     const void*data, nm_len_t len)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_header_short_data_s *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_HEADER_SHORT_DATA_SIZE;
  nm_header_init_short_data(h, tag, seq, len);
  if(len)
    {
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += len;
    }
  p_pw->length += NM_HEADER_SHORT_DATA_SIZE + len;
}

/** Add small data to pw, in header */
void nm_so_pw_add_data_in_header(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
				 const void*data, nm_len_t len, nm_len_t chunk_offset, uint8_t flags)
{
  assert(p_pw->flags & NM_PW_GLOBAL_HEADER);
  struct iovec*hvec = &p_pw->v[0];
  struct nm_header_data_s *h = hvec->iov_base + hvec->iov_len;
  nm_header_init_data(h, tag, seq, flags | NM_PROTO_FLAG_ALIGNED, 0, len, chunk_offset);
  hvec->iov_len += NM_HEADER_DATA_SIZE;
  p_pw->length  += NM_HEADER_DATA_SIZE;
  if(len)
    {
      const nm_len_t size = nm_so_aligned(len);
      assert(len <= nm_so_pw_remaining_data(p_pw));
      memcpy(hvec->iov_base + hvec->iov_len, data, len);
      hvec->iov_len += size;
      p_pw->length  += size;
    }
}

/** Add small data to pw, in iovec */
void nm_so_pw_add_data_in_iovec(struct nm_pkt_wrap*p_pw, nm_core_tag_t tag, nm_seq_t seq,
				const void*data, nm_len_t len, nm_len_t chunk_offset, uint8_t proto_flags)
{
  struct iovec*hvec = &p_pw->v[0];
  struct nm_header_data_s *h = hvec->iov_base + hvec->iov_len;
  hvec->iov_len += NM_HEADER_DATA_SIZE;
  struct iovec *dvec = nm_pw_grow_iovec(p_pw);
  dvec->iov_base = (void*)data;
  dvec->iov_len = len;
  /* We don't know yet the gap between header and data, so we
     temporary store the iovec index as the 'skip' value */
  nm_header_init_data(h, tag, seq, proto_flags, p_pw->v_nb, len, chunk_offset);
  p_pw->length += NM_HEADER_DATA_SIZE + len;
}

/** Add raw data to pw, without header */
void nm_so_pw_add_raw(struct nm_pkt_wrap*p_pw, const void*data, nm_len_t len, nm_len_t chunk_offset)
{
  assert(p_pw->flags & NM_PW_NOHEADER);
  struct iovec*vec = nm_pw_grow_iovec(p_pw);
  vec->iov_base = (void*)data;
  vec->iov_len = len;
  p_pw->length += len;
  p_pw->chunk_offset = chunk_offset;
}

int nm_so_pw_add_control(struct nm_pkt_wrap*p_pw, const union nm_header_ctrl_generic_s*p_ctrl)
{
  struct iovec*hvec = &p_pw->v[0];
  memcpy(hvec->iov_base + hvec->iov_len, p_ctrl, NM_HEADER_CTRL_SIZE);
  hvec->iov_len += NM_HEADER_CTRL_SIZE;
  p_pw->length += NM_HEADER_CTRL_SIZE;
  return NM_ESUCCESS;
}

/* ********************************************************* */

static inline void nm_pw_init(struct nm_pkt_wrap *p_pw)
{
  p_pw->p_drv  = NULL;
  p_pw->trk_id = NM_TRK_NONE;
  p_pw->p_gate = NULL;
  p_pw->p_gdrv = NULL;
  p_pw->drv_priv = NULL;

  p_pw->flags = 0;
  p_pw->length = 0;

  p_pw->p_data  = NULL;
  p_pw->v       = p_pw->prealloc_v;
  p_pw->v_size  = NM_SO_PREALLOC_IOV_LEN;
  p_pw->v_nb    = 0;

  p_pw->destructor = NULL;
  p_pw->destructor_key = NULL;
  
  p_pw->completions = p_pw->prealloc_completions;
  p_pw->completions_size = NM_SO_PREALLOC_IOV_LEN;
  p_pw->n_completions = 0;

  p_pw->p_unpack = NULL;
  p_pw->chunk_offset = 0;

  p_pw->ref_count = 0;
  nm_pw_ref_inc(p_pw);

#ifdef PIOMAN_POLL
  piom_ltask_init(&p_pw->ltask);
#endif
}



/** Allocate a suitable pkt wrapper for various SO purposes.
 *
 *  @param flags the flags indicating what pkt wrapper is needed.
 *  @param pp_pw a pointer to the pkt wrapper pointer where to store the result.
 *  @return The NM status.
 */
int nm_so_pw_alloc(int flags, struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw;
  int err = NM_ESUCCESS;

  nmad_lock_assert();

  if(flags & NM_PW_BUFFER) 
    {
      /* full buffer as v[0]- used for short receive */
      p_pw = (struct nm_pkt_wrap*)nm_pw_buf_malloc(nm_pw_buf_allocator);
      nm_pw_init(p_pw);
      
      /* pw flags */
      p_pw->flags = NM_PW_BUFFER;
      
      /* first entry: pkt header */
      p_pw->v_nb = 1;
      p_pw->v[0].iov_base = p_pw->buf;
      p_pw->v[0].iov_len = NM_SO_MAX_UNEXPECTED;
      p_pw->length = NM_SO_MAX_UNEXPECTED;
    } 
  else if(flags & NM_PW_NOHEADER)
    {
      /* no header preallocated- used for large send/recv */
      p_pw = nm_pw_nohd_malloc(nm_pw_nohd_allocator);
      nm_pw_init(p_pw);
      
      /* pw flags */
      p_pw->flags = NM_PW_NOHEADER;
      
      /* pkt is empty for now */
      p_pw->length = 0;
    }
  else if(flags & NM_PW_GLOBAL_HEADER)
    {
      /* global header preallocated as v[0]- used for short sends */
      p_pw = (struct nm_pkt_wrap*)nm_pw_buf_malloc(nm_pw_buf_allocator);
      nm_pw_init(p_pw);
      
      /* first entry: global header */
      p_pw->v_nb = 1;
      p_pw->v[0].iov_base = p_pw->buf;
      p_pw->v[0].iov_len = 0;
      p_pw->length = 0;
      /* reserve bits for v0 skip offset */
      const nm_len_t hlen = sizeof(struct nm_header_global_s);
      p_pw->v[0].iov_len += hlen;
      p_pw->length += hlen;
      
      /* pw flags */
      p_pw->flags = NM_PW_GLOBAL_HEADER;
    }
  else
    {
      TBX_FAILURE("nmad: no flag given for pw allocations.\n");
    }
  
  TBX_INIT_FAST_LIST_HEAD(&p_pw->link);

  NM_TRACEF("creating a pw %p (%d bytes)\n", p_pw, p_pw->length);

  *pp_pw = p_pw;

  return err;
}

/** Free a pkt wrapper and related structs.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @return The NM status.
 */
int nm_so_pw_free(struct nm_pkt_wrap *p_pw)
{
  int err;
  int flags = p_pw->flags;

  if(p_pw->destructor)
    {
      (*p_pw->destructor)(p_pw);
    }
  if(flags & NM_PW_DYNAMIC_V0)
    {
      TBX_FREE(p_pw->v[0].iov_base);
    }
  /* clean whole iov */
  if(p_pw->v != p_pw->prealloc_v)
    {
      TBX_FREE(p_pw->v);
    }
  /* clean the contribs */
  if(p_pw->completions != p_pw->prealloc_completions)
    {
      TBX_FREE(p_pw->completions);
    }
  
#ifdef DEBUG
  /* make sure no one can use this pw anymore ! */
  memset(p_pw, 0, sizeof(struct nm_pkt_wrap));
#ifdef PIOMAN_POLL
  p_pw->ltask.state = PIOM_LTASK_STATE_DESTROYED;
#endif
#endif
  /* Finally clean packet wrapper itself */
  if((flags & NM_PW_BUFFER) || (flags & NM_PW_GLOBAL_HEADER))
    {
      nm_pw_buf_free(nm_pw_buf_allocator, (struct nm_pw_buf*)p_pw);
    }
  else if(flags & NM_PW_NOHEADER)
    {
      nm_pw_nohd_free(nm_pw_nohd_allocator, p_pw);
    }

  err = NM_ESUCCESS;
  return err;
}


/** Split the data from p_pw into two parts between p_pw and p_pw2
 */
int nm_so_pw_split_data(struct nm_pkt_wrap *p_pw,
			struct nm_pkt_wrap *p_pw2,
			nm_len_t offset)
{
  assert(p_pw->flags & NM_PW_NOHEADER);
  assert(p_pw2->flags & NM_PW_NOHEADER);
  assert(p_pw2->length == 0);
  const nm_len_t total_length = p_pw->length;
  if(p_pw->p_data != NULL)
    {
      p_pw2->p_data = p_pw->p_data;
      p_pw2->length = total_length - offset;
      p_pw2->chunk_offset = p_pw->chunk_offset + offset;
      p_pw->length = offset;
    }
  else
    {
      int idx_pw = 0;
      nm_len_t len = 0;
      while(len < total_length)
	{
	  if(len >= offset)
	    {
	      /* consume the whole segment */
	      const nm_len_t chunk_len = p_pw->v[idx_pw].iov_len;
	      nm_so_pw_add_raw(p_pw2, p_pw->v[idx_pw].iov_base, chunk_len, 0);
	      len += p_pw->v[idx_pw].iov_len;
	      p_pw->length -= chunk_len;
	      p_pw->v_nb--;
	      assert(p_pw->v_nb <= p_pw->v_size);
	      p_pw->v[idx_pw].iov_len = 0;
	      p_pw->v[idx_pw].iov_base = NULL;
	    }
	  else if(len + p_pw->v[idx_pw].iov_len > offset)
	    {
	      /* cut in the middle of an iovec segment */
	      const nm_len_t iov_offset = offset - len;
	      const nm_len_t chunk_len = p_pw->v[idx_pw].iov_len - iov_offset;
	      nm_so_pw_add_raw(p_pw2, p_pw->v[idx_pw].iov_base + iov_offset, chunk_len, 0);
	      len += p_pw->v[idx_pw].iov_len;
	      p_pw->v[idx_pw].iov_len = iov_offset;
	      p_pw->length -= chunk_len;
	    }
	  else
	    {
	      len += p_pw->v[idx_pw].iov_len;
	    }
	  idx_pw++;
	}
      p_pw2->chunk_offset = p_pw->chunk_offset + offset;
    }

  /* check that no data was lost :-) */
  assert(p_pw->length + p_pw2->length == total_length);
  assert(p_pw->length == offset);

  return NM_ESUCCESS;
}

/** Append a chunk of data to the pkt wrapper being built for sending.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param tag the tag id which generated the fragment.
 *  @param seq the sequence number of the fragment.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param flags the flags controlling the way the fragment is appended.
 *  @return The NM status.
 */
void nm_so_pw_add_data_chunk(struct nm_pkt_wrap *p_pw,
			     struct nm_req_s*p_pack,
			     const void*ptr, nm_len_t len,
			     nm_len_t offset,
			     int flags)
{
  const nm_core_tag_t tag = p_pack->tag;
  const nm_seq_t seq = p_pack->seq;
  nm_proto_t proto_flags = 0;
  assert(!p_pw->p_unpack);

  /* add the contrib ref to the pw */
  nm_pw_completion_add(p_pw, p_pack, len);
  assert(offset + len <= p_pack->pack.len);
  if(offset + len == p_pack->pack.len)
    {
      proto_flags |= NM_PROTO_FLAG_LASTCHUNK;
    }
  if(p_pack->flags & NM_FLAG_PACK_SYNCHRONOUS)
    {
      proto_flags |= NM_PROTO_FLAG_ACKREQ;
    }
  if(p_pw->flags & NM_PW_GLOBAL_HEADER)
    {
      /* ** Data with a global header in v[0] */
      if(flags & NM_PW_DATA_ITERATOR)
	{
	  /* data defined with iterator */
	  const struct nm_data_s*p_data = ptr;
	  nm_data_pkt_pack(p_pw, tag, seq, p_data, offset, len, proto_flags);
	}
      else if((proto_flags == NM_PROTO_FLAG_LASTCHUNK) && (len < 255) && (offset == 0))
	{
	  /* Small data case */
	  nm_so_pw_add_short_data(p_pw, tag, seq, ptr, len);
	}
      else if(flags & NM_PW_DATA_USE_COPY)
	{
	  /* Data immediately follows its header */
	  nm_so_pw_add_data_in_header(p_pw, tag, seq, ptr, len, offset, proto_flags);
	}
      else 
	{
	  /* Data handled by a separate iovec entry */
	  nm_so_pw_add_data_in_iovec(p_pw, tag, seq, ptr, len, offset, proto_flags);
	}
    }
  else if(p_pw->flags & NM_PW_NOHEADER)
    {
      if(flags & NM_PW_DATA_ITERATOR)
	{
	  const struct nm_data_s*p_data = ptr;
	  p_pw->length = nm_data_size(p_data);
	  p_pw->chunk_offset = offset;
	  p_pw->p_data = p_data;
	}
      else
	{
	  /* ** Add raw data to pw, without header */
	  nm_so_pw_add_raw(p_pw, ptr, len, offset);
	}
    }
}


/** Finalize the incremental building of the packet.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @return The NM status.
 */
int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw)
{
  int err = NM_ESUCCESS;

  /* finalize only on *sender* side */
  assert(p_pw->p_unpack == NULL);

  if(!(p_pw->flags & NM_PW_FINALIZED) && (p_pw->flags & NM_PW_GLOBAL_HEADER))
    {
      /* Fix the 'skip' fields */
      const struct iovec*v0 = &p_pw->v[0];
      void*ptr = v0->iov_base + sizeof(struct nm_header_global_s);
      long remaining_bytes = v0->iov_len - sizeof(struct nm_header_global_s);
      long to_skip = 0;
      const struct iovec*last_treated_vec = v0;
      do 
	{
	  const nm_proto_t proto_id = *(nm_proto_t*)ptr & NM_PROTO_ID_MASK;
	  nm_len_t proto_hsize = 0;
	  if(proto_id == NM_PROTO_DATA)
	    {
	      /* Data header */
	      struct nm_header_data_s*h = ptr;
	      if(h->skip == 0)
		{
		  /* Data immediately follows */
		  proto_hsize = NM_HEADER_DATA_SIZE + nm_so_aligned(h->len);
		}
	      else
		{
		  /* Data occupy a separate iovec entry */
		  proto_hsize = NM_HEADER_DATA_SIZE;
		  h->skip = remaining_bytes - NM_HEADER_DATA_SIZE + to_skip;
		  last_treated_vec++;
		  to_skip += last_treated_vec->iov_len;
		}
	    }
	  else if(proto_id == NM_PROTO_SHORT_DATA)
	    {
	      struct nm_header_short_data_s*h = ptr;
	      proto_hsize = NM_HEADER_SHORT_DATA_SIZE + h->len;
	    }
	  else if(proto_id == NM_PROTO_PKT_DATA)
	    {
	      struct nm_header_pkt_data_s*h = ptr;
	      proto_hsize = h->hlen;
	    }
	  else if(proto_id == NM_PROTO_RDV || proto_id == NM_PROTO_RTR || proto_id == NM_PROTO_ACK)
	    {
	      /* Ctrl header */
	      proto_hsize = NM_HEADER_CTRL_SIZE;
	    }
	  else
	    {
	      padico_fatal("# nmad: unknown proto_id = %d while finalizing pw\n", proto_id);
	    }
	  assert(remaining_bytes >= proto_hsize);
	  remaining_bytes -= proto_hsize;
	  if(remaining_bytes == 0)
	    {
	      nm_proto_t*p_proto = ptr;
	      *p_proto |= NM_PROTO_LAST;
	    }
	  ptr += proto_hsize;
	  assert(remaining_bytes >= 0);
	}
      while(remaining_bytes > 0);
      nm_header_global_finalize(p_pw);
      p_pw->flags |= NM_PW_FINALIZED;
    }
#ifdef DEBUG
  if(p_pw->p_data == NULL)
  {
    int length = 0;
    int i;
    for(i = 0; i < p_pw->v_nb; i++)
      {
	length += p_pw->v[i].iov_len;
      }
    if(length != p_pw->length)
      padico_fatal("# nmad: pw length inconsistency.\n");
  }
#endif /* DEBUG */
  return err;
}


/* ********************************************************* */
/* ** completions */

void nm_pw_completion_add(struct nm_pkt_wrap*p_pw, struct nm_req_s*p_pack, nm_len_t len)
{
  if((p_pw->n_completions > 0) &&
     (p_pw->completions[p_pw->n_completions - 1].p_pack == p_pack))
    {
      p_pw->completions[p_pw->n_completions - 1].len += len;
    }
   else
     {
       const struct nm_pw_completion_s completion = { .p_pack = p_pack, .len = len };
       if(p_pw->n_completions >= p_pw->completions_size)
	 {
	   p_pw->completions_size *= 2;
	   if(p_pw->completions == p_pw->prealloc_completions)
	     {
	       p_pw->completions = TBX_MALLOC(sizeof(struct nm_pw_completion_s) * p_pw->completions_size);
	       memcpy(p_pw->completions, p_pw->prealloc_completions, sizeof(struct nm_pw_completion_s) * NM_SO_PREALLOC_IOV_LEN);
	     }
	   else
	     {
	       p_pw->completions = TBX_REALLOC(p_pw->completions, sizeof(struct nm_pw_completion_s) * p_pw->completions_size);
	     }
	 }
       memcpy(&p_pw->completions[p_pw->n_completions], &completion, sizeof(struct nm_pw_completion_s));
       p_pw->n_completions++;
     }
  p_pack->pack.scheduled += len;
}

