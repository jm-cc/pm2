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


/** Handle a source-less unpack.
 */
int __nm_so_unpack_any_src(struct nm_core *p_core, nm_tag_t tag, nm_so_flag_t flag,
			   void *data_description, uint32_t len)
{
  struct nm_so_sched *p_so_sched = &p_core->so_sched;
  int err = NM_ESUCCESS;
  struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);

  NM_SO_TRACE("Unpack_ANY_src - tag = %u\n", tag);

  if(any_src->status & NM_SO_STATUS_UNPACK_HERE)
    TBX_FAILURE("Simultaneous any_src reception on the same tag");

  any_src->data = data_description;
  any_src->p_gate = NM_ANY_GATE;
  any_src->expected_len  = len;
  any_src->cumulated_len = 0;

  int first = p_so_sched->next_gate_id;
  do {
    struct nm_gate *p_gate = &p_core->gate_array[p_so_sched->next_gate_id];
    if(p_gate->status == NM_GATE_STATUS_CONNECTED)
      {
	struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
	
	const int seq = p_so_tag->recv_seq_number;
	nm_so_status_t*status = &p_so_tag->status[seq];
	if(*status & NM_SO_STATUS_PACKET_HERE)
	  {
	    /* Wow! At least one data chunk already in! */
	    NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);
	    *status = 0;
	    p_so_tag->recv_seq_number++;
	    const struct nm_so_event_s event =
	      {
		.status = NM_SO_STATUS_UNPACK_HERE | flag,
		.p_gate = p_gate,
		.tag = tag,
		.seq = seq,
		.any_src = tbx_true
	      };
	    nm_so_status_event(p_core, &event);
	    any_src->p_gate = p_gate;
	    any_src->seq = seq;
	    nm_so_process_unexpected(tbx_true, p_gate, tag, seq, len, data_description);
	    goto out;
	  }
      }
    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");
  any_src->status = 0;
  const struct nm_so_event_s event =
    {
      .status = NM_SO_STATUS_UNPACK_HERE | flag,
      .p_gate = NM_GATE_NONE,
      .tag = tag,
      .seq = 0,
      .any_src = tbx_true
    };
  nm_so_status_event(p_core, &event);

 out:

  return err;
}


/** Handle a sourceful unpack.
 */
int __nm_so_unpack(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
		   nm_so_flag_t flag, void *data_description, uint32_t len)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  nm_so_status_t *status = &(p_so_tag->status[seq]);
  int err;

  NM_SO_TRACE("Internal unpack - gate_id = %u, tag = %u, seq = %u\n", p_gate->id, tag, seq);

  /* data not here- post an unpack request */
  p_so_tag->recv[seq].unpack_here.expected_len = len;
  p_so_tag->recv[seq].unpack_here.cumulated_len = 0;

  if(*status & NM_SO_STATUS_PACKET_HERE) 
    {
      /* data is already here- (at least one chunk) */
      NM_SO_TRACE("At least one data chunk already in!\n");
      *status |= flag;
      err = nm_so_process_unexpected(tbx_false, p_gate, tag, seq, len, data_description);
      if(err == NM_ESUCCESS)
	goto out;
    }

  *status = 0;
  const struct nm_so_event_s event =
    {
      .status = NM_SO_STATUS_UNPACK_HERE | flag,
      .p_gate = p_gate,
      .tag = tag,
      .seq = seq,
      .any_src = tbx_false
    };
  nm_so_status_event(p_gate->p_core, &event);

  p_so_tag->recv[seq].unpack_here.data = data_description;

  err = NM_ESUCCESS;

 out:
  return err;
}

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq)
{
  int err = -NM_ENOTIMPL;
  if(p_gate)
    {
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
      nm_so_status_t *status = &(p_so_tag->status[seq]);
      
      if(*status & (NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_PACKET_HERE  | NM_SO_STATUS_RDV_HERE))
	{
	  /* receive is already in progress- too late to cancel */
	  err = -NM_EINPROGRESS;
	}
      else if(seq == p_so_tag->recv_seq_number - 1)
	{
	  const struct nm_so_event_s event =
	    {
	      .status =  NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNPACK_CANCELLED,
	      .p_gate = p_gate,
	      .tag = tag,
	      .seq = seq,
	      .any_src = tbx_false
	    };
	  nm_so_status_event(p_core, &event);
	  p_so_tag->recv[seq].unpack_here.expected_len = 0;
	  p_so_tag->recv[seq].unpack_here.data = NULL;
	  p_so_tag->recv_seq_number--;
	  err = NM_ESUCCESS;
	}
      else
	{
	  err = -NM_ENOTIMPL;
	}
    }
  else
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_core->so_sched.any_src, tag);
      if(any_src->status & (NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_PACKET_HERE  | NM_SO_STATUS_RDV_HERE))
	{
	  /* receive is already in progress- too late to cancel */
	  err = -NM_EINPROGRESS;
	}
      else
	{
	  any_src->status = 0;
	  any_src->data = NULL;
	  any_src->expected_len = 0;
	  err = NM_ESUCCESS;
	}
    }
  return err;
}

static void process_small_data(tbx_bool_t is_any_src,
			       struct nm_pkt_wrap *p_pw,
			       void *ptr,
			       uint32_t len, nm_tag_t tag, uint8_t seq,
			       uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_sched *p_so_sched =  &p_gate->p_core->so_sched;
  nm_so_status_t *status = NULL;
  int32_t *p_cumulated_len;
  int32_t *p_expected_len;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_so_sched->any_src, tag) : NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  if(is_any_src)
    {
      status = &any_src->status;
      p_cumulated_len = &any_src->cumulated_len;
      p_expected_len  = &any_src->expected_len;
    }
  else
    {
      status = &(p_so_tag->status[seq]);
      p_cumulated_len = &(p_so_tag->recv[seq].unpack_here.cumulated_len);
      p_expected_len  = &(p_so_tag->recv[seq].unpack_here.expected_len);
    }

  /* Save the real length to receive */
  if(is_last_chunk){
    *p_expected_len = chunk_offset + len;
  }

  if(is_any_src && any_src->p_gate == NM_ANY_GATE)
    {
      p_so_tag->recv_seq_number++;
      any_src->p_gate = p_gate;
      any_src->seq = seq;
    }

  if(len)
    {
      /* Copy data to its final destination */
      if(*status & NM_SO_STATUS_UNPACK_IOV)
	{
	  /* destination is non contiguous */
	  struct iovec *iov = is_any_src ? any_src->data : p_so_tag->recv[seq].unpack_here.data;
	  _nm_so_copy_data_in_iov(iov, chunk_offset, ptr, len);
	}
      else if(*status & NM_SO_STATUS_IS_DATATYPE)
	{
	  /* data is described by a datatype */
	  DLOOP_Offset last = chunk_offset + len;
	  struct DLOOP_Segment *segp = is_any_src ? any_src->data : p_so_tag->recv[seq].unpack_here.data;
	  CCSI_Segment_unpack(segp, chunk_offset, &last, ptr);
	}
      else if(! (*status & NM_SO_STATUS_UNPACK_CANCELLED))
	{
	  /* contiguous data */
	  void *data = is_any_src ? (any_src->data + chunk_offset) : (p_so_tag->recv[seq].unpack_here.data + chunk_offset);
	  memcpy(data, ptr, len);
	}
      else
        {
	  nm_so_pw_free(p_pw);
	  return;
        }
    }

  *p_cumulated_len += len;

  /* check whether the communication is done */
  if(*p_cumulated_len >= *p_expected_len){
    NM_SO_TRACE("Wow! All data chunks were already in!\n");

    *status = 0;

    if(is_any_src)
      {
	any_src->p_gate = NM_ANY_GATE;
      }
    else 
      {
	p_so_tag->recv[seq].pkt_here.header = NULL;
      }
    const struct nm_so_event_s event =
      {
	.status = NM_SO_STATUS_UNPACK_COMPLETED,
	.p_gate = p_gate,
	.tag = tag,
	.seq = seq,
	.any_src = is_any_src
      };
    nm_so_status_event(p_gate->p_core, &event);
  }
}

static inline void store_data_or_rdv(nm_so_status_t status,
				     void *header, uint32_t len, nm_tag_t tag, uint8_t seq,
				     struct nm_pkt_wrap *p_pw)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_pw->p_gate->tags, tag);
  struct nm_so_chunk *chunk = &p_so_tag->recv[seq].pkt_here;
  if(!chunk->header)
    {
      INIT_LIST_HEAD(&chunk->link);
    }
  else
    {
      chunk = tbx_malloc(nm_so_chunk_mem);
      list_add_tail(&chunk->link, &p_so_tag->recv[seq].pkt_here.link);
    }
  chunk->header = header;
  chunk->p_pw = p_pw;
  const struct nm_so_event_s event =
    {
      .status = status,
      .p_gate = p_pw->p_gate,
      .tag = tag,
      .seq = seq,
      .len = len,
      .any_src = tbx_false
    };
  nm_so_status_event(p_pw->p_gate->p_core, &event);
}


/** Process a complete data request.
 */
static void data_completion_callback(struct nm_pkt_wrap *p_pw,
				    void *ptr,
				    nm_so_data_header_t*header, uint32_t len,
				    nm_tag_t proto_id, uint8_t seq,
				    uint32_t chunk_offset,
				    uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  const nm_tag_t tag = proto_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  NM_SO_TRACE("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u, is_last_chunk = %u\n",
	      ptr, len, tag, seq, chunk_offset, is_last_chunk);

  if(p_so_tag->status[seq] & NM_SO_STATUS_UNPACK_HERE)
    {
      /* Cool! We already have a waiting unpack for this packet */
      process_small_data(tbx_false, p_pw, ptr, len, tag, seq, chunk_offset, is_last_chunk);
      header->proto_id = NM_SO_PROTO_DATA_UNUSED; /* mark as read */
    }
  else
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag);
      if ((any_src->status & NM_SO_STATUS_UNPACK_HERE) &&
	  ((any_src->p_gate == NM_ANY_GATE && p_so_tag->recv_seq_number == seq)
	   || (any_src->p_gate == p_gate && any_src->seq == seq))) 
	{
	  NM_SO_TRACE("Pending any_src reception\n");
	  process_small_data(tbx_true, p_pw, ptr, len, tag, seq, chunk_offset, is_last_chunk);
	  header->proto_id = NM_SO_PROTO_DATA_UNUSED; /* mark as read */
	}
      else
	{
	  /* Receiver process is not ready, so store the information in the
	     recv array and keep the p_pw packet alive */
	  NM_SO_TRACE("Store the data chunk with tag = %u, seq = %u\n", tag, seq);
	  store_data_or_rdv(NM_SO_STATUS_PACKET_HERE, header, len, tag, seq, p_pw);
	  p_pw->header_ref_count++;
	}
    }
}

/** Process a complete rendez-vous request.
 */
static void rdv_callback(struct nm_pkt_wrap *p_pw,
			 nm_so_generic_ctrl_header_t*rdv,
			 nm_tag_t tag_id, uint8_t seq,
			 uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  const nm_tag_t tag = tag_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  nm_so_status_t *status = &(p_so_tag->status[seq]);

  NM_SO_TRACE("RDV completed for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE)
    {
      /* Application is already ready! */
      NM_SO_TRACE("RDV for a classic exchange on tag %u, seq = %u, len = %u, chunk_offset = %u, is_last_chunk = %u\n", 
		  tag, seq, len, chunk_offset, is_last_chunk);
      nm_so_rdv_success(tbx_false, p_gate, tag, seq, len, chunk_offset, is_last_chunk);
      rdv->r.proto_id = NM_SO_PROTO_CTRL_UNUSED;

    }
  else 
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag);
      if (( any_src->status & NM_SO_STATUS_UNPACK_HERE)
	  &&
	  ((any_src->p_gate == NM_ANY_GATE && p_so_tag->recv_seq_number == seq)
	   || (any_src->p_gate == p_gate && any_src->seq == seq)))
	{
	  /* Check if the received rdv matches with an any_src request */
	  NM_SO_TRACE("RDV for an ANY_SRC exchange for tag = %d, seq = %u len = %u, offset = %u\n",
		      tag, seq, len, chunk_offset);
	  nm_so_rdv_success(tbx_true, p_gate, tag, seq, len, chunk_offset, is_last_chunk);
	  rdv->r.proto_id = NM_SO_PROTO_CTRL_UNUSED;
	}
      else 
	{
	  /* Store rdv request */
	  NM_SO_TRACE("Store the RDV for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);
	  store_data_or_rdv(NM_SO_STATUS_RDV_HERE | NM_SO_STATUS_PACKET_HERE, rdv, len, tag, seq, p_pw);
	  p_pw->header_ref_count++;
	}
    }
}

/** Process a complete rendez-vous acknowledgement request.
 */
static void ack_callback(struct nm_pkt_wrap *p_ack_pw, struct nm_so_ctrl_ack_header*header)
{
  const nm_tag_t tag          = header->tag_id - 128;
  const uint8_t seq           = header->seq;
  const uint32_t chunk_offset = header->chunk_offset;
  const uint32_t chunk_len    = header->chunk_len;
  struct nm_gate *p_gate = p_ack_pw->p_gate;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  NM_SO_TRACE("ACK completed for tag = %d, seq = %u, offset = %u\n", tag, seq, chunk_offset);

  const struct nm_so_event_s event =
    {
      .status = NM_SO_STATUS_ACK_HERE,
      .p_gate = p_gate,
      .tag = tag,
      .seq = seq,
      .any_src = tbx_false
    };
  nm_so_status_event(p_gate->p_core, &event);

  struct nm_pkt_wrap *p_large_pw = NULL;
  list_for_each_entry(p_large_pw, &p_so_tag->pending_large_send, link)
    {
      NM_SO_TRACE("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n",
		  p_large_pw->seq, p_large_pw->chunk_offset);
      if(p_large_pw->seq == seq && p_large_pw->chunk_offset == chunk_offset)
	{
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_large_pw, p_gate->id, 1/* large output list*/);
	  list_del(&p_large_pw->link);
	  if(p_so_tag->status[seq] & NM_SO_STATUS_IS_DATATYPE)
	    {
	      /* large data is datatype */
	      int length = p_large_pw->length;
	      DLOOP_Offset first = p_large_pw->datatype_offset;
	      DLOOP_Offset last  = first + chunk_len;
	      int nb_entries = p_large_pw->v_size - p_large_pw->v_nb;
	      /* AD: TODO with resizable iovecs, packing datatype into vector or contigous
	       * should depend upon blocks density, not iovec current size.
	       */
	      int nb_blocks = 0;
	      CCSI_Segment_count_contig_blocks(p_large_pw->segp, first, &last, &nb_blocks);
	      if(nb_blocks < nb_entries)
		{
		  /* pack datatype into an iovec */
		  CCSI_Segment_pack_vector(p_large_pw->segp, first, &last, (DLOOP_VECTOR *)p_large_pw->v, &nb_entries);
		  NM_SO_TRACE("Pack with %d entries from byte %d to byte %d (%d bytes)\n",
			      nb_entries, p_large_pw->datatype_offset, last, last-p_large_pw->datatype_offset);
		}
	      else 
		{
		  /* pack datatype into a contiguous buffer (by copy) */
		  NM_SO_TRACE("There is no enough space in the iovec - copy in a contiguous buffer and send\n");
		  p_large_pw->datatype_copied_buf = tbx_true;
		  p_large_pw->v[0].iov_base = TBX_MALLOC(chunk_len);
		  CCSI_Segment_pack(p_large_pw->segp, first, &last, p_large_pw->v[0].iov_base);
		  p_large_pw->v[0].iov_len = last - first;
		  nb_entries = 1;
		}
	      p_large_pw->length = last - first;
	      p_large_pw->v_nb += nb_entries;
	      p_large_pw->datatype_offset = last;

	      if(chunk_offset + chunk_len < length)
		{
		  /* store the remaining data */
		  struct nm_pkt_wrap *p_large_pw2 = NULL;
		  nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_large_pw2);
		  p_large_pw2->p_gate   = p_large_pw->p_gate;
		  p_large_pw2->proto_id = p_large_pw->proto_id;
		  p_large_pw2->seq      = p_large_pw->seq;
		  p_large_pw2->length   = length;
		  p_large_pw2->v_nb     = 0;
		  p_large_pw2->segp     = p_large_pw->segp;
		  p_large_pw2->datatype_offset = last;
		  p_large_pw2->chunk_offset = p_large_pw2->datatype_offset;
		  list_add(&p_large_pw2->link, &p_so_tag->pending_large_send);
		}
	    }
	  else
	    {
	      /* large data is contiguous or iovec */
	      if(chunk_len < p_large_pw->length)
		{
		  /* partial ACK- split the packet  */
		  struct nm_pkt_wrap *p_large_pw2 = NULL;
		  nm_so_pw_split(p_large_pw, &p_large_pw2, chunk_len);
		  list_add(&p_large_pw2->link, &p_so_tag->pending_large_send);
		}
	    }
	  /* send the data */
	  nm_core_post_send(p_gate, p_large_pw, header->trk_id, header->drv_id);
	  header->proto_id = NM_SO_PROTO_CTRL_UNUSED; /* mark as read */
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
  struct nm_so_sched *p_so_sched = &p_core->so_sched;
  int err;

  NM_TRACEF("recv request complete: gate %d, drv %d, trk %d, proto %d, seq %d",
	    p_pw->p_gate->id,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id,
	    p_pw->seq);

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
      const nm_tag_t tag = p_pw->proto_id - 128;
      const uint8_t seq = p_pw->seq;
      const uint32_t len = p_pw->length;
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
      
      NM_SO_TRACE("Reception of a large packet\n");

      if(p_so_tag->status[seq] & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE
	 || any_src->status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE)
	{
	  /* ** 1.1. Large packet, datatype -> extract data in temp buffer */
	  DLOOP_Offset last = p_pw->length;
	  /* unpack contigous data into their final destination */
	  CCSI_Segment_unpack(p_pw->segp, 0, &last, p_pw->v[0].iov_base);
	  if(last < p_pw->length)
	    {
	      /* we are expecting more data */
	      p_pw->v[0].iov_base += last;
	      p_pw->v[0].iov_len -= last;
	      nm_so_direct_post_large_recv(p_gate, p_pw->p_drv->id, p_pw);
	      goto out;
	    }
	  else
	    {
	      TBX_FREE(p_pw->v[0].iov_base);
	      if(any_src->status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE)
		{
		  /* large packet, datatype, anysrc */
		  any_src->status = 0;
		  any_src->p_gate = NM_ANY_GATE;
		  const struct nm_so_event_s event =
		    {
		      .status = NM_SO_STATUS_UNPACK_COMPLETED,
		      .p_gate = p_gate,
		      .tag = tag,
		      .seq = seq,
		      .any_src = tbx_true
		    };
		  nm_so_status_event(p_core, &event);
		}
	      else 
		{
		  /* large packet, datatype, known source */
		  p_so_tag->recv[seq].pkt_here.header = NULL;
		  p_so_tag->status[seq] = 0;
		  const struct nm_so_event_s event =
		    {
		      .status = NM_SO_STATUS_UNPACK_COMPLETED,
		      .p_gate = p_gate,
		      .tag = tag,
		      .seq = seq,
		      .any_src = tbx_false
		    };
		  nm_so_status_event(p_core, &event);
		}
	    }
	}
      else if(any_src->status & NM_SO_STATUS_RDV_IN_PROGRESS)
	{
	  /* ** 1.2. Large packet, anysrc */
	  NM_SO_TRACE("Completion of a large any_src message with tag %d et seq %d\n", tag, seq);
	  any_src->cumulated_len += len;
	  if(any_src->cumulated_len >= any_src->expected_len)
	    {
	      any_src->status = 0;
	      any_src->p_gate = NM_ANY_GATE;
	      const struct nm_so_event_s event =
		{
		  .status = NM_SO_STATUS_UNPACK_COMPLETED,
		  .p_gate = p_gate,
		  .tag = tag,
		  .seq = seq,
		  .any_src = tbx_true
		};
	      nm_so_status_event(p_core, &event);
	    }
	}
      else
	{
	  /* ** 1.3. data received directly in its final destination */
	  NM_SO_TRACE("Completion of a large message on tag %d, seq %d and len = %d\n", tag, seq, len);
	  p_so_tag->recv[seq].unpack_here.cumulated_len += len;
	  if(p_so_tag->recv[seq].unpack_here.cumulated_len >= p_so_tag->recv[seq].unpack_here.expected_len)
	    {
	      NM_SO_TRACE("Wow! All data chunks in!\n");
	      p_so_tag->recv[seq].pkt_here.header = NULL;
	      p_so_tag->status[seq] = 0;
	      const struct nm_so_event_s event =
		{
		  .status = NM_SO_STATUS_UNPACK_COMPLETED,
		  .p_gate = p_gate,
		  .tag = tag,
		  .seq = seq,
		  .any_src = tbx_false
		};
	      nm_so_status_event(p_core, &event);
	    }
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


