/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

static void strat_aggreg_pack_data(void*_status, struct nm_req_s*p_pack, nm_len_t len, nm_len_t chunk_offset);
static void strat_aggreg_pack_ctrl(void*, nm_gate_t , const union nm_header_ctrl_generic_s*);
static int  strat_aggreg_try_and_commit(void*, nm_gate_t );
static void strat_aggreg_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_aggreg_driver =
  {
    .pack_data          = &strat_aggreg_pack_data,
    .pack_ctrl          = &strat_aggreg_pack_ctrl,
    .try_and_commit     = &strat_aggreg_try_and_commit,
    .rdv_accept         = &strat_aggreg_rdv_accept
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
struct nm_strat_aggreg_s
{
  int nm_copy_on_send_threshold;
};

static int num_instances = 0;

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
  struct nm_strat_aggreg_s*p_status = TBX_MALLOC(sizeof(struct nm_strat_aggreg_s));
  num_instances++;
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  return p_status;
}

/** Cleanup the gate storage for aggreg strategy.
 */
static void strat_aggreg_destroy(void*_status)
{
  TBX_FREE(_status);
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
static void strat_aggreg_pack_ctrl(void*_status, nm_gate_t p_gate,
				   const union nm_header_ctrl_generic_s *p_ctrl)
{
  struct nm_strat_aggreg_s*p_status = _status;
  struct nm_pkt_wrap_s*p_pw = nm_tactic_try_to_aggregate(&p_gate->out_list, NM_HEADER_CTRL_SIZE, NM_SO_DEFAULT_WINDOW);
  if(p_pw)
    {
      nm_pw_add_control(p_pw, p_ctrl);
    }
  else
    {
      nm_tactic_pack_ctrl(p_ctrl, &p_gate->out_list);
    }
}

static void strat_aggreg_pack_data(void*_status, struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset)
{
  struct nm_strat_aggreg_s*p_status = _status;
  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
#warning TODO- max block count and header size are rough estimates
  /* maximum header length- real length depends on block size */
  const nm_len_t max_blocks = (p_props->blocks > chunk_len) ? chunk_len : p_props->blocks;
  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + max_blocks * sizeof(struct nm_header_pkt_data_chunk_s) + NM_ALIGN_FRONTIER;
  const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : 0; /* average block size */
  if(chunk_len + max_header_len < nm_drv_max_small(p_pack->p_gate->p_core))
    {
      /* ** small send */
      struct nm_pkt_wrap_s*p_pw = nm_tactic_try_to_aggregate(&p_pack->p_gate->out_list, max_header_len + chunk_len, NM_SO_DEFAULT_WINDOW);
      if(!p_pw)
	{
	  p_pw = nm_pw_alloc_global_header();
	  nm_pkt_wrap_list_push_back(&p_pack->p_gate->out_list, p_pw);
	}
      
#warning TODO- select pack strategy depending on data sparsity
      
      nm_pw_add_data_chunk(p_pw, p_pack, &p_pack->data, chunk_len, chunk_offset, NM_PW_DATA_ITERATOR);
      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
    }
  else
    {
      /* ** large send */
      nm_pw_flag_t flags = NM_PW_NOHEADER | NM_PW_DATA_ITERATOR;
      if((!p_props->is_contig) && (density < NM_LARGE_MIN_DENSITY) && (p_pack->data.ops.p_generator == NULL))
	{
	  flags |= NM_PW_DATA_USE_COPY;
	}
      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_noheader();
      nm_pw_add_data_chunk(p_pw, p_pack, &p_pack->data, chunk_len, chunk_offset, flags);
      nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_pw);
      union nm_header_ctrl_generic_s ctrl;
      nm_header_init_rdv(&ctrl, p_pack, chunk_len, chunk_offset, (p_pack->pack.scheduled == p_pack->pack.len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
      struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pack->p_gate->strategy_receptacle;
      (*strategy->driver->pack_ctrl)(strategy->_status, p_pack->p_gate, &ctrl);
    }
  
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_aggreg_try_and_commit(void *_status, nm_gate_t p_gate)
{
  struct nm_strat_aggreg_s*p_status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if((p_gdrv->active_send[NM_TRK_SMALL] == 0) && (!nm_pkt_wrap_list_empty(&p_gate->out_list)))
    {
      struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_pop_front(&p_gate->out_list);
      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
    }
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_aggreg_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      nm_drv_t p_drv = nm_drv_default(p_gate);
      if(p_pw->length > NM_LARGE_MIN_DENSITY)
	{
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv->active_recv[NM_TRK_LARGE] == 0)
	    {
	      /* The large-packet track is available- post recv and RTR */
	      struct nm_rdv_chunk chunk = 
		{ .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_LARGE };
	      nm_pkt_wrap_list_erase(&p_gate->pending_large_recv, p_pw);
	      nm_tactic_rtr_pack(p_pw, 1, &chunk);
	    }
	}
      else
	{
	  /* small chunk in a large packet- send on trk#0 */
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_SMALL };
	  nm_pkt_wrap_list_erase(&p_gate->pending_large_recv, p_pw);
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
