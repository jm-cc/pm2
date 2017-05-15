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

static int  strat_aggreg_try_and_commit(void*, nm_gate_t );
static void strat_aggreg_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_aggreg_driver =
  {
    .pack_data          = NULL,
    .pack_ctrl          = NULL,
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
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  return p_status;
}

/** Cleanup the gate storage for aggreg strategy.
 */
static void strat_aggreg_destroy(void*_status)
{
  TBX_FREE(_status);
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
  struct nm_core*p_core = p_gate->p_core;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  if((p_gdrv->active_send[NM_TRK_SMALL] == 0) &&
     !(nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list) &&
       nm_req_chunk_list_empty(&p_gate->req_chunk_list)))
    {
      /* no active send && pending chunks */
      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
      int opt_window = 8;
      /* ** control */
      while(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	{
	  /* ** post ctrl on trk #0 */
	  struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_pop_front(&p_gate->ctrl_chunk_list);
	  if(nm_pw_remaining_buf(p_pw) >= NM_HEADER_CTRL_SIZE)
	    {
	      nm_pw_add_control(p_pw, &p_ctrl_chunk->ctrl);
	      nm_ctrl_chunk_free(p_core->ctrl_chunk_allocator, p_ctrl_chunk);
	    }
	  else
	    {
	      /* don't even try to aggregate data if pw cannot even contain any ctrl header */
	      nm_ctrl_chunk_list_push_front(&p_gate->ctrl_chunk_list, p_ctrl_chunk); /* rollback */
	      goto post_send;
	    }
	}
      /* ** data */
      while(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_gate->req_chunk_list);
	  struct nm_req_s*p_pack = p_req_chunk->p_req;
	  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
#warning TODO- max block count and header size are rough estimates
	  /* maximum header length- real leength depends on block size */
	  const nm_len_t chunk_len = p_req_chunk->chunk_len;
	  const nm_len_t chunk_offset = p_req_chunk->chunk_offset;
	  const nm_len_t max_blocks = (p_props->blocks > chunk_len) ? chunk_len : p_props->blocks;
	  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + max_blocks * sizeof(struct nm_header_pkt_data_chunk_s) + NM_ALIGN_FRONTIER;
	  const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : 0; /* average block size */
	  if(chunk_len + max_header_len < nm_drv_max_small(p_pack->p_gate->p_core))
	    {
	      /* ** small send */
	      if(nm_pw_remaining_buf(p_pw) >= max_header_len + chunk_len)
		{
#warning TODO- select pack strategy depending on data sparsity
		  nm_pw_add_data_chunk(p_pw, p_pack, chunk_len, chunk_offset, NM_PW_DATA_ITERATOR);
		  assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
		}
	      else
		{
#warning TODO- no optimize window for now.
		  nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
		  break;
		}
	    }
	  else
	    {
	      /* ** large send */
	      if(nm_pw_remaining_buf(p_pw) >= NM_HEADER_CTRL_SIZE)
		{
		  nm_pw_flag_t flags = NM_PW_NOHEADER | NM_PW_DATA_ITERATOR;
		  if((!p_props->is_contig) && (density < NM_LARGE_MIN_DENSITY) && (p_pack->data.ops.p_generator == NULL))
		    {
		      flags |= NM_PW_DATA_USE_COPY;
		    }
		  struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader();
		  nm_pw_add_data_chunk(p_large_pw, p_pack, chunk_len, chunk_offset, flags);
		  nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_large_pw);
		  union nm_header_ctrl_generic_s rdv;
		  nm_header_init_rdv(&rdv, p_pack, chunk_len, chunk_offset, (p_pack->pack.scheduled == p_pack->pack.len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
		  nm_pw_add_control(p_pw, &rdv);
		}
	      else
		{
		  nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
		  break;
		}
	    }
	  nm_req_chunk_destroy(p_core, p_req_chunk);
	}
    post_send:
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
