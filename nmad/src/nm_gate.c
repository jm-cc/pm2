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

#include <nm_private.h>

#ifdef NMAD_TRACE
#include <nm_trace.h>
#endif /* NMAD_TRACE */

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

/** Initialize a new gate.
 *
 * out parameters:
 * p_id - id of the gate
 */
int nm_core_gate_init(nm_core_t p_core, nm_gate_t*pp_gate)
{
  nm_gate_t p_gate = TBX_MALLOC(sizeof(struct nm_gate_s));

  p_gate->status = NM_GATE_STATUS_INIT;
  p_gate->p_core = p_core;
  p_gate->ref    = NULL;
  nm_gdrv_vect_init(&p_gate->gdrv_array);
  nm_so_tag_table_init(&p_gate->tags);

  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_recv);
  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_send);


  p_gate->strategy_instance = puk_component_instantiate(p_core->strategy_component);
  puk_instance_indirect_NewMad_Strategy(p_gate->strategy_instance, NULL,
					&p_gate->strategy_receptacle);

  nm_gate_list_push_back(&p_core->gate_list, p_gate);

#ifdef NMAD_TRACE
  static int nm_trace_gate_count = 0;
  p_gate->trace_connections_id = nm_trace_gate_count++;
#endif /* NMAD_TRACE */

  *pp_gate = p_gate;
  
  return NM_ESUCCESS;
}

/** Connect the process through a gate using a specified driver.
 */
int nm_core_gate_connect(struct nm_core	*p_core,
			 nm_gate_t	 p_gate,
			 nm_drv_t         p_drv,
			 const char	*url)
{
  assert(url != NULL);

  struct nm_gate_drv *p_gdrv = TBX_MALLOC(sizeof(struct nm_gate_drv));

  memset(p_gdrv->active_recv, 0, sizeof(p_gdrv->active_recv));
  memset(p_gdrv->active_send, 0, sizeof(p_gdrv->active_send));

  nm_trk_id_t trk_id;
  int err;

  p_gate->status = NM_GATE_STATUS_CONNECTING;

  if(!p_gate)
    {
      err = -NM_EINVAL;
      goto out;
    }
  if(!p_drv)
    {
      err = -NM_EINVAL;
      goto out;
    }

  /* instantiate driver */
  p_gdrv->instance = puk_component_instantiate(p_drv->assembly);
  puk_instance_indirect_NewMad_Driver(p_gdrv->instance, NULL, &p_gdrv->receptacle);

  /* init gate/driver fields */
  p_gdrv->p_drv	= p_drv;
  nm_gdrv_vect_push_back(&p_gate->gdrv_array, p_gdrv);

  /* connect/accept connections */
  for (trk_id = 0; trk_id < p_drv->nb_tracks; trk_id++)
    {
      p_gdrv->p_in_rq_array[trk_id] = NULL;
      err = p_gdrv->receptacle.driver->connect(p_gdrv->receptacle._status, p_gate, p_drv, trk_id, url);
      if (err != NM_ESUCCESS)
	{
	  NM_WARN("drv.ops.connect returned %d", err);
	  goto out;
	}
    }
  err = NM_ESUCCESS;
  p_gate->status = NM_GATE_STATUS_CONNECTED;

#ifdef NMAD_TRACE
  nm_trace_container(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_NEW_CONNECTION, p_gate->trace_connections_id);
#endif /* NMAD_TRACE */

 out:
  return err;

}


/** Get the user-registered per-gate data */
void*nm_gate_ref_get(nm_gate_t p_gate)
{
  return p_gate->ref;
}

/** Set the user-registered per-gate data */
void nm_gate_ref_set(nm_gate_t p_gate, void*ref)
{
  p_gate->ref = ref;
}

