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
 * MERCHANTABILITY or FITNESS FOR A PARTICULA R PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>

PADICO_MODULE_HOOK(NewMad_Core);

static void nm_short_data_handler(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_unpack_s*p_unpack,
				  const void*ptr, const nm_so_short_data_header_t*h, struct nm_pkt_wrap *p_pw);
static void nm_small_data_handler(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_unpack_s*p_unpack,
				  const void*ptr, const nm_so_data_header_t*h, struct nm_pkt_wrap *p_pw);
static void nm_rdv_handler(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_unpack_s*p_unpack,
			   const struct nm_so_ctrl_rdv_header*h, struct nm_pkt_wrap *p_pw);

/** a chunk of unexpected message to be stored */
struct nm_unexpected_s
{
  void*header;
  struct nm_pkt_wrap*p_pw;
  nm_gate_t p_gate;
  nm_seq_t seq;
  nm_core_tag_t tag;
  struct tbx_fast_list_head link;
};

/** fast allocator for struct nm_unexpected_s */
PUK_ALLOCATOR_TYPE(nm_unexpected, struct nm_unexpected_s);
static nm_unexpected_allocator_t nm_unexpected_allocator = NULL;

/** size of memory used by unexpected */
static size_t nm_unexpected_mem_size = 0;

#define NM_UNEXPECTED_PREALLOC (NM_TAGS_PREALLOC * 256)

static inline struct nm_unexpected_s*nm_unexpected_alloc(void)
{
  if(tbx_unlikely(nm_unexpected_allocator == NULL))
    {
      nm_unexpected_allocator = nm_unexpected_allocator_new(NM_UNEXPECTED_PREALLOC);
    }
  struct nm_unexpected_s*chunk = nm_unexpected_malloc(nm_unexpected_allocator);
  return chunk;
}

void nm_unexpected_clean(struct nm_core*p_core)
{
  struct nm_unexpected_s*chunk, *tmp;
  tbx_fast_list_for_each_entry_safe(chunk, tmp, &p_core->unexpected, link)
    {
      if(chunk)
	{
#ifdef DEBUG
	  fprintf(stderr, "nmad: WARNING- chunk %p is still in use\n", chunk);
#endif /* DEBUG */
	  if(chunk->p_pw) {
#if(defined(PIOMAN_POLL))
  	    piom_ltask_completed(&chunk->p_pw->ltask);
#else /* PIOMAN_POLL */
	    nm_pw_ref_dec(chunk->p_pw);
#endif /* PIOMAN_POLL */
	  }
	  tbx_fast_list_del(&chunk->link);
	  nm_unexpected_free(nm_unexpected_allocator, chunk);
	}
    }
  if(nm_unexpected_allocator != NULL)
    {
      nm_unexpected_allocator_delete(nm_unexpected_allocator);
      nm_unexpected_allocator = NULL;
    }
}

/** Find an unexpected chunk that matches an unpack request [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
static struct nm_unexpected_s*nm_unexpected_find_matching(struct nm_core*p_core, struct nm_unpack_s*p_unpack)
{
  struct nm_unexpected_s*chunk;
  tbx_fast_list_for_each_entry(chunk, &p_core->unexpected, link)
    {
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&chunk->p_gate->tags, chunk->tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(((p_unpack->p_gate == chunk->p_gate) || (p_unpack->p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_tag_match(chunk->tag, p_unpack->tag, p_unpack->tag_mask) && /* tag matches */
	 ((p_unpack->seq == chunk->seq) || ((p_unpack->seq == NM_SEQ_NONE) && (chunk->seq == next_seq))) /* seq number matches */ ) 
	{
	  if(p_unpack->seq == NM_SEQ_NONE)
	    {
	      p_so_tag->recv_seq_number = next_seq;
	    }
	  p_unpack->tag      = chunk->tag;
	  p_unpack->tag_mask = NM_CORE_TAG_MASK_FULL;
	  p_unpack->p_gate   = chunk->p_gate;
	  p_unpack->seq      = chunk->seq;
	  return chunk;
	}
    }
  return NULL;
}

/** Find an unpack that matches a given packet that arrived from [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
static struct nm_unpack_s*nm_unpack_find_matching(struct nm_core*p_core, nm_gate_t p_gate, nm_seq_t seq, nm_core_tag_t tag)
{
  struct nm_unpack_s*p_unpack = NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
  tbx_fast_list_for_each_entry(p_unpack, &p_core->unpacks, _link)
    {
      if(((p_unpack->p_gate == p_gate) || (p_unpack->p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_tag_match(tag, p_unpack->tag, p_unpack->tag_mask) && /* tag matches */
	 ((p_unpack->seq == seq) || ((p_unpack->seq == NM_SEQ_NONE) && (seq == next_seq))) /* seq number matches */ ) 
	{
	  if(p_unpack->seq == NM_SEQ_NONE)
	    {
	      p_so_tag->recv_seq_number = next_seq;
	    }
	  p_unpack->tag      = tag;
	  p_unpack->tag_mask = NM_CORE_TAG_MASK_FULL;
	  p_unpack->p_gate   = p_gate;
	  p_unpack->seq      = seq;
	  return p_unpack;
	}
    }
  return NULL;
}

/** copy received data to its final destination */
static void nm_so_copy_data(struct nm_unpack_s*p_unpack, nm_len_t chunk_offset, const void *ptr, nm_len_t len)
{
  assert(chunk_offset + len <= p_unpack->expected_len);
  assert(p_unpack->cumulated_len + len <= p_unpack->expected_len);
  if(len > 0)
    {
      /* Copy data to its final destination */
      if(p_unpack->status & NM_UNPACK_TYPE_CONTIGUOUS)
	{
	  /* contiguous data */
	  void*const data = p_unpack->data;
	  memcpy(data + chunk_offset, ptr, len);
	}
      else if(p_unpack->status & NM_UNPACK_TYPE_IOV)
	{
	  /* destination is non contiguous */
	  struct iovec*const iov = p_unpack->data;
	  nm_len_t pending_len = len;
	  nm_len_t offset = 0; /* current offset in the destination */
	  int i = 0;
	  while(offset + iov[i].iov_len <= chunk_offset)
	    {
	      offset += iov[i].iov_len;
	      i++;
	    }
	  assert(offset <= p_unpack->expected_len);
	  nm_len_t entry_offset = (offset < chunk_offset) ? (chunk_offset - offset) : 0;
	  while(pending_len > 0)
	    {
	      const nm_len_t entry_len = (pending_len > (iov[i].iov_len - entry_offset)) ? (iov[i].iov_len - entry_offset) : pending_len;
	      memcpy(iov[i].iov_base + entry_offset, ptr + len - pending_len, entry_len);
	      pending_len -= entry_len;
	      i++;
	      entry_offset = 0;
	    }
	}
      else if(p_unpack->status & NM_UNPACK_TYPE_DATATYPE)
	{
	  padico_fatal("nmad: copy data for datatype- not supported");
	}	    
    }
}

/** mark 'chunk_len' data as received in the given unpack, and check for unpack completion */
static inline void nm_so_unpack_check_completion(struct nm_core*p_core, struct nm_unpack_s*p_unpack, nm_len_t chunk_len)
{
  p_unpack->cumulated_len += chunk_len;
  if(p_unpack->cumulated_len == p_unpack->expected_len)
    {
#warning Paulette: lock
      tbx_fast_list_del(&p_unpack->_link);
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNPACK_COMPLETED,
	  .p_unpack = p_unpack
	};
      nm_core_status_event(p_core, &event, &p_unpack->status);
    }
  else if(p_unpack->cumulated_len > p_unpack->expected_len)
    {
      fprintf(stderr, "# nmad: received more data than expected in unpack (unpacked = %d; expected = %d; chunk = %d).\n",
	      p_unpack->cumulated_len, p_unpack->expected_len, chunk_len);
      abort();
    }
}

/** decode and manage data flags found in data/short_data/rdv packets */
static inline void nm_so_data_flags_decode(struct nm_unpack_s*p_unpack, uint8_t flags,
					   nm_len_t chunk_offset, nm_len_t chunk_len)
{
  const nm_len_t chunk_end = chunk_offset + chunk_len;
  if(chunk_end > p_unpack->expected_len)
    {
      fprintf(stderr, "# nmad: received more data than expected (received = %d; expected = %d).\n",
	      chunk_offset + chunk_len, p_unpack->expected_len);
      abort();
    }
  if(flags & NM_PROTO_FLAG_LASTCHUNK)
    {
      /* Update the real size to receive */
      assert(chunk_end <= p_unpack->expected_len);
      p_unpack->expected_len = chunk_end;
    }
  if((p_unpack->cumulated_len == 0) && (flags & NM_PROTO_FLAG_ACKREQ))
    nm_so_post_ack(p_unpack->p_gate, p_unpack->tag, p_unpack->seq);
}

/** store an unexpected chunk of data (data/short_data/rdv) */
static inline void nm_unexpected_store(struct nm_core*p_core, struct nm_gate*p_gate,
				       const void *header, nm_len_t len, nm_core_tag_t tag, nm_seq_t seq,
				       struct nm_pkt_wrap *p_pw)
{
  struct nm_unexpected_s*chunk = nm_unexpected_alloc();
  chunk->header = (void*)header;
  chunk->p_pw = p_pw;
  chunk->p_gate = p_gate;
  chunk->seq = seq;
  chunk->tag = tag;
  nm_pw_ref_inc(p_pw);
  nm_unexpected_mem_size++;
  if(nm_unexpected_mem_size > 32*1024)
    {
      fprintf(stderr, "nmad: WARNING- %d unexpected chunks allocated.\n", (int)nm_unexpected_mem_size);
      if(nm_unexpected_mem_size > 64*1024)
	TBX_FAILUREF("nmad: FATAL- %d unexpected chunks allocated; giving up.\n", (int)nm_unexpected_mem_size);
    }
#warning Paulette: lock
  tbx_fast_list_add_tail(&chunk->link, &p_core->unexpected);
}

void nm_core_unpack_iov(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const struct iovec*iov, int num_entries)
{ 
  p_unpack->status = NM_UNPACK_TYPE_IOV;
  p_unpack->data = (void*)iov;
  p_unpack->cumulated_len = 0;
  p_unpack->expected_len = nm_so_iov_len(iov, num_entries);

}

void nm_core_unpack_datatype(struct nm_core*p_core, struct nm_unpack_s*p_unpack, void*_datatype)
{ 
  p_unpack->status = NM_UNPACK_TYPE_DATATYPE;
  padico_fatal("nmad: unpack datatype- not supported.");
}

/** Handle an unpack request.
 */
int nm_core_unpack_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate,
			nm_core_tag_t tag, nm_core_tag_t tag_mask)
{
  nmad_lock();
  nm_lock_interface(p_core);
  /* fill-in the unpack request */
  p_unpack->status |= NM_STATUS_UNPACK_POSTED;
  p_unpack->p_gate = p_gate;
  p_unpack->tag = tag;
  p_unpack->tag_mask = tag_mask;
  p_unpack->seq = NM_SEQ_NONE;
  /* store the unpack request */
#warning Paulette: lock
  tbx_fast_list_add_tail(&p_unpack->_link, &p_core->unpacks);
  struct nm_unexpected_s*chunk = nm_unexpected_find_matching(p_core, p_unpack);
  while(chunk)
    {
      /* data is already here (at least one chunk)- process all matching chunks */
      void*header = chunk->header;
      const nm_proto_t proto_id = (*(nm_proto_t *)header) & NM_PROTO_ID_MASK;
      switch(proto_id)
	{
	case NM_PROTO_SHORT_DATA:
	  {
	    struct nm_so_short_data_header *h = header;
	    const void *ptr = header + NM_SO_SHORT_DATA_HEADER_SIZE;
	    nm_short_data_handler(p_core, p_gate, p_unpack, ptr, h, NULL);
	  }
	  break;
	case NM_PROTO_DATA:
	  {
	    struct nm_so_data_header *h = header;
	    const void *ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;
	    nm_small_data_handler(p_core, p_gate, p_unpack, ptr, h, NULL);
	  }
	  break;
	case NM_PROTO_RDV:
	  {
	    struct nm_so_ctrl_rdv_header *rdv = header;
	    nm_rdv_handler(p_core, p_gate, p_unpack, rdv, NULL);
	  }
	  break;
	}
      /* Decrement the packet wrapper reference counter. If no other
	 chunks are still in use, the pw will be destroyed. */
      nm_pw_ref_dec(chunk->p_pw);
#warning Paulette: lock
      tbx_fast_list_del(&chunk->link);
      nm_unexpected_free(nm_unexpected_allocator, chunk);
      nm_unexpected_mem_size--;
      if(p_unpack->cumulated_len < p_unpack->expected_len)
	chunk = nm_unexpected_find_matching(p_core, p_unpack);
      else
	chunk= NULL;
    }
  nmad_unlock();
  nm_unlock_interface(p_core);
  return NM_ESUCCESS;
}

int nm_core_iprobe(struct nm_core*p_core,
		   struct nm_gate*p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask,
		   struct nm_gate**pp_out_gate, nm_core_tag_t*p_out_tag, nm_len_t*p_out_size)
{
  struct nm_unexpected_s*chunk;
  tbx_fast_list_for_each_entry(chunk, &p_core->unexpected, link)
    {
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&chunk->p_gate->tags, chunk->tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(((p_gate == chunk->p_gate) || (p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_tag_match(chunk->tag, tag, tag_mask) && /* tag matches */
	 (chunk->seq == next_seq) /* seq number matches */ )
	{
	  const void*header = chunk->header;
	  const nm_proto_t proto_id = (*(nm_proto_t *)header) & NM_PROTO_ID_MASK;
	  int matches = 0;
	  nm_len_t len = -1;
	  switch(proto_id)
	    {
	    case NM_PROTO_SHORT_DATA:
	      {
		const struct nm_so_short_data_header *h = header;
		len = h->len;
		matches = 1;
	      }
	      break;
	    case NM_PROTO_DATA:
	      {
		const struct nm_so_data_header *h = header;
		if(h->flags & NM_PROTO_FLAG_LASTCHUNK)
		  {
		    len = h->len + h->chunk_offset;
		    matches = 1;
		  }
	      }
	      break;
	    case NM_PROTO_RDV:
	      {
		const struct nm_so_ctrl_rdv_header *rdv = header;
		if(rdv->flags & NM_PROTO_FLAG_LASTCHUNK)
		  {
		    len = rdv->len + rdv->chunk_offset;
		    matches = 1;
		  }
	      }
	      break;
	    }
	  if(matches)
	    {
	      if(pp_out_gate)
		*pp_out_gate = chunk->p_gate;
	      if(p_out_tag)
		*p_out_tag = chunk->tag;
	      if(p_out_size)
		*p_out_size = len;
	      return NM_ESUCCESS;
	    }
	}
    }
  *pp_out_gate = NM_ANY_GATE;
  return -NM_EAGAIN;
}

int nm_core_unpack_cancel(struct nm_core*p_core, struct nm_unpack_s*p_unpack)
{
  if(p_unpack->status & (NM_STATUS_UNPACK_COMPLETED | NM_STATUS_UNEXPECTED))
    {
      /* receive is already in progress- too late to cancel */
      return -NM_EINPROGRESS;
    }
  else if(p_unpack->seq == NM_SEQ_NONE)
    {
#warning Paulette: lock
      tbx_fast_list_del(&p_unpack->_link);
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNPACK_CANCELLED,
	  .p_unpack = p_unpack
	};
      nm_core_status_event(p_core, &event, &p_unpack->status);
      return NM_ESUCCESS;
    }
  else
    return -NM_ENOTIMPL;
}

/** Process a short data request (NM_PROTO_SHORT_DATA)- p_unpack may be NULL (unexpected)
 */
static void nm_short_data_handler(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_unpack_s*p_unpack,
				  const void*ptr, const nm_so_short_data_header_t*h, struct nm_pkt_wrap *p_pw)
{
  const nm_len_t len = h->len;
  const nm_len_t chunk_offset = 0;
  if(p_unpack)
    {
      if(!(p_unpack->status & NM_STATUS_UNPACK_CANCELLED))
	{
	  const uint8_t flags = NM_PROTO_FLAG_LASTCHUNK;
	  nm_so_data_flags_decode(p_unpack, flags, chunk_offset, len);
	  nm_so_copy_data(p_unpack, chunk_offset, ptr, len);
	  nm_so_unpack_check_completion(p_core, p_unpack, len);
	}
    }
  else
    {
      const nm_core_tag_t tag = h->tag_id;
      const nm_seq_t seq = h->seq;
      nm_unexpected_store(p_core, p_gate, h, len, tag, seq, p_pw);
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNEXPECTED,
	  .p_gate = p_gate,
	  .tag = tag,
	  .seq = seq,
	  .len = len
	};
      nm_core_status_event(p_pw->p_gate->p_core, &event, NULL);
    }
}

/** Process a small data request (NM_PROTO_DATA)- p_unpack may be NULL (unexpected)
 */
static void nm_small_data_handler(struct nm_core*p_core, struct nm_gate*p_gate,  struct nm_unpack_s*p_unpack,
				  const void*ptr, const nm_so_data_header_t*h, struct nm_pkt_wrap *p_pw)
{
  const nm_len_t chunk_len = h->len;
  const nm_len_t chunk_offset = h->chunk_offset;
  if(p_unpack)
    {
      if(!(p_unpack->status & NM_STATUS_UNPACK_CANCELLED))
	{
	  nm_so_data_flags_decode(p_unpack, h->flags, chunk_offset, chunk_len);
	  nm_so_copy_data(p_unpack, chunk_offset, ptr, chunk_len);
	  nm_so_unpack_check_completion(p_core, p_unpack, chunk_len);
	}
    }
  else
    {
      const nm_core_tag_t tag = h->tag_id;
      const nm_seq_t seq = h->seq;
      nm_unexpected_store(p_core, p_gate, h, chunk_len, tag, seq, p_pw);
      if(h->flags & NM_PROTO_FLAG_LASTCHUNK)
	{
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_UNEXPECTED,
	      .p_gate = p_gate,
	      .tag = tag,
	      .seq = seq,
	      .len = chunk_offset + chunk_len
	    };
	  nm_core_status_event(p_core, &event, NULL);
	}
    }
}

/** Process a received rendez-vous request (NM_PROTO_RDV)- 
 * either p_unpack may be NULL (storing unexpected) or p_pw (unpacking unexpected)
 */
static void nm_rdv_handler(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_unpack_s*p_unpack,
			   const struct nm_so_ctrl_rdv_header*h, struct nm_pkt_wrap *p_pw)
{
  const nm_len_t chunk_len = h->len;
  const nm_len_t chunk_offset = h->chunk_offset;
  if(p_unpack)
    {
      nm_so_data_flags_decode(p_unpack, h->flags, chunk_offset, chunk_len);
      nm_so_rdv_success(p_core, p_unpack, chunk_len, chunk_offset);
    }
  else
    {
      const nm_core_tag_t tag = h->tag_id;
      const nm_seq_t seq = h->seq;
      nm_unexpected_store(p_core, p_gate, h, chunk_len, tag, seq, p_pw);
      if(h->flags & NM_PROTO_FLAG_LASTCHUNK)
	{
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_UNEXPECTED,
	      .p_gate = p_gate,
	      .tag = tag,
	      .seq = seq,
	      .len = chunk_offset + chunk_len
	    };
	  nm_core_status_event(p_core, &event, NULL);
	}
    }
}

/** Process a complete rendez-vous ready-to-receive request.
 */
static void nm_rtr_handler(struct nm_pkt_wrap *p_rtr_pw, const struct nm_so_ctrl_rtr_header*header)
{
  const nm_core_tag_t tag     = header->tag_id;
  const nm_seq_t seq          = header->seq;
  const nm_len_t chunk_offset = header->chunk_offset;
  const nm_len_t chunk_len    = header->chunk_len;
  struct nm_gate *p_gate      = p_rtr_pw->p_gate;
  struct nm_pkt_wrap *p_large_pw = NULL;
  tbx_fast_list_for_each_entry(p_large_pw, &p_gate->pending_large_send, link)
    {
      assert(p_large_pw->n_completions == 1);
      const struct nm_pw_contrib_s*p_contrib = &p_large_pw->completions[0].data.contrib;
      const struct nm_pack_s*p_pack = p_contrib->p_pack;
      NM_TRACEF("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n",
		p_pack->seq, p_large_pw->chunk_offset);
      if((p_pack->seq == seq) && nm_tag_eq(p_pack->tag, tag) && (p_large_pw->chunk_offset == chunk_offset))
	{
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_large_pw, p_gate, 1/* large output list*/);
#warning Paulette: lock
	  tbx_fast_list_del(&p_large_pw->link);
	  if(chunk_len < p_large_pw->length)
	    {
	      /* ** partial RTR- split the packet  */
	      nm_so_pw_finalize(p_large_pw);
	      /* assert ack is partial */
	      assert(chunk_len > 0 && chunk_len < p_large_pw->length);
	      /* create a new pw with the remaining data */
	      struct nm_pkt_wrap *p_pw2 = NULL;
	      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw2);
	      p_pw2->p_drv    = p_large_pw->p_drv;
	      p_pw2->trk_id   = p_large_pw->trk_id;
	      p_pw2->p_gate   = p_gate;
	      p_pw2->p_gdrv   = p_large_pw->p_gdrv;
	      p_pw2->p_unpack = NULL;
	      /* this is a large pw with rdv- it must contain a single contrib */
	      assert(p_large_pw->n_completions == 1);
	      nm_pw_add_contrib(p_pw2, p_large_pw->completions[0].data.contrib.p_pack, p_large_pw->completions[0].data.contrib.len - chunk_len);
	      p_large_pw->completions[0].data.contrib.len = chunk_len; /* truncate the contrib */
	      /* populate p_pw2 iovec */
	      nm_so_pw_split_data(p_large_pw, p_pw2, chunk_len);
#warning Paulette: lock
	      tbx_fast_list_add(&p_pw2->link, &p_gate->pending_large_send);
	    }
	  /* send the data */
	  nm_drv_t p_drv = nm_drv_get_by_index(p_gate, header->drv_index);
	  nm_core_post_send(p_gate, p_large_pw, header->trk_id, p_drv);
	  return;
	}
    }
  fprintf(stderr, "nmad: FATAL- cannot find matching packet for received RTR: seq = %d; offset = %d- dumping pending large packets\n", seq, chunk_offset);
  tbx_fast_list_for_each_entry(p_large_pw, &p_gate->pending_large_send, link)
    {
      const struct nm_pack_s*p_pack = p_large_pw->completions[0].data.contrib.p_pack;
      fprintf(stderr, "  packet- seq = %d; chunk_offset = %d\n", p_pack->seq, p_large_pw->chunk_offset);
    }
  abort();
}
/** Process an acknowledgement.
 */
static void nm_ack_handler(struct nm_pkt_wrap *p_ack_pw, const struct nm_so_ctrl_ack_header*header)
{
  struct nm_core*p_core = p_ack_pw->p_gate->p_core;
  const nm_core_tag_t tag = header->tag_id;
  const nm_seq_t seq = header->seq;
  struct nm_pack_s*p_pack = NULL;
  tbx_fast_list_for_each_entry(p_pack, &p_core->pending_packs, _link)
    {
      if(nm_tag_eq(p_pack->tag, tag) && p_pack->seq == seq)
	{
#warning Paulette: lock
	  tbx_fast_list_del(&p_pack->_link);
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_ACK_RECEIVED,
	      .p_pack = p_pack
	    };
	  nm_core_status_event(p_core, &event, &p_pack->status);
	  return;
	}
    }
}

/** decode one chunk of headers.
 * @returns the number of processed bytes in global header, 
 *          -1 if done (last chunk)
 */
int nm_decode_header_chunk(struct nm_core*p_core, const void*ptr, struct nm_pkt_wrap *p_pw, struct nm_gate*p_gate)
{
  int rc = 0;
  const nm_proto_t proto_id = (*(nm_proto_t *)ptr) & NM_PROTO_ID_MASK;
  const nm_proto_t proto_flag = (*(nm_proto_t *)ptr) & NM_PROTO_FLAG_MASK;

  assert(proto_id != 0);
  switch(proto_id)
    {
    case NM_PROTO_SHORT_DATA:
      {
	const struct nm_so_short_data_header*sh = ptr;
	rc = NM_SO_SHORT_DATA_HEADER_SIZE;
	const nm_len_t len = sh->len;
	const nm_core_tag_t tag = sh->tag_id;
	const nm_seq_t seq = sh->seq;
	struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	nm_short_data_handler(p_core, p_gate, p_unpack, ptr + NM_SO_SHORT_DATA_HEADER_SIZE, sh, p_pw);
	rc += len;
      }
      break;
      
    case NM_PROTO_DATA:
      {
	/* Data header */
	const struct nm_so_data_header*dh = ptr;
	rc = NM_SO_DATA_HEADER_SIZE;
	/* Retrieve data location */
	const unsigned long skip = dh->skip;
	const void*data = ptr + NM_SO_DATA_HEADER_SIZE + skip;
	assert(p_pw->v_nb == 1);
	assert(data + dh->len <= p_pw->v->iov_base + p_pw->v->iov_len);
	const unsigned long size = (dh->flags & NM_PROTO_FLAG_ALIGNED) ? nm_so_aligned(dh->len) : dh->len;
	if(skip == 0)
	  { /* data is just after the header */
	    rc += size;
	  }  /* else the next header is just behind */
	const nm_core_tag_t tag = dh->tag_id;
	const nm_seq_t seq = dh->seq;
	struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	nm_small_data_handler(p_core, p_gate, p_unpack, data, dh, p_pw);
      }
      break;
      
    case NM_PROTO_RDV:
      {
	const union nm_so_generic_ctrl_header *ch = ptr;
	rc = NM_SO_CTRL_HEADER_SIZE;
	const nm_core_tag_t tag = ch->rdv.tag_id;
	const nm_seq_t seq = ch->rdv.seq;
	struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	nm_rdv_handler(p_core, p_gate, p_unpack, &ch->rdv, p_pw);
      }
      break;
      
    case NM_PROTO_RTR:
      {
	const union nm_so_generic_ctrl_header *ch = ptr;
	rc = NM_SO_CTRL_HEADER_SIZE;
	nm_rtr_handler(p_pw, &ch->rtr);
      }
      break;
      
    case NM_PROTO_ACK:
      {
	const union nm_so_generic_ctrl_header *ch = ptr;
	rc = NM_SO_CTRL_HEADER_SIZE;
	nm_ack_handler(p_pw, &ch->ack);
      }
      break;
      
    case NM_PROTO_STRAT:
      {
	const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	const struct nm_strat_header*h = ptr;
	if(strategy->driver->proto)
	  {
	    (*strategy->driver->proto)(strategy->_status, p_gate, p_pw, ptr, h->size);
	  }
	else
	  {
	    padico_fatal("nmad: strategy cannot process NM_PROTO_STRAT.\n");
	  }
	rc = h->size;
      }
      break;
      
    default:
      padico_fatal("nmad: received header with invalid proto_id %d\n", proto_id);
      break;
      
    }
  if(proto_flag & NM_PROTO_LAST)
    rc = -1;
  return rc;
}


/** Process a complete incoming request.
 */
int nm_so_process_complete_recv(struct nm_core *p_core,	struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  int err;

  assert(p_gate != NULL);

  nm_lock_interface(p_core);

  /* clear the input request field */
  if(p_pw->p_gdrv && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
    {
      /* request was posted on a given gate */
      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
      assert(p_pw->p_gdrv->active_recv[p_pw->trk_id] == 1);
      p_pw->p_gdrv->active_recv[p_pw->trk_id] = 0;
    }
  else if((!p_pw->p_gdrv) && p_pw->p_drv->p_in_rq == p_pw)
    {
      /* request was posted on a driver, for any gate */
      p_pw->p_drv->p_in_rq = NULL;
    }

#ifdef NMAD_POLL
  tbx_fast_list_del(&p_pw->link);
#endif /* NMAD_POLL */

  nm_unlock_interface(p_core);

  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      /* ** Small packets - track #0 *********************** */
      struct nm_gate*p_gate = p_pw->p_gate;
      struct nm_core*p_core = p_gate->p_core;
      struct iovec *vec = p_pw->v;
      void *ptr = vec->iov_base;
      nm_len_t done = 0;
      while(done != -1)
	{
	  /* Iterate over header chunks */
	  assert(ptr < vec->iov_base + vec->iov_len);
	  done = nm_decode_header_chunk(p_core, ptr, p_pw, p_gate);
	  ptr += done;
	}
    }
  else if(p_pw->trk_id == NM_TRK_LARGE)
    {
      /* ** Large packet - track #1 ************************ */
      struct nm_unpack_s*p_unpack = p_pw->p_unpack;
      const nm_len_t len = p_pw->length;
      /* ** Large packet, data received directly in its final destination */
      nm_so_unpack_check_completion(p_core, p_unpack, len);
      nm_so_process_large_pending_recv(p_gate);
    }
  
  nm_pw_ref_dec(p_pw);

  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;
  return err;
}


