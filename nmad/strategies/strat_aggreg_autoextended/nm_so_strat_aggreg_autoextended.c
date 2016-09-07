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

static int  strat_aggreg_autoextended_todo(void*, nm_gate_t );
static void strat_aggreg_autoextended_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);
static int  strat_aggreg_autoextended_pack_ctrl(void*, nm_gate_t , const union nm_header_ctrl_generic_s*);
static int  strat_aggreg_autoextended_try_and_commit(void*, nm_gate_t );
static void strat_aggreg_autoextended_rdv_accept(void*, nm_gate_t );
static int  strat_aggreg_autoextended_flush(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_so_strat_aggreg_autoextended_driver =
  {
    .pack_chunk     = &strat_aggreg_autoextended_pack_chunk,
    .pack_ctrl      = &strat_aggreg_autoextended_pack_ctrl,
    .try_and_commit = &strat_aggreg_autoextended_try_and_commit,
    .rdv_accept     = &strat_aggreg_autoextended_rdv_accept,
    .flush          = &strat_aggreg_autoextended_flush,
    .todo           = &strat_aggreg_autoextended_todo
  };

static void*strat_aggreg_autoextended_instantiate(puk_instance_t, puk_context_t);
static void strat_aggreg_autoextended_destroy(void*);

static const struct puk_component_driver_s nm_so_strat_aggreg_autoextended_component_driver =
  {
    .instantiate = &strat_aggreg_autoextended_instantiate,
    .destroy     = &strat_aggreg_autoextended_destroy
  };

/** Per-gate status for strat aggreg_autoextended instances.
 */
struct nm_so_strat_aggreg_autoextended_gate {
  /** List of raw outgoing packets. */
  struct nm_pkt_wrap_list_s out_list;
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
			puk_component_provides("PadicoComponent", "component", &nm_so_strat_aggreg_autoextended_component_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_aggreg_autoextended_driver),
			puk_component_attr("nm_so_max_small", "16342"),
			puk_component_attr("nm_so_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for aggreg_autoextended strategy.
 */
static void*strat_aggreg_autoextended_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_so_strat_aggreg_autoextended_gate*p_status = TBX_MALLOC(sizeof(struct nm_so_strat_aggreg_autoextended_gate));

  num_instances++;
  nm_pkt_wrap_list_init(&p_status->out_list);

  const char*nm_so_max_small = puk_instance_getattr(instance, "nm_so_max_small");
  p_status->nm_so_max_small = atoi(nm_so_max_small);

  const char*nm_so_copy_on_send_threshold = puk_instance_getattr(instance, "nm_so_copy_on_send_threshold");
  p_status->nm_so_copy_on_send_threshold = atoi(nm_so_copy_on_send_threshold);

  return p_status;
}

/** Cleanup the gate storage for aggreg_autoextended strategy.
 */
static void strat_aggreg_autoextended_destroy(void*_status)
{
  TBX_FREE(_status);
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
static int strat_aggreg_autoextended_pack_ctrl(void*_status, nm_gate_t p_gate,
                                               const union nm_header_ctrl_generic_s*p_ctrl)
{
  struct nm_pkt_wrap_s*p_pw = NULL;
  struct nm_so_strat_aggreg_autoextended_gate*p_status = _status;
  int err;

  /* We first try to find an existing packet to form an aggregate */
  puk_list_foreach(p_pw, &p_status->out_list)
    {
      if(nm_so_pw_remaining_buf(p_pw) < NM_HEADER_CTRL_SIZE) 
	{
	  /* There's not enough room to add our ctrl header to this paquet */
	  nm_so_pw_finalize(p_pw);
	}
      else
	{
	  err = nm_so_pw_add_control(p_pw, p_ctrl);
	  nb_ctrl_aggregation ++;
	  goto out;
	}
    }

  /* Simply form a new packet wrapper */
  p_pw = nm_pw_alloc_global_header();
  err = nm_so_pw_add_control(p_pw, p_ctrl);
  if(err != NM_ESUCCESS)
    goto out;

  /* Add the control packet to the out_list */
  nm_pkt_wrap_list_push_back(&p_status->out_list, p_pw);

 out:
  return err;
}

static int strat_aggreg_autoextended_flush(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = NULL;
  struct nm_so_strat_aggreg_autoextended_gate*p_status = _status;
  puk_list_foreach(p_pw, &p_status->out_list)
    {
      NM_TRACEF("Marking pw %p as completed\n", p_pw);
      nm_so_pw_finalize(p_pw);
    }
  return NM_ESUCCESS;
}

static int strat_aggreg_autoextended_todo(void*_status, nm_gate_t p_gate)
{
  struct nm_so_strat_aggreg_autoextended_gate*p_status = _status;
  return !(nm_pkt_wrap_list_empty(&p_status->out_list));
}

/** push message chunk */
static void strat_aggreg_autoextended_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset)
{
  struct nm_so_strat_aggreg_autoextended_gate*p_status = _status;
  struct nm_pkt_wrap_s*p_pw = NULL;
  int flags = 0;
  if(len <= p_status->nm_so_copy_on_send_threshold)
    flags |= NM_PW_DATA_USE_COPY;

  if(len <= p_status->nm_so_max_small)
    {
      /* small packet */
      puk_list_foreach(p_pw, &p_status->out_list)
	{
	  const nm_len_t rlen = nm_so_pw_remaining_buf(p_pw);
	  const nm_len_t size = NM_HEADER_DATA_SIZE + nm_so_aligned(len);
	  if(size > rlen)
	    {
	      nm_so_pw_finalize(p_pw);
	    }
	  else
	    {
	      nm_so_pw_add_data_chunk(p_pw, p_pack, ptr, len, chunk_offset, flags);
	      nb_data_aggregation ++;
	      return;
	    }
	}
      /* cannot aggregate- create a new pw */
      p_pw = nm_pw_alloc_global_header();
      nm_so_pw_add_data_chunk(p_pw, p_pack, ptr, len, chunk_offset, flags);
      nm_pkt_wrap_list_push_back(&p_status->out_list, p_pw);
    }
  else
    {
      /* large packet */
      p_pw = nm_pw_alloc_noheader();
      nm_so_pw_add_data_chunk(p_pw, p_pack, ptr, len, chunk_offset, flags);
      nm_so_pw_finalize(p_pw);
      nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_pw);
      union nm_header_ctrl_generic_s ctrl;
      nm_header_init_rdv(&ctrl, p_pack, len, 0, NM_PROTO_FLAG_LASTCHUNK);
      strat_aggreg_autoextended_pack_ctrl(p_status, p_pack->p_gate, &ctrl);
    }
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_aggreg_autoextended_try_and_commit(void*_status, nm_gate_t p_gate)
{
  struct nm_so_strat_aggreg_autoextended_gate*p_status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if(p_gdrv->active_send[NM_TRK_SMALL] == 1)
    /* We're done */
    goto out;

  if(nm_pkt_wrap_list_empty(&p_status->out_list))
    /* We're done */
    goto out;

  /* Simply take the head of the list */
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_status->out_list);
  if((p_pw->flags & NM_PW_FINALIZED) != 0)
    {
      NM_TRACEF("pw %p is completed\n", p_pw);
      nm_pkt_wrap_list_pop_front(&p_status->out_list);
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

/** Emit RTR for received RDV requests
 */
static void strat_aggreg_autoextended_rdv_accept(void*_status, nm_gate_t p_gate)
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

