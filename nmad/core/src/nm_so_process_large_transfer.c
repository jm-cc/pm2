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

static int nm_so_init_large_datatype_recv_with_multi_ack(struct nm_pkt_wrap *p_so_pw);

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

static int store_iov_waiting_transfer(struct nm_gate *p_gate,
                                      nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
                                      struct iovec *iov, int nb_entries, uint32_t pending_len,
                                      uint32_t first_iov_offset);

static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 nm_tag_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp);


/* ********************************************************* */

static int nm_so_build_multi_ack(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
				 int nb_chunks, struct nm_rdv_chunk*chunks)
{
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  union nm_so_generic_ctrl_header ctrl;
  struct nm_pkt_wrap *p_so_acks_pw = NULL;
  int err, i;

  NM_SO_TRACE("Building of a multi-ack with %d chunks on tag %d and seq %d\n", nb_chunks, tag, seq);

  nm_so_init_multi_ack(&ctrl, nb_chunks, tag, seq, chunk_offset);
  err = strategy->driver->pack_extended_ctrl(strategy->_status,
					     p_gate, NM_SO_CTRL_HEADER_SIZE * (nb_chunks+1), &ctrl, &p_so_acks_pw);
  for(i = 0; i < nb_chunks; i++)
    {
      NM_SO_TRACE("NM_SO_PROTO_ACK_CHUNK - drv_id = %d, trk_id = %d, chunk_len =%u\n",
		  chunks[i].drv_id, chunks[i].trk_id, chunks[i].len);
      nm_so_add_ack_chunk(&ctrl, &chunks[i]);
      err = strategy->driver->pack_ctrl_chunk(strategy->_status, p_so_acks_pw, &ctrl);
    }
  err = strategy->driver->pack_extended_ctrl_end(strategy->_status, p_gate, p_so_acks_pw);
  return err;
}

static inline int nm_so_post_large_recv(struct nm_gate *p_gate,
					struct nm_rdv_chunk*chunk,
					nm_tag_t tag, uint8_t seq,
					void *data)
{
  struct nm_pkt_wrap *p_pw = NULL;
  int err = nm_so_pw_alloc_and_fill_with_data(tag, seq, data, chunk->len,
					      0, 0, NM_SO_DATA_DONT_USE_HEADER,
					      &p_pw);
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
      nm_so_pw_split(p_pw, &p_pw2, chunks[i].len);
      nm_so_direct_post_large_recv(p_gate, chunks[i].drv_id, p_pw);
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
  struct nm_so_sched *p_so_sched = &p_gate->p_core->so_sched;
  int err;
  struct nm_so_any_src_s*any_src = is_any_src ? nm_so_any_src_get(&p_so_sched->any_src, tag) : NULL;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  nm_so_status_t *status = NULL;
  if(is_any_src){
    status = &(any_src->status);
  } else {
    status = &(p_so_tag->status[seq]);
  }

  // Update the real size to receive
  if(is_last_chunk){
    if(is_any_src){
      any_src->expected_len = chunk_offset + len;
    } else {
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

  // 1) The final destination of the data is desribed by an iov
  if(*status & NM_SO_STATUS_UNPACK_IOV){
    struct iovec *iov = NULL;

    if(is_any_src){
      iov = any_src->data;
    } else {
      iov = p_so_tag->recv[seq].unpack_here.data;
    }

    err = init_large_iov_recv(p_gate, tag, seq, iov, len, chunk_offset);


  // 2) The final destination of the data is desribed by a datatype
  } else if(*status & NM_SO_STATUS_IS_DATATYPE){
    err = init_large_datatype_recv(is_any_src, p_gate, tag, seq, len, chunk_offset);

  // 3) The final destination of the data is a contiguous buffer
  } else {
    void *data = NULL;
    NM_SO_TRACE("RDV_sucess - contiguous data with tag %d , seq %d, chunk_offset %d, any_src %d\n",
                tag, seq, chunk_offset, is_any_src);

    if(is_any_src){
      data = any_src->data + chunk_offset;
    } else {
      data = p_so_tag->recv[seq].unpack_here.data + chunk_offset;
    }

    err = init_large_contiguous_recv(p_gate, tag, seq, data, len, chunk_offset);
  }

  return err;
}






/*********************************************************************/
/***** Functions which ask for an available NIC      *****************/
/****  and send the data if there is at least one    *****************/
/****  or store them if not                          *****************/
/*********************************************************************/

//************ IOV ************/

/* fragment d'un message à envoyer qui s'étend sur plusieurs entrées de l'iov de réception*/
static int init_large_iov_recv(struct nm_gate *p_gate, nm_tag_t tag, uint8_t seq,
                               struct iovec *iov, uint32_t len, uint32_t chunk_offset)
{
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  struct nm_pkt_wrap *p_so_acks_pw = NULL;
  struct nm_rdv_chunk chunk = { .len = len, .drv_id = NM_DRV_DEFAULT, .trk_id = NM_TRK_LARGE };
  int nb_chunks = 1;
  uint32_t offset = 0;
  int i = 0;

  /* Go to the right iovec entry */
  while(offset + iov[i].iov_len <= chunk_offset)
    {
      offset += iov[i].iov_len;
      i++;
    }
  if(offset + iov[i].iov_len >= chunk_offset + len)
    {
    /* Data are contiguous */
      strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
      
      nm_so_post_large_recv(p_gate, &chunk, tag+128, seq,
			    iov[i].iov_base + (chunk_offset - offset));
      
      /* Launch the ACK */
      nm_so_build_multi_ack(p_gate, tag+128, seq, chunk_offset, nb_chunks, &chunk);

    goto out;
  }

  /* Data are on several entries of the iovec */
  union nm_so_generic_ctrl_header ctrl[NM_SO_PREALLOC_IOV_LEN];
  uint32_t pending_len = len;
  int nb_entries = 1;
  uint32_t cur_len = 0;

#warning TODO:multirail
  uint32_t chunk_len = iov[i].iov_len + (chunk_offset - offset);

  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, chunk_len, &nb_chunks, &chunk);
  if(err != NM_ESUCCESS)
    {
      /* No free track */
      err = store_iov_waiting_transfer(p_gate,
				       tag + 128, seq, chunk_offset,
				       &iov[i], nb_entries, pending_len,
				       iov[i].iov_len + (chunk_offset - offset));
      goto out;
    }
  nm_so_post_large_recv(p_gate, &chunk,
                        tag+128, seq,
                        iov[i].iov_base + (chunk_offset - offset));
  nm_so_add_ack_chunk(&ctrl[nb_entries], &chunk);

  nb_entries++;
  pending_len -= chunk_len;
  offset += iov[i].iov_len;
  cur_len += chunk_len;
  i++;

  while(pending_len)
    {
      /* We ask the current strategy to find an available track
	 for transfering this large data chunk.*/
      chunk_len = iov[i].iov_len;
      err = strategy->driver->rdv_accept(strategy->_status, p_gate, chunk_len, &nb_chunks, &chunk);
      if (err != NM_ESUCCESS)
	{
	  /* No free track */
	  err = store_iov_waiting_transfer(p_gate,
					   tag + 128, seq, chunk_offset+cur_len,
					   &iov[i], nb_entries, pending_len, 0);
	  goto ack_to_send;
	}
      nm_so_post_large_recv(p_gate, &chunk, tag+128, seq, iov[i].iov_base);
      nm_so_add_ack_chunk(&ctrl[nb_entries], &chunk);
      nb_entries++;
      pending_len -= chunk_len;
      offset += iov[i].iov_len;
      cur_len += chunk_len;
      i++;
    }
  nb_entries--;

 ack_to_send:
  /* Launch the acks */
  nm_so_init_multi_ack(&ctrl[0], nb_entries-1, tag+128, seq, chunk_offset);
  strategy->driver->pack_extended_ctrl(strategy->_status, p_gate,
				       NM_SO_CTRL_HEADER_SIZE * nb_entries, &ctrl[0], &p_so_acks_pw);
  for(i = 1; i < nb_entries; i++)
    {
      strategy->driver->pack_ctrl_chunk(strategy->_status, p_so_acks_pw, &ctrl[i]);
    }
  strategy->driver->pack_extended_ctrl_end(strategy->_status, p_gate, p_so_acks_pw);

 out:
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
	  union nm_so_generic_ctrl_header ctrl;
	  struct nm_pkt_wrap *p_so_pw = NULL;
	  void *data = TBX_MALLOC(len);
	  nm_so_pw_alloc_and_fill_with_data(tag, seq,
					    data, len,
					    0, 0, NM_SO_DATA_DONT_USE_HEADER,
					    &p_so_pw);
	  p_so_pw->segp = segp;
	  nm_core_post_recv(p_so_pw, p_gate, chunk.trk_id, chunk.drv_id);
	  if(is_any_src)
	    {
	      nm_so_any_src_get(&p_gate->p_core->so_sched.any_src, tag-128)->status |= NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE;
	    }
	  else
	    {
	      nm_so_tag_get(&p_gate->tags, tag - 128)->status[seq] |= NM_SO_STATUS_UNPACK_RETRIEVE_DATATYPE;
	    }
	  nm_so_init_ack(&ctrl, tag+128, seq, chunk.drv_id, chunk.trk_id, 0);
	  err = strategy->driver->pack_ctrl(strategy->_status, p_gate, &ctrl);
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
      struct nm_pkt_wrap *p_so_pw =  NULL;
      nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
      
      p_so_pw->p_gate   = p_gate;
      p_so_pw->proto_id = tag + 128;
      p_so_pw->seq      = seq;
      p_so_pw->length   = len;
      p_so_pw->v_nb     = 0;
      p_so_pw->segp     = segp;
      p_so_pw->datatype_offset = 0;
      p_so_pw->chunk_offset = chunk_offset;
      
      nm_so_init_large_datatype_recv_with_multi_ack(p_so_pw);
    }
  return NM_ESUCCESS;
}



static int nm_so_init_large_datatype_recv_with_multi_ack(struct nm_pkt_wrap *p_so_pw)
{
  struct nm_gate *p_gate = p_so_pw->p_gate;
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  struct nm_rdv_chunk chunk = { .drv_id = NM_DRV_DEFAULT, .trk_id = NM_TRK_LARGE };
  int nb_chunks = 1;
  nm_tag_t tag = p_so_pw->proto_id;
  uint8_t seq = p_so_pw->seq;
  uint32_t len = p_so_pw->length;

#warning Multi-rail?
  /* We ask the current strategy to find an available track for
     transfering this large data chunk. */
  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
  if(err == NM_ESUCCESS)
    {
      int nb_entries = 1;
      uint32_t last = len;
      struct DLOOP_Segment *segp = p_so_pw->segp;
      uint32_t first = p_so_pw->datatype_offset;
      CCSI_Segment_pack_vector(segp,
			       first, (DLOOP_Offset *)&last,
			       (DLOOP_VECTOR *)p_so_pw->v,
			       &nb_entries);
      assert(nb_entries == 1);
      p_so_pw->length = last - first;
      p_so_pw->v_nb = nb_entries;
      // depot de la reception
      nm_so_direct_post_large_recv(p_gate, chunk.drv_id, p_so_pw);
      
      // init le multi_ack
      NM_SO_TRACE("Multi-ack building for a block with tag %d, length %do, chunk_offset %d\n",
		  tag, last - first, first);
      chunk.len = p_so_pw->length;
      nm_so_build_multi_ack(p_gate, tag, seq, first, nb_chunks, &chunk);

    if(last < len){
      goto store;
    } else {
      goto out;
    }

  store:
    //stockage du reste du segment
    nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);

    p_so_pw->p_gate   = p_gate;
    p_so_pw->proto_id = tag;
    p_so_pw->seq      = seq;
    p_so_pw->length   = len;
    p_so_pw->v_nb     = 0;
    p_so_pw->segp     = segp;
    p_so_pw->datatype_offset += last - first;
    p_so_pw->chunk_offset = p_so_pw->datatype_offset;
  }

  /* No free track: postpone the ack */
  list_add_tail(&p_so_pw->link, &p_gate->pending_large_recv);

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
      if(nb_chunks == 1)
	{
	  /* one track/driver available (or strategy cannot handle multi-rail) */
	  union nm_so_generic_ctrl_header ctrl;
	  nm_so_post_large_recv(p_gate, &chunks[0], tag+128, seq, data);
	  nm_so_init_ack(&ctrl, tag+128, seq, chunks[0].drv_id, chunks[0].trk_id, chunk_offset);
	  err = strategy->driver->pack_ctrl(strategy->_status, p_gate, &ctrl);
	}
      else
	{
	  nm_so_post_multiple_data_recv(p_gate, nb_chunks, chunks, tag + 128, seq, data);
	  nm_so_build_multi_ack(p_gate, tag + 128, seq, chunk_offset, nb_chunks, chunks);
	}
    }
  else
    {
      /* No free track: postpone the ack */
      struct nm_pkt_wrap *p_so_pw = NULL;
      nm_so_pw_alloc_and_fill_with_data(tag+128, seq, data, len, chunk_offset, 0,
					NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
      list_add_tail(&p_so_pw->link, &p_gate->pending_large_recv);
    }
  return err;
}



/*****************************************************************************************/
/******* Functions which allow to store the data while there is not any available NIC ****/
/*****************************************************************************************/

static int store_iov_waiting_transfer(struct nm_gate *p_gate,
                                      nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
                                      struct iovec *iov, int nb_entries, uint32_t pending_len,
                                      uint32_t first_iov_offset){
  struct nm_pkt_wrap *p_so_pw = NULL;
  uint32_t cur_len = pending_len;
  int err, i;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  p_so_pw->p_gate   = p_gate;
  p_so_pw->proto_id = tag;
  p_so_pw->seq      = seq;
  p_so_pw->length   = pending_len;
  p_so_pw->v_nb     = nb_entries;

  p_so_pw->chunk_offset = chunk_offset;

  p_so_pw->prealloc_v[0].iov_len  = iov[0].iov_len - first_iov_offset;
  p_so_pw->prealloc_v[0].iov_base = iov[0].iov_base + first_iov_offset;
  cur_len -= p_so_pw->prealloc_v[0].iov_len;

  for(i = 1; i < nb_entries-1; i++){
    p_so_pw->prealloc_v[i].iov_len  = iov[i].iov_len;
    p_so_pw->prealloc_v[i].iov_base = iov[i].iov_base;
    cur_len -= p_so_pw->prealloc_v[i].iov_len;
  }

  p_so_pw->prealloc_v[i].iov_len  = cur_len;
  p_so_pw->prealloc_v[i].iov_base = iov[i].iov_base;

  list_add_tail(&p_so_pw->link, &p_gate->pending_large_recv);

  err = NM_ESUCCESS;
 out:
  return err;
}

static int store_large_datatype_waiting_transfer(struct nm_gate *p_gate,
                                                 nm_tag_t tag, uint8_t seq,
                                                 uint32_t len, uint32_t chunk_offset,
                                                 struct DLOOP_Segment *segp){
  struct nm_pkt_wrap *p_so_pw = NULL;

  nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);

  p_so_pw->p_gate   = p_gate;
  p_so_pw->proto_id = tag;
  p_so_pw->seq      = seq;
  p_so_pw->length   = len;
  p_so_pw->v_nb     = 0;
  p_so_pw->segp     = segp;
  p_so_pw->chunk_offset = chunk_offset;

  list_add_tail(&p_so_pw->link, &p_gate->pending_large_recv);

  return NM_ESUCCESS;
}


int nm_so_process_large_pending_recv(struct nm_gate*p_gate)
{
  struct nm_so_sched *p_so_sched = &p_gate->p_core->so_sched;

  /* ** Process postponed recv requests */
  if(!list_empty(&p_gate->pending_large_recv))
    {
      union nm_so_generic_ctrl_header ctrl;
      struct nm_pkt_wrap *p_large_pw = nm_l2so(p_gate->pending_large_recv.next);
      NM_SO_TRACE("Retrieve wrapper waiting for a reception\n");
      if(nm_so_tag_get(&p_gate->tags, p_large_pw->proto_id - 128)->status[p_large_pw->seq]
	 & NM_SO_STATUS_UNPACK_IOV
	 || nm_so_any_src_get(&p_so_sched->any_src, p_large_pw->proto_id-128)->status & NM_SO_STATUS_UNPACK_IOV)
	{
	  /* ** iov to be completed */
	  list_del(p_gate->pending_large_recv.next);
	  if(p_large_pw->prealloc_v[0].iov_len < p_large_pw->length)
	    {
	      struct nm_pkt_wrap *p_large_pw2 = NULL;
	      /* Only post the first entry */
	      nm_so_pw_split(p_large_pw, &p_large_pw2, p_large_pw->prealloc_v[0].iov_len);
	      list_add_tail(&p_large_pw2->link, &p_gate->pending_large_recv);
	    }
	  struct nm_rdv_chunk chunk = {
	    .drv_id = p_large_pw->p_drv->id,
	    .trk_id = p_large_pw->trk_id,
	    .len = p_large_pw->prealloc_v[0].iov_len 
	  };
	  nm_so_direct_post_large_recv(p_gate, chunk.drv_id, p_large_pw);
	  
	  /* Launch the ACK */
	  nm_so_build_multi_ack(p_gate, p_large_pw->proto_id, p_large_pw->seq, p_large_pw->chunk_offset, 
				1, &chunk);
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
	      if(nb_chunks == 1)
		{
		  nm_so_direct_post_large_recv(p_gate, chunks[0].drv_id, p_large_pw);
		  nm_so_init_ack(&ctrl, p_large_pw->proto_id, p_large_pw->seq,
				 chunks[0].drv_id, chunks[0].trk_id, p_large_pw->chunk_offset);
		  err = strategy->driver->pack_ctrl(strategy->_status, p_gate, &ctrl);
		}
	      else 
		{
		  err = nm_so_post_multiple_pw_recv(p_gate, p_large_pw, nb_chunks, chunks);
		  err = nm_so_build_multi_ack(p_gate,
					      p_large_pw->proto_id, p_large_pw->seq,
					      p_large_pw->chunk_offset,
					      nb_chunks, chunks);
		}
	    }
	}
    }
  return NM_ESUCCESS;
}
