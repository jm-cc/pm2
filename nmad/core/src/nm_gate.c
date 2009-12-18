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

/** Initialize a new gate.
 *
 * out parameters:
 * p_id - id of the gate
 */
int nm_core_gate_init(nm_core_t p_core, nm_gate_t*pp_gate)
{
  int err = NM_ESUCCESS;

  struct nm_gate *p_gate = TBX_MALLOC(sizeof(struct nm_gate));

  memset(p_gate, 0, sizeof(struct nm_gate));

  p_gate->status = NM_GATE_STATUS_INIT;
  p_gate->p_core = p_core;
  p_gate->ref    = NULL;

  FUT_DO_PROBE1(FUT_NMAD_INIT_GATE, p_gate);

  nm_so_tag_table_init(&p_gate->tags);

  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_recv);
  TBX_INIT_FAST_LIST_HEAD(&p_gate->pending_large_send);

  p_gate->strategy_instance = puk_adapter_instanciate(p_core->strategy_adapter);
  puk_instance_indirect_NewMad_Strategy(p_gate->strategy_instance, NULL,
					&p_gate->strategy_receptacle);

  tbx_fast_list_add_tail(&p_gate->_link, &p_core->gate_list);

  *pp_gate = p_gate;
  
  return err;
}

/** Connect the process through a gate using a specified driver.
 */
static int nm_core_gate_connect_accept(struct nm_core	*p_core,
				       nm_gate_t	 p_gate,
				       nm_drv_t	         p_drv,
				       const char	*url,
				       int		 connect_flag)
{
  struct nm_cnx_rq rq =
    {
      .p_gate			= p_gate,
      .p_drv			= p_drv,
      .trk_id			= 0,
      .remote_drv_url		= url
    };
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

  /* instanciate driver */
  p_gdrv->instance = puk_adapter_instanciate(p_drv->assembly);
  puk_instance_indirect_NewMad_Driver(p_gdrv->instance, NULL, &p_gdrv->receptacle);

  /* init gate/driver fields */
  p_gdrv->p_drv	= rq.p_drv;
  p_gate->p_gate_drv_array[p_drv->id] = p_gdrv;

  /* connect/accept connections */
  for (trk_id = 0; trk_id < rq.p_drv->nb_tracks; trk_id++)
    {
      p_gdrv->p_in_rq_array[trk_id] = NULL;
      rq.trk_id = trk_id;

      if (connect_flag) {
	err = p_gdrv->receptacle.driver->connect(p_gdrv->receptacle._status, &rq);
	if (err != NM_ESUCCESS) {
	  NM_DISPF("drv.ops.connect returned %d", err);
	  goto out;
	}
      } else {
	err = p_gdrv->receptacle.driver->accept(p_gdrv->receptacle._status, &rq);
	if (err != NM_ESUCCESS) {
	  NM_DISPF("drv.ops.accept returned %d", err);
	  goto out;
	}
      }

    }

  err = NM_ESUCCESS;
  p_gate->status = NM_GATE_STATUS_CONNECTED;

 out:
  return err;

}

/** Server side of connection establishment.
 */
int nm_core_gate_accept(nm_core_t	 p_core,
			nm_gate_t	 p_gate,
			nm_drv_t	 p_drv,
			const char	*url)
{
  return nm_core_gate_connect_accept(p_core, p_gate, p_drv, url, 0);
}

/** Client side of connection establishment.
 */
int nm_core_gate_connect(nm_core_t p_core,
                         nm_gate_t p_gate,
                         nm_drv_t  p_drv,
                         const char*url)
{
  return nm_core_gate_connect_accept(p_core, p_gate, p_drv, url, !0);
}

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(struct nm_gate*p_gate)
{
  return p_gate->ref;
}

/** Set the user-registered per-gate data */
void nm_gate_ref_set(struct nm_gate*p_gate, void*ref)
{
  p_gate->ref = ref;
}

