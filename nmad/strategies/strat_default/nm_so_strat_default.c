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

static int strat_default_todo(void*, struct nm_gate*);/* todo: s/nm_gate/nm_pack/ ? */
static int strat_default_pack(void*_status, struct nm_pack_s*p_pack);
static int strat_default_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_default_try_and_commit(void*, struct nm_gate*);
static int strat_default_rdv_accept(void*, struct nm_gate*, uint32_t, int*, struct nm_rdv_chunk*);

static const struct nm_strategy_iface_s nm_strat_default_driver =
  {
    .todo               = &strat_default_todo,
    .pack               = &strat_default_pack,
    .pack_ctrl          = &strat_default_pack_ctrl,
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
  struct tbx_fast_list_head out_list;
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

  TBX_INIT_FAST_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <default>]");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);
  NM_LOGF("[nm_so_max_small=%i]", status->nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_context_getattr(context, "nm_so_copy_on_send_threshold");
  status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);
  NM_LOGF("[nm_so_copy_on_send_threshold=%i]", status->nm_so_copy_on_send_threshold);

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
                                   struct nm_gate*p_gate,
				   const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_so_strat_default*status = _status;
  nm_tactic_pack_ctrl(p_ctrl, &status->out_list);
  return NM_ESUCCESS;
}

static int strat_default_todo(void* _status, struct nm_gate*p_gate)
{
  struct nm_so_strat_default*status = _status;
  return !(tbx_fast_list_empty(&status->out_list));
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
static int strat_default_pack(void*_status, struct nm_pack_s*p_pack)
{
  struct nm_so_strat_default*status = _status;

  if(p_pack->status & NM_PACK_TYPE_CONTIGUOUS)
    {
      const void*data = p_pack->data;
      const int len = p_pack->len;
      if(len <= status->nm_so_max_small)
	{
	  if(len <= status->nm_so_copy_on_send_threshold)
	    nm_tactic_pack_small_copy(p_pack, data, len, 0, &status->out_list);
	  else
	    nm_tactic_pack_small_ref(p_pack, data, len, 0, &status->out_list);
	}
      else
	{
	  nm_tactic_pack_rdv(p_pack, data, len, 0);
	}
    }
  else if(p_pack->status & NM_PACK_TYPE_IOV)
    {
      struct iovec*iov = p_pack->data;
      uint32_t offset = 0;
      int i;
      for(i = 0; offset < p_pack->len; i++)
	{
	  const char*data = iov[i].iov_base;
	  const int len = iov[i].iov_len;
	  if(len <= status->nm_so_max_small)
	    {
	      if(len <= status->nm_so_copy_on_send_threshold)
		nm_tactic_pack_small_copy(p_pack, data, len, 0, &status->out_list);
	      else
		nm_tactic_pack_small_ref(p_pack, data, len, 0, &status->out_list);
	    }
	  else
	    {
	      nm_tactic_pack_rdv(p_pack, data, len, 0);
	    }
	  offset += len;
	}
    }
  else
    {
      return -NM_ENOTIMPL;
    }
  return NM_ESUCCESS;
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
  struct tbx_fast_list_head *out_list = &status->out_list;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if((p_gdrv->active_send[NM_TRK_SMALL] == 0) &&
     !(tbx_fast_list_empty(out_list)))
    {
      struct nm_pkt_wrap *p_so_pw = nm_l2so(out_list->next);
      tbx_fast_list_del(out_list->next);
      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, p_drv);
    }
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
