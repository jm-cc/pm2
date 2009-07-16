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

/** Find an unexpected chunk that matches an unpack request [p_gate, seq, tag]
 *  including matching with any_gate / any_tag in the unpack
 */
struct nm_unexpected_s*nm_unexpected_find_matching(struct nm_core*p_core, struct nm_unpack_s*p_unpack)
{
  struct nm_unexpected_s*chunk;
  list_for_each_entry(chunk, &p_core->so_sched.unexpected, link)
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
struct nm_unpack_s*nm_unpack_find_matching(struct nm_core*p_core, nm_gate_t p_gate, nm_seq_t seq, nm_tag_t tag)
{
  struct nm_unpack_s*p_unpack = NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  const nm_seq_t last_seq = p_so_tag->recv_seq_number;
  list_for_each_entry(p_unpack, &p_core->so_sched.unpacks, link)
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

int _nm_so_copy_data_in_iov(struct iovec *iov, uint32_t chunk_offset, const void *data, uint32_t len)
{
  uint32_t chunk_len, pending_len = len;
  int offset = 0;
  int i = 0;

  while(offset + iov[i].iov_len <= chunk_offset)
    {
      offset += iov[i].iov_len;
      i++;
    }

  if(offset + iov[i].iov_len >= chunk_offset + len)
    {
      /* Data is contiguous */
      memcpy(iov[i].iov_base + (chunk_offset - offset), data, len);
      goto out;
    }

  /* Data are on several iovec entries */
  
  /* first entry: it can be a fragment of entry */
  chunk_len = iov[i].iov_len - (chunk_offset - offset);

  memcpy(iov[i].iov_base + (chunk_offset - offset), data, chunk_len);

  pending_len -= chunk_len;
  offset += iov[i].iov_len;
  i++;

  /* following entries: only full part */
  while(pending_len)
    {
      if(offset + iov[i].iov_len >= chunk_offset + pending_len)
	{
	  /* it is the last line */
	  memcpy(iov[i].iov_base, data+(len-pending_len), pending_len);
	  goto out;
	}
      
      chunk_len = iov[i].iov_len;
      
      memcpy(iov[i].iov_base, data+(len-pending_len), chunk_len);
      
      pending_len -= chunk_len;
      offset += iov[i].iov_len;
      i++;
    }
  
 out:
  return NM_ESUCCESS;
}


/** Handle an unpack request.
 */
int __nm_so_unpack(struct nm_core*p_core, struct nm_unpack_s*p_unpack, struct nm_gate *p_gate, nm_tag_t tag,
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
  struct nm_unexpected_s*chunk = nm_unexpected_find_matching(p_core, p_unpack);
  while(chunk)
    {
      /* data is already here (at least one chunk)- process all matching chunks */
      const void*header = chunk->header;
      const nm_proto_t proto_id = *(nm_proto_t *)header;
      switch(proto_id)
	{
	case NM_PROTO_DATA:
	  {
	    const struct nm_so_data_header *h = header;
	    const uint32_t len = h->len;
	    assert(len <= NM_SO_MAX_UNEXPECTED);
	    const void *ptr = header + NM_SO_DATA_HEADER_SIZE + h->skip;
	    const uint32_t chunk_offset = h->chunk_offset;
	    if(h->flags & NM_PROTO_FLAG_LASTCHUNK)
	      {
		p_unpack->expected_len = chunk_offset + len;
	      }
	    p_unpack->cumulated_len += len;
	    /* Copy data to its final destination */
	    if(p_unpack->status & NM_UNPACK_TYPE_IOV)
	      {
		struct iovec *iov = p_unpack->data;
		_nm_so_copy_data_in_iov(iov, chunk_offset, ptr, len);
	      }
	    else if(p_unpack->status & NM_UNPACK_TYPE_DATATYPE)
	      {
		struct DLOOP_Segment *segp = p_unpack->data;
		DLOOP_Offset last = chunk_offset + len;
		CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
	      }
	    else
	      {
		void*data = p_unpack->data;
		memcpy(data + chunk_offset, ptr, len);
	      }
	  }
	  break;
	  
	case NM_PROTO_RDV:
	  {
	    const struct nm_so_ctrl_rdv_header *rdv = header;
	    const uint32_t len = rdv->len;
	    const uint32_t chunk_offset = rdv->chunk_offset;
	    const uint8_t is_last_chunk = rdv->is_last_chunk;
	    if(is_last_chunk)
	      {
		/* Update the real size to receive */
		const uint32_t size = chunk_offset + len;
		assert(size <= p_unpack->expected_len);
		p_unpack->expected_len = chunk_offset + len;
	      }
	    nm_so_rdv_success(p_core, p_unpack, len, chunk_offset);
	  }
	  break;
	}
      /* Decrement the packet wrapper reference counter. If no other
	 chunks are still in use, the pw will be destroyed. */
      nm_so_pw_dec_header_ref_count(chunk->p_pw);
      list_del(&chunk->link);
      tbx_free(nm_so_unexpected_mem, chunk);
      if(p_unpack->cumulated_len < p_unpack->expected_len)
	chunk = nm_unexpected_find_matching(p_core, p_unpack);
      else
	{
	  /* completed- signal completion */
	  const struct nm_so_event_s event =
	    {
	      .status = NM_SO_STATUS_UNPACK_COMPLETED,
	      .p_unpack = p_unpack
	    };
	  nm_so_status_event(p_core, &event);
	  return NM_ESUCCESS;
	}
    }
  /* not completed yet- store the unpack request */
  list_add_tail(&p_unpack->link, &p_core->so_sched.unpacks);
  return NM_ESUCCESS;
}

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_gate *p_gate, nm_tag_t tag, nm_seq_t seq)
{
  int err = -NM_ENOTIMPL;
  struct nm_unpack_s*p_unpack = NULL;
  list_for_each_entry(p_unpack, &p_core->so_sched.unpacks, link)
    {
      if((p_unpack->p_gate == p_gate) && (p_unpack->tag == tag) && (p_unpack->seq == seq))
	{
	  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
	  if(p_unpack->status & (NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNEXPECTED))
	    {
	      /* receive is already in progress- too late to cancel */
	      err = -NM_EINPROGRESS;
	      return err;
	    }
	  else if(seq == p_so_tag->recv_seq_number - 1)
	    {
	      list_del(&p_unpack->link);
	      const struct nm_so_event_s event =
		{
		  .status =  NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNPACK_CANCELLED,
		  .p_unpack = p_unpack
		};
	      nm_so_status_event(p_core, &event);
	      p_so_tag->recv_seq_number--;
	      err = NM_ESUCCESS;
	      return err;
	    }
	}
    }
  return err;
}

static inline void store_data_or_rdv(struct nm_core*p_core, struct nm_gate*p_gate,
				     void *header, uint32_t len, nm_tag_t tag, nm_seq_t seq,
				     struct nm_pkt_wrap *p_pw)
{
  struct nm_unexpected_s*chunk = tbx_malloc(nm_so_unexpected_mem);
  list_add_tail(&chunk->link, &p_core->so_sched.unexpected);
  chunk->header = header;
  chunk->p_pw = p_pw;
  chunk->p_gate = p_gate;
  chunk->seq = seq;
  chunk->tag = tag;
  p_pw->header_ref_count++;
  const struct nm_so_event_s event =
    {
      .status = NM_SO_STATUS_UNEXPECTED,
      .p_gate = p_pw->p_gate,
      .tag = tag,
      .seq = seq,
      .len = len
    };
  nm_so_status_event(p_pw->p_gate->p_core, &event);
}


/** Process a complete data request.
 */
static void data_completion_callback(struct nm_pkt_wrap *p_pw,
				    void *ptr,
				    nm_so_data_header_t*header, uint32_t len,
				    nm_tag_t tag, nm_seq_t seq,
				    uint32_t chunk_offset,
				    uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_core*p_core = p_gate->p_core;
  struct nm_unpack_s*unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
  if(unpack)
    {
      /* Cool! We already have a matching unpack for this packet */
      if(is_last_chunk)
	{
	  /* Save the real length to receive */
	  unpack->expected_len = chunk_offset + len;
	}
      if(!(unpack->status & NM_SO_STATUS_UNPACK_CANCELLED))
	{
	  if(len > 0)
	    {
	      /* Copy data to its final destination */
	      if(unpack->status & NM_UNPACK_TYPE_IOV)
		{
		  /* destination is non contiguous */
		  struct iovec *iov = unpack->data;
		  _nm_so_copy_data_in_iov(iov, chunk_offset, ptr, len);
		}
	      else if(unpack->status & NM_UNPACK_TYPE_DATATYPE)
		{
		  /* data is described by a datatype */
		  DLOOP_Offset last = chunk_offset + len;
		  struct DLOOP_Segment *segp = unpack->data;
		  CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
		}
	      else
		{
		  /* contiguous data */
		  void *data = unpack->data;
		  memcpy(data, ptr, len);
		}
	    }
	  unpack->cumulated_len += len;
	  if(unpack->cumulated_len >= unpack->expected_len)
	    {
	      list_del(&unpack->link);
	      const struct nm_so_event_s event =
		{
		  .status = NM_SO_STATUS_UNPACK_COMPLETED,
		  .p_unpack = unpack
		};
	      nm_so_status_event(p_core, &event);
	    }
	  header->proto_id = NM_PROTO_DATA_UNUSED; /* mark as read */
	}
      else
	{
	  nm_so_pw_free(p_pw);
	  return;
	}
    }
  else
    {
      store_data_or_rdv(p_core, p_gate, header, len, tag, seq, p_pw);
    }
}

/** Process a received rendez-vous request.
 */
static void rdv_callback(struct nm_pkt_wrap *p_pw,
			 nm_so_generic_ctrl_header_t*rdv,
			 nm_tag_t tag, nm_seq_t seq,
			 uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_core*p_core = p_gate->p_core;
  struct nm_unpack_s*unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
  if(unpack)
    {
      /* Application is already ready! */
      if(is_last_chunk)
	{
	  /* Update the real size to receive */
	  const uint32_t size = chunk_offset + len;
	  assert(size <= unpack->expected_len);
	  unpack->expected_len = size;
	}
      nm_so_rdv_success(p_core, unpack, len, chunk_offset);
      rdv->r.proto_id = NM_PROTO_CTRL_UNUSED; /* mark as read */
    }
  else 
    {
      /* Store rdv request */
      store_data_or_rdv(p_core, p_gate, rdv, len, tag, seq, p_pw);
    }
}

/** Process a complete rendez-vous acknowledgement request.
 */
static void ack_callback(struct nm_pkt_wrap *p_ack_pw, struct nm_so_ctrl_ack_header*header)
{
  const nm_tag_t tag          = header->tag_id;
  const nm_seq_t seq          = header->seq;
  const uint32_t chunk_offset = header->chunk_offset;
  const uint32_t chunk_len    = header->chunk_len;
  struct nm_gate *p_gate      = p_ack_pw->p_gate;

  NM_SO_TRACE("ACK completed for tag = %d, seq = %u, offset = %u\n", tag, seq, chunk_offset);

  struct nm_pkt_wrap *p_large_pw = NULL;
  list_for_each_entry(p_large_pw, &p_gate->pending_large_send, link)
    {
      NM_SO_TRACE("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n",
		  p_large_pw->seq, p_large_pw->chunk_offset);
      if(p_large_pw->seq == seq && p_large_pw->chunk_offset == chunk_offset)
	{
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_large_pw, p_gate->id, 1/* large output list*/);
	  list_del(&p_large_pw->link);
	  if(chunk_len < p_large_pw->length)
	    {
	      /* partial ACK- split the packet  */
	      struct nm_pkt_wrap *p_large_pw2 = NULL;
	      nm_so_pw_split(p_large_pw, &p_large_pw2, chunk_len);
	      list_add(&p_large_pw2->link, &p_gate->pending_large_send);
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

#ifdef PIOMAN
  piom_req_success(&p_pw->inst);
#endif

  if(p_pw->trk_id == NM_TRK_SMALL)
    {
      /* ** Small packets - track #0 *********************** */
      NM_SO_TRACE("Reception of a small packet\n");
      p_gate->active_recv[p_pw->p_drv->id][NM_TRK_SMALL] = 0;
      
      nm_so_pw_iterate_over_headers(p_pw,
				    data_completion_callback,
				    rdv_callback,
				    ack_callback);
    }
  else if(p_pw->trk_id == NM_TRK_LARGE)
    {
      /* ** Large packet - track #1 ************************ */
      const nm_tag_t tag = p_pw->tag;
      const nm_seq_t seq = p_pw->seq;
      const uint32_t len = p_pw->length;
      struct nm_unpack_s*p_unpack = nm_unpack_find_matching(p_core, p_gate, seq, tag);
#warning TDOO (AD)- the matching unpack should be stored into the pw
      if(p_unpack->status & NM_UNPACK_TYPE_COPY_DATATYPE)
	{
	  /* ** Large packet, packed datatype -> finalize */
	  DLOOP_Offset last = p_pw->length;
	  /* unpack contigous data into their final destination */
	  CCSI_Segment_unpack(p_pw->segp, 0, &last, p_pw->v[0].iov_base);
	  p_unpack->cumulated_len += len;
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
	  NM_SO_TRACE("Completion of a large message on tag %d, seq %d and len = %d\n", tag, seq, len);
	  p_unpack->cumulated_len += len;
	}
      if(p_unpack->cumulated_len >= p_unpack->expected_len)
	{
	  /* notify completion */
	  list_del(&p_unpack->link);
	  const struct nm_so_event_s event =
	    {
	      .status = NM_SO_STATUS_UNPACK_COMPLETED,
	      .p_unpack = p_unpack
	    };
	  nm_so_status_event(p_core, &event);
	}
      p_gate->active_recv[p_pw->p_drv->id][NM_TRK_LARGE] = 0;
      nm_so_pw_free(p_pw);
      nm_so_process_large_pending_recv(p_gate);
    }

 out:
  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;
  return err;
}


