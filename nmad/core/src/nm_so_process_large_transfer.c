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

static int nm_so_init_large_datatype_recv_with_multi_rtr(struct nm_pkt_wrap *p_pw);

static int init_large_iov_recv(struct nm_core*p_core, struct nm_unpack_s*unpack,
			       uint32_t len, uint32_t chunk_offset);

static int init_large_datatype_recv(struct nm_core*p_core, struct nm_unpack_s*unpack,
				    uint32_t len, uint32_t chunk_offset);

static void init_large_contiguous_recv(struct nm_core*p_core, struct nm_unpack_s*unpack,
				       uint32_t len, uint32_t chunk_offset);

static inline void nm_so_pw_store_pending_large_recv(struct nm_pkt_wrap*p_pw, struct nm_gate*p_gate)
{
  assert(p_pw->p_drv == NULL);
  p_pw->p_gate = p_gate;
  tbx_fast_list_add_tail(&p_pw->link, &p_gate->pending_large_recv);
}

/* ********************************************************* */

static void nm_so_post_multi_rtr(struct nm_gate*p_gate, struct nm_pkt_wrap *p_pw,
				 uint32_t chunk_offset, int nb_chunks, struct nm_rdv_chunk*chunks)
{
  int i;
  const nm_seq_t seq = p_pw->p_unpack->seq;
  const nm_core_tag_t tag = p_pw->p_unpack->tag;
  struct nm_pkt_wrap *p_pw2 = NULL;
  for(i = 0; i < nb_chunks; i++)
    {
      if(chunks[i].len < p_pw->length)
	{
	  /* create a new pw with the remaining data */
	  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw2);
	  p_pw2->p_drv    = p_pw->p_drv;
	  p_pw2->trk_id   = p_pw->trk_id;
	  p_pw2->p_gate   = p_pw->p_gate;
	  p_pw2->p_gdrv   = p_pw->p_gdrv;
	  p_pw2->p_unpack = p_pw->p_unpack;
	  /* populate p_pw2 iovec */
	  nm_so_pw_split_data(p_pw, p_pw2, chunks[i].len);
	  assert(p_pw->length == chunks[i].len);
	}
      nm_core_post_recv(p_pw, p_gate, chunks[i].trk_id, chunks[i].p_drv);
      p_pw = p_pw2;
      p_pw2 = NULL;
    }
  for(i = 0; i < nb_chunks; i++)
    {
      nm_so_post_rtr(p_gate, tag, seq, chunks[i].p_drv, chunks[i].trk_id, chunk_offset, chunks[i].len);
      chunk_offset += chunks[i].len;
    }
}


/***************************************/

/** Process a rdv request with a matching unpack already posted
 */
int nm_so_rdv_success(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
                      uint32_t len, uint32_t chunk_offset)
{
  /* 1) The final destination of the data is an iov */
  if(p_unpack->status & NM_UNPACK_TYPE_IOV)
    {
      init_large_iov_recv(p_core, p_unpack, len, chunk_offset);
    }
  /* 2) The final destination of the data is a datatype */
  else if(p_unpack->status & NM_UNPACK_TYPE_DATATYPE)
    {
      init_large_datatype_recv(p_core, p_unpack, len, chunk_offset);
    }
  /* 3) The final destination of the data is a contiguous buffer */
  else
    {
      init_large_contiguous_recv(p_core, p_unpack, len, chunk_offset);
    }
  return NM_ESUCCESS;
}

/* ** IOV ************************************************** */

/** The received rdv describes a fragment 
 * (that may span across multiple entries of the recv-side iovec)
 */
static int init_large_iov_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
			       uint32_t len, uint32_t chunk_offset)
{
  struct iovec*iov      = p_unpack->data;
  struct nm_gate*p_gate = p_unpack->p_gate;
  int pending_len = len;
  uint32_t offset = 0;
  int i = 0;
  /* find the iovec entry for the given chunk_offset */
  while(offset + iov[i].iov_len <= chunk_offset)
    {
      offset += iov[i].iov_len;
      i++;
    }
  /* store pending large recv for processing later  */
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  p_pw->p_unpack = p_unpack;
  const uint32_t iov_offset = (offset < chunk_offset) ? (chunk_offset - offset) : 0; /* offset inside the iov entry */
  nm_so_pw_add_raw(p_pw, iov[i].iov_base + iov_offset, iov[i].iov_len - iov_offset, 0);
  pending_len -= iov[i].iov_len - iov_offset;
  i++;
  while(pending_len > 0)
    {
      nm_so_pw_add_raw(p_pw, iov[i].iov_base, iov[i].iov_len, 0);
      pending_len -= iov[i].iov_len;
      i++;
    }
  p_pw->chunk_offset = chunk_offset;
  nm_so_pw_store_pending_large_recv(p_pw, p_gate);
  
  /* enqueue in the list, and immediately process one item 
   * (not necessarily the one we enqueued) */
  nm_so_process_large_pending_recv(p_gate);

  return NM_ESUCCESS;
}



/* ** Datatype ********************************************* */

static int init_large_datatype_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
				    uint32_t len, uint32_t chunk_offset)
{
  int nb_blocks = 0;
  int last = len;
  struct nm_gate*p_gate = p_unpack->p_gate;
  struct DLOOP_Segment *segp = p_unpack->data;
  const nm_core_tag_t tag = p_unpack->tag;
  const nm_seq_t seq = p_unpack->seq;

  CCSI_Segment_count_contig_blocks(segp, 0, &last, &nb_blocks);
  const int density = len / nb_blocks; /* average block size */
  if(density <= NM_SO_DATATYPE_BLOCKSIZE)
    {
      /* ** Small blocks (low density) -> receive datatype in temporary buffer
       */
      struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
      struct nm_rdv_chunk chunk = { .p_drv = NM_DRV_NONE, .trk_id = NM_TRK_LARGE };
      int nb_chunks = 1;
#warning Multirail
      int err = strategy->driver->rdv_accept(strategy->_status, p_gate, len, &nb_chunks, &chunk);
      if(err == NM_ESUCCESS)
	{
	  struct nm_pkt_wrap *p_pw = NULL;
	  void *data = TBX_MALLOC(len);
	  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
	  p_pw->p_unpack = p_unpack;
	  p_unpack->status |= NM_UNPACK_TYPE_COPY_DATATYPE;
	  nm_so_pw_add_raw(p_pw, data, len, 0);
	  p_pw->segp = segp;
	  nm_core_post_recv(p_pw, p_gate, chunk. trk_id, chunk.p_drv);
	  nm_so_post_rtr(p_gate, tag, seq, chunk.p_drv, chunk.trk_id, 0, len);
	}
      else 
	{
	  /* No free track: postpone the ack */
	  struct nm_pkt_wrap *p_pw = NULL;
	  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
	  p_pw->p_unpack = p_unpack;
	  p_pw->p_gate   = p_gate;
	  p_pw->length   = len;
	  p_pw->v_nb     = 0;
	  p_pw->segp     = segp;
	  p_pw->chunk_offset = chunk_offset;
	  nm_so_pw_store_pending_large_recv(p_pw, p_gate);
	}
    }
  else 
    {
      /* ** Large blocks (high density) -> receive datatype directly in place (-> multi-ack)
       */
      struct nm_pkt_wrap *p_pw =  NULL;
      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      p_pw->p_unpack = p_unpack;
      p_pw->p_gate   = p_gate;
      p_pw->length   = len;
      p_pw->v_nb     = 0;
      p_pw->segp     = segp;
      p_pw->datatype_offset = 0;
      p_pw->chunk_offset = chunk_offset;
      
      nm_so_init_large_datatype_recv_with_multi_rtr(p_pw);
    }
  return NM_ESUCCESS;
}



static int nm_so_init_large_datatype_recv_with_multi_rtr(struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
  struct nm_rdv_chunk chunk = { .p_drv = NM_DRV_NONE, .trk_id = NM_TRK_LARGE };
  int nb_chunks = 1;
  const uint32_t len = p_pw->length;

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
      nm_so_post_multi_rtr(p_gate, p_pw, first, nb_chunks, &chunk);

      if(last < len)
	{
	  struct nm_pkt_wrap *p_pw2;
	  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw2);
	  p_pw2->p_unpack = p_pw->p_unpack;
	  p_pw2->p_gate   = p_gate;
	  p_pw2->length   = len;
	  p_pw2->v_nb     = 0;
	  p_pw2->segp     = segp;
	  p_pw2->datatype_offset += last - first;
	  p_pw2->chunk_offset = p_pw->datatype_offset;
	  p_pw = p_pw2;
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


/* ** Contiguous ******************************************* */


static void init_large_contiguous_recv(struct nm_core*p_core, struct nm_unpack_s*p_unpack,
				       uint32_t len, uint32_t chunk_offset)
{
  struct nm_gate*p_gate = p_unpack->p_gate;
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  p_pw->p_unpack = p_unpack;
  nm_so_pw_add_raw(p_pw, p_unpack->data + chunk_offset, len, chunk_offset);
  nm_so_pw_store_pending_large_recv(p_pw, p_gate);

  /* enqueue in the list, and immediately process one item 
   * (not necessarily the one we enqueued) */
  nm_so_process_large_pending_recv(p_gate);

}


int nm_so_process_large_pending_recv(struct nm_gate*p_gate)
{
  /* ** Process postponed recv requests */
  if(!tbx_fast_list_empty(&p_gate->pending_large_recv))
    {
      struct nm_pkt_wrap *p_large_pw = nm_l2so(p_gate->pending_large_recv.next);
      struct nm_unpack_s*p_unpack = p_large_pw->p_unpack;
      assert(p_large_pw->p_unpack != NULL);
      if(p_unpack->status & NM_UNPACK_TYPE_IOV)
	{
	  /* ** iov to be completed */
	  int nb_chunks = 1;
	  struct nm_rdv_chunk chunk = {
	    .p_drv  = p_large_pw->p_drv,
	    .trk_id = p_large_pw->trk_id,
	    .len    = p_large_pw->v[0].iov_len 
	  };
	  const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, p_large_pw->v[0].iov_len, &nb_chunks, &chunk);
	  if(err == NM_ESUCCESS)
	    {
	      tbx_fast_list_del(p_gate->pending_large_recv.next);
	      if(p_large_pw->v[0].iov_len < p_large_pw->length)
		{
		  /* iovec with multiple entries- post an rtr only for the first entry. */
		  struct nm_pkt_wrap *p_pw2 = NULL;
		  /* create a new pw with the remaining data */
		  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw2);
		  p_pw2->p_unpack = p_large_pw->p_unpack;
		  /* populate p_pw2 iovec */
		  nm_so_pw_split_data(p_large_pw, p_pw2, p_large_pw->v[0].iov_len); 
		  /* store the remaining pw to receive later */
		  nm_so_pw_store_pending_large_recv(p_pw2, p_gate);
		}
	      nm_so_post_multi_rtr(p_gate, p_large_pw, p_large_pw->chunk_offset, nb_chunks, &chunk);
	    }
	}
      else if(p_unpack->status & NM_UNPACK_TYPE_DATATYPE)
	{
	  /* ** datatype to be completed */
	  /* Post next iov entry on driver drv_id */
	  tbx_fast_list_del(p_gate->pending_large_recv.next);
	  nm_so_init_large_datatype_recv_with_multi_rtr(p_large_pw);
	}
      else 
	{
	  /* ** contiguous pw with rdv */
	  int nb_chunks = p_gate->p_core->nb_drivers;
	  struct nm_rdv_chunk chunks[nb_chunks];
	  const struct puk_receptacle_NewMad_Strategy_s*strategy = &p_gate->strategy_receptacle;
	  int err = strategy->driver->rdv_accept(strategy->_status, p_gate, p_large_pw->length, &nb_chunks, chunks);
	  if(err == NM_ESUCCESS)
	    {
	      tbx_fast_list_del(p_gate->pending_large_recv.next);
	      nm_so_post_multi_rtr(p_gate, p_large_pw,
				   p_large_pw->chunk_offset, nb_chunks, chunks);
	    }
	}
    }
  return NM_ESUCCESS;
}
