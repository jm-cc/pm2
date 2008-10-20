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

static int strat_aggreg_pack(void*, struct nm_gate*, nm_tag_t, uint8_t, const void*, uint32_t);
static int strat_aggreg_packv(void*, struct nm_gate*, nm_tag_t, uint8_t, const struct iovec *, int);
static int strat_aggreg_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_aggreg_pack_ctrl_chunk(void*, struct nm_pkt_wrap *, const union nm_so_generic_ctrl_header *);
static int strat_aggreg_pack_extended_ctrl(void*, struct nm_gate *, uint32_t, const union nm_so_generic_ctrl_header *, struct nm_pkt_wrap **);
static int strat_aggreg_pack_extended_ctrl_end(void*,
                                                struct nm_gate *p_gate,
                                                struct nm_pkt_wrap *p_so_pw);
static int strat_aggreg_try_and_commit(void*, struct nm_gate*);
static int strat_aggreg_rdv_accept(void*, struct nm_gate*, nm_drv_id_t*, nm_trk_id_t*);

static const struct nm_strategy_iface_s nm_so_strat_aggreg_driver =
  {
    .pack               = &strat_aggreg_pack,
    .packv              = &strat_aggreg_packv,
    .pack_extended      = NULL,
    .pack_ctrl          = &strat_aggreg_pack_ctrl,
    .pack_ctrl_chunk    = &strat_aggreg_pack_ctrl_chunk,
    .pack_extended_ctrl = &strat_aggreg_pack_extended_ctrl,
    .pack_extended_ctrl_end = &strat_aggreg_pack_extended_ctrl_end,
    .try                = NULL,
    .commit             = NULL,
    .try_and_commit     = &strat_aggreg_try_and_commit,
    .cancel             = NULL,
#ifdef NMAD_QOS
    .ack_callback       = NULL,
#endif /* NMAD_QOS */
    .rdv_accept          = &strat_aggreg_rdv_accept,
    .extended_rdv_accept = NULL,
    .flush               = NULL
};

static void*strat_aggreg_instanciate(puk_instance_t, puk_context_t);
static void strat_aggreg_destroy(void*);

static const struct puk_adapter_driver_s nm_so_strat_aggreg_adapter_driver =
  {
    .instanciate = &strat_aggreg_instanciate,
    .destroy     = &strat_aggreg_destroy
  };

/** Per-gate status for strat aggreg instances.
 */
struct nm_so_strat_aggreg_gate {
  /** List of raw outgoing packets. */
  struct list_head out_list;
  int nm_so_max_small;
  int nm_so_copy_on_send_threshold;
};

static int num_instances = 0;
static int nb_data_aggregation;
static int nb_ctrl_aggregation;

static int
try_to_agregate_small(void *_status,
		      struct nm_gate *p_gate,
		      nm_tag_t tag, uint8_t seq,
                      const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk);
static int
launch_large_chunk(void *_status,
		   struct nm_gate *p_gate,
                   nm_tag_t tag, uint8_t seq,
                   const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk);

/** Component declaration */
static int nm_strat_aggreg_load(void)
{
  puk_component_declare("NewMad_Strategy_aggreg",
			puk_component_provides("PadicoAdapter", "adapter", &nm_so_strat_aggreg_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_aggreg_driver),
			puk_component_attr("nm_so_max_small", NULL),
			puk_component_attr("nm_so_copy_on_send_threshold", NULL));
  return NM_ESUCCESS;
}
PADICO_MODULE_BUILTIN(NewMad_Strategy_aggreg, &nm_strat_aggreg_load, NULL, NULL);


/** Initialize the gate storage for aggreg strategy.
 */
static void*strat_aggreg_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_aggreg_gate*status = TBX_MALLOC(sizeof(struct nm_so_strat_aggreg_gate));

  num_instances++;
  INIT_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <aggreg>]");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[NM_SO_MAX_SMALL=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_context_getattr(context, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[NM_SO_COPY_ON_SEND_THRESHOLD=%i]", status->nm_so_copy_on_send_threshold);

  return (void*)status;
}

/** Cleanup the gate storage for aggreg strategy.
 */
static void strat_aggreg_destroy(void*status)
{
  TBX_FREE(status);
  num_instances--;
//  if(num_instances == 0)
//    {
//      DISP_VAL("Aggregation data", nb_data_aggregation);
//      DISP_VAL("Aggregation control", nb_ctrl_aggregation);
//    }
}


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param p_gate a pointer to the gate object.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_aggreg_pack_ctrl(void*_status,
				  struct nm_gate *p_gate,
				  const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_aggreg_gate *status = _status;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &status->out_list, link) {

    if(nm_so_pw_remaining_header_area(p_so_pw) < NM_SO_CTRL_HEADER_SIZE)
      /* There's not enough room to add our ctrl header to this paquet */
      goto next;

    err = nm_so_pw_add_control(p_so_pw, p_ctrl);
    nb_ctrl_aggregation ++;
    goto out;

  next:
    ;
  }

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl,
					     &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add_tail(&p_so_pw->link, &status->out_list);

 out:
  return err;
}

/** Handle a new packet submitted by the user code.
 *
 *  @note The strategy may already apply some optimizations at this point.
 *  @param p_gate a pointer to the gate object.
 *  @param tag the message tag.
 *  @param seq the fragment sequence number.
 *  @param data the data fragment pointer.
 *  @param len the data fragment length.
 *  @return The NM status.
 */
static int strat_aggreg_pack(void*_status,
			     struct nm_gate *p_gate,
			     nm_tag_t tag, uint8_t seq,
			     const void *data, uint32_t len)
{
  struct nm_so_strat_aggreg_gate *status = _status;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  int err;

  nm_so_tag_get(&p_so_gate->tags, tag)->send[seq] = len;

  if(len <= status->nm_so_max_small) {
    /* Small packet */
    err = try_to_agregate_small(_status, p_gate, tag, seq, data, len, 0, 1);

  } else {
    /* Large packet */
    err = launch_large_chunk(_status, p_gate, tag, seq, data, len, 0, 1);
  }

  return err;
}

static int
strat_aggreg_pack_extended_ctrl(void*_status,
				struct nm_gate *p_gate,
				uint32_t cumulated_header_len,
				const union nm_so_generic_ctrl_header *p_ctrl,
				struct nm_pkt_wrap **pp_so_pw){

  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_aggreg_gate *status = _status;
  uint32_t h_rlen;
  int err;

  if(!list_empty(&status->out_list)) {
    /* Inspect only the head of the list */
    p_so_pw = nm_l2so(status->out_list.next);

    h_rlen = nm_so_pw_remaining_header_area(p_so_pw);

    /* If the paquet is reasonably small, we can form an aggregate */
    if(cumulated_header_len <= h_rlen) {
      err = nm_so_pw_add_control(p_so_pw, p_ctrl);
      goto out;
    }
  }

  /* Otherwise, simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

 out:
  *pp_so_pw = p_so_pw;

  return err;
}

static int
strat_aggreg_pack_ctrl_chunk(void*_status,
			     struct nm_pkt_wrap *p_so_pw,
			     const union nm_so_generic_ctrl_header *p_ctrl){

  int err;

  err = nm_so_pw_add_control(p_so_pw, p_ctrl);

  return err;
}

static int
strat_aggreg_pack_extended_ctrl_end(void *_status,
				    struct nm_gate *p_gate,
				    struct nm_pkt_wrap *p_so_pw){
  struct nm_so_strat_aggreg_gate *status = _status;

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &status->out_list);

  return NM_ESUCCESS;
}

static int
try_to_agregate_small(void *_status,
		      struct nm_gate *p_gate,
		      nm_tag_t tag, uint8_t seq,
                      const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_pkt_wrap *p_so_pw;
  struct nm_so_strat_aggreg_gate *status = _status;
  int flags = 0;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &status->out_list, link) {
    uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
    uint32_t d_rlen = nm_so_pw_remaining_data(p_so_pw);
    uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

      if(size > d_rlen || NM_SO_DATA_HEADER_SIZE > h_rlen)
	/* There's not enough room to add our data to this paquet */
	goto next;

      if(len <= NM_SO_COPY_ON_SEND_THRESHOLD && size <= h_rlen)
	/* We can copy data into the header zone */
	flags = NM_SO_DATA_USE_COPY;
      else
	if(p_so_pw->v_nb == NM_SO_PREALLOC_IOV_LEN)
	  goto next;

      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, chunk_offset, is_last_chunk, flags);
      nb_data_aggregation ++;

      goto out;

  next:
      ;
  }

  if(len <= NM_SO_COPY_ON_SEND_THRESHOLD)
    flags = NM_SO_DATA_USE_COPY;

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk, flags, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  list_add_tail(&p_so_pw->link, &status->out_list);
 out:
  return err;
}

static int
launch_large_chunk(void *_status,
		   struct nm_gate *p_gate,
                   nm_tag_t tag, uint8_t seq,
                   const void *data, uint32_t len, uint32_t chunk_offset, uint8_t is_last_chunk){
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  int err;

  /* Large packets can not be sent immediately : we have to issue a RdV request. */

  /* First allocate a packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                          chunk_offset, is_last_chunk,
                                          NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Then place it into the appropriate list of large pending "sends". */
  list_add_tail(&p_so_pw->link, &(nm_so_tag_get(&p_so_gate->tags, tag)->pending_large_send));

  /* Signal we're waiting for an ACK */
  p_so_gate->pending_unpacks++;

  /* Finally, generate a RdV request */
  {
    union nm_so_generic_ctrl_header ctrl;

    nm_so_init_rdv(&ctrl, tag + 128, seq, len, chunk_offset, is_last_chunk);

    err = strat_aggreg_pack_ctrl(_status, p_gate, &ctrl);
    if(err != NM_ESUCCESS)
      goto out;
  }

  /* Check if we should post a new recv packet: we're waiting for an
     ACK! */
  nm_so_refill_regular_recv(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

static int
strat_aggreg_packv(void *_status,
		   struct nm_gate *p_gate,
		   nm_tag_t tag, uint8_t seq,
		   const struct iovec *iov, int nb_entries){
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_strat_aggreg_gate *status = _status;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);
  uint32_t offset = 0;
  uint8_t last_chunk = 0;
  int i;

  p_so_tag->send[seq] = 0;

  for(i = 0; i < nb_entries; i++){
    if(i == (nb_entries - 1)){
      last_chunk = 1;
    }

    if(iov[i].iov_len <= status->nm_so_max_small) {
      NM_SO_TRACE("PACK of a small one - tag = %u, seq = %u, len = %u, offset = %u, is_last_chunk = %u\n", tag, seq, (unsigned int)iov[i].iov_len, offset, last_chunk);

      /* Small packet */
      try_to_agregate_small(_status, p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);

    } else {
      NM_SO_TRACE("PACK of a large one - tag = %u, seq = %u, nb_entries = %u\n", tag, seq, nb_entries);

      /* Large packets are splited in 2 chunks. */
      launch_large_chunk(_status, p_gate, tag, seq, iov[i].iov_base, iov[i].iov_len, offset, last_chunk);
    }
    offset += iov[i].iov_len;
  }
  p_so_tag->send[seq] = offset;

  return NM_ESUCCESS;
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_aggreg_try_and_commit(void *_status,
				       struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_strat_aggreg_gate *status = _status;
  struct list_head *out_list =
    &(status)->out_list;
  struct nm_pkt_wrap *p_so_pw;

  if(p_so_gate->active_send[NM_SO_DEFAULT_NET][NM_TRK_SMALL] ==
     NM_SO_MAX_ACTIVE_SEND_PER_TRACK)
    /* We're done */
    goto out;

  if(list_empty(out_list))
    /* We're done */
    goto out;

  /* Simply take the head of the list */
  p_so_pw = nm_l2so(out_list->next);
  list_del(out_list->next);

  /* Finalize packet wrapper */
  nm_so_pw_finalize(p_so_pw);

  /* Post packet on track 0 */
  nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, NM_SO_DEFAULT_NET);

 out:
    return NM_ESUCCESS;
}

/** Accept or refuse a RDV on the suggested (driver/track/gate).
 *
 *  @warning @p drv_id and @p trk_id are IN/OUT parameters. They initially
 *  hold values "suggested" by the caller.
 *  @param p_gate a pointer to the gate object.
 *  @param drv_id the suggested driver id.
 *  @param trk_id the suggested track id.
 *  @return The NM status.
 */
static int strat_aggreg_rdv_accept(void*_status,
				   struct nm_gate *p_gate,
				   nm_drv_id_t *drv_id,
				   nm_trk_id_t *trk_id)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;

  if(p_so_gate->active_recv[*drv_id][*trk_id] == 0)
    /* Cool! The suggested track is available! */
    return NM_ESUCCESS;
  else
    /* We decide to postpone the acknowledgement. */
    return -NM_EAGAIN;
}