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
#include <ccs_public.h>
#include <segment.h>

static int nm_so_init_large_datatype_recv_with_multi_ack(struct nm_pkt_wrap *p_pw);

static int init_large_iov_recv(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
                               struct iovec *iov, uint32_t len, uint32_t chunk_offset);

static int init_large_datatype_recv(tbx_bool_t is_any_src,
                                    struct nm_gate *p_gate,
                                    nm_tag_t tag, uint8_t seq,
                                    uint32_t len, uint32_t chunk_offset);

static int init_large_contiguous_recv(struct nm_gate *p_gate,
                                      nm_tag_t tag, uint8_t seq,
                                      void *data, uint32_t len,
                                      uint32_t chunk_offset);

static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 nm_tag_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp);

static inline void nm_so_pw_store_pending_large_recv(struct nm_pkt_wrap*p_pw, struct nm_gate*p_gate)
{
  assert(p_pw->trk_id == NM_TRK_LARGE);
  assert(p_pw->p_drv != NULL);
  list_add_tail(&p_pw->link, &p_gate->pending_large_recv);
}

/* ********************************************************* */

static void nm_so_build_multi_ack(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
				  int nb_chunks, struct nm_rdv_chunk*chunks)
{
  int i;

  NM_SO_TRACE("Building of a multi-ack with %d chunks on tag %d and seq %d\n", nb_chunks, tag, seq);

  for(i = 0; i < nb_chunks; i++)
    {
      NM_SO_TRACE("NM_SO_PROTO_ACK_CHUNK - drv_id = %d, trk_id = %d, chunk_len =%u\n",
		  chunks[i].drv_id, chunks[i].trk_id, chunks[i].len);
      nm_so_post_ack(p_gate, tag, seq, chunks[i].drv_id, chunks[i].trk_id, chunk_offset, chunks[i].len);
      chunk_offset += chunks[i].len;
    }
}

static inline int nm_so_post_large_recv(struct nm_gate *p_gate,
					struct nm_rdv_chunk*chunk,
					nm_tag_t tag, uint8_t seq,
					void *data)
{
  struct nm_pkt_wrap *p_pw = NULL;
  int err = nm_so_pw_alloc_and_fill_with_data(tag, seq, data, chunk->len,
					      0, 0, NM_PW_NOHEADER, &p_pw);
  if(err == NM_ESUCCESS)
    {
      nm_core_post_recv(p_pw, p_gate, chunk->trk_id, chunk->drv_id);
      err = NM_ESUCCESS;
    }
  return err;
}

static inline int nm_so_post_multiple_data_recv(struct nm_gate *p_gate,
						int nb_chunks, struct nm_rdv_chunk*chunks,
						nm_tag_t tag, uint8_t seq, void *data)
{
  uint32_t offset = 0;
  int i;
  for(i = 0; i < nb_chunks; i++)
    {
      nm_so_post_large_recv(p_gate, &chunks[i], tag, seq, data + offset);
      offset += chunks[i].len;
    }
  return NM_ESUCCESS;
}

static inline int nm_so_post_multiple_pw_recv(struct nm_gate *p_gate,
					      struct nm_pkt_wrap *p_pw,
					      int nb_chunks, struct nm_rdv_chunk*chunks)
{
  struct nm_pkt_wrap *p_pw2 = NULL;
  int i;
  for(i = 0; i < nb_chunks; i++)
    {
      if(chunks[i].len < p_pw->length)
	nm_so_pw_split(p_pw, &p_pw2, chunks[i].len);
      nm_core_post_recv(p_pw, p_gate, chunks[i].trk_id, chunks[i].drv_id);
      p_pw = p_pw2;
    }
  return NM_ESUCCESS;

}


/***************************************/


int nm_so_rdv_success(tbx_bool_t is_any_src,
                      struct nm_gate *p_gate,
                      nm_tag_t tag, uint8_t seq,
                      uint32_t len,
                      uint32_t chunk_offset,
                      uint8_t is_last_chunk)
{
  int err;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag) : NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  nm_so_status_t *status = is_any_src ? &(any_src->status) : &(p_so_tag->status[seq]);

  /* Update the real size to receive */
  if(is_last_chunk)
    {
      if(is_any_src)
	{
	  any_src->expected_len = chunk_offset + len;
	} 
      else 
	{
	  p_so_tag->recv[seq].unpack_here.expected_len = chunk_offset + len;
	}
    }

  if(is_any_src)
    {
      const struct nm_so_event_s event = 
	{
	  .status = NM_SO_STATUS_RDV_IN_PROGRESS,
	  .p_gate = p_gate,
	  .tag = tag,
	  .seq = seq,
	  .any_src = tbx_true
	};
      nm_so_status_event(p_gate->p_core, &event);
    }

  if(is_any_src && any_src->p_gate == NM_ANY_GATE)
    {
      p_so_tag->recv_seq_number++;
      any_src->p_gate = p_gate;
      any_src->seq = seq;
    }
  
  /* 1) The final destination of the data is an iov */
  if(*status & NM_SO_STATUS_UNPACK_IOV)
    {
      struct iovec *iov = is_any_src ?
	any_src->data : p_so_tag->recv[seq].unpack_here.data;
      err = init_large_iov_recv(p_gate, tag, seq, iov, len, chunk_offset);
      
    }
  /* 2) The final destination of the data is a datatype */
  else if(*status & NM_SO_STATUS_IS_DATATYPE)
    {
      err = init_large_datatype_recv(is_any_src, p_gate, tag, seq, len, chunk_offset);
    }
  /* 3) The final destination of the data is a contiguous buffer */
  else
    {
      void *data = is_any_src ?
	( any_src->data + chunk_offset) : (p_so_tag->recv[seq].unpack_here.data + chunk_offset);
      NM_SO_TRACE("RDV_sucess - contiguous data with tag %d , seq %d, chunk_offset %d, any_src %d\n",
		  tag, seq, chunk_offset, is_any_src);
      err = init_large_contiguous_recv(p_gate, tag, seq, data, len, chunk_offset);
    }

  return err;
}

//************ IOV ************/

/** The received rdv describes a fragment 
 * (that may span across multiple entries of the recv-side iovec)
 */
static int init_large_iov_recv(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
                               struct iovec *iov, uint32_t len, uint32_t chunk_offset)
{
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  struct nm_rdv_chunk chunk = { .len = len, .drv_id = NM_DRV_DEFAULT, .trk_id = NM_TRK_LARGE };
  int nb_chunks = 1;
  uint32_t offset = 0;
  int i = 0;
 
  /* find the iovec entry for the given chunk_offset */
  while(offset + iov[i].iov_len <= chunk_offset)
    {
      offset += iov[i].iov_len;
      i++;
    }

  if(offset + iov[i].iov_len >= chunk_offset + len)
    {
      /* Single entry- regular ack */
      strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
      nm_so_post_large_recv(p_gate, &chunk, tag+128, seq, iov[i].iov_base + (chunk_offset - offset));
      /* Launch the ACK */
      nm_so_build_multi_ack(p_gate, tag+128, seq, chunk_offset, nb_chunks, &chunk);
    }
  else
    {
      /* Data spans across multiples entries of the iovec */
      uint32_t pending_len = len;
      int nb_entries = 1;
      uint32_t cur_len = 0;
      
#warning TODO:multirail

      int err = NM_ESUCCESS;
      while(err == NM_ESUCCESS && pending_len > 0)
	{
	  const uint32_t iov_offset = (offset < chunk_offset) ? (chunk_offset - offset) : 0; /* offset inside the iov entry, for partial entries */
	  const uint32_t chunk_len = iov[i].iov_len - iov_offset;
	  err = strategy->driver->rdv_accept(strategy->_status, p_gate, chunk_len, &nb_chunks, &chunk);
	  if(err != NM_ESUCCESS)
	    {
	      /* no free track- store pending large recv for processing later  */
	      struct nm_pkt_wrap *p_pw = NULL;
	      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
	      p_pw->proto_id = tag + 128;
	      p_pw->seq      = seq;
	      p_pw->length   = pending_len;
	      p_pw->v_nb     = nb_entries;
	      p_pw->chunk_offset = chunk_offset + cur_len;
	      /* TODO (AD)- convert this iov filling into nm_so_pw_add_data for resizable iovec */
	      p_pw->v[0].iov_len  = iov[i].iov_len - iov_offset;
	      p_pw->v[0].iov_base = iov[i].iov_base + iov_offset;
	      pending_len -= chunk_len;
	      int j;
	      for(j = 1; pending_len > 0; j++)
		{
		  p_pw->v[j].iov_len  = iov[j+i].iov_len;
		  p_pw->v[j].iov_base = iov[j+i].iov_base;
		  cur_len     -= p_pw->v[j].iov_len;
		  pending_len -= p_pw->v[j].iov_len;
		}
	      nm_so_pw_assign(p_pw, chunk.trk_id, chunk.drv_id, p_gate);
	      nm_so_pw_store_pending_large_recv(p_pw, p_gate);
	    }
	  else
	    {
	      /* post a recv and prepare an ack for the given chunk */
	      nm_so_post_large_recv(p_gate, &chunk, tag + 128, seq, iov[i].iov_base + iov_offset);
	      nm_so_post_ack(p_gate, tag + 128, seq, chunk.drv_id, chunk.trk_id, chunk_offset, chunk_len);
	      nb_entries++;
	      pending_len -= chunk_len;
	      offset += iov[i].iov_len;
	      cur_len += chunk_len;
	      i++;
	    }
	}
    }
  
  return NM_ESUCCESS;
}



//************ DATATYPE ************/

static int init_large_datatype_recv(tbx_bool_t is_any_src,
                                    struct nm_gate *p_gate,
                                    nm_tag_t tag, uint8_t seq,
                                    uint32_t len, uint32_t chunk_offset){
  int nb_blocks = 0;
  int last = len;
  NM_SO_TRACE("RDV reception for data described by a datatype\n");
  struct DLOOP_Segment *segp = (is_any_src) ?
    nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag)->data
    : nm_so_tag_get(&p_gate->tags, tag)->recv[seq].unpack_here.data;

  CCSI_Segment_count_contig_blocks(segp, 0, &last, &nb_blocks);
  const int density = len / nb_blocks; /* average block size */
  if(density <= NM_SO_DATATYPE_BLOCKSIZE)
    {
      /* ** Small blocks (low density) -> receive datatype in temporary buffer
       */
      struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
      struct nm_rdv_chunk chunk = { .drv_id = NM_DRV_DEFAULT, .trk_id = NM_TRK_LARGE };
      int nb_chunks = 1;
#warning Multirail
      int err = strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
      if(err == NM_ESUCCESS)
	{
	  struct nm_pkt_wrap *p_pw = NULL;
	  void *data = TBX_MALLOC(len);
	  nm_so_pw_alloc_and_fill_with_data(tag, seq, data, len,
					    0, 0, NM_PW_NOHEADER, &p_pw);
	  p_pw->segp = segp;
	  nm_core_post_recv(p_pw, p_gate, chunk. trk_id, chunk.drv_id);
	  if(is_any_src)
	    {
	      nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag-128)->status |= NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE;
	    }
	  else
	    {
	      nm_so_tag_get(&p_gate->tags, tag - 128)->status[seq] |= NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE;
	    }
	  nm_so_post_ack(p_gate, tag + 128, seq, chunk.drv_id, chunk.trk_id, 0, len);
	}
      else 
	{
	  /* No free track: postpone the ack */
	  err = store_large_datatype_waiting_transfer(p_gate, tag + 128, seq, len, chunk_offset, segp);
	}
    }
  else 
    {
      /* ** Large blocks (high density) -> receive datatype directly in place (-> multi-ack)
       */
      struct nm_pkt_wrap *p_pw =  NULL;
      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      p_pw->p_gate   = p_gate;
      p_pw->proto_id = tag + 128;
      p_pw->seq      = seq;
      p_pw->length   = len;
      p_pw->v_nb     = 0;
      p_pw->segp     = segp;
      p_pw->datatype_offset = 0;
      p_pw->chunk_offset = chunk_offset;
      
      nm_so_init_large_datatype_recv_with_multi_ack(p_pw);
    }
  return NM_ESUCCESS;
}



static int nm_so_init_large_datatype_recv_with_multi_ack(struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  struct nm_rdv_chunk chunk = { .drv_id = NM_DRV_DEFAULT, .trk_id = NM_TRK_LARGE };
  int nb_chunks = 1;
  nm_tag_t tag = p_pw->proto_id;
  uint8_t seq = p_pw->seq;
  uint32_t len = p_pw->length;

#warning Multi-rail?
  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
  if(err == NM_ESUCCESS)
    {
      int nb_entries = 1;
      uint32_t last = len;
      struct DLOOP_Segment *segp = p_pw->segp;
      uint32_t first = p_pw->datatype_offset;
      CCSI_Segment_pack_vector(segp,
			       first, (DLOOP_Offset *)&last,
			       (DLOOP_VECTOR *)p_pw->v,
			       &nb_entries);
      assert(nb_entries == 1);
      p_pw->length = last - first;
      p_pw->v_nb = nb_entries;
      chunk.len = p_pw->length; /* TODO- should be given by rdv_accept */
      nm_so_post_multiple_pw_recv(p_gate, p_pw, 1, &chunk);
      nm_so_build_multi_ack(p_gate, tag, seq, first, nb_chunks, &chunk);

      if(last < len)
	{
	  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
	  p_pw->p_gate   = p_gate;
	  p_pw->proto_id = tag;
	  p_pw->seq      = seq;
	  p_pw->length   = len;
	  p_pw->v_nb     = 0;
	  p_pw->segp     = segp;
	  p_pw->datatype_offset += last - first;
	  p_pw->chunk_offset = p_pw->datatype_offset;
	}
      else 
	{
	  goto out;
	}
    }

  /* No free track: postpone the ack */
  nm_so_pw_store_pending_large_recv(p_pw, p_gate);

 out:
  return err;
}


//************ CONTIGUOUS ************/


static int init_large_contiguous_recv(struct nm_gate *p_gate,
                                      nm_tag_t tag, uint8_t seq,
                                      void *data, uint32_t len,
                                      uint32_t chunk_offset)
{
  int err;
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  /** Handle a matching rendez-vous.
      rdv_success is called when a RdV request and an UNPACK operation
      match together. Basically, we just have to check if there is a
      track available for the large data transfer and to send an ACK if
      so. Otherwise, we simply postpone the acknowledgement.
  */
  struct nm_rdv_chunk chunks[NM_DRV_MAX];
  int nb_chunks = NM_DRV_MAX;
  NM_SO_TRACE("-->single_rdv with tag %d, seq %d, len %d, chunk_offset %d\n", tag, seq, len, chunk_offset);
  err = strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, chunks);
  if(err == NM_ESUCCESS)
    {
      nm_so_post_multiple_data_recv(p_gate, nb_chunks, chunks, tag + 128, seq, data);
      nm_so_build_multi_ack(p_gate, tag + 128, seq, chunk_offset, nb_chunks, chunks);
    }
  else
    {
      /* No free track: postpone the ack */
      struct nm_pkt_wrap *p_pw = NULL;
      nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len, chunk_offset, 0,
					NM_PW_NOHEADER, &p_pw);
      nm_so_pw_assign(p_pw, NM_TRK_LARGE, NM_DRV_DEFAULT, p_gate); /* TODO */
      nm_so_pw_store_pending_large_recv(p_pw, p_gate);
    }
  return err;
}



/*****************************************************************************************/
/******* Functions which allow to store the data while there is not any available NIC ****/
/*****************************************************************************************/


static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 nm_tag_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp){
  struct nm_pkt_wrap *p_pw = NULL;

  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);

  p_pw->p_gate   = p_gate;
  p_pw->proto_id = tag;
  p_pw->seq      = seq;
  p_pw->length   = len;
  p_pw->v_nb     = 0;
  p_pw->segp     = segp;
  p_pw->chunk_offset = chunk_offset;

  nm_so_pw_store_pending_large_recv(p_pw, p_gate);

  return NM_ESUCCESS;
}


int nm_so_process_large_pending_recv(struct nm_gate*p_gate)
{
  struct nm_so_sched *p_so_sched = &p_gate->p_core->so_sched;

  /* ** Process postponed recv requests */
  if(!list_empty(&p_gate->pending_large_recv))
    {
      struct nm_pkt_wrap *p_large_pw = nm_l2so(p_gate->pending_large_recv.next);
      NM_SO_TRACE("Retrieve wrapper waiting for a reception\n");
      if(nm_so_tag_get(&p_gate->tags, p_large_pw->proto_id - 128)->status[p_large_pw->seq] & NM_SO_STATUS_UNPACK_IOV
	 || nm_so_any_src_get(&p_so_sched->any_src, p_large_pw->proto_id-128)->status & NM_SO_STATUS_UNPACK_IOV)
	{
	  /* ** iov to be completed */
	  list_del(p_gate->pending_large_recv.next);
	  if(p_large_pw->v[0].iov_len < p_large_pw->length)
	    {
	      struct nm_pkt_wrap *p_large_pw2 = NULL;
	      /* Only post the first entry */
	      nm_so_pw_split(p_large_pw, &p_large_pw2, p_large_pw->v[0].iov_len);
	      nm_so_pw_store_pending_large_recv(p_large_pw2, p_gate);
	    }
	  struct nm_rdv_chunk chunk = {
	    .drv_id = p_large_pw->p_drv->id,
	    .trk_id = p_large_pw->trk_id,
	    .len = p_large_pw->v[0].iov_len 
	  };
	  nm_so_post_multiple_pw_recv(p_gate, p_large_pw, 1, &chunk);
	  nm_so_build_multi_ack(p_gate, p_large_pw->proto_id, p_large_pw->seq, p_large_pw->chunk_offset, 1, &chunk);
	}
      else if ((nm_so_tag_get(&p_gate->tags, p_large_pw->proto_id - 128)->status[p_large_pw->seq]
		& NM_SO_STATUS_IS_DATATYPE
		|| nm_so_any_src_get(&p_so_sched->any_src, p_large_pw->proto_id - 128)->status
		& NM_SO_STATUS_IS_DATATYPE)
	       && p_large_pw->segp)
	{
	  /* ** datatype to be completed */
	  /* Post next iov entry on driver drv_id */
	  list_del(p_gate->pending_large_recv.next);
	  nm_so_init_large_datatype_recv_with_multi_ack(p_large_pw);
	}
      else 
	{
	  /* ** contiguous pw with rdv */
	  struct nm_rdv_chunk chunks[NM_DRV_MAX];
	  int nb_chunks = NM_DRV_MAX;
	  const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, p_large_pw->length,
						 &nb_chunks, chunks);
	  if(err == NM_ESUCCESS)
	    {
	      list_del(p_gate->pending_large_recv.next);
	      err = nm_so_post_multiple_pw_recv(p_gate, p_large_pw, nb_chunks, chunks);
	      nm_so_build_multi_ack(p_gate, p_large_pw->proto_id, p_large_pw->seq,
				    p_large_pw->chunk_offset, nb_chunks, chunks);
	    }
	}
    }
  return NM_ESUCCESS;
}
