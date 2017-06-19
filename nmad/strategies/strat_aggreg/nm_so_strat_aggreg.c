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
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <Padico/Module.h>

static int nm_strat_aggreg_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_aggreg, &nm_strat_aggreg_load, NULL, NULL);

/* Components structures:
 */

static void strat_aggreg_try_and_commit(void*, nm_gate_t );
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
 */
static void strat_aggreg_try_and_commit(void *_status, nm_gate_t p_gate)
{
  struct nm_strat_aggreg_s*p_status = _status;
  struct nm_trk_s*p_trk_small = &p_gate->trks[NM_TRK_SMALL];
  const struct nm_drv_s*p_drv = p_trk_small->p_drv;
  if((p_trk_small->p_pw_send == NULL) &&
     !(nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list) &&
       nm_req_chunk_list_empty(&p_gate->req_chunk_list)))
    {
      /* no active send && pending chunks */
      struct nm_pkt_wrap_s*p_pw = NULL;
      const nm_len_t drv_max = ((p_drv->props.capabilities.max_msg_size == 0) ||
				(p_drv->props.capabilities.max_msg_size > NM_SO_MAX_UNEXPECTED)) ?
	NM_SO_MAX_UNEXPECTED : p_drv->props.capabilities.max_msg_size;
      const nm_len_t max_small =  drv_max - NM_ALIGN_FRONTIER - sizeof(struct nm_header_global_s);
      if(p_drv->props.capabilities.supports_buf_send)
	{
	  p_pw = nm_pw_alloc_driver_header(p_trk_small);
	}
      else
	{
	  p_pw = nm_pw_alloc_global_header();
	  p_pw->max_len = max_small;
	}
      int opt_window = 8;
      /* ** control */
      while(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	{
	  /* ** post ctrl on trk #0 */
	  struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_begin(&p_gate->ctrl_chunk_list);
	  int rc = nm_tactic_pack_ctrl(p_gate, p_drv, p_ctrl_chunk, p_pw);
	  if(rc)
	    {
	      /* don't even try to aggregate data if pw cannot even contain any ctrl header */
	      goto post_send;
	    }
	}
      /* ** data */
      while(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_begin(&p_gate->req_chunk_list);
	  struct nm_req_s*p_pack = p_req_chunk->p_req;
	  const nm_len_t chunk_len = p_req_chunk->chunk_len;
	  const nm_len_t chunk_offset = p_req_chunk->chunk_offset;
	  if((chunk_offset == 0) && (chunk_len < 255) && (chunk_offset + chunk_len == p_pack->pack.len))
	    {
	      /* ** short send */
	      if(NM_HEADER_SHORT_DATA_SIZE + chunk_len + p_pw->length <= max_small)
		{
		  nm_req_chunk_list_remove(&p_gate->req_chunk_list, p_req_chunk);
		  nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_SHORT_CHUNK);
		  assert(p_pw->length <= max_small);
		}
	      else
		{
		  goto post_send;
		}
	    }
	  else
	    {
	      const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
#warning TODO- max block count and header size are rough estimates
	      /* maximum header length- real leength depends on block size */
	      const nm_len_t max_blocks = (p_props->blocks > chunk_len) ? chunk_len : p_props->blocks;
	      const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + max_blocks * sizeof(struct nm_header_pkt_data_chunk_s) + NM_ALIGN_FRONTIER;
	      const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : 0; /* average block size */
	      if(chunk_len + max_header_len < max_small)
		{
		  /* ** small send */
		  if(max_header_len + chunk_len < nm_pw_remaining_buf(p_pw))
		    {
#warning TODO- select pack strategy depending on data sparsity
		      nm_req_chunk_list_remove(&p_gate->req_chunk_list, p_req_chunk);
		      nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_NONE);
		      assert(p_pw->length <= max_small);
		    }
		  else
		    {
#warning TODO- no optimize window for now.
		      goto post_send;
		    }
		}
	      else
		{
		  /* ** large send */
		  int rc = nm_tactic_pack_rdv(p_gate, p_drv, p_req_chunk, p_pw);
		  if(rc)
		    {
		      goto post_send;
		    }
		}
	    }
	}
    post_send:
      nm_core_post_send(p_pw, p_gate, NM_TRK_SMALL);
    }
}

/** Emit RTR for received RDV requests
 */
static void strat_aggreg_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      struct nm_trk_s*p_trk_large = &p_gate->trks[NM_TRK_LARGE];
#if 0
      const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_unpack->data);
      const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : p_props->size; /* average block size */
      if(p_trk_large->p_drv->props.capabilities.supports_data || density < NM_LARGE_MIN_DENSITY)
	{
	  /* iterator-based data & driver supports data natively || low-density -> send all & copy */
	}
      else
	{
	  /* one rdv per chunk */
	  struct nm_large_chunk_context_s large_chunk = { .p_unpack = p_unpack, .p_gate = p_unpack->p_gate, .chunk_offset = chunk_offset };
	  nm_data_chunk_extractor_traversal(&p_unpack->data, chunk_offset, chunk_len, &nm_large_chunk_store, &large_chunk);
	}
#endif
      
      if(p_pw->length > NM_LARGE_MIN_DENSITY)
	{
	  if(p_trk_large->p_pw_recv == NULL)
	    {
	      /* The large-packet track is available- post recv and RTR */
	      struct nm_rdv_chunk chunk = 
		{ .len = p_pw->length, .trk_id = NM_TRK_LARGE };
	      nm_pkt_wrap_list_remove(&p_gate->pending_large_recv, p_pw);
	      nm_tactic_rtr_pack(p_pw, 1, &chunk);
	    }
	}
      else
	{
	  /* small chunk in a large packet- send on trk#0 */
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .trk_id = NM_TRK_SMALL };
	  nm_pkt_wrap_list_remove(&p_gate->pending_large_recv, p_pw);
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
