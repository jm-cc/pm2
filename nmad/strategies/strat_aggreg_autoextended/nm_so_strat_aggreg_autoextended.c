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

static int strat_aggreg_autoextended_pack(void*, struct nm_gate*, nm_tag_t, uint8_t, const void*, uint32_t);
static int strat_aggreg_autoextended_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_aggreg_autoextended_try_and_commit(void*, struct nm_gate*);
static int strat_aggreg_autoextended_rdv_accept(void*, struct nm_gate*, nm_drv_id_t*, nm_trk_id_t*);
static int strat_aggreg_autoextended_flush(void*, struct nm_gate*);

static const struct nm_strategy_iface_s nm_so_strat_aggreg_autoextended_driver =
  {
    .pack           = &strat_aggreg_autoextended_pack,
    .pack_ctrl      = &strat_aggreg_autoextended_pack_ctrl,
    .try_and_commit = &strat_aggreg_autoextended_try_and_commit,
#ifdef NMAD_QOS
    .ack_callback   = NULL,
#endif /* NMAD_QOS */
    .rdv_accept     = &strat_aggreg_autoextended_rdv_accept,
    .flush          = &strat_aggreg_autoextended_flush,
  };

static void*strat_aggreg_autoextended_instanciate(puk_instance_t, puk_context_t);
static void strat_aggreg_autoextended_destroy(void*);

static const struct puk_adapter_driver_s nm_so_strat_aggreg_autoextended_adapter_driver =
  {
    .instanciate = &strat_aggreg_autoextended_instanciate,
    .destroy     = &strat_aggreg_autoextended_destroy
  };

/** Per-gate status for strat aggreg_autoextended instances.
 */
struct nm_so_strat_aggreg_autoextended_gate {
  /** List of raw outgoing packets. */
  struct list_head out_list;
  int nm_so_max_small;
  int nm_so_copy_on_send_threshold;
};

static int num_instances = 0;
static int nb_data_aggregation = 0;
static int nb_ctrl_aggregation = 0;


/** Component declaration */
static int nm_strat_aggreg_autoextended_load(void)
{
  puk_component_declare("NewMad_Strategy_aggreg_autoextended",
			puk_component_provides("PadicoAdapter", "adapter", &nm_so_strat_aggreg_autoextended_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_aggreg_autoextended_driver),
			puk_component_attr("nm_so_max_small", NULL),
			puk_component_attr("nm_so_copy_on_send_threshold", NULL));
  return NM_ESUCCESS;
}
PADICO_MODULE_BUILTIN(NewMad_Strategy_aggreg_autoextended, &nm_strat_aggreg_autoextended_load, NULL, NULL);


/** Initialize the gate storage for aggreg_autoextended strategy.
 */
static void*strat_aggreg_autoextended_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_aggreg_autoextended_gate*status = TBX_MALLOC(sizeof(struct nm_so_strat_aggreg_autoextended_gate));

  num_instances++;
  INIT_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <aggreg_autoextended>]");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[NM_SO_MAX_SMALL=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_context_getattr(context, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[NM_SO_COPY_ON_SEND_THRESHOLD=%i]", status->nm_so_copy_on_send_threshold);

  return (void*)status;
}

/** Cleanup the gate storage for aggreg_autoextended strategy.
 */
static void strat_aggreg_autoextended_destroy(void*status)
{
  TBX_FREE(status);
  num_instances--;
//  if(num_instances == 0)
//    {
//      DISP_VAL("Autoextended Aggregation data", nb_data_aggregation);
//      DISP_VAL("Autoextended Aggregation control", nb_ctrl_aggregation);
//    }
}


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param p_gate a pointer to the gate object.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_aggreg_autoextended_pack_ctrl(void*_status,
                                               struct nm_gate *p_gate,
                                               const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  list_for_each_entry(p_so_pw, &status->out_list, link) {

    if(nm_so_pw_remaining_header_area(p_so_pw) < NM_SO_CTRL_HEADER_SIZE) {
      /* There's not enough room to add our ctrl header to this paquet */
      p_so_pw->is_completed = tbx_true;
      goto next;
    }

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

static int strat_aggreg_autoextended_flush(void*_status,
                                           struct nm_gate *p_gate)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;

  list_for_each_entry(p_so_pw, &status->out_list, link) {
    NM_SO_TRACE("Marking pw %p as completed\n", p_so_pw);
    p_so_pw->is_completed = tbx_true;
  }

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
static int strat_aggreg_autoextended_pack(void*_status,
                                          struct nm_gate *p_gate,
                                          nm_tag_t tag, uint8_t seq,
                                          const void *data, uint32_t len)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
  int flags = 0;
  int err;

  if(len <= status->nm_so_max_small) {
    NM_SO_TRACE("Small packet\n");

    NM_SO_TRACE("We first try to find an existing packet to form an aggregate\n");
    list_for_each_entry(p_so_pw, &status->out_list, link) {
      uint32_t h_rlen = nm_so_pw_remaining_header_area(p_so_pw);
      uint32_t d_rlen = nm_so_pw_remaining_data(p_so_pw);
      uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);

      if(size > d_rlen || NM_SO_DATA_HEADER_SIZE > h_rlen) {
        p_so_pw->is_completed = tbx_true;
	NM_SO_TRACE("There's not enough room to add our data to this paquet\n");
	goto next;
      }

      if(len <= status->nm_so_copy_on_send_threshold && size <= h_rlen) {
	NM_SO_TRACE("We can copy data into the header zone\n");
	flags = NM_SO_DATA_USE_COPY;
      }
      else
	if(p_so_pw->v_nb == NM_SO_PREALLOC_IOV_LEN) {
          NM_SO_TRACE("Full packet. Checking next ...\n");
          p_so_pw->is_completed = tbx_true;
	  goto next;
        }

      err = nm_so_pw_add_data(p_so_pw, tag + 128, seq, data, len, 0, 1, flags);
      nb_data_aggregation ++;
      goto out;

    next:
      ;
    }

    if(len <= status->nm_so_copy_on_send_threshold)
      flags = NM_SO_DATA_USE_COPY;

    NM_SO_TRACE("We didn't have a chance to form an aggregate, so simply form a new packet wrapper and add it to the out_list\n");
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					    data, len,
					    0, 1,
					    flags,
					    &p_so_pw);
    p_so_pw->is_completed = tbx_false;

    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, &status->out_list);

  } else {
    NM_SO_TRACE("Large packets can not be sent immediately : we have to issue a RdV request.\n");

    NM_SO_TRACE("First allocate a packet wrapper\n");
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
                                            data, len,
					    0, 1,
                                            NM_SO_DATA_DONT_USE_HEADER,
                                            &p_so_pw);
    p_so_pw->is_completed = tbx_true;

    if(err != NM_ESUCCESS)
      goto out;

    NM_SO_TRACE("Then place it into the appropriate list of large pending sends.\n");
    list_add_tail(&p_so_pw->link,
                  &(nm_so_tag_get(&p_so_gate->tags, tag)->pending_large_send));

    /* Signal we're waiting for an ACK */
    p_so_gate->pending_unpacks++;

    /* Finally, generate a RdV request */
    {
      union nm_so_generic_ctrl_header ctrl;

      nm_so_init_rdv(&ctrl, tag + 128, seq, len, 0, 1);

      err = strat_aggreg_autoextended_pack_ctrl(status, p_gate, &ctrl);
      if(err != NM_ESUCCESS)
	goto out;
    }

    /* Check if we should post a new recv packet: we're waiting for an
       ACK! */
    nm_so_refill_regular_recv(p_gate);

  }

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
static int strat_aggreg_autoextended_try_and_commit(void*_status,
                                                    struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
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
  if(p_so_pw->is_completed == tbx_true) {
    NM_SO_TRACE("pw %p is completed\n", p_so_pw);
    list_del(out_list->next);
  } else {
    NM_SO_TRACE("pw is not completed\n");
    goto out;
  }

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
static int strat_aggreg_autoextended_rdv_accept(void*_status,
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
