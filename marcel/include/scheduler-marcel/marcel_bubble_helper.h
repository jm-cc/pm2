/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#section marcel_functions
#depend "scheduler/marcel_holder.h[types]"
/* Recursively computes the considered entity's load. 
   A previous call to ma_bubble_synthesize_stats() is needed. */
long ma_entity_load(marcel_entity_t *);

/* Recursively gathers the load of entities scheduled from the
   _father_ level's children. */
unsigned int ma_load_from_children (struct marcel_topo_level *father);

/* Returns the runqueue the entity is scheduled on. If the entity is
   scheduled inside a bubble, the function returns the runqueue the
   holding bubble is scheduled on. */
ma_runqueue_t *ma_get_parent_rq (marcel_entity_t *e);

/* Returns the last vp the entity was running on. Returns
   MA_VPSTATS_NO_LAST_VP if the entity has never been executed, and
   MA_VPSTATS_CONFLICT if the entity is a bubble containing
   conflicting last_vp statistics.
   A previous call to ma_bubble_synthesize_stats() is needed. */
long ma_favourite_vp (marcel_entity_t *e);

/* Returns the last time (cpu cycles) the entity was running. If the
   entity is a bubble, it returns the last time one of its contained
   entity was running. */
long ma_last_ran (marcel_entity_t *e);

/* Returns true if the considered entity is en seed or a bubble that only contains seeds */
unsigned ma_is_a_seed(marcel_entity_t *);

/* Returns true if the considered entity is currently running (i.e.,
   if e is a thread, e is running, if e is a bubble, e contains a
   running thread.) */
unsigned ma_entity_is_running (marcel_entity_t *e);

#ifdef MA__NUMA_MEMORY
/* Compares memory attraction only */
int decreasing_order_entity_attraction_compar(const void *_e1, const void *_e2);
int increasing_order_entity_attraction_compar(const void *_e1, const void *_e2);

/* Compares load attribute and memory attraction */
int decreasing_order_entity_both_compar(const void *_e1, const void *_e2);
int increasing_order_entity_both_compar(const void *_e1, const void *_e2);
#endif /* MA__NUMA_MEMORY */

/* Compares load attribute only */
int ma_decreasing_order_entity_load_compar(const void *_e1, const void *_e2);
int ma_increasing_order_entity_load_compar(const void *_e1, const void *_e2);

/* Compares the number of threads*/
int ma_decreasing_order_threads_compar(const void *_e1, const void *_e2);
int ma_increasing_order_threads_compar(const void *_e1, const void *_e2);

/* Handling threads on a runqueue */
/* Returns the number of entities directly included in _rq_. */
unsigned int ma_count_entities_on_rq (ma_runqueue_t *rq);
/* Returns the load recursively computed from the entities scheduled
   on _rq_. */
unsigned int ma_load_on_rq (ma_runqueue_t *rq);
/* \e rq must be already locked because else entities may die under your hand.  */
int ma_get_entities_from_rq(ma_runqueue_t *rq, marcel_entity_t *e[], int ne);
int ma_gather_all_bubbles_on_rq(ma_runqueue_t *rq);
void ma_resched_existing_threads(struct marcel_topo_level *l);
int ma_count_threads_in_entity(marcel_entity_t *entity);

/* Burst bubble _bubble_ (i.e. extract its content) if _bubble_ is on
   a runqueue. */
int ma_burst_bubble (marcel_bubble_t *bubble);

/* This function steals _entity_to_steal_ and moves it to
   _starving_level_, while moving up "everything that needs to be" (c), 
   thus avoiding hierachical locking issues. */
int ma_bsched_steal (marcel_entity_t *entity_to_steal,  
		     struct marcel_topo_level *starving_level);

/* Debug function that prints information about the _ne_ entities
   stored in _e_ */
void ma_debug_show_entities(const char *func_name, marcel_entity_t *e[], int ne);

#section marcel_macros

/* Iterates over every entity directly scheduled in bubble b */
#define for_each_entity_scheduled_in_bubble_begin(e,b) \
	list_for_each_entry(e, &(b)->natural_entities, bubble_entity_list) {	\
      if(e->sched_holder)\
        if (e->sched_holder->type == MA_BUBBLE_HOLDER) { \
           /* scheduling holder of e is a bubble, that must be a ancestry of b */

#define for_each_entity_scheduled_in_bubble_end() \
      } \
   }

/* Iterates over all the entities contained in bubble b */
#define for_each_entity_held_in_bubble(e,b) \
  list_for_each_entry(e, &(b)->natural_entities, bubble_entity_list)

/* Iterates over every entity scheduled on the runqueue r */
#define for_each_entity_scheduled_on_runqueue(e,r) \
  list_for_each_entry(e, &(r)->as_holder.sched_list, sched_list)
