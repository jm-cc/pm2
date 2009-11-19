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

#include <ccs_public.h>
#include <segment.h>


static void nm_short_data_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const void*ptr, nm_so_short_data_header_t*h);
static void nm_small_data_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const void*ptr, nm_so_data_header_t*h);
static void nm_rdv_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_so_ctrl_rdv_header*h);


/** Find an unexpected chunk that matches an unpack request [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
static struct nm_unexpected_s*nm_unexpected_find_matching(struct nm_core*p_core, struct nm_unpack_s*p_unpack)
{
  struct nm_unexpected_s*chunk;
  tbx_fast_list_for_each_entry(chunk, &p_core->so_sched.unexpected, link)
    {
      if((chunk->p_gate == p_unpack->p_gate) && (chunk->seq == p_unpack->seq) && (chunk->tag == p_unpack->tag))
	{
	  /* regular matching */
	  return chunk;
	}
      else if((p_unpack->p_gate == NM_ANY_GATE) && (chunk->tag == p_unpack->tag))
	{
	  /* any gate */
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&chunk->p_gate->tags, chunk->tag);
	  p_so_tag->recv_seq_number++;
	  p_unpack->seq = chunk->seq;
	  p_unpack->p_gate = chunk->p_gate;
	  return chunk;
	}
      else if((p_unpack->p_gate == chunk->p_gate) && (p_unpack->tag == NM_ANY_TAG))
	{
	  /* any tag */
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&chunk->p_gate->tags, chunk->tag);
	  p_so_tag->recv_seq_number++;
	  p_unpack->seq = chunk->seq;
	  p_unpack->tag = chunk->tag;
	  return chunk;
	}
      else if((p_unpack->p_gate == NM_ANY_GATE) && (p_unpack->tag == NM_ANY_TAG))
	{
	  /* any gate, any tag */
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&chunk->p_gate->tags, chunk->tag);
	  p_so_tag->recv_seq_number++;
	  p_unpack->seq = chunk->seq;
	  p_unpack->tag = chunk->tag;
	  return chunk;
	}
    }
  return NULL;
}

/** Find an unpack that matches a given packet that arrived from [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
static struct nm_unpack_s*nm_unpack_find_matching(struct nm_core*p_core, nm_gate_t p_gate, nm_seq_t seq, nm_tag_t tag)
{
  struct nm_unpack_s*p_unpack = NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t last_seq = p_so_tag->recv_seq_number;
  tbx_fast_list_for_each_entry(p_unpack, &p_core->so_sched.unpacks, _link)
    {
      if((p_unpack->p_gate == p_gate) && (p_unpack->seq == seq) && (p_unpack->tag == tag))
	{
	  /* regular matching */
	  return p_unpack;
	}
      else if((p_unpack->p_gate == NM_ANY_GATE) && (last_seq == seq) && (p_unpack->tag == tag))
	{
	  /* any gate matching */
	  p_so_tag->recv_seq_number++;
	  p_unpack->p_gate = p_gate;
	  p_unpack->seq = seq;
	  return p_unpack;
	}
      else if((p_unpack->p_gate == p_gate) && (p_unpack->tag == NM_ANY_TAG))
	{
	  /* any tag matching */
	  p_unpack->tag = tag;
	  p_unpack->seq = seq;
	  return p_unpack;
	}
      else if((p_unpack->p_gate == NM_ANY_GATE) && (last_seq == seq) && (p_unpack->tag == NM_ANY_TAG))
	{
	  /* any gate, any tag */
	  p_so_tag->recv_seq_number++;
	  p_unpack->p_gate = p_gate;
	  p_unpack->tag = tag;
	  p_unpack->seq = seq;
	  return p_unpack;
	}
    }
  return NULL;
}

/** copy received data to its final destination */
static void nm_so_copy_data(struct nm_unpack_s*p_unpack, uint32_t chunk_offset, const void *ptr, uint32_t len)
{
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
	  uint32_t pending_len = len;
	  int offset = 0;
	  int i = 0;
	  while(offset + iov[i].iov_len <= chunk_offset)
	    {
	      offset += iov[i].iov_len;
	      i++;
	    }
	  uint32_t chunk_len = iov[i].iov_len - (chunk_offset - offset);
	  if(chunk_len > len)
	    chunk_len = len;
	  memcpy(iov[i].iov_base + (chunk_offset - offset), ptr, chunk_len);
	  pending_len -= chunk_len;
	  offset += iov[i].iov_len;
	  i++;
	  while(pending_len)
	    {
	      if(offset + iov[i].iov_len >= chunk_offset + pending_len)
		{
		  memcpy(iov[i].iov_base, ptr + (len-pending_len), pending_len);
		  pending_len = 0;
		}
	      else
		{
		  chunk_len = iov[i].iov_len;
		  memcpy(iov[i].iov_base, ptr + (len-pending_len), chunk_len);
		  pending_len -= chunk_len;
		  offset += iov[i].iov_len;
		  i++;
		}
	    }
	}
      else if(p_unpack->status & NM_UNPACK_TYPE_DATATYPE)
	{
	  /* data is described by a datatype */
	  DLOOP_Offset last = chunk_offset + len;
	  struct DLOOP_Segment*const segp = p_unpack->data;
	  CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
	}	    
    }
}

/** mark 'chunk_len' data as received in the given unpack, and check for unpack completion */
static inline void nm_so_unpack_check_completion(struct nm_core*p_core, struct nm_unpack_s*p_unpack, int chunk_len)
{
  p_unpack->cumulated_len += chunk_len;
  if(p_unpack->cumulated_len >= p_unpack->expected_len)
    {
#warning Paulette: lock
      tbx_fast_list_del(&p_unpack->_link);
      const struct nm_so_event_s event =
	{
	  .status = NM_SO_STATUS_UNPACK_COMPLETED,
	  .p_unpack = p_unpack
	};
      nm_so_status_event(p_core, &event, &p_unpack->status);
    }
}

/** decode and manage data flags found in data/short_data/rdv packets */
static inline void nm_so_data_flags_decode(struct nm_unpack_s*p_unpack, uint8_t flags,
					   uint32_t chunk_offset, uint32_t chunk_len)
{
  assert(chunk_len <= p_unpack->expected_len);
  if(flags & NM_PROTO_FLAG_LASTCHUNK)
    {
      /* Update the real size to receive */
      const uint32_t size = chunk_offset + chunk_len;
      assert(size <= p_unpack->expected_len);
      p_unpack->expected_len = size;
    }
  if((p_unpack->cumulated_len == 0) && (flags & NM_PROTO_FLAG_ACKREQ))
    nm_so_post_ack(p_unpack->p_gate, p_unpack->tag, p_unpack->seq);
}

/** store an unexpected chunk of data (data/short_data/rdv) */
static inline void nm_so_unexpected_store(struct nm_core*p_core, struct nm_gate*p_gate,
					  void *header, uint32_t len, nm_tag_t tag, nm_seq_t seq,
					  struct nm_pkt_wrap *p_pw)
{
  struct nm_unexpected_s*chunk = tbx_malloc(nm_so_unexpected_mem);
  chunk->header = header;
  chunk->p_pw = p_pw;
  chunk->p_gate = p_gate;
  chunk->seq = seq;
  chunk->tag = tag;
  p_pw->header_ref_count++;
#warning Paulette: lock
  tbx_fast_list_add_tail(&chunk->link, &p_core->so_sched.unexpected);
  const struct nm_so_event_s event =
    {
      .status = NM_SO_STATUS_UNEXPECTED,
      .p_gate = p_pw->p_gate,
      .tag = tag,
      .seq = seq,
      .len = len
    };
  nm_so_status_event(p_pw->p_gate->p_core, &event, NULL);
}


/** Handle an unpack request.
 */
int nm_so_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_tag_t tag,
		 nm_so_flag_t flag, void *data_description, uint32_t len)
{
  const nm_seq_t seq = (p_gate == NM_ANY_GATE) ? 0 : nm_so_tag_get(&p_gate->tags, tag)->recv_seq_number++;
  /* fill-in the unpack request */
  p_unpack->status = flag;
  p_unpack->data = data_description;
  p_unpack->cumulated_len = 0;
  p_unpack->expected_len = len;
  p_unpack->p_gate = p_gate;
  p_unpack->seq = seq;
  p_unpack->tag = tag;
  /* store the unpack request */
#warning Paulette: lock
  tbx_fast_list_add_tail(&p_unpack->_link, &p_core->so_sched.unpacks);
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
	    nm_short_data_handler(p_core, p_unpack, ptr, h);
	  }
	  break;
	case NM_PROTO_DATA:
	  {
	    struct nm_so_data_header *h = header;
	    const void *ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;
	    nm_small_data_handler(p_core, p_unpack, ptr, h);
	  }
	  break;
	case NM_PROTO_RDV:
	  {
	    struct nm_so_ctrl_rdv_header *rdv = header;
	    nm_rdv_handler(p_core, p_unpack, rdv);
	  }
	  break;
	}
      /* Decrement the packet wrapper reference counter. If no other
	 chunks are still in use, the pw will be destroyed. */
      nm_so_pw_dec_header_ref_count(chunk->p_pw);
#warning Paulette: lock
      tbx_fast_list_del(&chunk->link);
      tbx_free(nm_so_unexpected_mem, chunk);
      if(p_unpack->cumulated_len < p_unpack->expected_len)
	chunk = nm_unexpected_find_matching(p_core, p_unpack);
      else
	chunk= NULL;
    }
  return NM_ESUCCESS;
}

int nm_so_iprobe(struct nm_core*p_core, struct nm_gate*p_gate, struct nm_gate**pp_out_gate, nm_tag_t tag)
{
  struct nm_unexpected_s*chunk;
  tbx_fast_list_for_each_entry(chunk, &p_core->so_sched.unexpected, link)
    {
      if( ((chunk->p_gate == p_gate) && (chunk->tag == tag)) ||
	  ((p_gate == NM_ANY_GATE)   && (chunk->tag == tag)) ||
	  ((chunk->p_gate == p_gate) && (tag == NM_ANY_TAG)) )
	{
	  *pp_out_gate = p_gate;
	  return NM_ESUCCESS;
	}
    }
  *pp_out_gate = NM_ANY_GATE;
  return -NM_EAGAIN;
}

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack)
{
  struct nm_so_tag_s*p_so_tag = (p_unpack->p_gate == NM_ANY_GATE) ? 
    NULL : nm_so_tag_get(&p_unpack->p_gate->tags, p_unpack->tag);
  if(p_unpack->status & (NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNEXPECTED))
    {
      /* receive is already in progress- too late to cancel */
      return -NM_EINPROGRESS;
    }
  else if((p_so_tag == NULL) || (p_unpack->seq == p_so_tag->recv_seq_number - 1))
    {
#warning Paulette: lock
      tbx_fast_list_del(&p_unpack->_link);
      const struct nm_so_event_s event =
	{
	  .status =  NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNPACK_CANCELLED,
	  .p_unpack = p_unpack
	};
      nm_so_status_event(p_core, &event, &p_unpack->status);
      if(p_so_tag)
	p_so_tag->recv_seq_number--;
      return NM_ESUCCESS;
    }
  else
    return -NM_ENOTIMPL;
}

/** Process a short data request with a matching unpack (NM_PROTO_SHORT_DATA).
 */
static void nm_short_data_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const void*ptr, nm_so_short_data_header_t*h)
{
  if(!(p_unpack->status & NM_SO_STATUS_UNPACK_CANCELLED))
    {
      const uint32_t len = h->len;
      const uint32_t chunk_offset = 0;
      const uint8_t flags = NM_PROTO_FLAG_LASTCHUNK;
      nm_so_data_flags_decode(p_unpack, flags, chunk_offset, len);
      nm_so_copy_data(p_unpack, chunk_offset, ptr, len);
      nm_so_unpack_check_completion(p_core, p_unpack, len);
      struct nm_so_unused_header*uh = (struct nm_so_unused_header*)h;
      uh->proto_id = NM_PROTO_UNUSED; /* mark as read */
      uh->len = NM_SO_SHORT_DATA_HEADER_SIZE + len;
    }
}

/** Process a small data request with a matching unpack (NM_PROTO_DATA).
 */
static void nm_small_data_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, const void*ptr, nm_so_data_header_t*h)
{
  if(!(p_unpack->status & NM_SO_STATUS_UNPACK_CANCELLED))
    {
      const uint32_t len = h->len;
      const uint32_t chunk_offset = h->chunk_offset;
      nm_so_data_flags_decode(p_unpack, h->flags, chunk_offset, len);
      nm_so_copy_data(p_unpack, chunk_offset, ptr, len);
      nm_so_unpack_check_completion(p_core, p_unpack, len);
      const unsigned long size = (h->flags & NM_PROTO_FLAG_ALIGNED) ? nm_so_aligned(len) : len;
      const uint32_t unused_len = (h->skip == 0) ? size : 0;
      struct nm_so_unused_header*uh = (struct nm_so_unused_header*)h;
      uh->proto_id = NM_PROTO_UNUSED; /* mark as read */
      uh->len = NM_SO_DATA_HEADER_SIZE + unused_len;
    }
}

/** Process a received rendez-vous request with a matching unpack
 */
static void nm_rdv_handler(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_so_ctrl_rdv_header*h)
{
  const uint32_t len = h->len;
  const uint32_t chunk_offset = h->chunk_offset;
  nm_so_data_flags_decode(p_unpack, h->flags, chunk_offset, len);
  nm_so_rdv_success(p_core, p_unpack, len, chunk_offset);
  h->proto_id = NM_PROTO_CTRL_UNUSED; /* mark as read */
}

/** Process a complete rendez-vous ready-to-receive request.
 */
static void nm_rtr_handler(struct nm_pkt_wrap *p_rtr_pw, struct nm_so_ctrl_rtr_header*header)
{
  const nm_tag_t tag          = header->tag_id;
  const nm_seq_t seq          = header->seq;
  const uint32_t chunk_offset = header->chunk_offset;
  const uint32_t chunk_len    = header->chunk_len;
  struct nm_gate *p_gate      = p_rtr_pw->p_gate;

  NM_SO_TRACE("rtr completed for tag = %d, seq = %u, offset = %u\n", (int)tag, seq, chunk_offset);

  struct nm_pkt_wrap *p_large_pw = NULL;
  tbx_fast_list_for_each_entry(p_large_pw, &p_gate->pending_large_send, link)
    {
      assert(p_large_pw->n_contribs == 1);
      const struct nm_pw_contrib_s*p_contrib = &p_large_pw->contribs[0];
      const struct nm_pack_s*p_pack = p_contrib->p_pack;
      NM_SO_TRACE("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n",
		  p_pack->seq, p_large_pw->chunk_offset);
      if((p_pack->seq == seq) && (p_pack->tag == tag) && (p_large_pw->chunk_offset == chunk_offset))
	{
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_large_pw, p_gate->id, 1/* large output list*/);
#warning Paulette: lock
	  tbx_fast_list_del(&p_large_pw->link);
	  if(chunk_len < p_large_pw->length)
	    {
	      /* partial ACK- split the packet  */
	      struct nm_pkt_wrap *p_large_pw2 = NULL;
	      nm_so_pw_split(p_large_pw, &p_large_pw2, chunk_len);
#warning Paulette: lock
	      tbx_fast_list_add(&p_large_pw2->link, &p_gate->pending_large_send);
	    }
	  /* send the data */
	  nm_core_post_send(p_gate, p_large_pw, header->trk_id, header->drv_id);
	  header->proto_id = NM_PROTO_CTRL_UNUSED; /* mark as read */
	  return;
	}
    }
  TBX_FAILUREF("PANIC- ack_callback cannot find pending packet wrapper for seq = %d; offset = %d!\n",
	      seq, chunk_offset);
}
/** Process an acknowledgement.
 */
static void nm_ack_handler(struct nm_pkt_wrap *p_ack_pw, struct nm_so_ctrl_ack_header*header)
{
  struct nm_core*p_core = p_ack_pw->p_gate->p_core;
  const nm_tag_t tag = header->tag_id;
  const nm_seq_t seq = header->seq;
  struct nm_pack_s*p_pack = NULL;
  tbx_fast_list_for_each_entry(p_pack, &p_core->so_sched.pending_packs, _link)
    {
      if(p_pack->tag == tag && p_pack->seq == seq)
	{
#warning Paulette: lock
	  tbx_fast_list_del(&p_pack->_link);
	  const struct nm_so_event_s event =
	    {
	      .status = NM_SO_STATUS_ACK_RECEIVED,
	      .p_pack = p_pack
	    };
	  nm_so_status_event(p_core, &event, &p_pack->status);
	  return;
	}
    }
}

/** Iterate over the headers of a freshly received ctrl pkt.
 *
 *  @param p_pw the pkt wrapper pointer.
 */
static void nm_decode_headers(struct nm_pkt_wrap *p_pw)
{
#ifdef NMAD_QOS
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pw->p_gate->strategy_receptacle;
  unsigned ack_received = 0;
#endif /* NMAD_QOS */

  /* Each 'unread' header will increment this counter. When the
     counter will reach 0 again, the packet wrapper can (and will) be
     safely destroyed.
     Increment it here for our own use in the header decoder uses the header. */
  p_pw->header_ref_count++;

  struct iovec *vec = p_pw->v;
  void *ptr = vec->iov_base;
  int done = 0;

  while(!done)
    {
      /* Decode header */
      const nm_proto_t proto_id = (*(nm_proto_t *)ptr) & NM_PROTO_ID_MASK;
      const nm_proto_t proto_flag = (*(nm_proto_t *)ptr) & NM_PROTO_FLAG_MASK;
      if(proto_flag & NM_PROTO_LAST)
	done = 1;
      assert(proto_id != 0);
      switch(proto_id)
	{
	case NM_PROTO_SHORT_DATA:
	  {
	    struct nm_so_short_data_header*sh = ptr;
	    ptr += NM_SO_SHORT_DATA_HEADER_SIZE;
	    const uint32_t len = sh->len;
	    const nm_tag_t tag = sh->tag_id;
	    const nm_seq_t seq = sh->seq;
	    struct nm_gate*p_gate = p_pw->p_gate;
	    struct nm_core*p_core = p_gate->p_core;
	    struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	    if(p_unpack)
	      {
		nm_short_data_handler(p_core, p_unpack, ptr, sh);
	      }
	    else
	      { 
		nm_so_unexpected_store(p_core, p_gate, sh, len, tag, seq, p_pw);
	      }
	    ptr += len;
	  }
	  break;

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
	    const unsigned long size = (dh->flags & NM_PROTO_FLAG_ALIGNED) ? nm_so_aligned(dh->len) : dh->len;
	    if(dh->skip == 0)
	      { /* data is just after the header */
		ptr += size;
	      }  /* else the next header is just behind */
	    const nm_tag_t tag = dh->tag_id;
	    const nm_seq_t seq = dh->seq;
	    struct nm_gate*p_gate = p_pw->p_gate;
	    struct nm_core*p_core = p_gate->p_core;
	    struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	    if(p_unpack)
	      {
		/* Cool! We already have a matching unpack for this packet */
		nm_small_data_handler(p_core, p_unpack, data, dh);
	      }
	    else
	      { 
		nm_so_unexpected_store(p_core, p_gate, dh, dh->len, tag, seq, p_pw);
	      }
	  }
	  break;

	case NM_PROTO_UNUSED:
	  {
	    /* Unused data header- skip */
	    struct nm_so_unused_header*uh = ptr;
	    ptr += uh->len;
	  }
	  break;
	  
	case NM_PROTO_RDV:
	  {
	    union nm_so_generic_ctrl_header *ch = ptr;
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	    const nm_tag_t tag = ch->rdv.tag_id;
	    const nm_seq_t seq = ch->rdv.seq;
	    const uint32_t len = ch->rdv.len;
	    struct nm_gate *p_gate = p_pw->p_gate;
	    struct nm_core*p_core = p_gate->p_core;
	    struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
	    if(p_unpack)
	      {
		nm_rdv_handler(p_core, p_unpack, &ch->rdv);
	      }
	    else 
	      {
		nm_so_unexpected_store(p_core, p_gate, ch, len, tag, seq, p_pw);
	      }
	  }
	  break;

	case NM_PROTO_RTR:
	  {
	    union nm_so_generic_ctrl_header *ch = ptr;
	    ptr += NM_SO_CTRL_HEADER_SIZE;
#ifdef NMAD_QOS
	    int r;
	    if(strategy->driver->ack_callback != NULL)
	      {
		ack_received = 1;
		r = strategy->driver->ack_callback(strategy->_status,
						   p_pw,
						   ch->rtr.tag_id,
						   ch->rtr.seq,
						   ch->rtr.track_id,
						   0);
	      }
	    else
#endif /* NMAD_QOS */
	      nm_rtr_handler(p_pw, &ch->rtr);
	  }
	  break;

	case NM_PROTO_ACK:
	  {
	    union nm_so_generic_ctrl_header *ch = ptr;
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	    nm_ack_handler(p_pw, &ch->ack);
	  }
	  break;
	  
	case NM_PROTO_CTRL_UNUSED:
	  {
	    ptr += NM_SO_CTRL_HEADER_SIZE;
	  }
	  break;
	  
	default:
	  TBX_FAILUREF("nmad: received header with invalid proto_id %d\n", proto_id);
	  break;
	  
	} /* switch */
    } /* while */
  
#ifdef NMAD_QOS
  if(ack_received)
    strategy->driver->ack_callback(strategy->_status,
				   p_pw, 0, 0, 128, 1);
#endif /* NMAD_QOS */

  nm_so_pw_dec_header_ref_count(p_pw);
}


/** Process a complete incoming request.
 */
int nm_so_process_complete_recv(struct nm_core *p_core,	struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  int err;

  /* clear the input request field in gate track */
  if (p_pw->p_gate && p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] == p_pw)
    {
      p_pw->p_gdrv->p_in_rq_array[p_pw->trk_id] = NULL;
    }

#ifdef PIOM_ENABLE_LTASKS
  piom_ltask_completed(&p_pw->ltask);
#else
#ifdef PIOMAN_POLL
  piom_req_success(&p_pw->inst);
#endif
#endif	/* PIOM_ENABLE_LTASKS */
  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      /* ** Small packets - track #0 *********************** */
      nm_lock_interface(p_core);
      p_gate->active_recv[p_pw->p_drv->id][NM_TRK_SMALL] = 0;
      nm_unlock_interface(p_core);
      
      nm_decode_headers(p_pw);
    }
  else if(p_pw->trk_id == NM_TRK_LARGE)
    {
      /* ** Large packet - track #1 ************************ */
      struct nm_unpack_s*p_unpack = p_pw->p_unpack;
      const uint32_t len = p_pw->length;
      if(p_unpack->status & NM_UNPACK_TYPE_COPY_DATATYPE)
	{
	  /* ** Large packet, packed datatype -> finalize */
	  DLOOP_Offset last = p_pw->length;
	  /* unpack contigous data into their final destination */
	  CCSI_Segment_unpack(p_pw->segp, 0, &last, p_pw->v[0].iov_base);
	  nm_so_unpack_check_completion(p_core, p_unpack, len);
	  if(last < p_pw->length)
	    {
	      /* we are expecting more data */
	      p_pw->v[0].iov_base += last;
	      p_pw->v[0].iov_len -= last;
	      nm_core_post_recv(p_pw, p_gate, NM_TRK_LARGE, p_pw->p_drv->id);
	      goto out;
	    }
	}
      else
	{
	  /* ** Large packet, data received directly in its final destination */
	  nm_so_unpack_check_completion(p_core, p_unpack, len);
	}
      nm_lock_interface(p_core);
      p_gate->active_recv[p_pw->p_drv->id][NM_TRK_LARGE] = 0;
      nm_unlock_interface(p_core);
    

      /* Free the wrapper */
      nm_so_pw_free(p_pw);
      nm_so_process_large_pending_recv(p_gate);
    }

 out:
  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;
  return err;
}


