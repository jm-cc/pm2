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


/* ********************************************************* */

static void strat_prio_try_and_commit(void*, nm_gate_t );
static void strat_prio_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_prio_driver =
  {
    .pack_data          = NULL,
    .pack_ctrl          = NULL,
    .try_and_commit     = &strat_prio_try_and_commit,
    .rdv_accept         = &strat_prio_rdv_accept
};

static void*strat_prio_instantiate(puk_instance_t, puk_context_t);
static void strat_prio_destroy(void*);

static const struct puk_component_driver_s nm_strat_prio_component_driver =
  {
    .instantiate = &strat_prio_instantiate,
    .destroy     = &strat_prio_destroy
  };

/* ********************************************************* */

/** Component declaration */
PADICO_MODULE_COMPONENT(NewMad_Strategy_prio,
                        puk_component_declare("NewMad_Strategy_prio",
                                              puk_component_provides("PadicoComponent", "component", &nm_strat_prio_component_driver),
                                              puk_component_provides("NewMad_Strategy", "strat", &nm_strat_prio_driver),
                                              puk_component_attr("nm_copy_on_send_threshold", "4096"))
                        );

/* ********************************************************* */


PUK_LIST_TYPE(nm_prio_tag_chunk,
	      struct nm_req_chunk_s*p_req_chunk;
	      );

PUK_ALLOCATOR_TYPE(nm_prio_tag_chunk, struct nm_prio_tag_chunk_s);

static nm_prio_tag_chunk_allocator_t nm_prio_tag_chunk_allocator = NULL;

/** per-tag strat prio status */
struct nm_prio_tag_s
{
  struct nm_prio_tag_chunk_list_s chunks; /**< list of chunks submitted on the tag, ordered by submission date */
};
static inline void nm_prio_tag_ctor(struct nm_prio_tag_s*p_prio_tag, nm_core_tag_t tag)
{
  nm_prio_tag_chunk_list_init(&p_prio_tag->chunks);
}
static inline void nm_prio_tag_dtor(struct nm_prio_tag_s*p_prio_tag)
{
  nm_prio_tag_chunk_list_destroy(&p_prio_tag->chunks);
}
NM_TAG_TABLE_TYPE(nm_prio_tag, struct nm_prio_tag_s);

/** Per-gate status for strat prio instances
 */
struct nm_strat_prio_s
{
  struct nm_req_chunk_list_s req_chunk_list; /**< list of chunks, ordered by priority */
  struct nm_prio_tag_table_s tags;
  int nm_copy_on_send_threshold;
};

/** Initialize the gate storage for prio strategy.
 */
static void*strat_prio_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_strat_prio_s*p_status = malloc(sizeof(struct nm_strat_prio_s));
  const char*nm_copy_on_send_threshold = puk_instance_getattr(instance, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  nm_req_chunk_list_init(&p_status->req_chunk_list);
  nm_prio_tag_table_init(&p_status->tags);
  if(nm_prio_tag_chunk_allocator == NULL)
    nm_prio_tag_chunk_allocator = nm_prio_tag_chunk_allocator_new(16);
  return p_status;
}

/** Cleanup the gate storage for prio strategy.
 */
static void strat_prio_destroy(void*_status)
{
  struct nm_strat_prio_s*p_status = _status;
  nm_req_chunk_list_destroy(&p_status->req_chunk_list);
  nm_prio_tag_table_destroy(&p_status->tags);
  free(_status);
}

/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 */
static void strat_prio_try_and_commit(void*_status, nm_gate_t p_gate)
{
  struct nm_strat_prio_s*p_status = _status;
  struct nm_trk_s*p_trk_small = &p_gate->trks[NM_TRK_SMALL];
  struct nm_drv_s*p_drv = p_trk_small->p_drv;
  struct nm_core*p_core = p_drv->p_core;
  while(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
    {
      /* ** store req chunk in priority list */
      struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_gate->req_chunk_list);
      const int prio = p_req_chunk->p_req->pack.priority;
      nm_req_chunk_itor_t i = nm_req_chunk_list_rend(&p_status->req_chunk_list);
      while((i != NULL) && (prio > i->p_req->pack.priority))
	{
	  i = nm_req_chunk_list_rnext(i);
	}
      if(i)
	nm_req_chunk_list_insert_after(&p_status->req_chunk_list, i, p_req_chunk);
      else
	nm_req_chunk_list_push_front(&p_status->req_chunk_list, p_req_chunk);
      /* ** store req chunk in tag table */
      struct nm_prio_tag_s*p_prio_tag = nm_prio_tag_get(&p_status->tags, p_req_chunk->p_req->tag);
      struct nm_prio_tag_chunk_s*p_prio_tag_chunk = nm_prio_tag_chunk_malloc(nm_prio_tag_chunk_allocator);
      p_prio_tag_chunk->p_req_chunk = p_req_chunk;
      nm_prio_tag_chunk_list_push_back(&p_prio_tag->chunks, p_prio_tag_chunk);
    }
  if((p_trk_small->p_pw_send == NULL) &&
     !(nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list) &&
       nm_req_chunk_list_empty(&p_status->req_chunk_list)))
    {
      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header(p_core, p_trk_small);
      if(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	{
	  /* post ctrl on trk #0 */
	  struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_begin(&p_gate->ctrl_chunk_list);
	  int rc __attribute__((unused));
	  rc = nm_tactic_pack_ctrl(p_gate, p_drv, p_ctrl_chunk, p_pw);
	  assert(rc == NM_ESUCCESS);
	}
      else if(!nm_req_chunk_list_empty(&p_status->req_chunk_list))
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_begin(&p_status->req_chunk_list);
	  /* take the head of the lists of chunks in the tag of the highest priority chunk */
	  const nm_core_tag_t tag = p_req_chunk->p_req->tag;
	  struct nm_prio_tag_s*p_prio_tag = nm_prio_tag_get(&p_status->tags, tag);
	  struct nm_prio_tag_chunk_s*p_prio_tag_chunk = nm_prio_tag_chunk_list_begin(&p_prio_tag->chunks);
	  if(p_req_chunk != p_prio_tag_chunk->p_req_chunk)
	    {
	      /* there is an older req_chunk on the same tag as the max prio */
	      p_req_chunk = p_prio_tag_chunk->p_req_chunk;
	    }	  
	  struct nm_req_s*p_pack = p_req_chunk->p_req;
	  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
	  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + p_props->blocks * sizeof(struct nm_header_pkt_data_chunk_s);
	  if(p_req_chunk->chunk_len + max_header_len <= p_pw->max_len)
	    {
	      /* post short data on trk #0 */
	      nm_req_chunk_list_remove(&p_status->req_chunk_list, p_req_chunk);
	      nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_CHUNK_FLAG_USE_COPY);
	    }
	  else
	    {
	      /* post RDV for large data */
	      nm_tactic_pack_rdv(p_gate, p_drv, &p_status->req_chunk_list, p_req_chunk, p_pw);
	    }
	  assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
	  nm_prio_tag_chunk_list_pop_front(&p_prio_tag->chunks);
	  nm_prio_tag_chunk_free(nm_prio_tag_chunk_allocator, p_prio_tag_chunk);
	  if(nm_prio_tag_chunk_list_empty(&p_prio_tag->chunks))
	    {
	      /* garbage-collect empty tags */
	      nm_prio_tag_delete(&p_status->tags, p_prio_tag);
	    }
	}
      nm_core_post_send(p_pw, p_gate, NM_TRK_SMALL);
    }
}

/** Emit RTR for received RDV requests
 */
static void strat_prio_rdv_accept(void*_status, nm_gate_t p_gate)
{
  if(!nm_pkt_wrap_list_empty(&p_gate->pending_large_recv))
    {
      struct nm_trk_s*p_trk_large = &p_gate->trks[NM_TRK_LARGE];
      if(p_trk_large->p_pw_recv == NULL)
	{
	  /* The large-packet track is available- post recv and RTR */
	  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_pop_front(&p_gate->pending_large_recv);
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .trk_id = NM_TRK_LARGE };
	  nm_tactic_rtr_pack(p_gate->p_core, p_pw, 1, &chunk);
	}
    }
}
