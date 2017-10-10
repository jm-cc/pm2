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

PUK_LIST_DECLARE_TYPE2(nm_prio_tag_chunk, struct nm_prio_chunk_s)
PUK_LIST_DECLARE_TYPE2(nm_prio_queue_chunk, struct nm_prio_chunk_s);

/** a req_chunk wrapper, to be inserted in both tag-indexed lists and priority queues */
struct nm_prio_chunk_s
{
  struct nm_req_chunk_s*p_req_chunk;
  PUK_LIST_LINK(nm_prio_tag_chunk);
  PUK_LIST_LINK(nm_prio_queue_chunk);
};

PUK_LIST_CREATE_FUNCS(nm_prio_tag_chunk);
PUK_LIST_CREATE_FUNCS(nm_prio_queue_chunk);

PUK_ALLOCATOR_TYPE(nm_prio_chunk, struct nm_prio_chunk_s);
static nm_prio_chunk_allocator_t nm_prio_chunk_allocator = NULL;

PUK_PRIO_DECLARE_TYPE2(nm_prio_queue, struct nm_prio_queue_s)
struct nm_prio_queue_s
{
  struct nm_prio_queue_chunk_list_s chunks; /**< list of chunks for a given priority level */
  PUK_PRIO_LINK(nm_prio_queue);
};
PUK_PRIO_CREATE_FUNCS(nm_prio_queue);

PUK_ALLOCATOR_TYPE(nm_prio_queue, struct nm_prio_queue_s);
static nm_prio_queue_allocator_t nm_prio_queue_allocator = NULL;

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
  struct nm_prio_queue_prio_list_s prio_queues; /**< priority list for queues */
  puk_hashtable_t queues_by_prio;               /**< chunks queues, indexed by priority */
  struct nm_prio_tag_table_s tags;              /**< list of chunks, indexed by tag */
  int nm_copy_on_send_threshold;
};

/** compute hashkey for given priority */
static inline void*nm_strat_prio_hashkey(int prio)
{
  assert(prio >= 0);
  const uintptr_t hprio = 1 + (uintptr_t)prio;
  return (void*)hprio;
}

/** Initialize the gate storage for prio strategy.
 */
static void*strat_prio_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_strat_prio_s*p_status = malloc(sizeof(struct nm_strat_prio_s));
  const char*nm_copy_on_send_threshold = puk_instance_getattr(instance, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  nm_prio_tag_table_init(&p_status->tags);
  nm_prio_queue_prio_list_init(&p_status->prio_queues);
  p_status->queues_by_prio = puk_hashtable_new_int();
  if(nm_prio_chunk_allocator == NULL)
    nm_prio_chunk_allocator = nm_prio_chunk_allocator_new(16);
  if(nm_prio_queue_allocator == NULL)
    nm_prio_queue_allocator = nm_prio_queue_allocator_new(16);
  return p_status;
}

/** Cleanup the gate storage for prio strategy.
 */
static void strat_prio_destroy(void*_status)
{
  struct nm_strat_prio_s*p_status = _status;
  /* TODO- destroy hashtable and prio list */
  nm_prio_tag_table_destroy(&p_status->tags);
  free(_status);
}

/** flush req chunks from list in gate to priority queues */
static inline void strat_prio_flush_reqs(struct nm_strat_prio_s*p_status, nm_gate_t p_gate)
{
  while(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
    {
      struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_gate->req_chunk_list);
      const int prio = p_req_chunk->p_req->pack.priority;
      struct nm_prio_queue_s*p_prio_queue = puk_hashtable_lookup(p_status->queues_by_prio, nm_strat_prio_hashkey(prio));
      if(p_prio_queue == NULL)
	{
	  /* create new priority queue for the given level */
	  p_prio_queue = nm_prio_queue_malloc(nm_prio_queue_allocator);
	  nm_prio_queue_chunk_list_init(&p_prio_queue->chunks);
	  nm_prio_queue_prio_list_insert(&p_status->prio_queues, prio, p_prio_queue);
	  puk_hashtable_insert(p_status->queues_by_prio, nm_strat_prio_hashkey(prio), p_prio_queue);
	}
      struct nm_prio_chunk_s*p_prio_chunk = nm_prio_chunk_malloc(nm_prio_chunk_allocator);
      p_prio_chunk->p_req_chunk = p_req_chunk;
      nm_prio_queue_chunk_list_push_back(&p_prio_queue->chunks, p_prio_chunk);
      /* ** store req chunk in tag table */
      struct nm_prio_tag_s*p_prio_tag = nm_prio_tag_get(&p_status->tags, p_req_chunk->p_req->tag);
      nm_prio_tag_chunk_list_push_back(&p_prio_tag->chunks, p_prio_chunk);
    }
}

static inline struct nm_prio_queue_s*strat_prio_queue_normalize(struct nm_strat_prio_s*p_status)
{
  /* ** normalize prio queue- purge empty lists */
  struct nm_prio_queue_s*p_prio_queue = nm_prio_queue_prio_list_top(&p_status->prio_queues);
  while((p_prio_queue != NULL) && nm_prio_queue_chunk_list_empty(&p_prio_queue->chunks))
    {
      nm_prio_queue_prio_list_pop(&p_status->prio_queues);
      nm_prio_queue_chunk_list_destroy(&p_prio_queue->chunks);
      puk_hashtable_remove(p_status->queues_by_prio, nm_strat_prio_hashkey(nm_prio_queue_prio_cell_get_prio(p_prio_queue)));
      nm_prio_queue_free(nm_prio_queue_allocator, p_prio_queue);
      p_prio_queue = nm_prio_queue_prio_list_top(&p_status->prio_queues);
    }
  return p_prio_queue;
}

/** dequeue a prio chunk from all queues */
static inline void strat_prio_chunk_dequeue(struct nm_strat_prio_s*p_status,
					    struct nm_prio_chunk_s*p_prio_chunk,
					    struct nm_prio_queue_s*p_prio_queue,
					    struct nm_prio_tag_s*p_prio_tag)
{
  nm_prio_queue_chunk_list_remove(&p_prio_queue->chunks, p_prio_chunk);
  nm_prio_tag_chunk_list_pop_front(&p_prio_tag->chunks);
  nm_prio_chunk_free(nm_prio_chunk_allocator, p_prio_chunk);
  if(nm_prio_tag_chunk_list_empty(&p_prio_tag->chunks))
    {
      /* garbage-collect empty tags */
      nm_prio_tag_delete(&p_status->tags, p_prio_tag);
    }
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

  strat_prio_flush_reqs(p_status, p_gate);
  
  struct nm_prio_queue_s*p_prio_queue = strat_prio_queue_normalize(p_status);
  
  /* ** issue a new packet */
  if((p_trk_small->p_pw_send == NULL) &&
     ( (p_prio_queue != NULL) ||
       (!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list)) ))
    {
      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header(p_core, p_trk_small);
      /* ** pack control */
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
      /* ** pack data */
      while(p_prio_queue != NULL)
	{
	  struct nm_prio_chunk_s*p_prio_chunk = nm_prio_queue_chunk_list_begin(&p_prio_queue->chunks);
	  struct nm_req_chunk_s*p_req_chunk = p_prio_chunk->p_req_chunk;
	  /* take the head of the lists of chunks in the tag of the highest priority chunk */
	  const nm_core_tag_t tag = p_req_chunk->p_req->tag;
	  struct nm_prio_tag_s*p_prio_tag = nm_prio_tag_get(&p_status->tags, tag);
	  struct nm_prio_chunk_s*p_prio_tag_chunk = nm_prio_tag_chunk_list_begin(&p_prio_tag->chunks);
	  if(p_prio_chunk != p_prio_tag_chunk)
	    {
	      /* there is an older req_chunk on the same tag as the max prio */
	      if(nm_prio_queue_prio_cell_get_prio(p_prio_queue) !=
		 p_prio_tag_chunk->p_req_chunk->p_req->pack.priority)
		{
		  /* different priority- find the right queue */
		  p_prio_queue = puk_hashtable_lookup(p_status->queues_by_prio, nm_strat_prio_hashkey(p_prio_tag_chunk->p_req_chunk->p_req->pack.priority));
		}
	      p_prio_chunk = p_prio_tag_chunk;
	      p_req_chunk = p_prio_tag_chunk->p_req_chunk;
	    }
	  
	  if(nm_tactic_req_is_short(p_req_chunk))
	    {
	      /* ** short send */
	      if(nm_tactic_req_short_size(p_req_chunk) <= nm_pw_remaining_buf(p_pw))
		{
		  strat_prio_chunk_dequeue(p_status, p_prio_chunk, p_prio_queue, p_prio_tag);
		  nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_CHUNK_FLAG_SHORT);
		}
	      else
		{
		  goto post_send;
		}
	    }
	  else if(nm_tactic_req_data_max_size(p_req_chunk) < p_pw->max_len)
	    {
	      /* ** small data- post on trk #0 */
	      if(nm_tactic_req_data_max_size(p_req_chunk) < nm_pw_remaining_buf(p_pw))
		{
		  strat_prio_chunk_dequeue(p_status, p_prio_chunk, p_prio_queue, p_prio_tag);
		  nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_CHUNK_FLAG_NONE);
		}
	      else
		{
		  goto post_send;
		}
	    }
	  else
	    {
	      /* ** large data- post RDV */
	      int rc = nm_tactic_pack_rdv(p_gate, p_drv, NULL, p_req_chunk, p_pw);
	      if(rc == NM_ESUCCESS)
		{
		  strat_prio_chunk_dequeue(p_status, p_prio_chunk, p_prio_queue, p_prio_tag);
		}
	      else
		{
		  goto post_send;
		}
	    }
	  assert(p_pw->length <= p_pw->max_len);
	  p_prio_queue = strat_prio_queue_normalize(p_status);
	}
    post_send:
      assert(p_pw->length <= p_pw->max_len);
      assert(p_pw->length > sizeof(struct nm_header_global_s));
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
