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

static int nm_strat_split_balance_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_split_balance, &nm_strat_split_balance_load, NULL, NULL);

/* Components structures:
 */

static void strat_split_balance_try_and_commit(void*, nm_gate_t );
static void strat_split_balance_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_split_balance_driver =
  {
    .pack_data          = NULL,
    .pack_ctrl          = NULL,
    .try_and_commit     = &strat_split_balance_try_and_commit,
    .rdv_accept         = &strat_split_balance_rdv_accept
};

static void*strat_split_balance_instantiate(puk_instance_t, puk_context_t);
static void strat_split_balance_destroy(void*);

static const struct puk_component_driver_s nm_strat_split_balance_component_driver =
  {
    .instantiate = &strat_split_balance_instantiate,
    .destroy     = &strat_split_balance_destroy
  };

struct nm_strat_split_balance
{
  int nm_max_small;
  int nm_copy_on_send_threshold;
};

/** Component declaration */
static int nm_strat_split_balance_load(void)
{
  puk_component_declare("NewMad_Strategy_split_balance",
			puk_component_provides("PadicoComponent", "component", &nm_strat_split_balance_component_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_strat_split_balance_driver),
			puk_component_attr("nm_max_small", "16342"),
			puk_component_attr("nm_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for split_balance strategy.
 */
static void*strat_split_balance_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_split_balance*p_status = TBX_MALLOC(sizeof(struct nm_strat_split_balance));
  const char*nm_max_small = puk_instance_getattr(ai, "nm_max_small");
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  p_status->nm_max_small = atoi(nm_max_small);
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  return p_status;
}

/** Cleanup the gate storage for split_balance strategy.
 */
static void strat_split_balance_destroy(void*_status)
{
  TBX_FREE(_status);
}

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static void strat_split_balance_try_and_commit(void *_status, nm_gate_t p_gate)
{
  struct nm_strat_split_balance*p_status = _status;
  struct nm_core*p_core = p_gate->p_core;
  int n = 0;
  nm_drv_t const*p_drvs = NULL;
  int nb_drivers = p_gate->n_trks;
  nm_ns_inc_lats(p_gate->p_core, &p_drvs, &nb_drivers);
  for(n = 0; n < nb_drivers; n++)
    {
      struct nm_trk_s*p_trk = &p_gate->trks[n];
      if(p_trk->kind != nm_trk_small)
	continue;
      struct nm_drv_s*p_drv = p_trk->p_drv;
      if(nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list) &&
	 nm_req_chunk_list_empty(&p_gate->req_chunk_list))
	break;
      /* find a driver with no active message on any track */
      /* TODO- */
      if(p_trk->p_pw_send == NULL)
	{
	  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header(p_core, p_trk);
	  /* ** control */
	  while(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	    {
	      /* ** post ctrl on trk #0 */
	      struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_pop_front(&p_gate->ctrl_chunk_list);
	      if(nm_pw_remaining_buf(p_pw) >= NM_HEADER_CTRL_SIZE)
		{
		  nm_pw_add_control(p_pw, &p_ctrl_chunk->ctrl);
		  nm_ctrl_chunk_free(&p_core->ctrl_chunk_allocator, p_ctrl_chunk);
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
	      const nm_len_t chunk_len = p_req_chunk->chunk_len;
	      const nm_len_t chunk_offset = p_req_chunk->chunk_offset;
	      if((chunk_offset == 0) && (chunk_len < 255) && (chunk_offset + chunk_len == p_pack->pack.len))
		{
		  /* ** short send */
		  if(nm_pw_remaining_buf(p_pw) >= NM_HEADER_SHORT_DATA_SIZE + chunk_len)
		    {
		      nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_NONE);
		      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
		    }
		  else
		    {
		      nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
		      break;
		    }
		}
	      else
		{
		  /* We aggregate ONLY if data are very small */
		  if((chunk_len > 4096) && (p_pw->length > sizeof(struct nm_header_global_s)))
		    {
		      nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
		      goto post_send;
		    }
		  if(NM_HEADER_DATA_SIZE + chunk_len < nm_drv_max_small(p_drv))
		    {
		      /* ** small send- always flatten data */
		      if(NM_HEADER_DATA_SIZE + chunk_len <= nm_pw_remaining_buf(p_pw))
			{
			  nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_USE_COPY);
			  assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
			}
		      else
			{
			  nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
			  break;
			}
		    }
		  else
		    {
		      /* ** large send */
		      const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
		      const nm_len_t density = (p_props->blocks > 0) ? p_props->size / p_props->blocks : 0; /* average block size */
		      if(nm_pw_remaining_buf(p_pw) >= NM_HEADER_CTRL_SIZE)
			{
			  const int is_lastchunk = (p_req_chunk->chunk_offset + p_req_chunk->chunk_len == p_pack->pack.len);
			  nm_req_flag_t flags = NM_REQ_FLAG_NONE;
			  if((!p_props->is_contig) && (density < NM_LARGE_MIN_DENSITY) && (p_pack->data.ops.p_generator == NULL))
			    {
			      flags |= NM_REQ_FLAG_USE_COPY;
			    }
			  struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader(p_core);
			  nm_pw_add_req_chunk(p_large_pw, p_req_chunk, flags);
			  nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_large_pw);
			  union nm_header_ctrl_generic_s rdv;
			  nm_header_init_rdv(&rdv, p_pack, chunk_len, chunk_offset, is_lastchunk ? NM_PROTO_FLAG_LASTCHUNK : 0);
			  nm_pw_add_control(p_pw, &rdv);
			}
		      else
			{
			  nm_req_chunk_list_push_front(&p_gate->req_chunk_list, p_req_chunk); /* rollback */
			  break;
			}
		    }
		}
	    }
	post_send:
	  nm_core_post_send(p_pw, p_gate, NM_TRK_SMALL);
	}
    }
}

/** Emit RTR for received RDV requests
 */
static void strat_split_balance_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      int nb_drivers = p_gate->n_trks;
      int chunk_index = 0;
      struct nm_rdv_chunk chunks[nb_drivers];
      nm_drv_t const*ordered_drv_id_by_bw = NULL;
      nm_ns_dec_bws(p_gate->p_core, &ordered_drv_id_by_bw, &nb_drivers);
      int i;
      for(i = 0; i < nb_drivers; i++)
	{
	  nm_drv_t p_drv = ordered_drv_id_by_bw[i];
	  struct nm_trk_s*p_trk = nm_gate_trk_get(p_gate, p_drv);
	  if(p_trk != NULL && p_trk->p_pw_recv == NULL)
	    {
	      chunks[chunk_index].trk_id = i;
	      chunk_index++;
	    }
	}
      int nb_chunks = chunk_index;
      if(nb_chunks == 1)
	{
	  chunks[0].len = p_pw->length;
	}
      else if(nb_chunks > 1)
	{
	  nm_ns_multiple_split_ratio(p_pw->length, p_gate->p_core, p_gate, &nb_chunks, chunks);
	}      
      if(nb_chunks > 0)
	{
	  nm_pkt_wrap_list_remove(&p_gate->pending_large_recv, p_pw);
	  nm_tactic_rtr_pack(p_gate->p_core, p_pw, nb_chunks, chunks);
	}
    }
}


