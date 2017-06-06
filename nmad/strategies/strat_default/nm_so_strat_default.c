/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <assert.h>


#include <nm_private.h>
#include <Padico/Module.h>

static int nm_strat_default_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_default, &nm_strat_default_load, NULL, NULL);


/* Components structures:
 */

static void strat_default_try_and_commit(void*, nm_gate_t );
static void strat_default_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_default_driver =
  {
    .pack_data          = NULL,
    .pack_ctrl          = NULL,
    .try_and_commit     = &strat_default_try_and_commit,
    .rdv_accept         = &strat_default_rdv_accept
};

static void*strat_default_instantiate(puk_instance_t, puk_context_t);
static void strat_default_destroy(void*);

static const struct puk_component_driver_s nm_strat_default_component_driver =
  {
    .instantiate = &strat_default_instantiate,
    .destroy     = &strat_default_destroy
  };


/** Per-gate status for strat default instances
 */
struct nm_strat_default_s
{
  int nm_max_small;
  int nm_copy_on_send_threshold;
};

/** Component declaration */
static int nm_strat_default_load(void)
{
  puk_component_declare("NewMad_Strategy_default",
			puk_component_provides("PadicoComponent", "component", &nm_strat_default_component_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_strat_default_driver),
			puk_component_attr("nm_max_small", "16336"),
			puk_component_attr("nm_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for default strategy.
 */
static void*strat_default_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_default_s*p_status = TBX_MALLOC(sizeof(struct nm_strat_default_s));
  const char*nm_max_small = puk_instance_getattr(ai, "nm_max_small");
  p_status->nm_max_small = atoi(nm_max_small);
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  return (void*)p_status;
}

/** Cleanup the gate storage for default strategy.
 */
static void strat_default_destroy(void*_status)
{
  TBX_FREE(_status);
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 */
static void strat_default_try_and_commit(void*_status, nm_gate_t p_gate)
{
  struct nm_strat_default_s*p_status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct nm_core*p_core = p_gate->p_core;
  if(p_gdrv->p_pw_send[NM_TRK_SMALL] == NULL)
    {
      if(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	{
	  /* post ctrl on trk #0 */
	  struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_begin(&p_gate->ctrl_chunk_list);
	  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
	  int rc __attribute__((unused));
	  rc = nm_tactic_pack_ctrl(p_gate, p_drv, p_ctrl_chunk, p_pw);
	  assert(rc == NM_ESUCCESS);
	  nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
	}
      else if(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_gate->req_chunk_list);
	  struct nm_req_s*p_pack = p_req_chunk->p_req;
	  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
	  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + p_props->blocks * sizeof(struct nm_header_pkt_data_chunk_s);
	  if(p_req_chunk->chunk_len + max_header_len <= p_status->nm_max_small)
	    {
	      /* post short data on trk #0 */
	      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
	      nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_USE_COPY);
	      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
	      nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
	    }
	  else
	    {
	      /* post RDV for large data */
	      const int is_lastchunk = (p_req_chunk->chunk_offset + p_req_chunk->chunk_len == p_pack->pack.len);
	      union nm_header_ctrl_generic_s rdv;
	      nm_header_init_rdv(&rdv, p_pack, p_req_chunk->chunk_len, p_req_chunk->chunk_offset,
				 is_lastchunk ? NM_PROTO_FLAG_LASTCHUNK : 0);
	      struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader();
	      nm_pw_add_req_chunk(p_large_pw, p_req_chunk, NM_REQ_FLAG_NONE);
	      nm_pkt_wrap_list_push_back(&p_gate->pending_large_send, p_large_pw);
	      struct nm_pkt_wrap_s*p_rdv_pw = nm_pw_alloc_global_header();
	      nm_pw_add_control(p_rdv_pw, &rdv);
	      nm_core_post_send(p_gate, p_rdv_pw, NM_TRK_SMALL, p_drv);
	    }
	}
    }
}

/** Emit RTR for received RDV requests
 */
static void strat_default_rdv_accept(void*_status, nm_gate_t p_gate)
{
  if(!nm_pkt_wrap_list_empty(&p_gate->pending_large_recv))
    {
      nm_drv_t p_drv = nm_drv_default(p_gate);
      struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
      if(p_gdrv->p_pw_recv[NM_TRK_LARGE] == NULL)
	{
	  /* The large-packet track is available- post recv and RTR */
	  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_pop_front(&p_gate->pending_large_recv);
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_LARGE };
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
