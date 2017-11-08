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
void nm_core_gate_init(nm_core_t p_core, nm_gate_t*pp_gate, nm_drv_vect_t*p_drvs)
{
  nm_gate_t p_gate = nm_gate_new();

  p_gate->status = NM_GATE_STATUS_INIT;
  p_gate->p_core = p_core;
  p_gate->trks = NULL;
  p_gate->n_trks = 0;
  nm_gtag_table_init(&p_gate->tags);

  nm_pkt_wrap_list_init(&p_gate->pending_large_recv);
  nm_pkt_wrap_list_init(&p_gate->pending_large_send);

  nm_req_chunk_list_init(&p_gate->req_chunk_list);
  nm_ctrl_chunk_list_init(&p_gate->ctrl_chunk_list);
  
  p_gate->strategy_instance = puk_component_instantiate(p_core->strategy_component);
  puk_instance_indirect_NewMad_Strategy(p_gate->strategy_instance, NULL,
					&p_gate->strategy_receptacle);
  nm_gate_list_push_back(&p_core->gate_list, p_gate);

#ifdef NMAD_TRACE
  static int nm_trace_gate_count = 0;
  p_gate->trace_connections_id = nm_trace_gate_count++;
#endif /* NMAD_TRACE */

  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->strategy_receptacle;
  if(r->driver->connect)
    {
      nm_drv_vect_t v = (*r->driver->connect)(r->_status, p_gate, *p_drvs);
      nm_drv_vect_delete(*p_drvs);
      *p_drvs = v;
    }
  p_gate->n_trks = nm_drv_vect_size(*p_drvs);
  p_gate->trks = malloc(p_gate->n_trks * sizeof(struct nm_trk_s));
  assert(p_gate->n_trks > 0);
  int i;
  for(i = 0; i < p_gate->n_trks; i++)
    {
      struct nm_trk_s*p_trk = &p_gate->trks[i];
      struct nm_drv_s*p_drv = nm_drv_vect_at(*p_drvs, i);
      /* instantiate driver */
      p_trk->p_drv = p_drv;
      p_trk->instance = puk_component_instantiate(p_drv->assembly);
      puk_instance_indirect_NewMad_minidriver(p_trk->instance, NULL, &p_trk->receptacle);
      p_trk->p_pw_send = NULL;
      p_trk->p_pw_recv = NULL;
      nm_data_null_build(&p_trk->sdata);
      nm_data_null_build(&p_trk->rdata);
      p_trk->kind = nm_trk_undefined;
      if(p_drv->props.capabilities.trk_rdv)
	p_trk->kind = nm_trk_large;
      else if(i == 0)
	p_trk->kind = nm_trk_small;
      else
	p_trk->kind = nm_trk_large;
    }

  *pp_gate = p_gate;
}

/** Connect the process through a gate using a specified driver.
 */
void nm_core_gate_connect(struct nm_core*p_core, nm_gate_t p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*url)
{
  assert(url != NULL);
  nm_core_lock(p_core);
  struct nm_trk_s*p_trk = nm_trk_get_by_index(p_gate, trk_id);
  assert(p_trk != NULL);
  assert(p_trk->p_drv == p_drv);
  p_gate->status = NM_GATE_STATUS_CONNECTING;
  int url_size = strlen(url);
  void*binary_url = puk_hex_decode(url, &url_size, NULL);
  const size_t binary_url_size = url_size;
  struct puk_receptacle_NewMad_minidriver_s*r = &p_trk->receptacle;
  (*r->driver->connect)(r->_status, binary_url, binary_url_size);
  p_gate->status = NM_GATE_STATUS_CONNECTED;
  /* pre-fill recv on trk #0 */
  if(p_trk->kind == nm_trk_small)
    nm_drv_refill_recv(p_drv, p_gate);
#ifdef NMAD_TRACE
  nm_trace_container(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_NEW_CONNECTION, p_gate->trace_connections_id);
#endif /* NMAD_TRACE */

  nm_core_unlock(p_core);
}

/** notify from driver p_drv that gate p_gate has been closed by peer.
 */
void nm_gate_disconnected(struct nm_core*p_core, nm_gate_t p_gate, nm_drv_t p_drv)
{
  p_gate->status = NM_GATE_STATUS_DISCONNECTING;
  int connected = 0;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
        connected++;
    }
  if((connected > 0) && (p_drv->props.capabilities.has_recv_any))
    {
      nm_drv_refill_recv(p_drv, NULL);
    }
}
