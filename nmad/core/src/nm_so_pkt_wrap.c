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

#include <pm2_common.h>

#include <nm_private.h>



/** Fast packet allocator constant for initial number of entries. */
#if defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
#  define INITIAL_PKT_NUM  4
#else
#  define INITIAL_PKT_NUM  16
#endif

/** Allocator struct for headerless pkt wrapper.
 */
static p_tbx_memory_t nm_so_pw_nohd_mem = NULL;

/** Allocator struct for pkt wrapper with contiguous data block.
 */
static p_tbx_memory_t nm_so_pw_buf_mem = NULL;


/** Initialize the fast allocator structs for SO pkt wrapper.
 *
 *  @param p_core a pointer to the NM core object.
 *  @return The NM status.
 */
int nm_so_pw_init(struct nm_core *p_core TBX_UNUSED)
{
  tbx_malloc_init(&nm_so_pw_nohd_mem,
		  sizeof(struct nm_pkt_wrap),
		  INITIAL_PKT_NUM, "nmad/core/nm_pkt_wrap/nohd");

  tbx_malloc_init(&nm_so_pw_buf_mem,
		  sizeof(struct nm_pkt_wrap) + NM_SO_MAX_UNEXPECTED,
		  INITIAL_PKT_NUM, "nmad/core/nm_pkt_wrap/buf");

  return NM_ESUCCESS;
}

/** Cleanup the fast allocator structs for SO pkt wrapper.
 *
 *  @return The NM status.
 */
int nm_so_pw_exit(void)
{
  tbx_malloc_clean(nm_so_pw_nohd_mem);
  tbx_malloc_clean(nm_so_pw_buf_mem);

  return NM_ESUCCESS;
}


static void nm_so_pw_raz(struct nm_pkt_wrap *p_pw)
{
  int i;

  p_pw->p_drv  = NULL;
  p_pw->trk_id = NM_TRK_NONE;
  p_pw->p_gate = NULL;
  p_pw->p_gdrv = NULL;
  p_pw->tag = 0;
  p_pw->seq = 0;
  p_pw->drv_priv   = NULL;

  p_pw->flags = 0;
  p_pw->length = 0;

  p_pw->v       = p_pw->prealloc_v;
  p_pw->v_size  = NM_SO_PREALLOC_IOV_LEN;
  p_pw->v_nb    = 0;

#ifdef PIO_OFFLOAD
  p_pw->data_to_offload = tbx_false;
#endif

  p_pw->header_ref_count = 0;
  p_pw->pending_skips = 0;

  for(i = 0; i < NM_SO_PREALLOC_IOV_LEN; i++)
    {
      p_pw->prealloc_v[i].iov_len  = 0;
      p_pw->prealloc_v[i].iov_base = NULL;
    }

  p_pw->chunk_offset = 0;
  p_pw->is_completed = tbx_true;
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
      p_pw->v[0].iov_len = NM_SO_GLOBAL_HEADER_SIZE;
      p_pw->length = NM_SO_GLOBAL_HEADER_SIZE;
      
      /* pw flags */
      p_pw->flags = NM_PW_GLOBAL_HEADER;
      p_pw->pending_skips = 0;
    }
  else
    {
      TBX_FAILURE("nmad: no flag given for pw allocations.\n");
    }
  
  INIT_LIST_HEAD(&p_pw->link);

  NM_SO_TRACE_LEVEL(3,"creating a pw %p (%d bytes)\n", p_pw, p_pw->length);

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

  NM_SO_TRACE_LEVEL(3,"destructing the pw %p\n", p_pw);

#ifdef PIOMAN
  piom_req_free(&p_pw->inst);
#endif

  if(p_pw->flags & NM_PW_DYNAMIC_V0)
    {
      TBX_FREE(p_pw->v[0].iov_base);
    }

  /* Then clean whole iov */
  if(p_pw->v != p_pw->prealloc_v)
    {
      TBX_FREE(p_pw->v);
    }

  /* Finally clean packet wrapper itself */
  if((flags & NM_PW_BUFFER) || (flags & NM_PW_GLOBAL_HEADER))
    {
      tbx_free(nm_so_pw_buf_mem, p_pw);
      NM_SO_TRACE_LEVEL(3,"pw %p is removed from nm_so_pw_buf_mem\n", p_pw);
    }
  else if(flags & NM_PW_NOHEADER) {
    NM_SO_TRACE_LEVEL(3,"pw %p is removed from nm_so_pw_nohd_mem\n", p_pw);
    tbx_free(nm_so_pw_nohd_mem, p_pw);
  }

  err = NM_ESUCCESS;
  return err;
}

/** Ensures p_pw->v contains at least n entries and returns v[n]
 */
static inline struct iovec*nm_pw_grow_iovec(struct nm_pkt_wrap*p_pw)
{
  if(p_pw->v_nb >= p_pw->v_size)
    {
      p_pw->v_size *= 2;
      if(p_pw->v == p_pw->prealloc_v)
	p_pw->v = TBX_MALLOC(sizeof(struct iovec) * p_pw->v_size);
      else
	p_pw->v = TBX_REALLOC(p_pw->v, sizeof(struct iovec) * p_pw->v_size);
    }
  return &p_pw->v[p_pw->v_nb++];
}

// Attention!! split only enable when the p_pw is finished
int nm_so_pw_split(struct nm_pkt_wrap *p_pw,
                   struct nm_pkt_wrap **pp_pw2,
                   uint32_t offset)
{
  struct nm_pkt_wrap *p_pw2 = NULL;
  int err;

  nm_so_pw_finalize(p_pw);

  assert(offset > 0 && offset < p_pw->length);

  err = nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw2);
  if(err != NM_ESUCCESS)
    goto err;

  /* Copy the pw part */
  p_pw2->p_drv = p_pw->p_drv;
  p_pw2->trk_id = p_pw->trk_id;
  p_pw2->p_gate = p_pw->p_gate;
  p_pw2->p_gdrv = p_pw->p_gdrv;
  p_pw2->tag = p_pw->tag;
  p_pw2->seq = p_pw->seq;

  const uint32_t total_length = p_pw->length;
  int idx_pw = 0;
  uint32_t len = 0;
  while(len < total_length)
    {
      if(len >= offset)
	{
	  /* consume the whole segment */
	  const uint32_t chunk_len = p_pw->v[idx_pw].iov_len;
	  nm_so_pw_add_data(p_pw2, p_pw->tag, p_pw->seq,
			    p_pw->v[idx_pw].iov_base, chunk_len,
			    p_pw->chunk_offset, (len + chunk_len == total_length), 0);
	  len += p_pw->v[idx_pw].iov_len;
	  p_pw->length -= chunk_len;
	  p_pw->v_nb--;
	  p_pw->v[idx_pw].iov_len = 0;
	  p_pw->v[idx_pw].iov_base = NULL;
	}
      else if(len + p_pw->v[idx_pw].iov_len > offset)
	{
	  /* cut in the middle of an iovec segment */
	  const int iov_offset = offset - len;
	  const uint32_t chunk_len = p_pw->v[idx_pw].iov_len - iov_offset;
	  nm_so_pw_add_data(p_pw2, p_pw->tag, p_pw->seq,
			    p_pw->v[idx_pw].iov_base + iov_offset, chunk_len,
			    p_pw->chunk_offset, (len + chunk_len == total_length), 0);
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

  assert(p_pw->length + p_pw2->length == total_length);
  assert(p_pw->length == offset);

  p_pw2->chunk_offset = p_pw->chunk_offset + offset;

  *pp_pw2 = p_pw2;

 err:
  return err;
}

/** Append a fragment of data to the pkt wrapper being built.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param tag the tag id which generated the fragment.
 *  @param seq the sequence number of the fragment.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @param flags the flags controlling the way the fragment is appended.
 *  @return The NM status.
 */
int nm_so_pw_add_data(struct nm_pkt_wrap *p_pw,
		      nm_tag_t tag, nm_seq_t seq,
		      const void *data, uint32_t len,
		      uint32_t offset,
		      uint8_t is_last_chunk,
		      int flags)
{
  int err;

  if(p_pw->flags & NM_PW_GLOBAL_HEADER) /* ** Data with a global header in v[0] */
    {
      struct iovec*hvec = &p_pw->v[0];
      /* Small data case */
      struct nm_so_data_header *h = hvec->iov_base + hvec->iov_len;
      hvec->iov_len += NM_SO_DATA_HEADER_SIZE;
      if(tbx_likely(flags & NM_SO_DATA_USE_COPY))
	{
	  /* Data immediately follows its header */
	  const uint32_t size = nm_so_aligned(len);
	  const uint8_t flags = (is_last_chunk ? NM_SO_DATA_FLAG_LASTCHUNK : 0) | NM_SO_DATA_FLAG_ALIGNED;
	  nm_so_init_data(h, tag, seq, flags, 0, len, offset);
	  if(len)
	    {
#ifdef PIO_OFFLOAD
	      struct iovec *dvec = nm_pw_grow_iovec(p_pw);
	      p_pw->data_to_offload = tbx_true;
	      dvec->iov_base = data;
	      dvec->iov_len  = len;
#else
	      memcpy(hvec->iov_base + hvec->iov_len, data, len);
#endif
	      hvec->iov_len += size;
	    }
	  p_pw->length += NM_SO_DATA_HEADER_SIZE + size;
	}
      else 
	{
	  /* Data handled by a separate iovec entry */
	  struct iovec *dvec = nm_pw_grow_iovec(p_pw);
	  dvec->iov_base = (void*)data;
	  dvec->iov_len = len;
	  /* We don't know yet the gap between header and data, so we
	     temporary store the iovec index as the 'skip' value */
	  const uint8_t flags = (is_last_chunk ? NM_SO_DATA_FLAG_LASTCHUNK : 0);
	  nm_so_init_data(h, tag, seq, flags, p_pw->v_nb, len, offset);
	  p_pw->pending_skips++;
	  p_pw->length += NM_SO_DATA_HEADER_SIZE + len;
	}
    }
  else if(p_pw->flags & NM_PW_NOHEADER)
    {
      /* ** Add raw data to pw, without header */
      struct iovec*vec = nm_pw_grow_iovec(p_pw);
      vec->iov_base = (void*)data;
      vec->iov_len = len;
      p_pw->tag = tag;
      p_pw->seq = seq;
      p_pw->length += len;
    }
  err = NM_ESUCCESS;
  return err;
}

int nm_so_pw_store_datatype(struct nm_pkt_wrap *p_pw,
			    nm_tag_t tag, nm_seq_t seq,
			    uint32_t len, const struct DLOOP_Segment *segp)
{
  p_pw->tag = tag;
  p_pw->seq = seq;

  p_pw->length += len;

  p_pw->segp = (struct DLOOP_Segment*)segp;
  p_pw->datatype_offset = 0;

  return NM_ESUCCESS;
}

// function dedicated to the datatypes which do not require a rendezvous
int nm_so_pw_add_datatype(struct nm_pkt_wrap *p_pw,
			  nm_tag_t tag, nm_seq_t seq,
			  uint32_t len, const struct DLOOP_Segment *segp)
{
  if(len) 
    {
      const uint32_t size = nm_so_aligned(len);
      DLOOP_Offset first = 0;
      DLOOP_Offset last  = len;
      {
	struct iovec *vec =  &p_pw->v[0];
	/* Add header */
	struct nm_so_data_header *h = vec->iov_base + vec->iov_len;
	nm_so_init_data(h, tag, seq, NM_SO_DATA_FLAG_LASTCHUNK | NM_SO_DATA_FLAG_ALIGNED, 0, len, 0);
	vec->iov_len += NM_SO_DATA_HEADER_SIZE;
	p_pw->length += NM_SO_DATA_HEADER_SIZE;
      }
      
      /* flatten datatype into contiguous memory */
      CCSI_Segment_pack(segp, first, &last, p_pw->v[0].iov_base + p_pw->v[0].iov_len);
      
      p_pw->v[0].iov_len += size;
      p_pw->length += size;
    }

  return NM_ESUCCESS;
}

/* TODO- is this function ever used? (AD) */
int nm_so_pw_copy_contiguously_datatype(struct nm_pkt_wrap *p_pw,
                                    nm_tag_t tag, nm_seq_t seq,
                                    uint32_t len, struct DLOOP_Segment *segp)
{
  DLOOP_Offset first = 0;
  DLOOP_Offset last  = len;
  void *buf = TBX_MALLOC(len); /* TODO: where is it freed? (AD) */

  int err = nm_so_pw_add_data(p_pw, tag, seq, buf, len, 0, 1, NM_PW_NOHEADER);

  struct iovec *vec = p_pw->v;

  /* flatten datatype into contiguous memory */
  CCSI_Segment_pack(segp, first, &last, vec->iov_base);

  vec->iov_len = len;

  err = NM_ESUCCESS;
  return err;
}

/** Finalize the incremental building of the packet.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @return The NM status.
 */
int nm_so_pw_finalize(struct nm_pkt_wrap *p_pw)
{
  int err = NM_ESUCCESS;

  if(!(p_pw->flags & NM_PW_FINALIZED) && (p_pw->flags & NM_PW_GLOBAL_HEADER))
    {
      /* update the length field of the global header */
      struct nm_so_global_header*header = p_pw->v->iov_base;
      header->len = p_pw->length;
      if(p_pw->pending_skips > 0)
	{
	  /* Fix the 'skip' fields */
	  struct iovec *vec = p_pw->v;
	  void *ptr = vec->iov_base + NM_SO_GLOBAL_HEADER_SIZE;
	  unsigned long remaining_bytes = vec->iov_len - NM_SO_GLOBAL_HEADER_SIZE;
	  unsigned long to_skip = 0;
	  struct iovec *last_treated_vec = vec;
	  do 
	    {
	      const nm_proto_t proto_id = *(nm_proto_t*)ptr;
	      if(proto_id == NM_PROTO_DATA)
		{
		  /* Data header */
		  struct nm_so_data_header *h = ptr;
		  if(h->skip == 0)
		    {
		      /* Data immediately follows */
		      ptr += NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);
		      remaining_bytes -= NM_SO_DATA_HEADER_SIZE + nm_so_aligned(h->len);
		    }
		  else
		    {
		      /* Data occupy a separate iovec entry */
		      ptr += NM_SO_DATA_HEADER_SIZE;
		      remaining_bytes -= NM_SO_DATA_HEADER_SIZE;
		      h->skip = remaining_bytes + to_skip;
		      last_treated_vec++;
		      to_skip += last_treated_vec->iov_len;
		      p_pw->pending_skips--;
		    }
		}
	      else
		{
		  /* Ctrl header */
		  ptr += NM_SO_CTRL_HEADER_SIZE;
		  remaining_bytes -= NM_SO_CTRL_HEADER_SIZE;
		}
	    }
	  while (p_pw->pending_skips);
	}
      p_pw->flags |= NM_PW_FINALIZED;
    }
  return err;
}

/** Iterate over the fields of a freshly received ctrl pkt.
 *
 *  @param p_pw the pkt wrapper pointer.
 *  @param data_handler the inline data callback.
 *  @param rdv_handler the rdv request callback.
 *  @param ack_handler the rdv ack callback.
 *  @return The NM status.
 */
int nm_so_pw_iterate_over_headers(struct nm_pkt_wrap *p_pw,
				  nm_so_pw_data_handler data_handler,
				  nm_so_pw_rdv_handler rdv_handler,
				  nm_so_pw_ack_handler ack_handler)
{
#ifdef NMAD_QOS
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pw->p_gate->strategy_receptacle;
  unsigned ack_received = 0;
#endif /* NMAD_QOS */

  /* Each 'unread' header will increment this counter. When the
     counter will reach 0 again, the packet wrapper can (and will) be
     safely destroyed */
  p_pw->header_ref_count = 0;

  struct iovec *vec = p_pw->v;
  void *ptr = vec->iov_base;
  const struct nm_so_global_header*gh = ptr;
  unsigned long remaining_len = gh->len - NM_SO_GLOBAL_HEADER_SIZE;
  ptr += NM_SO_GLOBAL_HEADER_SIZE;

  while(remaining_len)
    {
      assert(remaining_len > 0);
      /* Decode header */
      const nm_proto_t proto_id = *(nm_proto_t *)ptr;
      assert(proto_id != 0);
      switch(proto_id)
	{
	case NM_PROTO_DATA:
	  {
	    /* Data header */
	    struct nm_so_data_header *dh = ptr;
	    ptr += NM_SO_DATA_HEADER_SIZE;
	    /* Retrieve data location */
	    unsigned long skip = dh->skip;
	    void*data = ptr;
	    if(dh->len) 
	      {
		const struct iovec *v = vec;
		uint32_t rlen = (v->iov_base + v->iov_len) - data;
		if (skip < rlen)
		  {
		    data += skip;
		  }
		else
		  {
		    do
		      {
			skip -= rlen;
			v++;
			rlen = v->iov_len;
		      } while (skip >= rlen);
		    data = v->iov_base + skip;
		  }
	      }
	    const unsigned long size = (dh->flags & NM_SO_DATA_FLAG_ALIGNED) ? nm_so_aligned(dh->len) : dh->len;
	    remaining_len -= NM_SO_DATA_HEADER_SIZE + size;
	    if(dh->skip == 0)
	      { /* data is just after the header */
		ptr += size;
	      }  /* else the next header is just behind */
	    if(data_handler)
	      {
		const uint8_t is_last_chunk = (dh->flags & NM_SO_DATA_FLAG_LASTCHUNK);
		data_handler(p_pw, data, dh, dh->len, dh->tag_id, dh->seq, dh->chunk_offset, is_last_chunk);
	      }
	  }
	  break;

	case NM_PROTO_DATA_UNUSED:
	  {
	    /* Unused data header- skip */
	    struct nm_so_data_header *dh = ptr;
	    ptr += NM_SO_DATA_HEADER_SIZE;
	    const unsigned long size = (dh->flags & NM_SO_DATA_FLAG_ALIGNED) ? nm_so_aligned(dh->len) : dh->len;
	    remaining_len -= NM_SO_DATA_HEADER_SIZE + size;
	    if(dh->skip == 0)
	      {
		ptr += size;
	      }
	  }
	  break;
	  
	case NM_PROTO_RDV:
	  {
	    union nm_so_generic_ctrl_header *ch = ptr;
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	    remaining_len -= NM_SO_CTRL_HEADER_SIZE;
	    if(rdv_handler)
	      {
		rdv_handler(p_pw, ch, ch->r.tag_id, ch->r.seq,
			    ch->r.len, ch->r.chunk_offset, ch->r.is_last_chunk);
		} else {
		NM_SO_TRACE("Sent completed of a RDV on tag = %d, seq = %d\n", ch->r.tag_id, ch->r.seq);
	      }
	    }
	  break;

	case NM_PROTO_ACK:
	  {
	    union nm_so_generic_ctrl_header *ch = ptr;
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	    remaining_len -= NM_SO_CTRL_HEADER_SIZE;
	    if(ack_handler)
	      {
#ifdef NMAD_QOS
		int r;
		if(strategy->driver->ack_callback != NULL)
		  {
		    ack_received = 1;
		    r = strategy->driver->ack_callback(strategy->_status,
						       p_pw,
						       ch->a.tag_id,
						       ch->a.seq,
						       ch->a.track_id,
						       0);
		  }
		else
#endif /* NMAD_QOS */
		  ack_handler(p_pw, &ch->a);
	      }
	    else
	      {
		NM_SO_TRACE("Sent completed of an ACK on tag = %d, seq = %d, offset = %d\n", ch->a.tag_id, ch->a.seq, ch->a.chunk_offset);
	      }
	  }
	  break;
	  
	case NM_PROTO_CTRL_UNUSED:
	  {
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	    remaining_len -= NM_SO_CTRL_HEADER_SIZE;
	  }
	  break;
	  
	default:
	  return -NM_EINVAL;
	  
	} /* switch */
    } /* while */
  
#ifdef NMAD_QOS
  if(ack_received)
    strategy->driver->ack_callback(strategy->_status,
				   p_pw, 0, 0, 128, 1);
#endif /* NMAD_QOS */

  if (!p_pw->header_ref_count){
    nm_so_pw_free(p_pw);
  }

  return NM_ESUCCESS;
}
