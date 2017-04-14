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
 * MERCHANTABILITY or FITNESS FOR A PARTICULA R PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>

PADICO_MODULE_HOOK(NewMad_Core);

static void nm_pkt_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				const struct nm_header_pkt_data_s*h, struct nm_pkt_wrap_s *p_pw);
static void nm_short_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				  const struct nm_header_short_data_s*h, struct nm_pkt_wrap_s *p_pw);
static void nm_small_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				  const struct nm_header_data_s*h, struct nm_pkt_wrap_s *p_pw);
static void nm_rdv_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s*p_unpack,
			   const struct nm_header_ctrl_rdv_s*h, struct nm_pkt_wrap_s *p_pw);

/* ********************************************************* */
/* ** unexpected/matching */


/** fast allocator for struct nm_unexpected_s */
PUK_ALLOCATOR_TYPE(nm_unexpected, struct nm_unexpected_s);
static nm_unexpected_allocator_t nm_unexpected_allocator = NULL;

#define NM_UNEXPECTED_PREALLOC 256

static inline struct nm_unexpected_s*nm_unexpected_alloc(void)
{
  if(tbx_unlikely(nm_unexpected_allocator == NULL))
    {
      nm_unexpected_allocator = nm_unexpected_allocator_new(NM_UNEXPECTED_PREALLOC);
    }
  struct nm_unexpected_s*p_unexpected = nm_unexpected_malloc(nm_unexpected_allocator);
  nm_unexpected_list_cell_init(p_unexpected);
  return p_unexpected;
}

void nm_unexpected_clean(struct nm_core*p_core)
{
  struct nm_unexpected_s*p_chunk = nm_unexpected_list_pop_front(&p_core->unexpected);
  while(p_chunk)
    {
#ifdef DEBUG
      NM_DISPF("nmad: WARNING- chunk %p is still in use (gate = %p; seq = %d; tag = %llx:%llx)\n",
	      p_chunk, p_chunk->p_gate, p_chunk->seq,
	      (unsigned long long)nm_core_tag_get_hashcode(p_chunk->tag), (unsigned long long)nm_core_tag_get_tag(p_chunk->tag));
#endif /* DEBUG */
      if(p_chunk->p_pw)
	{
#if(defined(PIOMAN))
	  piom_ltask_completed(&p_chunk->p_pw->ltask);
#else /* PIOMAN */
	  nm_pw_ref_dec(p_chunk->p_pw);
#endif /* PIOMAN */
	}
      nm_unexpected_free(nm_unexpected_allocator, p_chunk);
      p_chunk = nm_unexpected_list_pop_front(&p_core->unexpected);
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
static struct nm_unexpected_s*nm_unexpected_find_matching(struct nm_core*p_core, struct nm_req_s*p_unpack)
{
  struct nm_unexpected_s*p_chunk;
  assert(nm_status_test(p_unpack, NM_STATUS_UNPACK_POSTED));
  assert(!nm_status_test(p_unpack, NM_STATUS_FINALIZED));
  nm_core_lock_assert(p_core);
  puk_list_foreach(p_chunk, &p_core->unexpected)
    {
      struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_chunk->p_gate->tags, p_chunk->tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(((p_unpack->p_gate == p_chunk->p_gate) || (p_unpack->p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_core_tag_match(p_chunk->tag, p_unpack->tag, p_unpack->unpack.tag_mask) && /* tag matches */
	 ((p_unpack->seq == p_chunk->seq) || ((p_unpack->seq == NM_SEQ_NONE) && (p_chunk->seq == next_seq))) /* seq number matches */ ) 
	{
	  if(p_unpack->seq == NM_SEQ_NONE)
	    {
	      p_so_tag->recv_seq_number = next_seq;
	    }
	  p_unpack->tag      = p_chunk->tag;
	  p_unpack->unpack.tag_mask = NM_CORE_TAG_MASK_FULL;
	  p_unpack->p_gate   = p_chunk->p_gate;
	  p_unpack->seq      = p_chunk->seq;
	  return p_chunk;
	}
    }
  return NULL;
}

/** Find an unpack that matches a given packet that arrived from [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
static struct nm_req_s*nm_unpack_find_matching(struct nm_core*p_core, nm_gate_t p_gate, nm_seq_t seq, nm_core_tag_t tag)
{
  struct nm_req_s*p_unpack = NULL;
  struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_gate->tags, tag);
  const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
  nm_core_lock_assert(p_core);
  puk_list_foreach(p_unpack, &p_core->unpacks)
    {
      assert(nm_status_test(p_unpack, NM_STATUS_UNPACK_POSTED));
      assert(!nm_status_test(p_unpack, NM_STATUS_FINALIZED));
      if(((p_unpack->p_gate == p_gate) || (p_unpack->p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_core_tag_match(tag, p_unpack->tag, p_unpack->unpack.tag_mask) && /* tag matches */
	 ((p_unpack->seq == seq) || ((p_unpack->seq == NM_SEQ_NONE) && (seq == next_seq))) /* seq number matches */ ) 
	{
	  if(p_unpack->seq == NM_SEQ_NONE)
	    {
	      p_so_tag->recv_seq_number = next_seq;
	    }
	  p_unpack->tag      = tag;
	  p_unpack->unpack.tag_mask = NM_CORE_TAG_MASK_FULL;
	  p_unpack->p_gate   = p_gate;
	  p_unpack->seq      = seq;
	  return p_unpack;
	}
    }
  return NULL;
}


/** register a large pending pw for each chunk in the rdv
 */
struct nm_large_chunk_context_s
{
  struct nm_req_s*p_unpack;
  nm_gate_t p_gate;
  nm_len_t chunk_offset;
};
static void nm_large_chunk_store(void*ptr, nm_len_t len, void*_context)
{
  struct nm_large_chunk_context_s*p_large_chunk = _context;
  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_noheader();
  p_pw->p_unpack = p_large_chunk->p_unpack;
  nm_pw_add_raw(p_pw, ptr, len, p_large_chunk->chunk_offset);
  assert(p_pw->p_drv == NULL);
  p_pw->p_gate = p_large_chunk->p_gate;
  nm_pkt_wrap_list_push_back(&p_large_chunk->p_gate->pending_large_recv, p_pw);
  p_large_chunk->chunk_offset += len;
}

/** mark 'chunk_len' data as received in the given unpack, and check for unpack completion, sets pp_unpack to NULL if finalized */
static inline void nm_so_unpack_check_completion(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw, struct nm_req_s**pp_unpack, nm_len_t chunk_len)
{
  struct nm_req_s*p_unpack = *pp_unpack;
  p_unpack->unpack.cumulated_len += chunk_len;
  if(p_unpack->unpack.cumulated_len == p_unpack->unpack.expected_len)
    {
      nm_core_lock_assert(p_core);
      nm_req_list_erase(&p_core->unpacks, p_unpack);
      nm_core_polling_level(p_core);
      if((p_pw != NULL) && (p_pw->trk_id == NM_TRK_LARGE) && (p_pw->flags & NM_PW_DYNAMIC_V0))
	{
	  nm_data_copy_to(&p_unpack->data, 0 /* offset */, chunk_len, p_pw->v[0].iov_base);
	}
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNPACK_COMPLETED | NM_STATUS_FINALIZED,
	  .p_req = p_unpack
	};
      nm_core_status_event(p_core, &event, p_unpack);
      *pp_unpack = NULL;
    }
  else if(p_unpack->unpack.cumulated_len > p_unpack->unpack.expected_len)
    {
      NM_FATAL("copied more data than expected in unpack (unpacked = %lu; expected = %lu; chunk = %lu).\n",
	       p_unpack->unpack.cumulated_len, p_unpack->unpack.expected_len, chunk_len);
      abort();
    }
}

/** decode and manage data flags found in data/short_data/rdv packets */
static inline void nm_so_data_flags_decode(struct nm_req_s*p_unpack, uint8_t flags, nm_len_t chunk_offset, nm_len_t chunk_len)
{
  const nm_len_t chunk_end = chunk_offset + chunk_len;
  assert(p_unpack->unpack.expected_len != NM_LEN_UNDEFINED);
  if(chunk_end > p_unpack->unpack.expected_len)
    {
      NM_FATAL("received more data than expected-"
	       "   received: chunk offset = %lu; chunk len = %lu; chunk end = %lu\n"
	       "   matched request = %p; expected len = %lu; already received = %lu\n",
	       chunk_offset, chunk_len, chunk_end,
	       p_unpack, p_unpack->unpack.expected_len, p_unpack->unpack.cumulated_len);
      abort();
    }
  assert(chunk_end <= p_unpack->unpack.expected_len);
  assert(p_unpack->unpack.cumulated_len + chunk_len <= p_unpack->unpack.expected_len);
  if(flags & NM_PROTO_FLAG_LASTCHUNK)
    {
      /* Update the real size to receive */
      p_unpack->unpack.expected_len = chunk_end;
    }
  if((p_unpack->unpack.cumulated_len == 0) && (flags & NM_PROTO_FLAG_ACKREQ))
    {
      nm_core_post_ack(p_unpack->p_gate, p_unpack->tag, p_unpack->seq);
    }
}

/** store an unexpected chunk of data (data/short_data/rdv) */
static inline void nm_unexpected_store(struct nm_core*p_core, nm_gate_t p_gate, const struct nm_header_generic_s*p_header,
				       nm_len_t chunk_offset, nm_len_t chunk_len, nm_core_tag_t tag, nm_seq_t seq,
				       struct nm_pkt_wrap_s *p_pw)
{
  struct nm_unexpected_s*p_chunk = nm_unexpected_alloc();
  p_chunk->p_header = p_header;
  p_chunk->p_pw   = p_pw;
  p_chunk->p_gate = p_gate;
  p_chunk->seq    = seq;
  p_chunk->tag    = tag;
  nm_pw_ref_inc(p_pw);
#ifdef DEBUG
  const long unsigned int nm_unexpected_mem_size = nm_unexpected_list_size(&p_core->unexpected);
  if(nm_unexpected_mem_size > 32*1024)
    {
      NM_WARN("%lu unexpected chunks allocated.\n", nm_unexpected_mem_size);
      if(nm_unexpected_mem_size > 64*1024)
	NM_FATAL("%lu unexpected chunks allocated; giving up.\n", nm_unexpected_mem_size);
    }
#endif /* DEBUG */
  nm_core_lock_assert(p_core);
  nm_unexpected_list_push_back(&p_core->unexpected, p_chunk);
  const nm_proto_t proto_id = p_header->proto_id;
  if(proto_id & NM_PROTO_FLAG_LASTCHUNK)
    {
      p_chunk->msg_len = chunk_offset + chunk_len;
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNEXPECTED,
	  .p_gate = p_gate,
	  .tag    = tag,
	  .seq    = seq,
	  .len    = chunk_offset + chunk_len
	};
      nm_core_status_event(p_core, &event, NULL);
    }
  else
    {
      p_chunk->msg_len = NM_LEN_UNDEFINED;
    }
}

void nm_core_unpack_init(struct nm_core*p_core, struct nm_req_s*p_unpack)
{
  nm_status_init(p_unpack, NM_STATUS_UNPACK_INIT);
  nm_req_list_cell_init(p_unpack);
#ifdef DEBUG
  nm_data_null_build(&p_unpack->data);
#endif
  p_unpack->flags   = NM_FLAG_UNPACK;
  p_unpack->monitor = NM_MONITOR_NULL;
  p_unpack->unpack.cumulated_len = 0;
  p_unpack->unpack.expected_len  = NM_LEN_UNDEFINED;
}

void nm_core_unpack_data(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_data_s*p_data)
{
  nm_status_assert(p_unpack, NM_STATUS_UNPACK_INIT);
  p_unpack->data = *p_data;
  p_unpack->unpack.expected_len = nm_data_size(p_data);
}

static inline void nm_core_unpack_match(struct nm_core*p_core, struct nm_req_s*p_unpack,
					nm_gate_t p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask, nm_seq_t seq)
{
  /* fill-in the unpack request */
  nm_status_assert(p_unpack, NM_STATUS_UNPACK_INIT);
  p_unpack->p_gate = p_gate;
  p_unpack->seq    = seq;
  p_unpack->tag    = tag;
  p_unpack->unpack.tag_mask = tag_mask;
  p_unpack->flags |= NM_FLAG_UNPACK_MATCHED;
}
void nm_core_unpack_match_event(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_core_event_s*p_event)
{
  nm_core_unpack_match(p_core, p_unpack, p_event->p_gate, p_event->tag, NM_CORE_TAG_MASK_FULL, p_event->seq);
}

/** Handle an unpack request.
 */
void nm_core_unpack_match_recv(struct nm_core*p_core, struct nm_req_s*p_unpack, nm_gate_t p_gate,
			       nm_core_tag_t tag, nm_core_tag_t tag_mask)
{
  nm_core_unpack_match(p_core, p_unpack, p_gate, tag, tag_mask, NM_SEQ_NONE);
}

/** probes whether an incoming packet matched this request */
int nm_core_unpack_iprobe(struct nm_core*p_core, struct nm_req_s*p_unpack)
{
  if(!(p_unpack->flags & NM_FLAG_UNPACK_MATCHED))
    {
      NM_WARN("cannot probe unmatched request.\n");
      return -NM_EINVAL;
    }
  struct nm_unexpected_s*p_unexpected = nm_unexpected_find_matching(p_core, p_unpack);
  if(p_unexpected == NULL)
    {
      nm_schedule(p_core);
      return -NM_EAGAIN;
    }
  else
    return NM_ESUCCESS;
}

int nm_core_unpack_peek(struct nm_core*p_core, struct nm_req_s*p_unpack, const struct nm_data_s*p_data)
{
  if((p_unpack->seq == NM_SEQ_NONE) || (p_unpack->p_gate == NM_GATE_NONE) || !(p_unpack->flags & NM_FLAG_UNPACK_MATCHED))
    {
      NM_WARN("cannot peek unmatched request.\n");
      return -NM_EINVAL;
    }
  nm_len_t peek_len = nm_data_size(p_data);
  nm_len_t done = 0;
  nm_core_lock(p_core);
  struct nm_unexpected_s*p_chunk;
  puk_list_foreach(p_chunk, &p_core->unexpected)
    {
      if((p_unpack->p_gate == p_chunk->p_gate) && /* gate matches */
	 nm_core_tag_match(p_chunk->tag, p_unpack->tag, p_unpack->unpack.tag_mask) && /* tag matches */
	 (p_unpack->seq == p_chunk->seq) /* seq number matches */ )
	{
	  const nm_header_data_generic_t*p_header = (const nm_header_data_generic_t*)p_chunk->p_header;
	  const nm_proto_t proto_id = p_chunk->p_header->proto_id & NM_PROTO_ID_MASK;
	  switch(proto_id)
	    {
	    case NM_PROTO_PKT_DATA:
	      {
		nm_len_t chunk_len    = p_header->pkt_data.data_len;
		nm_len_t chunk_offset = p_header->pkt_data.chunk_offset;
		if(chunk_offset < peek_len)
		  {
		    if(chunk_offset + chunk_len > peek_len)
		      {
			chunk_len = peek_len - chunk_offset;
		      }
		    nm_data_pkt_unpack(p_data, &p_header->pkt_data, p_chunk->p_pw, chunk_offset, chunk_len);
		    done += chunk_len;
		  }
	      }
	      break;
	    case NM_PROTO_SHORT_DATA:
	      {
		nm_len_t chunk_len    = p_header->short_data.len;
		nm_len_t chunk_offset = 0;
		const void*ptr = ((void*)p_header) + NM_HEADER_SHORT_DATA_SIZE;
		if(chunk_offset < peek_len)
		  {
		    if(chunk_offset + chunk_len > peek_len)
		      {
			chunk_len = peek_len - chunk_offset;
		      }
		    nm_data_copy_to(p_data, chunk_offset, chunk_len, ptr);
		    done += chunk_len;
		  }
	      }
	      break;
	    case NM_PROTO_DATA:
	      {
		nm_len_t chunk_len    = p_header->data.len;
		nm_len_t chunk_offset = p_header->data.chunk_offset;
		const void*ptr = (p_header->data.skip == 0xFFFF) ? (((void*)p_header) + NM_HEADER_DATA_SIZE) :
		  p_chunk->p_pw->v[0].iov_base + (p_header->data.skip + nm_header_global_v0len(p_chunk->p_pw));
		if(chunk_offset < peek_len)
		  {
		    if(chunk_offset + chunk_len > peek_len)
		      {
			chunk_len = peek_len - chunk_offset;
		      }
		    nm_data_copy_to(p_data, chunk_offset, chunk_len, ptr);
		    done += chunk_len;
		  }
	      }
	      break;	      
	    case NM_PROTO_RDV:
	      {
		const nm_header_ctrl_generic_t*p_ctrl_header = (const nm_header_ctrl_generic_t*)p_chunk->p_header;
		const nm_len_t chunk_offset = p_ctrl_header->rdv.chunk_offset;
		if(chunk_offset < peek_len)
		  {
		    NM_FATAL("# nmad: nm_core_unpack_peek()- not implemented for large messages. Use nm_sr_send_header() to send header eagerly.\n");
		  }
		else
		  {
		    /* rdv chunk not in peeked heder- ignore */
		  }
	      }
	      break;
	    default:
	      NM_FATAL("unknown proto_id %d", proto_id);
	      break;
	    }
	}
      if(done == peek_len)
	break;
    }
  nm_core_unlock(p_core);
  if(done == peek_len)
    return NM_ESUCCESS;
  else
    return -NM_EAGAIN;
}

int nm_core_unpack_submit(struct nm_core*p_core, struct nm_req_s*p_unpack, nm_req_flag_t flags)
{
  /* store the unpack request */
  nm_status_add(p_unpack, NM_STATUS_UNPACK_POSTED);
  nm_core_lock(p_core);
  nm_req_list_push_back(&p_core->unpacks, p_unpack);
  nm_core_polling_level(p_core);
  struct nm_unexpected_s*p_unexpected = nm_unexpected_find_matching(p_core, p_unpack);
  while(p_unexpected)
    {
      /* data is already here (at least one chunk)- process all matching chunks */
      const void*p_header = p_unexpected->p_header;
      const nm_proto_t proto_id = p_unexpected->p_header->proto_id & NM_PROTO_ID_MASK;
      switch(proto_id)
	{
	case NM_PROTO_PKT_DATA:
	  {
	    const struct nm_header_pkt_data_s*h = p_header;
	    nm_pkt_data_handler(p_core, p_unpack->p_gate, &p_unpack, h, p_unexpected->p_pw);
	  }
	  break;
	case NM_PROTO_SHORT_DATA:
	  {
	    const struct nm_header_short_data_s*h = p_header;
	    nm_short_data_handler(p_core, p_unpack->p_gate, &p_unpack, h, p_unexpected->p_pw);
	  }
	  break;
	case NM_PROTO_DATA:
	  {
	    const struct nm_header_data_s*h = p_header;
	    nm_small_data_handler(p_core, p_unpack->p_gate, &p_unpack, h, p_unexpected->p_pw);
	  }
	  break;
	case NM_PROTO_RDV:
	  {
	    const struct nm_header_ctrl_rdv_s*h = p_header;
	    nm_rdv_handler(p_core, p_unpack->p_gate, p_unpack, h, p_unexpected->p_pw);
	  }
	  break;
	}
      /* Decrement the packet wrapper reference counter. If no other
	 chunks are still in use, the pw will be destroyed. */
      nm_pw_ref_dec(p_unexpected->p_pw);
      nm_unexpected_list_erase(&p_core->unexpected, p_unexpected);
      nm_unexpected_free(nm_unexpected_allocator, p_unexpected);
      p_unexpected = p_unpack ? nm_unexpected_find_matching(p_core, p_unpack) : NULL;
    }
  nm_core_unlock(p_core);
  return NM_ESUCCESS;
}

int nm_core_iprobe(struct nm_core*p_core,
		   nm_gate_t p_gate, nm_core_tag_t tag, nm_core_tag_t tag_mask,
		   nm_gate_t *pp_out_gate, nm_core_tag_t*p_out_tag, nm_len_t*p_out_size)
{
  struct nm_unexpected_s*p_chunk;
  puk_list_foreach(p_chunk, &p_core->unexpected)
    {
      struct nm_gtag_s*p_so_tag = nm_gtag_get(&p_chunk->p_gate->tags, p_chunk->tag);
      const nm_seq_t next_seq = nm_seq_next(p_so_tag->recv_seq_number);
      if(((p_gate == p_chunk->p_gate) || (p_gate == NM_ANY_GATE)) && /* gate matches */
	 nm_core_tag_match(p_chunk->tag, tag, tag_mask) && /* tag matches */
	 (p_chunk->seq == next_seq) /* seq number matches */ )
	{
	  const void*header = p_chunk->p_header;
	  const nm_proto_t proto_id = (*(nm_proto_t*)header) & NM_PROTO_ID_MASK;
	  const nm_proto_t proto_flags = (*(nm_proto_t*)header) & NM_PROTO_FLAG_MASK;
	  int matches = 0;
	  nm_len_t len = -1;
	  switch(proto_id)
	    {
	    case NM_PROTO_PKT_DATA:
	      {
		const struct nm_header_pkt_data_s*h = header;
		if(proto_flags & NM_PROTO_FLAG_LASTCHUNK)
		  {
		    len = h->data_len + h->chunk_offset;
		    matches = 1;
		  }
	      }
	      break;
	    case NM_PROTO_SHORT_DATA:
	      {
		const struct nm_header_short_data_s *h = header;
		len = h->len;
		matches = 1;
	      }
	      break;
	    case NM_PROTO_DATA:
	      {
		const struct nm_header_data_s *h = header;
		if(proto_flags & NM_PROTO_FLAG_LASTCHUNK)
		  {
		    len = h->len + h->chunk_offset;
		    matches = 1;
		  }
	      }
	      break;
	    case NM_PROTO_RDV:
	      {
		const struct nm_header_ctrl_rdv_s *rdv = header;
		if(proto_flags & NM_PROTO_FLAG_LASTCHUNK)
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
		*pp_out_gate = p_chunk->p_gate;
	      if(p_out_tag)
		*p_out_tag = p_chunk->tag;
	      if(p_out_size)
		*p_out_size = len;
	      return NM_ESUCCESS;
	    }
	}
    }
  *pp_out_gate = NM_ANY_GATE;
  return -NM_EAGAIN;
}

int nm_core_unpack_cancel(struct nm_core*p_core, struct nm_req_s*p_unpack)
{
  int rc = NM_ESUCCESS;
  nm_core_lock(p_core);
  if(nm_status_test(p_unpack, NM_STATUS_UNPACK_CANCELLED))
    {
      /* received already canacelled */
      rc = -NM_ECANCELED;
    }
  else if(nm_status_test(p_unpack, NM_STATUS_UNPACK_COMPLETED))
    {
      /* receive already completed */
      rc = -NM_EALREADY;
    }
  else if(p_unpack->seq == NM_SEQ_NONE)
    {
      nm_req_list_erase(&p_core->unpacks, p_unpack);
      nm_core_polling_level(p_core);
      const struct nm_core_event_s event =
	{
	  .status = NM_STATUS_UNPACK_CANCELLED | NM_STATUS_FINALIZED,
	  .p_req = p_unpack
	};
      nm_core_status_event(p_core, &event, p_unpack);
      rc = NM_ESUCCESS;
    }
  else
    {
      /* receive is already in progress- too late to cancel */
      rc = -NM_EINPROGRESS;
    }
  nm_core_unlock(p_core);
  return rc;
}

/** Process a packed data request (NM_PROTO_PKT_DATA)- p_unpack may be NULL (unexpected)
 */
static void nm_pkt_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				const struct nm_header_pkt_data_s*h, struct nm_pkt_wrap_s *p_pw)
{
  const nm_len_t chunk_len = h->data_len;
  const nm_len_t chunk_offset = h->chunk_offset;
  if(*pp_unpack)
    {
      if(!(nm_status_test(*pp_unpack, NM_STATUS_UNPACK_CANCELLED)))
	{
	  assert(nm_status_test(*pp_unpack, NM_STATUS_UNPACK_POSTED));
	  nm_so_data_flags_decode(*pp_unpack, h->proto_id & NM_PROTO_FLAG_MASK, chunk_offset, chunk_len);
	  nm_data_pkt_unpack(&(*pp_unpack)->data, h, p_pw, chunk_offset, chunk_len);
	  nm_so_unpack_check_completion(p_core, p_pw, pp_unpack, chunk_len);
	}
    }
  else
    {
      nm_unexpected_store(p_core, p_gate, (void*)h, chunk_offset, chunk_len, h->tag_id, h->seq, p_pw);
    }
}

/** Process a short data request (NM_PROTO_SHORT_DATA)- p_unpack may be NULL (unexpected)
 */
static void nm_short_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				  const struct nm_header_short_data_s*h, struct nm_pkt_wrap_s *p_pw)
{
  const nm_len_t chunk_len = h->len;
  const nm_len_t chunk_offset = 0;
  if(*pp_unpack)
    {
      if(!nm_status_test(*pp_unpack, NM_STATUS_UNPACK_CANCELLED))
	{
	  const uint8_t flags = NM_PROTO_FLAG_LASTCHUNK;
	  const void*ptr = ((void*)h) + NM_HEADER_SHORT_DATA_SIZE;
	  nm_so_data_flags_decode(*pp_unpack, flags, chunk_offset, chunk_len);
	  nm_data_copy_to(&(*pp_unpack)->data, chunk_offset, chunk_len, ptr);
	  nm_so_unpack_check_completion(p_core, p_pw, pp_unpack, chunk_len);
	}
    }
  else
    {
      nm_unexpected_store(p_core, p_gate, (void*)h, chunk_offset, chunk_len, h->tag_id, h->seq, p_pw);
    }
}

/** Process a small data request (NM_PROTO_DATA)- p_unpack may be NULL (unexpected)
 */
static void nm_small_data_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s**pp_unpack,
				  const struct nm_header_data_s*h, struct nm_pkt_wrap_s*p_pw)
{
  const void*ptr = (h->skip == 0xFFFF) ? (((void*)h) + NM_HEADER_DATA_SIZE) :
    p_pw->v[0].iov_base + (h->skip + nm_header_global_v0len(p_pw));
  assert(p_pw->v_nb == 1);
  assert(ptr + h->len <= p_pw->v->iov_base + p_pw->v->iov_len);
  const nm_len_t chunk_len = h->len;
  const nm_len_t chunk_offset = h->chunk_offset;
  if(*pp_unpack)
    {
      if(!nm_status_test(*pp_unpack, NM_STATUS_UNPACK_CANCELLED))
	{
	  nm_so_data_flags_decode(*pp_unpack, h->proto_id & NM_PROTO_FLAG_MASK, chunk_offset, chunk_len);
	  nm_data_copy_to(&(*pp_unpack)->data, chunk_offset, chunk_len, ptr);
	  nm_so_unpack_check_completion(p_core, p_pw, pp_unpack, chunk_len);
	}
    }
  else
    {
      nm_unexpected_store(p_core, p_gate, (void*)h, chunk_offset, chunk_len, h->tag_id, h->seq, p_pw);
    }
}

/** Process a received rendez-vous request (NM_PROTO_RDV)- 
 * either p_unpack may be NULL (storing unexpected) or p_pw (unpacking unexpected)
 */
static void nm_rdv_handler(struct nm_core*p_core, nm_gate_t p_gate, struct nm_req_s*p_unpack,
			   const struct nm_header_ctrl_rdv_s*h, struct nm_pkt_wrap_s *p_pw)
{
  const nm_len_t chunk_len = h->len;
  const nm_len_t chunk_offset = h->chunk_offset;
  if(p_unpack)
    {
      assert(p_unpack->p_gate != NULL);
      assert(!nm_data_isnull(&p_unpack->data));
      nm_drv_t p_drv = nm_drv_default(p_gate);
      const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_unpack->data);
      const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : p_props->size; /* average block size */
      nm_so_data_flags_decode(p_unpack, h->proto_id & NM_PROTO_FLAG_MASK, chunk_offset, chunk_len);
      if(p_drv->trk_caps[NM_TRK_LARGE].supports_data || density < NM_LARGE_MIN_DENSITY)
	{
	  /* iterator-based data & driver supports data natively || low-density -> send all & copy */
	  struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader();
	  p_large_pw->p_unpack     = p_unpack;
	  p_large_pw->length       = chunk_len;
	  p_large_pw->chunk_offset = chunk_offset;
	  p_large_pw->p_data       = &p_unpack->data;
	  p_large_pw->p_gate       = p_gate;
	  nm_pkt_wrap_list_push_back(&p_gate->pending_large_recv, p_large_pw);
	}
      else
	{
	  /* one rdv per chunk */
	  struct nm_large_chunk_context_s large_chunk = { .p_unpack = p_unpack, .p_gate = p_unpack->p_gate, .chunk_offset = chunk_offset };
	  nm_data_chunk_extractor_traversal(&p_unpack->data, chunk_offset, chunk_len, &nm_large_chunk_store, &large_chunk);
	}
    }
  else
    {
      nm_unexpected_store(p_core, p_gate, (void*)h, chunk_offset, chunk_len, h->tag_id, h->seq, p_pw);
    }
}

/** Process a complete rendez-vous ready-to-receive request.
 */
static void nm_rtr_handler(struct nm_pkt_wrap_s*p_rtr_pw, const struct nm_header_ctrl_rtr_s*header)
{
  const nm_core_tag_t tag     = header->tag_id;
  const nm_seq_t seq          = header->seq;
  const nm_len_t chunk_offset = header->chunk_offset;
  const nm_len_t chunk_len    = header->chunk_len;
  nm_gate_t p_gate      = p_rtr_pw->p_gate;
  struct nm_pkt_wrap_s*p_large_pw = NULL, *p_pw_save;
  nm_core_lock_assert(p_rtr_pw->p_drv->p_core);
  puk_list_foreach_safe(p_large_pw, p_pw_save, &p_gate->pending_large_send)
    {
      assert(p_large_pw->n_completions == 1);
      const struct nm_pw_completion_s*p_completion = &p_large_pw->completions[0];
      struct nm_req_s*p_pack = p_completion->p_pack;
      NM_TRACEF("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n",
		p_pack->seq, p_large_pw->chunk_offset);
      if((p_pack->seq == seq) && nm_core_tag_eq(p_pack->tag, tag) && (p_large_pw->chunk_offset == chunk_offset))
	{
	  nm_pkt_wrap_list_erase(&p_gate->pending_large_send, p_large_pw);
	  if(chunk_len < p_large_pw->length)
	    {
	      /* ** partial RTR- split the packet  */
	      /* assert ack is partial */
	      assert(chunk_len > 0 && chunk_len < p_large_pw->length);
	      /* create a new pw with the remaining data */
	      struct nm_pkt_wrap_s*p_pw2 = nm_pw_alloc_noheader();
	      p_pw2->p_drv    = p_large_pw->p_drv;
	      p_pw2->trk_id   = p_large_pw->trk_id;
	      p_pw2->p_gate   = p_gate;
	      p_pw2->p_gdrv   = p_large_pw->p_gdrv;
	      p_pw2->p_unpack = NULL;
	      /* this is a large pw with rdv- it must contain a single completion */
	      assert(p_large_pw->n_completions == 1);
	      nm_pw_completion_add(p_pw2, p_large_pw->completions[0].p_pack, p_large_pw->completions[0].len - chunk_len);
	      p_large_pw->completions[0].len = chunk_len; /* truncate the contrib */
	      /* populate p_pw2 iovec */
	      nm_pw_split_data(p_large_pw, p_pw2, chunk_len);
	      nm_pkt_wrap_list_push_front(&p_gate->pending_large_send, p_pw2);
	    }
	  /* send the data */
	  nm_drv_t p_drv = nm_drv_get_by_index(p_gate, header->drv_index);
	  if(header->trk_id == NM_TRK_LARGE)
	    {
	      nm_core_post_send(p_gate, p_large_pw, header->trk_id, p_drv);
	    }
	  else
	    {
	      /* rdv eventually accepted on trk#0- rollback and repack */
	      struct puk_receptacle_NewMad_Strategy_s*r = &p_pack->p_gate->strategy_receptacle;
	      p_pack->pack.scheduled -= chunk_len;
	      if(p_large_pw->p_data == NULL)
		{
		  void*ptr = p_large_pw->v[0].iov_base;
		  /* repack chunk on trk#0 */
		  (*r->driver->pack_chunk)(r->_status, p_pack, ptr, chunk_len, chunk_offset);
		}
	      else
		{
		  /* repack chunk on trk#0 */
		  (*r->driver->pack_data)(r->_status, p_pack, chunk_len, chunk_offset);
		}
	      nm_pw_free(p_large_pw);
	    }
	  return;
	}
    }
  NM_WARN("FATAL- cannot find matching packet for received RTR: seq = %d; offset = %lu- dumping pending large packets\n", seq, chunk_offset);
  puk_list_foreach(p_large_pw, &p_gate->pending_large_send)
    {
      const struct nm_req_s*p_pack = p_large_pw->completions[0].p_pack;
      fprintf(stderr, "  packet- seq = %d; chunk_offset = %lu\n", p_pack->seq, p_large_pw->chunk_offset);
    }
  abort();
}
/** Process an acknowledgement.
 */
static void nm_ack_handler(struct nm_pkt_wrap_s *p_ack_pw, const struct nm_header_ctrl_ack_s*header)
{
  struct nm_core*p_core = p_ack_pw->p_gate->p_core;
  const nm_core_tag_t tag = header->tag_id;
  const nm_seq_t seq = header->seq;
  struct nm_req_s*p_pack = NULL;
  
  puk_list_foreach(p_pack, &p_core->pending_packs)
    {
      if(nm_core_tag_eq(p_pack->tag, tag) && p_pack->seq == seq)
	{
	  nm_core_lock_assert(p_core);
	  nm_req_list_erase(&p_core->pending_packs, p_pack);
	  nm_core_polling_level(p_core);
	  const struct nm_core_event_s event =
	    {
	      .status = NM_STATUS_ACK_RECEIVED | (nm_status_test(p_pack, NM_STATUS_PACK_COMPLETED) ? NM_STATUS_FINALIZED : 0),
	      .p_req = p_pack
	    };
	  nm_core_status_event(p_core, &event, p_pack);
	  return;
	}
    }
}

/** decode one chunk of headers.
 * @returns the number of processed bytes in global header, 
 */
int nm_decode_header_chunk(struct nm_core*p_core, const void*ptr, struct nm_pkt_wrap_s*p_pw, nm_gate_t p_gate)
{
  int rc = 0;
  const nm_proto_t proto_id   = (*(const nm_proto_t*)ptr) & NM_PROTO_ID_MASK;
  const nm_proto_t proto_flag = (*(const nm_proto_t*)ptr) & NM_PROTO_FLAG_MASK;

  assert(proto_id != 0);
  switch(proto_id)
    {
    case NM_PROTO_PKT_DATA:
      {
	const struct nm_header_pkt_data_s*h = ptr;
	struct nm_req_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, h->seq, h->tag_id);
	nm_pkt_data_handler(p_core, p_gate, &p_unpack, h, p_pw);
	rc = h->hlen;
      }
      break;
      
    case NM_PROTO_SHORT_DATA:
      {
	const struct nm_header_short_data_s*h = ptr;
	struct nm_req_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, h->seq, h->tag_id);
	nm_short_data_handler(p_core, p_gate, &p_unpack, h, p_pw);
	rc = NM_HEADER_SHORT_DATA_SIZE + h->len;
      }
      break;
      
    case NM_PROTO_DATA:
      {
	const struct nm_header_data_s*h = ptr;
	rc = NM_HEADER_DATA_SIZE;
	if(h->skip == 0xFFFF)
	  {
	    const nm_len_t size = (proto_flag & NM_PROTO_FLAG_ALIGNED) ? nm_aligned(h->len) : h->len;
	    rc += size;
	  }
	struct nm_req_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, h->seq, h->tag_id);
	nm_small_data_handler(p_core, p_gate, &p_unpack, h, p_pw);
      }
      break;
      
    case NM_PROTO_RDV:
      {
	const struct nm_header_ctrl_rdv_s*h = ptr;
	rc = NM_HEADER_CTRL_SIZE;
	struct nm_req_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, h->seq, h->tag_id);
	nm_rdv_handler(p_core, p_gate, p_unpack, h, p_pw);
      }
      break;
      
    case NM_PROTO_RTR:
      {
	const struct nm_header_ctrl_rtr_s*h = ptr;
	rc = NM_HEADER_CTRL_SIZE;
	nm_rtr_handler(p_pw, h);
      }
      break;
      
    case NM_PROTO_ACK:
      {
	const struct nm_header_ctrl_ack_s*h = ptr;
	rc = NM_HEADER_CTRL_SIZE;
	nm_ack_handler(p_pw, h);
      }
      break;
      
    case NM_PROTO_STRAT:
      {
	const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	const struct nm_header_strat_s*h = ptr;
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
      padico_fatal("nmad: received header with invalid proto_id 0x%02X\n", proto_id);
      break;
    }
  return rc;
}


/** Process a complete incoming request.
 */
void nm_pw_process_complete_recv(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw)
{
  nm_gate_t const p_gate = p_pw->p_gate;
  assert(p_gate != NULL);
  nm_core_lock_assert(p_core);
  
  /* clear the input request field */
  if(p_pw->p_gdrv && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
    {
      /* request was posted on a given gate */
      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
      p_pw->p_gdrv->active_recv[p_pw->trk_id]--;
      assert(p_pw->p_gdrv->active_recv[p_pw->trk_id] == 0);
    }
  else if((!p_pw->p_gdrv) && p_pw->p_drv->p_in_rq == p_pw)
    {
      /* request was posted on a driver, for any gate */
      p_pw->p_drv->p_in_rq = NULL;
    }

#ifndef PIOMAN
  nm_pkt_wrap_list_erase(&p_core->pending_recv_list, p_pw);
#endif /* !PIOMAN */

  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      /* ** Small packets - track #0 *********************** */
      struct iovec*const v0 = p_pw->v;
      const void*ptr = v0->iov_base + sizeof(struct nm_header_global_s);
      nm_len_t done = 0;
      do
	{
	  /* Iterate over header chunks */
	  assert(ptr < v0->iov_base + v0->iov_len);
	  done = nm_decode_header_chunk(p_core, ptr, p_pw, p_gate);
	  ptr += done;
	}
      while(ptr < v0->iov_base + nm_header_global_v0len(p_pw));
    }
  else if(p_pw->trk_id == NM_TRK_LARGE)
    {
      /* ** Large packet - track #1 ************************ */
      struct nm_req_s*p_unpack = p_pw->p_unpack;
      const nm_len_t len = p_pw->length;
      /* ** Large packet, data received directly in its final destination */
      nm_so_unpack_check_completion(p_core, p_pw, &p_unpack, len);
    }
  nm_pw_ref_dec(p_pw);
  /* Hum... Well... We're done guys! */
}


