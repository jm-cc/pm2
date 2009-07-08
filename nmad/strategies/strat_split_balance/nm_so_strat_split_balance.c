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

/* Components structures:
 */

static int strat_split_balance_pack(void*, struct nm_gate*, nm_tag_t, nm_seq_t, const void*, uint32_t);
static int strat_split_balance_packv(void*, struct nm_gate*, nm_tag_t, nm_seq_t, const struct iovec *, int);
static int strat_split_balance_pack_datatype(void*, struct nm_gate*, nm_tag_t, uint8_t, const struct DLOOP_Segment*);
static int strat_split_balance_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_split_balance_try_and_commit(void*, struct nm_gate*);
static int strat_split_balance_rdv_accept(void*, struct nm_gate*, uint32_t, int*, struct nm_rdv_chunk*);

static const struct nm_strategy_iface_s nm_so_strat_split_balance_driver =
  {
    .pack               = &strat_split_balance_pack,
    .packv              = &strat_split_balance_packv,
    .pack_datatype      = &strat_split_balance_pack_datatype,
    .pack_ctrl          = &strat_split_balance_pack_ctrl,
    .try_and_commit     = &strat_split_balance_try_and_commit,
#ifdef NMAD_QOS
    .ack_callback    = NULL,
#endif /* NMAD_QOS */
    .rdv_accept          = &strat_split_balance_rdv_accept,
    .flush               = NULL
};

static void*strat_split_balance_instanciate(puk_instance_t, puk_context_t);
static void strat_split_balance_destroy(void*);

static const struct puk_adapter_driver_s nm_so_strat_split_balance_adapter_driver =
  {
    .instanciate = &strat_split_balance_instanciate,
    .destroy     = &strat_split_balance_destroy
  };

struct nm_so_strat_split_balance {
  /* list of raw outgoing packets */
  struct list_head out_list;
  unsigned nb_packets;
  int nm_so_max_small;
  int nm_so_copy_on_send_threshold;
};

/** Component declaration */
static int nm_strat_split_balance_load(void)
{
  puk_component_declare("NewMad_Strategy_split_balance",
			puk_component_provides("PadicoAdapter", "adapter", &nm_so_strat_split_balance_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_split_balance_driver),
			puk_component_attr("nm_so_max_small", NULL),
			puk_component_attr("nm_so_copy_on_send_threshold", NULL));
  return NM_ESUCCESS;
}
PADICO_MODULE_BUILTIN(NewMad_Strategy_split_balance, &nm_strat_split_balance_load, NULL, NULL);


/** Initialize the gate storage for split_balance strategy.
 */
static void*strat_split_balance_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_split_balance *status = TBX_MALLOC(sizeof(struct nm_so_strat_split_balance));

  INIT_LIST_HEAD(&status->out_list);

  status->nb_packets = 0;

  NM_LOGF("[loading strategy: <split_balance>]");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[nm_so_max_small=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_context_getattr(context, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[nm_so_copy_on_send_threshold=%i]", status->nm_so_copy_on_send_threshold);

  return (void*)status;
}

/** Cleanup the gate storage for split_balance strategy.
 */
static void strat_split_balance_destroy(void*status)
{
  TBX_FREE(status);
}

/* Add a new control "header" to the flow of outgoing packets */
static int
strat_split_balance_pack_ctrl(void *_status,
			      struct nm_gate *p_gate,
			      const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_split_balance*status = _status;
  int err;

  if(!list_empty(&status->out_list)) {
    /* Inspect only the head of the list */
    p_so_pw = nm_l2so(status->out_list.next);

    /* If the paquet is reasonably small, we can form an aggregate */
    if(NM_SO_CTRL_HEADER_SIZE <= nm_so_pw_remaining_header_area(p_so_pw)){

      struct nm_pkt_wrap TBX_UNUSED dummy_p_so_pw;
      FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET, &dummy_p_so_pw, 0, 0, 0);
      FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, &dummy_p_so_pw);
      FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG, p_gate->id, 0, 0, &dummy_p_so_pw, p_so_pw);

      err = nm_so_pw_add_control(p_so_pw, p_ctrl);
      goto out;
    }
  }

  /* Otherwise, simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* TODO tag should be filled correctly */
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET, p_so_pw, 0, 0, p_so_pw->length);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 0, p_so_pw);

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &status->out_list);
  status->nb_packets++;

 out:
  return err;
}


static int
strat_split_balance_launch_large_chunk(void *_status,
				       struct nm_gate *p_gate,
				       nm_tag_t tag, nm_seq_t seq,
				       const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  int err;

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                          data, len, chunk_offset, is_last_chunk,
                                          NM_PW_NOHEADER,
                                          &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;


  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 1, p_so_pw);

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(nm_so_tag_get(&p_gate->tags, tag)->pending_large_send));

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, chunk_offset, is_last_chunk);

    NM_SO_TRACE("RDV pack_ctrl\n");
    err = strat_split_balance_pack_ctrl(_status, p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }
  err = NM_ESUCCESS;
 out:
  return err;
}

static int
strat_split_balance_try_to_agregate_small(void *_status,
					  struct nm_gate *p_gate,
					  nm_tag_t tag, nm_seq_t seq,
					  const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_pkt_wrap *p_so_pw;
  struct nm_so_strat_split_balance*status = _status;
  int flags = 0;
  int err;

  /* We aggregate ONLY if data are very small OR if there are
     already two ready packets */
  if(len <= 512 || status->nb_packets >= 2)
    {
      /* We first try to find an existing packet to form an aggregate */
      list_for_each_entry(p_so_pw, &status->out_list, link)
	{
	  const uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
	  const uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);
	  if(size <= h_rlen)
	    {
	      /* We can copy data into the header zone */
	      flags = NM_SO_DATA_USE_COPY;
	      struct nm_pkt_wrap TBX_UNUSED dummy_p_so_pw;
	      FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, &dummy_p_so_pw, tag, seq, len);
	      FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, &dummy_p_so_pw);
	      FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG, p_gate->id, 0, 0, &dummy_p_so_pw, p_so_pw);
	      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, chunk_offset, is_last_chunk, flags);
	      goto out;
	    }
	}
    }

  flags = NM_PW_GLOBAL_HEADER | NM_SO_DATA_USE_COPY;

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk, flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 0, p_so_pw);

  list_add_tail(&p_so_pw->link, &status->out_list);
  status->nb_packets++;

  err = NM_ESUCCESS;
 out:
  return err;
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int
strat_split_balance_pack(void *_status,
			 struct nm_gate *p_gate,
			 nm_tag_t tag, nm_seq_t seq,
			 const void *data, uint32_t len)
{
  int err;
  struct nm_so_strat_split_balance*status = _status;

  nm_so_tag_get(&p_gate->tags, tag)->send[seq] = len;

  if(len <= status->nm_so_max_small) {
    NM_SO_TRACE("PACK of a small one - tag = %u, seq = %u, len = %u\n", tag, seq, len);

    /* Small packet */
    err = strat_split_balance_try_to_agregate_small(_status, p_gate, tag, seq, data, len, 0, 1);

  } else {
    NM_SO_TRACE("PACK of a large one - tag = %u, seq = %u, len = %u\n", tag, seq, len);

    /* Large packets are splited in 2 chunks. */
    err = strat_split_balance_launch_large_chunk(_status, p_gate, tag, seq, data, len, 0, 1);
  }

  return err;
}

static int
strat_split_balance_packv(void *_status,
			  struct nm_gate *p_gate,
			  nm_tag_t tag, nm_seq_t seq,
			  const struct iovec *iov, int nb_entries)
{
  struct nm_so_strat_split_balance*status = _status;
  uint32_t offset = 0;
  uint8_t last_chunk = 0;
  int i;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  p_so_tag->send[seq] = 0;

  for(i = 0; i < nb_entries; i++){
    if(i == (nb_entries - 1)){
      last_chunk = 1;
    }

    if(iov[i].iov_len <= status->nm_so_max_small) {
      NM_SO_TRACE("PACK of a small iov entry - tag = %u, seq = %u, len = %ld, offset = %u, is_last_chunk = %u\n", tag, seq, (long)iov[i].iov_len, offset, last_chunk);
      /* Small packet */
      strat_split_balance_try_to_agregate_small(_status, p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);

    } else {
      NM_SO_TRACE("PACK of a large iov entry - tag = %u, seq = %u, nb_entries = %u\n", tag, seq, nb_entries);
      /* Large packets are splited in 2 chunks. */
      strat_split_balance_launch_large_chunk(_status, p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);
    }
    offset += iov[i].iov_len;
  }
  p_so_tag->send[seq] = offset;

  return NM_ESUCCESS;
}

static int
strat_split_balance_agregate_datatype(void*_status, struct nm_gate *p_gate,
				      nm_tag_t tag, nm_seq_t seq,
				      uint32_t len, const struct DLOOP_Segment *segp)
{

  struct nm_pkt_wrap *p_so_pw;
  struct nm_so_strat_split_balance*status = _status;
  int err;

  // Look for a wrapper to fullfill
  if(len <= 512 || status->nb_packets >= 2) {

    /* We first try to find an existing packet to form an aggregate */
    list_for_each_entry(p_so_pw, &status->out_list, link) {
      uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
      uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

      if(size > h_rlen)
        /* There's not enough room to add our data to this paquet */
        goto next;

      // add the datatype. Actually, we add the header and copy the data just after
      err = nm_so_pw_add_datatype(p_so_pw, tag + 128, seq, len, segp);

      goto out;

    next:
      ;
    }
  }
  // We don't find any free wrapper so we build a new one
  int flags = NM_SO_DATA_USE_COPY;

  err = nm_so_pw_alloc(flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  err = nm_so_pw_add_datatype(p_so_pw, tag + 128, seq, len, segp);

  list_add_tail(&p_so_pw->link, &status->out_list);
  status->nb_packets++;

  err = NM_ESUCCESS;
 out:
  return err;
}

static int
strat_split_balance_launch_large_datatype(void*_status, struct nm_gate *p_gate,
					  nm_tag_t tag, nm_seq_t seq,
					  uint32_t len, const struct DLOOP_Segment *segp)
{
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  struct nm_pkt_wrap *p_so_pw = NULL;
  int err;

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc(NM_PW_NOHEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  nm_so_pw_store_datatype(p_so_pw, tag + 128, seq, len, segp);

  p_so_tag->status[seq] |= NM_SO_STATUS_IS_DATATYPE;

  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 1, p_so_pw);
  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 1, 1, p_so_pw);

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(p_so_tag->pending_large_send));

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

    NM_SO_TRACE("RDV pack_ctrl\n");
    err = strat_split_balance_pack_ctrl(_status, p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }
  err = NM_ESUCCESS;
 out:
  return err;
}



// Si c'est un petit, on copie systématiquement (pour le moment)
// Si c'est un long, on passe par un iov qu'on va recharger au nieavu du driver
// s'il n'y a pas assez d'entrées. Les données sont (pour l'instant) systématiquement reçus en contigu
static int
strat_split_balance_pack_datatype(void*_status, struct nm_gate *p_gate,
				  nm_tag_t tag, nm_seq_t seq,
				  const struct DLOOP_Segment *segp)
{
  struct nm_so_strat_split_balance*status = _status;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  DLOOP_Handle handle;
  int data_sz;

  handle = segp->handle;
  CCSI_datadesc_get_size_macro(handle, data_sz); // * count?
#warning a revoir dans ccs
  //data_sz *= segp->handle.ref_count;

  p_so_tag->send[seq] = data_sz;

  NM_SO_TRACE("Send a datatype on gate %d and tag %d with length %d\n", p_gate->id, tag, data_sz);

  if(data_sz <= status->nm_so_max_small) {
    NM_SO_TRACE("Short datatype : try to aggregate it\n");
     strat_split_balance_agregate_datatype(_status, p_gate, tag, seq, data_sz, segp);

  } else {
    NM_SO_TRACE("Large datatype : send a rdv\n");
    strat_split_balance_launch_large_datatype(_status, p_gate, tag, seq, data_sz, segp);
  }

  p_so_tag->status[seq] = NM_SO_STATUS_IS_DATATYPE;

  return NM_ESUCCESS;
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int strat_split_balance_try_and_commit(void *_status, struct nm_gate *p_gate)
{
  struct nm_so_strat_split_balance*status = _status;
  struct list_head *out_list = &status->out_list;
  int nb_drivers = p_gate->p_core->nb_drivers;
  int n = 0;
  const nm_drv_id_t*drv_ids = NULL;
  nm_ns_inc_lats(p_gate->p_core, &drv_ids, &nb_drivers);
  assert(nb_drivers > 0);
  while(n < nb_drivers && !list_empty(out_list))
    {
      const nm_drv_id_t drv_id = drv_ids[n];
      assert(drv_id >= 0 && drv_id < nb_drivers);
      if(p_gate->active_send[drv_id][NM_TRK_SMALL] == 0 &&
	 p_gate->active_send[drv_id][NM_TRK_LARGE] == 0)
	{
	  /* We found an idle NIC */
	  const nm_drv_id_t drv_id = drv_ids[n];
	  /* Take the packet at the head of the list and post it on trk #0 */
	  struct nm_pkt_wrap *p_so_pw = nm_l2so(out_list->next);
	  list_del(out_list->next);
	  status->nb_packets--;
	  nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, drv_id);
	}
      n++;
    }
  return NM_ESUCCESS;
}

/* Warning: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
static int strat_split_balance_rdv_accept(void *_status, struct nm_gate *p_gate, uint32_t len,
					  int*nb_chunks, struct nm_rdv_chunk*chunks)

{
  if(*nb_chunks == 1)
    {
      /* Is there any large data track available? */
      nm_drv_id_t n;
      for(n = 0; n < *nb_chunks; n++)
	{
	  /* We explore the drivers in sequence. 
	   * We assume they are registered in order from the fastest to the slowest. 
	   * TODO: we should use latency/bandwdith information from drivers capabilities
	   */
	  if(p_gate->active_recv[n][NM_TRK_LARGE] == 0)
	    {
	      chunks[0].len    = len;
	      chunks[0].drv_id = n;
	      chunks[0].trk_id = NM_TRK_LARGE;
	      return NM_ESUCCESS;
	    }
	}
    }
  else
    {
      int nb_drivers = p_gate->p_core->nb_drivers;
      int chunk_index = 0;
      const nm_trk_id_t trk_id = NM_TRK_LARGE;
      const nm_drv_id_t *ordered_drv_id_by_bw = NULL;
      nm_ns_dec_bws(p_gate->p_core, &ordered_drv_id_by_bw, &nb_drivers);

      int i;
      for(i = 0; i < nb_drivers; i++)
	{
	  if(p_gate->active_recv[ordered_drv_id_by_bw[i]][trk_id] == 0)
	    {
	      chunks[chunk_index].drv_id = i;
	      chunks[chunk_index].trk_id = NM_TRK_LARGE;
	      chunk_index++;
	    }
	}
      *nb_chunks = chunk_index;
      if(chunk_index == 1)
	{
	  chunks[0].len = len;
	  return NM_ESUCCESS;
	}
      else if(chunk_index > 1)
	{
	  nm_ns_multiple_split_ratio(len, p_gate->p_core, nb_chunks, chunks);
	  return NM_ESUCCESS;
	}      
    }

  /* postpone the acknowledgement. */
  return -NM_EAGAIN;
}


