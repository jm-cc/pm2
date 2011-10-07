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



/** Fast packet allocator constant for initial number of entries. */
#define INITIAL_PKT_NUM  8

/** Allocator struct for headerless pkt wrapper.
 */
static p_tbx_memory_t nm_so_pw_nohd_mem = NULL;

/** Allocator struct for pkt wrapper with contiguous data block.
 */
static p_tbx_memory_t nm_so_pw_buf_mem = NULL;

/** Allocator for packet contributions
 */
static p_tbx_memory_t nm_so_pw_contrib_mem = NULL;


/** Initialize the fast allocator structs for SO pkt wrapper.
 *
 *  @param p_core a pointer to the NM core object.
 *  @return The NM status.
 */
int nm_so_pw_init(struct nm_core *p_core TBX_UNUSED)
{
  tbx_malloc_extended_init(&nm_so_pw_contrib_mem,
		  sizeof(struct nm_pw_contrib_s),
		  INITIAL_PKT_NUM, "nmad/core/pw_contrib",
		  1);

  tbx_malloc_extended_init(&nm_so_pw_nohd_mem,
			   sizeof(struct nm_pkt_wrap),
			   INITIAL_PKT_NUM, "nmad/core/nm_pkt_wrap/nohd", 
			   1);

  tbx_malloc_extended_init(&nm_so_pw_buf_mem,
			   sizeof(struct nm_pkt_wrap) + NM_SO_MAX_UNEXPECTED,
			   INITIAL_PKT_NUM, "nmad/core/nm_pkt_wrap/buf", 
			   1);

  return NM_ESUCCESS;
}

/** Cleanup the fast allocator structs for SO pkt wrapper.
 *
 *  @return The NM status.
 */
int nm_so_pw_exit()
{
  tbx_malloc_clean(nm_so_pw_contrib_mem);
  tbx_malloc_clean(nm_so_pw_nohd_mem);
  tbx_malloc_clean(nm_so_pw_buf_mem);

  return NM_ESUCCESS;
}


void nm_so_pw_raz(struct nm_pkt_wrap *p_pw)
{
  p_pw->p_drv  = NULL;
  p_pw->trk_id = NM_TRK_NONE;
  p_pw->p_gate = NULL;
  p_pw->p_gdrv = NULL;
  p_pw->drv_priv = NULL;

  p_pw->flags = 0;
  p_pw->length = 0;

  p_pw->v       = p_pw->prealloc_v;
  p_pw->v_size  = NM_SO_PREALLOC_IOV_LEN;
  p_pw->v_nb    = 0;

  p_pw->contribs = p_pw->prealloc_contribs;
  p_pw->contribs_size = NM_SO_PREALLOC_IOV_LEN;
  p_pw->n_contribs = 0;

  p_pw->p_unpack = NULL;

  p_pw->header_ref_count = 0;

  p_pw->chunk_offset = 0;
  p_pw->is_completed = tbx_true;

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
      p_pw = tbx_malloc(nm_so_pw_buf_mem);
      if (!p_pw)
	{
	  err = -NM_ENOMEM;
	  goto out;
	}
      nm_so_pw_raz(p_pw);
      
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
      p_pw = tbx_malloc(nm_so_pw_nohd_mem);
      if (!p_pw)
	{
	  err = -NM_ENOMEM;
	  goto out;
	}
      nm_so_pw_raz(p_pw);
      
      /* pw flags */
      p_pw->flags = NM_PW_NOHEADER;
      
      /* pkt is empty for now */
      p_pw->length = 0;
    }
  else if(flags & NM_PW_GLOBAL_HEADER)
    {
      /* global header preallocated as v[0]- used for short sends */
      p_pw = tbx_malloc(nm_so_pw_buf_mem);
      if (!p_pw)
	{
	  err = -NM_ENOMEM;
	  goto out;
	}
      nm_so_pw_raz(p_pw);
      
      /* first entry: global header */
      p_pw->v_nb = 1;
      p_pw->v[0].iov_base = p_pw->buf;
      p_pw->v[0].iov_len = 0;
      p_pw->length = 0;
      
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

 out:
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

  nmad_lock_assert();
  
  if(p_pw->flags & NM_PW_DYNAMIC_V0)
    {
      TBX_FREE(p_pw->v[0].iov_base);
    }

  /* clean whole iov */
  if(p_pw->v != p_pw->prealloc_v)
    {
      TBX_FREE(p_pw->v);
    }
  /* clean the contribs */
  if(p_pw->contribs != p_pw->prealloc_contribs)
    {
      TBX_FREE(p_pw->contribs);
    }
  
#ifdef DEBUG
  /* make sure no one can use this pw anymore ! */
  memset(p_pw, 0, sizeof(struct nm_pkt_wrap));
#endif
  /* Finally clean packet wrapper itself */
  if((flags & NM_PW_BUFFER) || (flags & NM_PW_GLOBAL_HEADER))
    {
      tbx_free(nm_so_pw_buf_mem, p_pw);
      NM_TRACEF("pw %p is removed from nm_so_pw_buf_mem\n", p_pw);
    }
  else if(flags & NM_PW_NOHEADER) {
    NM_TRACEF("pw %p is removed from nm_so_pw_nohd_mem\n", p_pw);
    tbx_free(nm_so_pw_nohd_mem, p_pw);
  }

  err = NM_ESUCCESS;
  return err;
}


/** Split the data from p_pw into two parts between p_pw and p_pw2
 */
int nm_so_pw_split_data(struct nm_pkt_wrap *p_pw,
			struct nm_pkt_wrap *p_pw2,
			uint32_t offset)
{
  assert(p_pw->flags & NM_PW_NOHEADER);
  assert(p_pw2->flags & NM_PW_NOHEADER);
  assert(p_pw2->length == 0);
  const uint32_t total_length = p_pw->length;
  int idx_pw = 0;
  uint32_t len = 0;
  while(len < total_length)
    {
      if(len >= offset)
	{
	  /* consume the whole segment */
	  const uint32_t chunk_len = p_pw->v[idx_pw].iov_len;
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
	  const int iov_offset = offset - len;
	  const uint32_t chunk_len = p_pw->v[idx_pw].iov_len - iov_offset;
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

  /* check that no data was lost :-) */
  assert(p_pw->length + p_pw2->length == total_length);
  assert(p_pw->length == offset);

  return NM_ESUCCESS;
}

/** Append a fragment of data to the pkt wrapper being built for sending.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param tag the tag id which generated the fragment.
 *  @param seq the sequence number of the fragment.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param flags the flags controlling the way the fragment is appended.
 *  @return The NM status.
 */
void nm_so_pw_add_data(struct nm_pkt_wrap *p_pw,
		       struct nm_pack_s*p_pack,
		       const void *data, uint32_t len,
		       uint32_t offset,
		       int flags)
{
  const nm_core_tag_t tag = p_pack->tag;
  const nm_seq_t seq = p_pack->seq;
  nm_proto_t proto_flags = 0;
  assert(!p_pw->p_unpack);

  /* add the contrib ref to the pw */
  nm_pw_add_contrib(p_pw, p_pack, len);
  if(p_pack->scheduled == p_pack->len)
    {
      proto_flags |= NM_PROTO_FLAG_LASTCHUNK;
    }
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    {
      proto_flags |= NM_PROTO_FLAG_ACKREQ;
    }

  if(p_pw->flags & NM_PW_GLOBAL_HEADER) /* ** Data with a global header in v[0] */
    {
      /* Small data case */
      if((proto_flags == NM_PROTO_FLAG_LASTCHUNK) && (len < 255) && (offset == 0))
	{
	  nm_so_pw_add_short_data(p_pw, tag, seq, data, len);
	}
      else if(flags & NM_SO_DATA_USE_COPY)
	{
	  /* Data immediately follows its header */
	  nm_so_pw_add_data_in_header(p_pw, tag, seq, data, len, offset, proto_flags);
	}
      else 
	{
	  /* Data handled by a separate iovec entry */
	  nm_so_pw_add_data_in_iovec(p_pw, tag, seq, data, len, offset, proto_flags);
	}
    }
  else if(p_pw->flags & NM_PW_NOHEADER)
    {
      /* ** Add raw data to pw, without header */
      nm_so_pw_add_raw(p_pw, data, len, offset);
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
      struct iovec *vec = p_pw->v;
      void *ptr = vec->iov_base;
      unsigned long remaining_bytes = vec->iov_len;
      unsigned long to_skip = 0;
      struct iovec *last_treated_vec = vec;
      do 
	{
	  const nm_proto_t proto_id = *(nm_proto_t*)ptr;
	  uint32_t proto_hsize = 0;
	  if(proto_id == NM_PROTO_DATA)
	    {
	      /* Data header */
	      struct nm_so_data_header *h = ptr;
	      if(h->skip == 0)
		{
		  /* Data immediately follows */
		  proto_hsize = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);
		}
	      else
		{
		  /* Data occupy a separate iovec entry */
		  proto_hsize = NM_SO_DATA_HEADER_SIZE;
		  h->skip = remaining_bytes - NM_SO_DATA_HEADER_SIZE + to_skip;
		  last_treated_vec++;
		  to_skip += last_treated_vec->iov_len;
		}
	    }
	  else if(proto_id == NM_PROTO_SHORT_DATA)
	    {
	      struct nm_so_short_data_header*h = ptr;
	      proto_hsize = NM_SO_SHORT_DATA_HEADER_SIZE + h->len;
	    }
	  else
	    {
	      /* Ctrl header */
	      proto_hsize = NM_SO_CTRL_HEADER_SIZE;
	    }
	  assert(remaining_bytes >= proto_hsize);
	  remaining_bytes -= proto_hsize;
	  if(remaining_bytes == 0)
	    {
	      nm_proto_t*p_proto = ptr;
	      *p_proto |= NM_PROTO_LAST;
	    }
	  ptr += proto_hsize;
	}
      while(remaining_bytes > 0);
      p_pw->flags |= NM_PW_FINALIZED;
    }
  return err;
}


