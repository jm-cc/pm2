/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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

static int nm_strat_default_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_default, &nm_strat_default_load, NULL, NULL);


/* Components structures:
 */

static void strat_default_pack_data(void*_status, struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset);
static int  strat_default_pack_ctrl(void*, nm_gate_t , const union nm_header_ctrl_generic_s*);
static int  strat_default_try_and_commit(void*, nm_gate_t );
static void strat_default_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_default_driver =
  {
    .pack_data          = &strat_default_pack_data,
    .pack_chunk         = NULL,
    .pack_ctrl          = &strat_default_pack_ctrl,
    .try_and_commit     = &strat_default_try_and_commit,
    .rdv_accept         = &strat_default_rdv_accept,
    .flush              = NULL
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


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param _status the strat_default instance status.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_default_pack_ctrl(void*_status, nm_gate_t p_gate, const union nm_header_ctrl_generic_s *p_ctrl)
{
  nm_tactic_pack_ctrl(p_ctrl, &p_gate->out_list);
  return NM_ESUCCESS;
}

/** push a message chunk */
static void strat_default_pack_data(void*_status, struct nm_req_s*p_pack, nm_len_t chunk_len, nm_len_t chunk_offset)
{
  struct nm_strat_default_s*p_status = _status;
  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + p_props->blocks * sizeof(struct nm_header_pkt_data_chunk_s);
  if(chunk_len + max_header_len <= p_status->nm_max_small)
    {
      nm_tactic_pack_small_new_pw(p_pack, chunk_len, chunk_offset, &p_pack->p_gate->out_list);
    }
  else
    {
      nm_tactic_pack_data_rdv(p_pack, chunk_len, chunk_offset);
    }
}


/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_default_try_and_commit(void*_status, nm_gate_t p_gate)
{
#ifdef PROFILE_NMAD
  static long double wait_time = 0.0;
  static int count = 0, send_count = 0;
  static long long int send_size = 0;
  static tbx_tick_t t_orig;
#endif /* PROFILE_NMAD */

  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if((p_gdrv->active_send[NM_TRK_SMALL] == 0) &&
     !(nm_pkt_wrap_list_empty(&p_gate->out_list)))
    {
#ifdef PROFILE_NMAD
      if(count != 0)
	{
	  tbx_tick_t t2;
	  TBX_GET_TICK(t2);
	  const long double t = TBX_TIMING_DELAY(t_orig, t2);
	  wait_time += t;
	  if(random() < 100000)
	    fprintf(stderr, "wait time = %fus (%fs) send = %d; size = %dMB; avg %d bytes/msg\n", 
		    (double)t, (double)wait_time / 1000000.0, send_count, (int)(send_size / 1000000),
		    (int)(send_size/send_count));
	}
      count = 0;
      send_count++;
      send_size += p_pw->length;
#endif /* PROFILE_NMAD */
      struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_pop_front(&p_gate->out_list);
      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
    }
  else if((p_gdrv->active_send[NM_TRK_SMALL] != 0) && !(nm_pkt_wrap_list_empty(&p_gate->out_list)))
    {
#ifdef PROFILE_NMAD
      if(count == 0)
	{
	  TBX_GET_TICK(t_orig);
	}
      count++;
#endif /* PROFILE_NMAD */
    }
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_default_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      nm_drv_t p_drv = nm_drv_default(p_gate);
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
}
