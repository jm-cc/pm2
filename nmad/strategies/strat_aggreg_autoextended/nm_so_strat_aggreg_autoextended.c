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

#include <Padico/Module.h>

static int nm_strat_aggreg_autoextended_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_aggreg_autoextended, &nm_strat_aggreg_autoextended_load, NULL, NULL);


/* Components structures:
*/

static int strat_aggreg_autoextended_todo(void*, struct nm_gate*);
static int strat_aggreg_autoextended_pack(void*_status, struct nm_pack_s*p_pack);
static int strat_aggreg_autoextended_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_aggreg_autoextended_try_and_commit(void*, struct nm_gate*);
static int strat_aggreg_autoextended_rdv_accept(void*, struct nm_gate*, uint32_t, int*, struct nm_rdv_chunk*);
static int strat_aggreg_autoextended_flush(void*, struct nm_gate*);

static const struct nm_strategy_iface_s nm_so_strat_aggreg_autoextended_driver =
  {
    .pack           = &strat_aggreg_autoextended_pack,
    .pack_ctrl      = &strat_aggreg_autoextended_pack_ctrl,
    .try_and_commit = &strat_aggreg_autoextended_try_and_commit,
    .rdv_accept     = &strat_aggreg_autoextended_rdv_accept,
    .flush          = &strat_aggreg_autoextended_flush,
    .todo           = &strat_aggreg_autoextended_todo
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
  struct tbx_fast_list_head out_list;
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
			puk_component_attr("nm_so_max_small", "16342"),
			puk_component_attr("nm_so_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for aggreg_autoextended strategy.
 */
static void*strat_aggreg_autoextended_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_aggreg_autoextended_gate*status = TBX_MALLOC(sizeof(struct nm_so_strat_aggreg_autoextended_gate));

  num_instances++;
  TBX_INIT_FAST_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <aggreg_autoextended>]");

  const char*nm_so_max_small = puk_instance_getattr(ai, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[nm_so_max_small=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_instance_getattr(ai, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[nm_so_copy_on_send_threshold=%i]", status->nm_so_copy_on_send_threshold);

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
  tbx_fast_list_for_each_entry(p_so_pw, &status->out_list, link) {

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
  tbx_fast_list_add_tail(&p_so_pw->link, &status->out_list);

 out:
  return err;
}

static int strat_aggreg_autoextended_flush(void*_status,
                                           struct nm_gate *p_gate)
{
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;

  tbx_fast_list_for_each_entry(p_so_pw, &status->out_list, link) {
    NM_TRACEF("Marking pw %p as completed\n", p_so_pw);
    p_so_pw->is_completed = tbx_true;
  }

  return NM_ESUCCESS;
}

static int strat_aggreg_autoextended_todo(void*_status,
					  struct nm_gate *p_gate)
{
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
  struct tbx_fast_list_head *out_list = &(status)->out_list;
  return !(tbx_fast_list_empty(out_list));
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
static int strat_aggreg_autoextended_pack(void*_status, struct nm_pack_s*p_pack)
{
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
  struct nm_pkt_wrap *p_pw = NULL;
  int flags = 0;
  const uint32_t len = p_pack->len;

  if(!(p_pack->status & NM_PACK_TYPE_CONTIGUOUS))
    TBX_FAILURE("strat_aggreg_autoextended supports only contiguous packs.");

  if(len <= status->nm_so_max_small)
    {
      /* small packet */
      tbx_fast_list_for_each_entry(p_pw, &status->out_list, link)
	{
	  const uint32_t h_rlen = nm_so_pw_remaining_header_area(p_pw);
	  const uint32_t d_rlen = nm_so_pw_remaining_data(p_pw);
	  const uint32_t size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);
	  if(size > d_rlen || NM_SO_DATA_HEADER_SIZE > h_rlen)
	    {
	      p_pw->is_completed = tbx_true;
	    }
	  else
	    {
	      if(len <= status->nm_so_copy_on_send_threshold && size <= h_rlen)
		{
		  flags = NM_SO_DATA_USE_COPY;
		}
	      nm_so_pw_add_data(p_pw, p_pack, p_pack->data, len, 0, flags);
	      nb_data_aggregation ++;
	      goto out;
	    }
	}
      /* cannot aggregate- create a new pw */
      flags = NM_PW_GLOBAL_HEADER;
      if(len <= status->nm_so_copy_on_send_threshold)
	flags |= NM_SO_DATA_USE_COPY;
      nm_so_pw_alloc_and_fill_with_data(p_pack, p_pack->data, len, 0, 1, flags, &p_pw);
      p_pw->is_completed = tbx_false;
      tbx_fast_list_add_tail(&p_pw->link, &status->out_list);
    }
  else
    {
      /* large packet */
      nm_so_pw_alloc_and_fill_with_data(p_pack, p_pack->data, len, 0, 1, NM_PW_NOHEADER, &p_pw);
      p_pw->is_completed = tbx_true;
      tbx_fast_list_add_tail(&p_pw->link, &p_pack->p_gate->pending_large_send);
      union nm_so_generic_ctrl_header ctrl;
      nm_so_init_rdv(&ctrl, p_pack, len, 0, NM_PROTO_FLAG_LASTCHUNK);
      strat_aggreg_autoextended_pack_ctrl(status, p_pack->p_gate, &ctrl);
    }
 out:
  return NM_ESUCCESS;
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
  struct nm_so_strat_aggreg_autoextended_gate *status = _status;
  struct tbx_fast_list_head *out_list = &status->out_list;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if(p_gdrv->active_send[NM_TRK_SMALL] == 1)
    /* We're done */
    goto out;

  if(tbx_fast_list_empty(out_list))
    /* We're done */
    goto out;

  /* Simply take the head of the list */
  struct nm_pkt_wrap *p_pw = nm_l2so(out_list->next);
  if(p_pw->is_completed == tbx_true) 
    {
      NM_TRACEF("pw %p is completed\n", p_pw);
      tbx_fast_list_del(out_list->next);
    } 
  else 
    {
      NM_TRACEF("pw is not completed\n");
      goto out;
    }

  /* Post packet on track 0 */
  nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);

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
static int strat_aggreg_autoextended_rdv_accept(void*_status, struct nm_gate *p_gate, uint32_t len,
						int*nb_chunks, struct nm_rdv_chunk*chunks)
{
  *nb_chunks = 1;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if(p_gdrv->active_recv[NM_TRK_LARGE] == 0)
    {
      /* The large-packet track is available! */
      chunks[0].len = len;
      chunks[0].p_drv = p_drv;
      chunks[0].trk_id = NM_TRK_LARGE;
      return NM_ESUCCESS;
    }
  else
    /* postpone the acknowledgement. */
    return -NM_EAGAIN;
}

