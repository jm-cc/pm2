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

static int strat_default_pack(void*, struct nm_gate*, nm_tag_t, uint8_t, const void*, uint32_t);
static int strat_default_packv(void*, struct nm_gate*, nm_tag_t, uint8_t, const struct iovec *, int);
static int strat_default_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_default_pack_ctrl_chunk(void*, struct nm_pkt_wrap *, const union nm_so_generic_ctrl_header *);
static int strat_default_pack_extended_ctrl(void*, struct nm_gate *, uint32_t, const union nm_so_generic_ctrl_header *, struct nm_pkt_wrap **);
static int strat_default_pack_extended_ctrl_end(void*,
                                                struct nm_gate *p_gate,
                                                struct nm_pkt_wrap *p_so_pw);
static int strat_default_try_and_commit(void*, struct nm_gate*);
static int strat_default_rdv_accept(void*, struct nm_gate*, uint32_t, int*, struct nm_rdv_chunk*);

static const struct nm_strategy_iface_s nm_strat_default_driver =
  {
    .pack               = &strat_default_pack,
    .packv              = &strat_default_packv,
    .pack_ctrl          = &strat_default_pack_ctrl,
    .pack_ctrl_chunk    = &strat_default_pack_ctrl_chunk,
    .pack_extended_ctrl = &strat_default_pack_extended_ctrl,
    .pack_extended_ctrl_end = &strat_default_pack_extended_ctrl_end,
    .try_and_commit     = &strat_default_try_and_commit,
#ifdef NMAD_QOS
    .ack_callback    = NULL,
#endif /* NMAD_QOS */
    .rdv_accept          = &strat_default_rdv_accept,
    .flush               = NULL
};

static void*strat_default_instanciate(puk_instance_t, puk_context_t);
static void strat_default_destroy(void*);

static const struct puk_adapter_driver_s nm_strat_default_adapter_driver =
  {
    .instanciate = &strat_default_instanciate,
    .destroy     = &strat_default_destroy
  };


/** Per-gate status for strat default instances
 */
struct nm_so_strat_default {
  /** List of raw outgoing packets. */
  struct list_head out_list;
  int nm_so_max_small;
  int nm_so_copy_on_send_threshold;
};

/** Component declaration */
static int nm_strat_default_load(void)
{
  puk_component_declare("NewMad_Strategy_default",
			puk_component_provides("PadicoAdapter", "adapter", &nm_strat_default_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_strat_default_driver),
			puk_component_attr("nm_so_max_small", NULL),
			puk_component_attr("nm_so_copy_on_send_threshold", NULL));
  return NM_ESUCCESS;
}
PADICO_MODULE_BUILTIN(NewMad_Strategy_default, &nm_strat_default_load, NULL, NULL);


/** Initialize the gate storage for default strategy.
 */
static void*strat_default_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_default *status = TBX_MALLOC(sizeof(struct nm_so_strat_default));

  INIT_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <default>]");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[NM_SO_MAX_SMALL=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_context_getattr(context, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[NM_SO_COPY_ON_SEND_THRESHOLD=%i]", status->nm_so_copy_on_send_threshold);

  return (void*)status;
}

/** Cleanup the gate storage for default strategy.
 */
static void strat_default_destroy(void*status)
{
  TBX_FREE(status);
}


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param _status the strat_default instance status.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_default_pack_ctrl(void*_status,
                                   struct nm_gate *p_gate,
				   const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_default*status = _status;
  int err;

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  list_add_tail(&p_so_pw->link,
                &status->out_list);

 out:
  return err;
}

static int
strat_default_pack_extended_ctrl(void*_status,
                                 struct nm_gate *p_gate,
                                 uint32_t cumulated_header_len,
                                 const union nm_so_generic_ctrl_header *p_ctrl,
                                 struct nm_pkt_wrap **pp_so_pw){
  struct nm_pkt_wrap *p_so_pw = NULL;
  int err;

  /* Simply form a new packet wrapper */
  err = nm_so_pw_alloc_and_fill_with_control(p_ctrl, &p_so_pw);

  *pp_so_pw = p_so_pw;

  return err;
}

static int
strat_default_pack_ctrl_chunk(void*_status,
                              struct nm_pkt_wrap *p_so_pw,
                              const union nm_so_generic_ctrl_header *p_ctrl){

  int err;

  err = nm_so_pw_add_control(p_so_pw, p_ctrl);

  return err;
}

static int
strat_default_pack_extended_ctrl_end(void*_status,
                                     struct nm_gate *p_gate,
                                     struct nm_pkt_wrap *p_so_pw){

  struct nm_so_strat_default*status = _status;

  /* Add the control packet to the BEGINING of out_list */
  list_add(&p_so_pw->link, &status->out_list);

  return NM_ESUCCESS;
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
static int strat_default_pack(void*_status,
			      struct nm_gate *p_gate,
			      nm_tag_t tag, uint8_t seq,
			      const void *data, uint32_t len)
{
  struct nm_pkt_wrap *p_so_pw;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);
  struct nm_so_strat_default*status = _status;
  int flags = 0;
  int err;

  p_so_tag->send[seq] = len;

  if(len <= status->nm_so_max_small) {
    /* Small packet */

    if(len <= status->nm_so_copy_on_send_threshold)
      flags = NM_SO_DATA_USE_COPY;

    /* Simply form a new packet wrapper and add it to the out_list */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
					    0, 1, flags, &p_so_pw);

    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, &status->out_list);

  } else {
    /* Large packets can not be sent immediately : we have to issue a
       RdV request. */

    /* First allocate a packet wrapper */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, data, len,
                                            0, 0, NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    /* Then place it into the appropriate list of large pending
       "sends". */
    list_add_tail(&p_so_pw->link,
                  &p_so_tag->pending_large_send);

    /* Finally, generate a RdV request */
    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

      err = strat_default_pack_ctrl(status, p_gate, &ctrl);
      if(err != NM_ESUCCESS)
	goto out;
    }
  }

  err = NM_ESUCCESS;
 out:
  return err;
}

static int
strat_default_packv(void*_status,
                    struct nm_gate *p_gate,
                    nm_tag_t tag, uint8_t seq,
                    const struct iovec *iov, int nb_entries){
  struct nm_so_strat_default*status = _status;
  struct nm_pkt_wrap *p_so_pw;
  uint32_t offset = 0;
  uint8_t is_last_chunk = 0;
  int flags = 0;
  int i, err;

  for(i = 0; i < nb_entries; i++){
    if(i == (nb_entries - 1)){
      is_last_chunk = 1;
    }

    if(iov[i].iov_len <= NM_SO_MAX_SMALL) {
      /* Small packet */

      if(iov[i].iov_len <= NM_SO_COPY_ON_SEND_THRESHOLD)
        flags = NM_SO_DATA_USE_COPY;

      /* Simply form a new packet wrapper and add it to the out_list */
      err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, iov[i].iov_base, iov[i].iov_len,
                                              offset, is_last_chunk, flags, &p_so_pw);
      if(err != NM_ESUCCESS)
        goto out;

      list_add_tail(&p_so_pw->link,
                    &status->out_list);

    } else {
      /* Large packets can not be sent immediately : we have to issue a
         RdV request. */

      /* First allocate a packet wrapper */
      err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, iov[i].iov_base, iov[i].iov_len,
                                              offset, is_last_chunk,
                                              NM_SO_DATA_DONT_USE_HEADER, &p_so_pw);
      if(err != NM_ESUCCESS)
        goto out;

      /* Then place it into the appropriate list of large pending "sends". */
      list_add_tail(&p_so_pw->link, &(nm_so_tag_get(&p_gate->tags, tag)->pending_large_send));

      /* Finally, generate a RdV request */
      {
        union nm_so_generic_ctrl_header ctrl;

        nm_so_init_rdv(&ctrl, tag + 128, seq, iov[i].iov_len, offset, is_last_chunk);

        err = strat_default_pack_ctrl(_status, p_gate, &ctrl);
        if(err != NM_ESUCCESS)
          goto out;
      }
    }

    offset += iov[i].iov_len;
  }
  nm_so_tag_get(&p_gate->tags, tag)->send[seq] = offset;

  err = NM_ESUCCESS;
 out:
  return err;
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_default_try_and_commit(void*_status,
					struct nm_gate *p_gate)
{
  struct nm_so_strat_default*status = _status;
  struct list_head *out_list = &(status->out_list);
  struct nm_pkt_wrap *p_so_pw;

  if(p_gate->active_send[NM_SO_DEFAULT_NET][NM_TRK_SMALL] ==
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
static int strat_default_rdv_accept(void*_status, struct nm_gate *p_gate, uint32_t len,
				    int*nb_chunks, struct nm_rdv_chunk*chunks)
{
  *nb_chunks = 1;
  if(p_gate->active_recv[NM_DRV_DEFAULT][NM_TRK_LARGE] == 0)
    {
      /* The large-packet track is available! */
      chunks[0].len = len;
      chunks[0].drv_id = NM_DRV_DEFAULT;
      chunks[0].trk_id = NM_TRK_LARGE;
      return NM_ESUCCESS;
    }
  else
    /* postpone the acknowledgement. */
    return -NM_EAGAIN;
}
