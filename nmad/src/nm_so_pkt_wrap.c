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
int nm_so_pw_exit()
{
  nm_pw_nohd_allocator_delete(nm_pw_nohd_allocator);
  nm_pw_buf_allocator_delete(nm_pw_buf_allocator);
  return NM_ESUCCESS;
}


static inline void nm_so_pw_raz(struct nm_pkt_wrap *p_pw)
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
      p_pw = nm_pw_nohd_malloc(nm_pw_nohd_allocator);
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
      p_pw = (struct nm_pkt_wrap*)nm_pw_buf_malloc(nm_pw_buf_allocator);
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

  if(p_pw->destructor)
    {
      (*p_pw->destructor)(p_pw);
    }
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
		       const void *data, nm_len_t len,
		       nm_len_t offset,
		       int flags)
{
  const nm_core_tag_t tag = p_pack->tag;
  const nm_seq_t seq = p_pack->seq;
  nm_proto_t proto_flags = 0;
  assert(!p_pw->p_unpack);

  /* add the contrib ref to the pw */
  nm_pw_add_contrib(p_pw, p_pack, len);
  assert(offset + len <= p_pack->len);
  if(offset + len == p_pack->len)
    {
      proto_flags |= NM_PROTO_FLAG_LASTCHUNK;
    }
  if(p_pack->status & NM_PACK_SYNCHRONOUS)
    {
      proto_flags |= NM_PROTO_FLAG_ACKREQ;
    }
  if(p_pw->flags & NM_PW_GLOBAL_HEADER)
    {
      /* ** Data with a global header in v[0] */
      if((proto_flags == NM_PROTO_FLAG_LASTCHUNK) && (len < 255) && (offset == 0))
	{
	  /* Small data case */
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
	  nm_len_t proto_hsize = 0;
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


/* ********************************************************* */
/* ** completions */

void nm_pw_add_completion(struct nm_pkt_wrap*p_pw, const struct nm_pw_completion_s*p_completion)
{
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
  memcpy(&p_pw->completions[p_pw->n_completions], p_completion, sizeof(struct nm_pw_completion_s));
  p_pw->n_completions++;
}

void nm_pw_add_contrib(struct nm_pkt_wrap*p_pw, struct nm_pack_s*p_pack, nm_len_t len)
{
   if(p_pw->n_completions > 0 &&
      p_pw->completions[p_pw->n_completions - 1].notifier == &nm_pw_contrib_complete &&
      p_pw->completions[p_pw->n_completions - 1].data.contrib.p_pack == p_pack)
    {
      p_pw->completions[p_pw->n_completions - 1].data.contrib.len += len;
    }
   else
     {
       struct nm_pw_completion_s completion;
       completion.notifier       = &nm_pw_contrib_complete;
       completion.data.contrib.p_pack = p_pack;
       completion.data.contrib.len    = len;
       nm_pw_add_completion(p_pw, &completion);
     }
  p_pack->scheduled += len;
}

/** fires completion handlers on the given pw */
void nm_pw_notify_completions(struct nm_pkt_wrap*p_pw)
{
  int i;
  for(i = 0; i < p_pw->n_completions; i++)
    {
      struct nm_pw_completion_s*p_completion = &p_pw->completions[i];
      (*p_completion->notifier)(p_pw, p_completion);
    }
}
