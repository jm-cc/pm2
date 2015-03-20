/*
 * NewMadeleine
 * Copyright (C) 2006-2014 (see AUTHORS file)
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

static int nm_strat_aggreg_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_aggreg, &nm_strat_aggreg_load, NULL, NULL);

/* Components structures:
 */

static int  strat_aggreg_todo(void*, struct nm_gate*);
static void strat_aggreg_pack_chunk(void*_status, struct nm_pack_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);
static int  strat_aggreg_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int  strat_aggreg_try_and_commit(void*, struct nm_gate*);
static void strat_aggreg_rdv_accept(void*, struct nm_gate*);

static const struct nm_strategy_iface_s nm_strat_aggreg_driver =
  {
    .pack_chunk         = &strat_aggreg_pack_chunk,
    .pack_ctrl          = &strat_aggreg_pack_ctrl,
    .try_and_commit     = &strat_aggreg_try_and_commit,
    .rdv_accept         = &strat_aggreg_rdv_accept,
    .flush              = NULL,
    .todo               = &strat_aggreg_todo
};

static void*strat_aggreg_instantiate(puk_instance_t, puk_context_t);
static void strat_aggreg_destroy(void*);

static const struct puk_component_driver_s nm_strat_aggreg_component_driver =
  {
    .instantiate = &strat_aggreg_instantiate,
    .destroy     = &strat_aggreg_destroy
  };

/** Per-gate status for strat aggreg instances.
 */
struct nm_strat_aggreg_gate 
{
  /** List of raw outgoing packets. */
  struct tbx_fast_list_head out_list;
  int nm_copy_on_send_threshold;
};

static int num_instances = 0;
static int nb_data_aggregation = 0;
static int nb_ctrl_aggregation = 0;


static inline nm_len_t strat_aggreg_max_small(struct nm_core*p_core)
{
  static ssize_t nm_max_small = -1;
    /* lazy init */
  if(nm_max_small > 0)
    return nm_max_small;
  else
    {
      
      struct nm_drv*p_drv;
      NM_FOR_EACH_DRIVER(p_drv, p_core)
	{
	  if(nm_max_small <= 0 || (p_drv->driver->capabilities.max_unexpected > 0 && p_drv->driver->capabilities.max_unexpected < nm_max_small))
	    {
	      nm_max_small = p_drv->driver->capabilities.max_unexpected;
	    }
	}
      if(nm_max_small <= 0 || nm_max_small > (NM_SO_MAX_UNEXPECTED - NM_SO_DATA_HEADER_SIZE))
	{
	  nm_max_small = (NM_SO_MAX_UNEXPECTED - NM_SO_DATA_HEADER_SIZE);
	}
      NM_DISPF("# nmad: aggreg- max_small = %lu\n", nm_max_small);
    }
  return nm_max_small;
}

/** Component declaration */
static int nm_strat_aggreg_load(void)
{
  puk_component_declare("NewMad_Strategy_aggreg",
			puk_component_provides("PadicoComponent", "component", &nm_strat_aggreg_component_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_strat_aggreg_driver),
			puk_component_attr("nm_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for aggreg strategy.
 */
static void*strat_aggreg_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_aggreg_gate*status = TBX_MALLOC(sizeof(struct nm_strat_aggreg_gate));
  num_instances++;
  TBX_INIT_FAST_LIST_HEAD(&status->out_list);
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
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
static int strat_aggreg_pack_ctrl(void*_status, struct nm_gate *p_gate,
				  const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_strat_aggreg_gate *status = _status;
  struct nm_pkt_wrap*p_pw = nm_tactic_try_to_aggregate(&status->out_list, NM_SO_CTRL_HEADER_SIZE, 0);
  if(p_pw)
    {
      nm_so_pw_add_control(p_pw, p_ctrl);
      nb_ctrl_aggregation++;
    }
  else
    {
      nm_tactic_pack_ctrl(p_ctrl, &status->out_list);
    }
  return NM_ESUCCESS;
}


static int strat_aggreg_todo(void*_status, struct nm_gate *p_gate)
{
  struct nm_strat_aggreg_gate *status = _status;
  return !(tbx_fast_list_empty(&status->out_list));
}

/** push message chunk */
static void strat_aggreg_pack_chunk(void*_status, struct nm_pack_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset)
{
  struct nm_strat_aggreg_gate*status = _status;
  if(len <= strat_aggreg_max_small(p_pack->p_gate->p_core))
    {
      struct nm_pkt_wrap*p_pw = nm_tactic_try_to_aggregate(&status->out_list, NM_SO_DATA_HEADER_SIZE, len);
      if(p_pw)
	{
	  nb_data_aggregation++;
	  nm_tactic_pack_small_into_pw(p_pack, ptr, len, chunk_offset, status->nm_copy_on_send_threshold, p_pw);
	}
      else
	{
	  nm_tactic_pack_small_new_pw(p_pack, ptr, len, chunk_offset, status->nm_copy_on_send_threshold, &status->out_list);
	}
    }
  else
    {
      nm_tactic_pack_rdv(p_pack, ptr, len, chunk_offset);
    }
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_aggreg_try_and_commit(void *_status, struct nm_gate *p_gate)
{
  struct nm_strat_aggreg_gate *status = _status;
  struct tbx_fast_list_head *out_list = &status->out_list;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if( (p_gdrv->active_send[NM_TRK_SMALL] == 0) &&
      (!tbx_fast_list_empty(out_list)))
    {
      struct nm_pkt_wrap *p_so_pw = nm_l2so(out_list->next);
      tbx_fast_list_del(out_list->next);
      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, p_drv);
    }
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_aggreg_rdv_accept(void*_status, struct nm_gate *p_gate)
{
  if(!tbx_fast_list_empty(&p_gate->pending_large_recv))
    {
      struct nm_pkt_wrap*p_pw = nm_l2so(p_gate->pending_large_recv.next);
      nm_drv_t p_drv = nm_drv_default(p_gate);
      if(p_pw->length > strat_aggreg_max_small(p_drv->p_core))
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv->active_recv[NM_TRK_LARGE] == 0)
	    {
	      /* The large-packet track is available- post recv and RTR */
	      struct nm_rdv_chunk chunk = 
		{ .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_LARGE };
	      tbx_fast_list_del(p_gate->pending_large_recv.next);
	      nm_tactic_rtr_pack(p_pw, 1, &chunk);
	    }
	}
      else
	{
	  /* small chunk in a large packet- send on trk#0 */
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_SMALL };
	  tbx_fast_list_del(p_gate->pending_large_recv.next);
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
