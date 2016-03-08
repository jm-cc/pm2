/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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


/* Components structures:
 */

static int  strat_immediate_todo(void*, struct nm_gate*);
static void strat_immediate_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);
static int  strat_immediate_pack_ctrl(void*, struct nm_gate *, const union nm_header_ctrl_generic_s*);
static int  strat_immediate_try_and_commit(void*, struct nm_gate*);
static void strat_immediate_rdv_accept(void*, struct nm_gate*);

static const struct nm_strategy_iface_s nm_strat_immediate_driver =
  {
    .todo               = &strat_immediate_todo,
    .pack_chunk         = &strat_immediate_pack_chunk,
    .pack_ctrl          = &strat_immediate_pack_ctrl,
    .try_and_commit     = &strat_immediate_try_and_commit,
    .rdv_accept         = &strat_immediate_rdv_accept,
    .flush              = NULL
};

static void*strat_immediate_instantiate(puk_instance_t, puk_context_t);
static void strat_immediate_destroy(void*);

static const struct puk_component_driver_s nm_strat_immediate_component_driver =
  {
    .instantiate = &strat_immediate_instantiate,
    .destroy     = &strat_immediate_destroy
  };

/* ********************************************************* */

/** Per-gate status for strat instances
 */
struct nm_strat_immediate
{
  int max_small;
  int nm_copy_on_send_threshold;
};

PADICO_MODULE_COMPONENT(NewMad_Strategy_immediate,
                        puk_component_declare("NewMad_Strategy_immediate",
                                              puk_component_provides("PadicoComponent", "component", &nm_strat_immediate_component_driver),
                                              puk_component_provides("NewMad_Strategy", "strat", &nm_strat_immediate_driver),
                                              puk_component_attr("max_small", NULL),
                                              puk_component_attr("copy_on_send_threshold", "4096"))
                        );





/** Initialize the gate storage for default strategy.
 */
static void*strat_immediate_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_immediate *status = TBX_MALLOC(sizeof(struct nm_strat_immediate));
  const char*max_small = puk_instance_getattr(ai, "max_small");
  status->max_small = max_small ? atoi(max_small) : (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE - NM_SO_ALIGN_FRONTIER - 4);
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "copy_on_send_threshold");
  status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  return (void*)status;
}

/** Cleanup the gate storage for default strategy.
 */
static void strat_immediate_destroy(void*status)
{
  TBX_FREE(status);
}


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param _status the strat_immediate instance status.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_immediate_pack_ctrl(void*_status, struct nm_gate*p_gate, const union nm_header_ctrl_generic_s *p_ctrl)
{
  struct nm_strat_immediate*status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_pkt_wrap*p_pw = NULL;
  nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  nm_so_pw_add_control(p_pw, p_ctrl);
  nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
  return NM_ESUCCESS;
}

static int strat_immediate_todo(void* _status, struct nm_gate*p_gate)
{
  struct nm_strat_immediate*status = _status;
  return 0;
}

/** push a message chunk */
static void strat_immediate_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset)
{
  struct nm_strat_immediate*status = _status;
  if(len < status->max_small)
    {
      struct nm_pkt_wrap*p_pw = NULL;
      nm_gate_t p_gate = p_pack->p_gate;
      nm_drv_t p_drv = nm_drv_default(p_gate);
      nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
      nm_so_pw_add_data_chunk(p_pw, p_pack, ptr, len, chunk_offset, NM_PW_GLOBAL_HEADER | NM_PW_DATA_USE_COPY);
      nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
      return NM_ESUCCESS;
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
static int strat_immediate_try_and_commit(void*_status, struct nm_gate *p_gate)
{
  struct nm_strat_immediate*status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_immediate_rdv_accept(void*_status, struct nm_gate*p_gate)
{
  if(!tbx_fast_list_empty(&p_gate->pending_large_recv))
    {
      struct nm_pkt_wrap*p_pw = nm_l2so(p_gate->pending_large_recv.next);
      nm_drv_t p_drv = nm_drv_default(p_gate);
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
}
