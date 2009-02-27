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
  int first, i, err = NM_ESUCCESS;
  struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);

  NM_SO_TRACE("Unpack_ANY_src - tag = %u\n", tag);

  if(any_src->status & NM_SO_STATUS_UNPACK_HERE)
    TBX_FAILURE("Simultaneous any_src reception on the same tag");

  first = p_so_sched->next_gate_id;
  do {
    struct nm_gate *p_gate = &p_core->gate_array[p_so_sched->next_gate_id];
    if(p_gate->status == NM_GATE_STATUS_CONNECTED)
      {
	struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
	struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
	
	const int seq = p_so_tag->recv_seq_number;
	nm_so_status_t*status = &p_so_tag->status[seq];
	
	if(*status & NM_SO_STATUS_PACKET_HERE) {
	  /* Wow! At least one data chunk already in! */
	  NM_SO_TRACE("At least one data chunk already in on gate %u\n", p_gate->id);
	  
	  *status = 0;
	  p_so_tag->recv_seq_number++;
	  
	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_HERE | flag, p_gate, tag, seq, tbx_true);
	  
	  any_src->data = data_description;
	  any_src->p_gate = p_gate;
	  any_src->seq = seq;
	  
	  p_so_sched->pending_any_src_unpacks++;
	  
	  nm_so_process_unexpected(tbx_true, p_gate, tag, seq, len, data_description);
	  goto out;
	}
      }
    p_so_sched->next_gate_id = (p_so_sched->next_gate_id + 1) % p_core->nb_gates;

  } while(p_so_sched->next_gate_id != first);

  NM_SO_TRACE("No data chunk already in - gate_id initialized at -1\n");
  any_src->status = 0;
  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_HERE | flag, NM_GATE_NONE, tag, 0, tbx_true);

  any_src->data = data_description;
  any_src->p_gate = NM_ANY_GATE;

  any_src->expected_len  = len;
  any_src->cumulated_len = 0;

  p_so_sched->pending_any_src_unpacks++;

  /* Make sure that each gate has a posted receive */
  for(i = 0; i < p_core->nb_gates; i++)
    {
      struct nm_gate*p_gate = &p_core->gate_array[i];
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  nm_so_refill_regular_recv(p_gate);
	}
    }

 out:

  return err;
}


/** Handle a sourceful unpack.
 */
int __nm_so_unpack(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
		   nm_so_flag_t flag, void *data_description, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
  nm_so_status_t *status = &(p_so_tag->status[seq]);
  int err;

  NM_SO_TRACE("Internal unpack - gate_id = %u, tag = %u, seq = %u\n", p_gate->id, tag, seq);

  if(*status & NM_SO_STATUS_PACKET_HERE) 
    {
      /* data is already here- (at least one chunk) */
      NM_SO_TRACE("At least one data chunk already in!\n");
      
      *status |= flag;
      
      /**********************************************************************************/
      /* Never access to the data stored in the 'unpack_here' side of the status matrix */
      /* It is required to retrieve the data stored in  the 'pkt_here' side before!     */
      /**********************************************************************************/
      
      err = nm_so_process_unexpected(tbx_false, p_gate, tag, seq, len, data_description);
      if(err == NM_ESUCCESS)
	goto out;
    }
  else 
    {
      /* data not here- post an unpack request */
      p_so_tag->recv[seq].unpack_here.expected_len = len;
      p_so_tag->recv[seq].unpack_here.cumulated_len = 0;
    }

  *status = 0;
  nm_so_status_event(p_gate->p_core, NM_SO_STATUS_UNPACK_HERE | flag, p_gate, tag, seq, tbx_false);

  p_so_tag->recv[seq].unpack_here.data = data_description;

  p_so_gate->pending_unpacks++;

  /* Check if we should post a new recv packet */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

int nm_so_cancel_unpack(struct nm_core*p_core, struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq)
{
  int err = -NM_ENOTIMPL;
  if(p_gate)
    {
      struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
      nm_so_status_t *status = &(p_so_tag->status[seq]);
      
      if(*status & (NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_PACKET_HERE  | NM_SO_STATUS_RDV_HERE))
	{
	  /* receive is already in progress- too late to cancel */
	  err = -NM_EINPROGRESS;
	}
      else if(seq == p_so_tag->recv_seq_number - 1)
	{
	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED | NM_SO_STATUS_UNPACK_CANCELLED,
			     p_gate, tag, seq, tbx_false);
	  p_so_tag->recv[seq].unpack_here.expected_len = 0;
	  p_so_tag->recv[seq].unpack_here.data = NULL;
	  p_so_gate->pending_unpacks--;
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
	  p_core->so_sched.pending_any_src_unpacks--;
	  err = NM_ESUCCESS;
	}
    }
  return err;
}

static int process_small_data(tbx_bool_t is_any_src,
			      struct nm_pkt_wrap *p_pw,
			      void *ptr,
			      uint32_t len, nm_tag_t tag, uint8_t seq,
			      uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  nm_so_status_t *status = NULL;
  int32_t *p_cumulated_len;
  int32_t *p_expected_len;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_so_sched->any_src, tag) : NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

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
	  return NM_SO_HEADER_MARK_UNREAD;
        }
    }

  *p_cumulated_len += len;

  /* check whether the communication is done */
  if(*p_cumulated_len >= *p_expected_len){
    NM_SO_TRACE("Wow! All data chunks were already in!\n");

    *status = 0;

    if(is_any_src)
      {
	p_so_sched->pending_any_src_unpacks--;
	any_src->p_gate = NM_ANY_GATE;
      }
    else 
      {
	p_so_gate->pending_unpacks--;
	p_so_tag->recv[seq].pkt_here.header = NULL;
      }
    nm_so_status_event(p_gate->p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, is_any_src);
  }
  return NM_SO_HEADER_MARK_READ;
}

static int store_data_or_rdv(tbx_bool_t rdv,
			     void *header, nm_tag_t tag, uint8_t seq,
			     struct nm_pkt_wrap *p_pw)
{
  struct nm_so_gate *p_so_gate = p_pw->p_gate->p_so_gate;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

  if(!(p_so_tag->status[seq] & NM_SO_STATUS_PACKET_HERE))
    {
      p_so_tag->recv[seq].pkt_here.header = header;
      p_so_tag->recv[seq].pkt_here.p_so_pw = p_pw;
    }
  else 
    {
      if(p_so_tag->recv[seq].pkt_here.chunks == NULL)
	{
	  p_so_tag->recv[seq].pkt_here.chunks = TBX_MALLOC(sizeof(struct list_head));
	  INIT_LIST_HEAD(p_so_tag->recv[seq].pkt_here.chunks);
	}
      struct nm_so_chunk *chunk = tbx_malloc(nm_so_chunk_mem);
      if (!chunk) {
	TBX_FAILURE("chunk allocation - err = -NM_ENOMEM");
      }
      chunk->header = header;
      chunk->p_so_pw = p_pw;
      
      list_add_tail(&chunk->link, p_so_tag->recv[seq].pkt_here.chunks);
    }
  const nm_so_status_t status = rdv ? (NM_SO_STATUS_RDV_HERE | NM_SO_STATUS_PACKET_HERE) : NM_SO_STATUS_PACKET_HERE;
  nm_so_status_event(p_pw->p_gate->p_core, status, p_pw->p_gate, tag, seq, tbx_false);

  return NM_SO_HEADER_MARK_UNREAD;
}


/** Process a complete data request.
 */
static int data_completion_callback(struct nm_pkt_wrap *p_pw,
				    void *ptr,
				    void *header, uint32_t len,
				    nm_tag_t proto_id, uint8_t seq,
				    uint32_t chunk_offset,
				    uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  nm_tag_t tag = proto_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

  NM_SO_TRACE("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u, is_last_chunk = %u\n",
	      ptr, len, tag, seq, chunk_offset, is_last_chunk);

  if(p_so_tag->status[seq] & NM_SO_STATUS_UNPACK_HERE)
    {
      /* Cool! We already have a waiting unpack for this packet */
      return process_small_data(tbx_false, p_pw, ptr,
				len, tag, seq, chunk_offset, is_last_chunk);
    }
  else
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);
      if ((any_src->status & NM_SO_STATUS_UNPACK_HERE)
	  &&
	  ((any_src->p_gate == NM_ANY_GATE && p_so_tag->recv_seq_number == seq)
	   || (any_src->p_gate == p_gate && any_src->seq == seq))) 
	{
	  NM_SO_TRACE("Pending any_src reception\n");
	  return process_small_data(tbx_true, p_pw, ptr,
				    len, tag, seq, chunk_offset, is_last_chunk);
	}
      else
	{
	  /* Receiver process is not ready, so store the information in the
	     recv array and keep the p_pw packet alive */
	  NM_SO_TRACE("Store the data chunk with tag = %u, seq = %u\n", tag, seq);
	  
	  return store_data_or_rdv(tbx_false, header, tag, seq, p_pw);
	}
    }
}

/** Process a complete rendez-vous request.
 */
static int rdv_callback(struct nm_pkt_wrap *p_pw,
			void *rdv,
			nm_tag_t tag_id, uint8_t seq,
			uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_tag_t tag = tag_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
  nm_so_status_t *status = &(p_so_tag->status[seq]);
  int err;

  NM_SO_TRACE("RDV completed for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);

  if(*status & NM_SO_STATUS_UNPACK_HERE)
    {
      /* Application is already ready! */
      NM_SO_TRACE("RDV for a classic exchange on tag %u, seq = %u, len = %u, chunk_offset = %u, is_last_chunk = %u\n", 
		  tag, seq, len, chunk_offset, is_last_chunk);
      err = nm_so_rdv_success(tbx_false, p_gate, tag, seq,
			      len, chunk_offset, is_last_chunk);
    }
  else 
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);
      if (( any_src->status & NM_SO_STATUS_UNPACK_HERE)
	  &&
	  ((any_src->p_gate == NM_ANY_GATE && p_so_tag->recv_seq_number == seq)
	   || (any_src->p_gate == p_gate && any_src->seq == seq)))
	{
	  /* Check if the received rdv matches with an any_src request */
	  NM_SO_TRACE("RDV for an ANY_SRC exchange for tag = %d, seq = %u len = %u, offset = %u\n",
		      tag, seq, len, chunk_offset);
	  err = nm_so_rdv_success(tbx_true, p_gate, tag, seq,
				  len, chunk_offset, is_last_chunk);
	}
      else 
	{
	  /* Store rdv request */
	  NM_SO_TRACE("Store the RDV for tag = %d, seq = %u len = %u, offset = %u\n", tag, seq, len, chunk_offset);
	  
	  return store_data_or_rdv(tbx_true, rdv, tag, seq, p_pw);
	}
    }

  return NM_SO_HEADER_MARK_READ;
}

/** Process a complete rendez-vous acknowledgement request.
 */
static int ack_callback(struct nm_pkt_wrap *p_pw,
			nm_tag_t tag_id, uint8_t seq,
			nm_trk_id_t track_id, uint32_t chunk_offset)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  struct nm_pkt_wrap *p_so_large_pw = NULL;
  nm_tag_t tag = tag_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

  NM_SO_TRACE("ACK completed for tag = %d, seq = %u, offset = %u\n", tag, seq, chunk_offset);

  p_so_gate->pending_unpacks--;
  nm_so_status_event(p_gate->p_core, NM_SO_STATUS_ACK_HERE, p_gate, tag, seq, tbx_false);

  list_for_each_entry(p_so_large_pw, &p_so_tag->pending_large_send, link) {
    NM_SO_TRACE("Searching the pw corresponding to the ack - cur_seq = %d - cur_offset = %d\n", p_so_large_pw->seq, p_so_large_pw->chunk_offset);

    if(p_so_large_pw->seq == seq && p_so_large_pw->chunk_offset == chunk_offset) {
      FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_so_large_pw, p_gate->id, 1/* large output list*/);
      list_del(&p_so_large_pw->link);

      if(p_so_tag->status[seq] & NM_SO_STATUS_IS_DATATYPE
         || nm_so_any_src_get(&p_so_sched->any_src, tag)->status & NM_SO_STATUS_IS_DATATYPE){
        int nb_blocks = 0;
        int last = p_so_large_pw->length;
        int nb_entries = NM_SO_PREALLOC_IOV_LEN - p_so_large_pw->v_nb;

        CCSI_Segment_count_contig_blocks(p_so_large_pw->segp, 0, &last, &nb_blocks);

        if(nb_blocks < nb_entries){
          NM_SO_TRACE("The data will be sent through an iovec directly from their location\n");
          CCSI_Segment_pack_vector(p_so_large_pw->segp,
                                   p_so_large_pw->datatype_offset, (DLOOP_Offset *)&last,
                                   (DLOOP_VECTOR *)p_so_large_pw->v,
                                   &nb_entries);
          NM_SO_TRACE("Pack with %d entries from byte %d to byte %d (%d bytes)\n", nb_entries, p_so_large_pw->datatype_offset, last, last-p_so_large_pw->datatype_offset);

        } else {
          NM_SO_TRACE("There is no enough space in the iovec - copy in a contiguous buffer and send\n");
          p_so_large_pw->datatype_copied_buf = tbx_true;

          p_so_large_pw->v[0].iov_base = TBX_MALLOC(p_so_large_pw->length);

          CCSI_Segment_pack(p_so_large_pw->segp,
                            p_so_large_pw->datatype_offset, &last,
                            p_so_large_pw->v[0].iov_base);
          p_so_large_pw->v[0].iov_len = last-p_so_large_pw->datatype_offset;
          nb_entries = 1;
        }

        p_so_large_pw->length = last - p_so_large_pw->datatype_offset;
        p_so_large_pw->v_nb += nb_entries;
        p_so_large_pw->datatype_offset = last;
      }

      /* Send the data */
      nm_core_post_send(p_gate, p_so_large_pw,
			track_id % NM_SO_MAX_TRACKS,
			track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }
  }
  TBX_FAILURE("PANIC!\n");
}

static int
ack_chunk_callback(struct nm_pkt_wrap *p_pw,
                   nm_tag_t tag_id, uint8_t seq, uint32_t chunk_offset,
                   nm_trk_id_t track_id, uint32_t chunk_len){
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  struct nm_pkt_wrap *p_so_large_pw, *p_so_large_pw2;
  nm_tag_t tag = tag_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

  NM_SO_TRACE("ACK_CHUNK completed for tag = %d, track_id = %u, seq = %u, chunk_len = %u, chunk_offset = %u\n", tag, track_id, seq, chunk_len, chunk_offset);

  list_for_each_entry(p_so_large_pw, &p_so_tag->pending_large_send, link) {
    NM_SO_TRACE("Searching the pw corresponding to the ack_chunk - cur_seq = %d - cur_offset = %d\n", p_so_large_pw->seq, p_so_large_pw->chunk_offset);

    if(p_so_large_pw->seq == seq && p_so_large_pw->chunk_offset == chunk_offset) {
      list_del(&p_so_large_pw->link);

      // Les données à envoyer sont décrites par un datatype
      if(p_so_tag->status[seq] & NM_SO_STATUS_IS_DATATYPE
         || nm_so_any_src_get(&p_so_sched->any_src, tag)->status & NM_SO_STATUS_IS_DATATYPE){

        int length = p_so_large_pw->length;
        int32_t first = p_so_large_pw->datatype_offset;
        int32_t last  = first + chunk_len;
        int nb_entries = 1;
        int nb_blocks = 0;
        CCSI_Segment_count_contig_blocks(p_so_large_pw->segp, first, &last, &nb_blocks);
        nb_entries = NM_SO_PREALLOC_IOV_LEN - p_so_large_pw->v_nb;

        // dans p_so_large_pw, je pack les données à émettre pour qu'il y en est chunk_len o
        if(nb_blocks < nb_entries){
          CCSI_Segment_pack_vector(p_so_large_pw->segp,
                                   first, (DLOOP_Offset *)&last,
                                   (DLOOP_VECTOR *)p_so_large_pw->v,
                                   &nb_entries);
          NM_SO_TRACE("Pack with %d entries from byte %d to byte %d (%d bytes)\n", nb_entries, p_so_large_pw->datatype_offset, last, last-p_so_large_pw->datatype_offset);
        } else {
          // on a pas assez d'entrées dans notre pw pour pouvoir envoyer tout en 1 seule fois
          // donc on copie en contigu et on envoie
          p_so_large_pw->v[0].iov_base = TBX_MALLOC(chunk_len);

          CCSI_Segment_pack(p_so_large_pw->segp, first, &last, p_so_large_pw->v[0].iov_base);
          p_so_large_pw->v[0].iov_len = last-first;
          nb_entries = 1;
        }

        p_so_large_pw->length = last - first;
        p_so_large_pw->v_nb += nb_entries;
        p_so_large_pw->datatype_offset = last;

        if(chunk_offset + chunk_len < length){
          //dans p_so_large_pw2, je laisse la fin du segment
          nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_large_pw2);
          p_so_large_pw2->p_gate   = p_so_large_pw->p_gate;
          p_so_large_pw2->proto_id = p_so_large_pw->proto_id;
          p_so_large_pw2->seq      = p_so_large_pw->seq;
          p_so_large_pw2->length   = length;
          p_so_large_pw2->v_nb     = 0;
          p_so_large_pw2->segp     = p_so_large_pw->segp;
          p_so_large_pw2->datatype_offset = last;
          p_so_large_pw2->chunk_offset = p_so_large_pw2->datatype_offset;

          list_add(&p_so_large_pw2->link, &p_so_tag->pending_large_send);
        } else {
          p_so_gate->pending_unpacks--;
        }

      // Les données sont envoyées depuis un buffer contigu ou un iov
      } else {
        if(chunk_len < p_so_large_pw->length){
	  FUT_DO_PROBE3(FUT_NMAD_NIC_RECV_ACK_RNDV, p_so_large_pw,
	  		p_gate->id, 1/* large output list*/);
          nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, chunk_len);
	  FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT, p_so_large_pw,
	  		p_so_large_pw2, chunk_len, p_gate->id, 1/* outlist*/);
          list_add(&p_so_large_pw2->link, &p_so_tag->pending_large_send);
        } else {
          p_so_gate->pending_unpacks--;
        }
      }

      /* Send the data */
      nm_core_post_send(p_gate, p_so_large_pw,
			track_id % NM_SO_MAX_TRACKS,
			track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }
  }
  TBX_FAILURE("PANIC!\n");
}

/** Process a complete incoming request.
 */
int nm_so_process_complete_recv(struct nm_core *p_core,
				struct nm_pkt_wrap *p_pw, int _err)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
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

  if (_err != NM_ESUCCESS)
    {
      TBX_FAILUREF("nm_so_process_complete_recv failed- err = %d", _err);
      goto out;
    }

  if(p_pw->trk_id == NM_TRK_SMALL) {
    /* Track 0 */
    NM_SO_TRACE("Reception of a small packet\n");
    p_so_gate->active_recv[p_pw->p_drv->id][NM_TRK_SMALL] = 0;

    nm_so_pw_iterate_over_headers(p_pw,
				  data_completion_callback,
				  rdv_callback,
                                  ack_callback,
                                  ack_chunk_callback);

    if(p_so_gate->pending_unpacks ||
       p_so_gate->p_so_sched->pending_any_src_unpacks) {

      NM_LOG_VAL("pending_unpacks", p_so_gate->pending_unpacks);
      NM_LOG_VAL("pending_any_src_unpacks", p_so_gate->p_so_sched->pending_any_src_unpacks);

      /* Check if we should post a new recv packet */
      nm_so_refill_regular_recv(p_gate);
    }

  } else if(p_pw->trk_id == NM_TRK_LARGE) {

    /* This is the completion of a large message. */
    nm_drv_id_t drv_id = p_pw->p_drv->id;
    nm_tag_t tag = p_pw->proto_id - 128;
    uint8_t seq = p_pw->seq;
    uint32_t len = p_pw->length;
    struct puk_receptacle_NewMad_Strategy_s*strategy = &p_so_gate->strategy_receptacle;
    const int nb_drivers = p_gate->p_core->nb_drivers;
    struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_so_sched->any_src, tag);
    struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

    NM_SO_TRACE("********Reception of a large packet\n");

    if(nb_drivers == 1){

      //1) C'est un datatype à extraire d'un tampon intermédiaire
      if(p_so_tag->status[seq] & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE
         || any_src->status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE){ //if(p_pw->segp != NULL){
        DLOOP_Offset last = p_pw->length;

        // Les données on été réçues en contigues -> il faut les redistribuer vers leur destination finale\n");
        CCSI_Segment_unpack(p_pw->segp, 0, &last, p_pw->v[0].iov_base);

        if(last < p_pw->length){
          // on a pas reçu assez et on le sait car on a la taille exacte à recevoir dans le rdv
          p_pw->v[0].iov_base += last; // idx global ou nb de bytes consommés?
          p_pw->v[0].iov_len -= last;

          nm_so_direct_post_large_recv(p_gate, p_pw->p_drv->id, p_pw);
          goto out;

        } else {
          TBX_FREE(p_pw->v[0].iov_base);

          if( any_src->status & NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE){
            /* Completion of an ANY_SRC unpack */
            any_src->status = 0;
            any_src->p_gate = NM_ANY_GATE;

            p_so_sched->pending_any_src_unpacks--;
            nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_true);

          } else {
            p_so_gate->pending_unpacks--;

            /* Reset the status matrix*/
            p_so_tag->recv[seq].pkt_here.header = NULL;
            p_so_tag->status[seq] = 0;

            nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_false);
          }

        }
        //2) C'est un any_src
      } else if(any_src->status & NM_SO_STATUS_RDV_IN_PROGRESS) {
        NM_SO_TRACE("Completion of a large any_src message with tag %d et seq %d\n", tag, seq);

        any_src->cumulated_len += len;

        if(any_src->cumulated_len >= any_src->expected_len){
          /* Completion of an ANY_SRC unpack */
          any_src->status = 0;
          any_src->p_gate = NM_ANY_GATE;

          p_so_sched->pending_any_src_unpacks--;
	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_true);
        }

        // 3) On a reçu directement les données à leur destination finale
      } else {
        NM_SO_TRACE("****Completion of a large message on tag %d, seq %d and len = %d\n", tag, seq, len);

        p_so_tag->recv[seq].unpack_here.cumulated_len += len;

        /* Verify if the communication is done */
        if(p_so_tag->recv[seq].unpack_here.cumulated_len >= p_so_tag->recv[seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_tag->recv[seq].pkt_here.header = NULL;
          p_so_tag->status[seq] = 0;

	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_false);
        }
       }

    } else { //Multirail case
      if((any_src->status & NM_SO_STATUS_RDV_IN_PROGRESS)
         &&
         (any_src->p_gate == p_gate)
         &&
         (any_src->seq == seq)) {
        NM_SO_TRACE("Completion of a large ANY_SRC message with tag %d et seq %d\n", tag, seq);

        any_src->cumulated_len += len;
        if(any_src->cumulated_len >= any_src->expected_len){
          /* Completion of an ANY_SRC unpack */
          any_src->status = 0;
          any_src->p_gate = NM_ANY_GATE;

          p_so_sched->pending_any_src_unpacks--;
	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_true);
        }

      } else {
        NM_SO_TRACE("Completion of a large message - tag %d, seq %d\n", tag, seq);

        p_so_tag->recv[seq].unpack_here.cumulated_len += len;
        /* Verify if the communication is done */
        if(p_so_tag->recv[seq].unpack_here.cumulated_len >= p_so_tag->recv[seq].unpack_here.expected_len){
          NM_SO_TRACE("Wow! All data chunks in!\n");

          p_so_gate->pending_unpacks--;

          /* Reset the status matrix*/
          p_so_tag->recv[seq].pkt_here.header = NULL;
          p_so_tag->status[seq] = 0;

	  nm_so_status_event(p_core, NM_SO_STATUS_UNPACK_COMPLETED, p_gate, tag, seq, tbx_false);
        }
      }
    }

    p_so_gate->active_recv[drv_id][NM_TRK_LARGE] = 0;

    /* Free the wrapper */
    nm_so_pw_free(p_pw);

    /*--------------------------------------------*/
    /* Check if some recv requests were postponed */
    if(!list_empty(&p_so_gate->pending_large_recv)) {
      union nm_so_generic_ctrl_header ctrl;
      nm_trk_id_t trk_id = NM_TRK_LARGE;

      struct nm_pkt_wrap *p_so_large_pw = nm_l2so(p_so_gate->pending_large_recv.next);

      NM_SO_TRACE("Retrieve wrapper waiting for a reception\n");
      // 1) C'est un iov à finir de recevoir
      if(nm_so_tag_get(&p_so_gate->tags, p_so_large_pw->proto_id - 128)->status[p_so_large_pw->seq]
         & NM_SO_STATUS_UNPACK_IOV
         || nm_so_any_src_get(&p_so_sched->any_src, p_so_large_pw->proto_id-128)->status & NM_SO_STATUS_UNPACK_IOV){

        /* Post next iov entry on driver drv_id */
        list_del(p_so_gate->pending_large_recv.next);

        if(p_so_large_pw->prealloc_v[0].iov_len < p_so_large_pw->length){
          struct nm_pkt_wrap *p_so_large_pw2 = NULL;

          /* Only post the first entry */
          nm_so_pw_split(p_so_large_pw, &p_so_large_pw2, p_so_large_pw->prealloc_v[0].iov_len);

          list_add_tail(&p_so_large_pw2->link, &p_so_gate->pending_large_recv);
        }

        nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

        /* Launch the ACK */
        nm_so_build_multi_ack(p_gate, p_so_large_pw->proto_id, p_so_large_pw->seq, p_so_large_pw->chunk_offset, 1, &drv_id, (uint32_t *)(&p_so_large_pw->prealloc_v[0].iov_len));



      // 2) C'est un datatype à finir de recevoir
      } else if ((nm_so_tag_get(&p_so_gate->tags, p_so_large_pw->proto_id - 128)->status[p_so_large_pw->seq]
                  & NM_SO_STATUS_IS_DATATYPE
                  || nm_so_any_src_get(&p_so_sched->any_src, p_so_large_pw->proto_id - 128)->status
                  & NM_SO_STATUS_IS_DATATYPE)
                 && p_so_large_pw->segp) {
        /* Post next iov entry on driver drv_id */
        list_del(p_so_gate->pending_large_recv.next);

        nm_so_init_large_datatype_recv_with_multi_ack(p_so_large_pw);

      // 3) C'est un wrapper normal dont on a reçu un RDV
      } else {
        list_del(p_so_gate->pending_large_recv.next);

        if(nb_drivers == 1){ // we do not search to have more NICs
          nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

          nm_so_init_ack(&ctrl, p_so_large_pw->proto_id, p_so_large_pw->seq,
                         drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
          err = strategy->driver->pack_ctrl(strategy->_status, p_gate, &ctrl);

        } else {
          int nb_drv = NM_DRV_MAX;
          nm_drv_id_t drv_ids[NM_DRV_MAX];
          uint32_t chunk_lens[NM_DRV_MAX];

          /* We ask the current strategy to find other available tracks for
             transfering this large data chunk. */
          err = strategy->driver->extended_rdv_accept(strategy->_status,
						      p_gate, p_so_large_pw->length,
						      &nb_drv, drv_ids, chunk_lens);

          if(err == NM_ESUCCESS){
            if(nb_drv == 1){ // le réseau qui vient de se libérer est le seul disponible
              nm_so_direct_post_large_recv(p_gate, drv_id, p_so_large_pw);

              nm_so_init_ack(&ctrl, p_so_large_pw->proto_id, p_so_large_pw->seq,
                             drv_id * NM_SO_MAX_TRACKS + trk_id, p_so_large_pw->chunk_offset);
              err = strategy->driver->pack_ctrl(strategy->_status, p_gate, &ctrl);

            } else {
              err = nm_so_post_multiple_pw_recv(p_gate, p_so_large_pw, nb_drv, drv_ids, chunk_lens);
              err = nm_so_build_multi_ack(p_gate,
                                          p_so_large_pw->proto_id, p_so_large_pw->seq,
                                          p_so_large_pw->chunk_offset,
                                          nb_drv, drv_ids, chunk_lens);
            }
          }
        }
      }
    }
  }

 out:
  /* Hum... Well... We're done guys! */
  err = NM_ESUCCESS;
  return err;
}


