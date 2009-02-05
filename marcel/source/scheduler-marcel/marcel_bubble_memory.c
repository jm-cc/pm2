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

#include "marcel.h"

#ifdef MA__BUBBLES
#ifdef MARCEL_MAMI_ENABLED

#define MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS 0

#if MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS
static void
ma_memory_print_affinities (marcel_bubble_t *bubble) {
  unsigned int i;
  marcel_entity_t *e;
  
  marcel_fprintf (stderr, "Printing memory affinity hints for bubble %p:\n", bubble);
  for_each_entity_scheduled_in_bubble_begin (e, bubble);
  if (e->type != MA_BUBBLE_ENTITY) {
    marcel_fprintf (stderr, "Entity %p (%s): [ ", e, ma_task_entity (e)->name);
    long * nodes = (long *) ma_stats_get (e, ma_stats_memnode_offset);
    for (i = 0; i < marcel_nbnodes; i++) {
      marcel_fprintf (stderr, "%8ld%s", nodes[i], (i == (marcel_nbnodes - 1)) ? " ]\n" : ", ");
    }
  } else {
    marcel_fprintf (stderr, "Entity %p (%s): [ ", e, "bubble");
    long * nodes = (long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_memnode_offset);
    for (i = 0; i < marcel_nbnodes; i++) {
      marcel_fprintf (stderr, "%8ld%s", nodes[i], (i == (marcel_nbnodes - 1)) ? " ]\n" : ", ");
    }
  }
  for_each_entity_scheduled_in_bubble_end ();
}

static void
ma_memory_print_previous_location (marcel_bubble_t *bubble) {
  marcel_entity_t *e;
  
  marcel_fprintf (stderr, "Printing previous location of threads scheduling in bubble %p:\n", bubble);
  for_each_entity_scheduled_in_bubble_begin (e, bubble);
  if (e->type != MA_BUBBLE_ENTITY) {
    marcel_fprintf (stderr, "Entity %p (%s): ", e, ma_task_entity (e)->name);
    long vp = *(long *) ma_stats_get (e, ma_stats_last_vp_offset);
    marcel_fprintf (stderr, "%ld\n", vp);
  } else {
    marcel_fprintf (stderr, "Entity %p (%s): [ ", e, "bubble");
    long vp = *(long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_last_vp_offset);
    marcel_fprintf (stderr, "%ld\n", vp);
  }
  for_each_entity_scheduled_in_bubble_end ();
}
#endif /* MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS */

static int
memory_sched_start () {
  return 0;
}

/* This function returns the "favourite location" for the considered
   entity. 

   For now, we assume that the favourite location of:
     - a thread is the topology level corresponding to the numa node 
       where the thread allocated the greatest amount of memory. 

     - a bubble is the topology level corresponding to the common 
       ancestor of all the favourite locations of the contained entities. 
*/
/* TODO: Once a thread is located on the right numa node, we could do
   even better by scheduling it on the last vp it was running on, if
   this vp is on the considered numa node, to benefit from cache
   memory. */
static struct marcel_topo_level * 
ma_favourite_location (marcel_entity_t *e) {
  int i, j;
  long *nodes;
  if (e->type == MA_THREAD_ENTITY) {
    long greater_amount_of_mem = 0;
    int best_node = -1;
    nodes = (long *) ma_stats_get (e, ma_stats_memnode_offset);
    for (i = 0; i < marcel_nbnodes; i++) {
      if (nodes[i] > greater_amount_of_mem) {
	greater_amount_of_mem = nodes[i];
	best_node = i;
      }
    }
    return (best_node != -1) ? &marcel_topo_node_level[best_node] : NULL;
  } else {
    /* The considered entity is a bubble, let's try to find the common
       level to all favourite locations of the contained entities. */
    MA_BUG_ON (e->type != MA_BUBBLE_ENTITY);
    struct marcel_topo_level *from = NULL, *attraction_level = NULL;
    nodes = (long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_memnode_offset);
    
    for (i = 0; i < marcel_nbnodes; i++) {
       if (nodes[i]) {
	 from = &marcel_topo_node_level[i];
	 break;
       }
     }
    if (from) {
      attraction_level = from;
      for (j = marcel_nbnodes - 1; j > i; j--) {
	if (nodes[j]) {
	  if (attraction_level != &marcel_topo_node_level[j]) {
	    attraction_level = ma_topo_lower_ancestor (attraction_level, &marcel_topo_node_level[j]);
	  }
	}
      }
    }
    return attraction_level;
  }
  return NULL;
}

static int
ma_memory_schedule_from (struct marcel_topo_level *from) {
  unsigned int i, ne, arity = from->arity;

  /* There are no other runqueues to browse underneath, distribution
     is over. */
  if (!arity)
    return 0;

  ne = ma_count_entities_on_rq (&from->rq);

  /* If nothing was found on the current runqueue, let's browse its
     children. */
  if (!ne) {
    for (i = 0; i < arity; i++)
      ma_memory_schedule_from (from->children[i]);
    return 0;
  }

  marcel_entity_t *e[ne];
  ma_get_entities_from_rq (&from->rq, e, ne);
    
  /* We _strictly for now_ move each entity to their favourite
     location. */
  /* TODO: This offers an ideal distribution in terms of thread/memory
     location, but this could completely unbalance the
     architecture. We should definitely have some work stealing
     primitives to fill the blanks, by moving threads that have not
     been executed yet for example, and so benefit from the first
     touch allocation policy. */
  for (i = 0; i < ne; i++) {
    struct marcel_topo_level *favourite_location = ma_favourite_location (e[i]);
    if (favourite_location) {
      if (favourite_location != from) {
	/* The e[i] entity hasn't reached its favourite location yet,
	   let's put it there. */
	ma_move_entity (e[i], &favourite_location->rq.as_holder);
      } else {
	/* We're already on the right location. */
	if (favourite_location->type != MARCEL_LEVEL_NODE) {
	  if (e[i]->type == MA_BUBBLE_ENTITY) {
	    ma_burst_bubble (ma_bubble_entity (e[i]));
	    return ma_memory_schedule_from (from);
	  }
	}
      }
    }
    /* If the considered entity has no favourite location, just leave
       it here to ensure load balancing. */
  }
  
  /* Recurse over underlying levels */ 
  for (i = 0; i < arity; i++)
    ma_memory_schedule_from (from->children[i]);
  
  return 0;
}

static int
ma_memory_sched_submit (marcel_bubble_t *bubble, struct marcel_topo_level *from) {
  bubble_sched_debug("marcel_root_bubble: %p \n", &marcel_root_bubble);
  
  ma_bubble_synthesize_stats (bubble);

#if MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS
  ma_memory_print_previous_location (bubble);
  ma_memory_print_affinities (bubble);
#endif 
  
  ma_bubble_lock_all (bubble, from);
  ma_memory_schedule_from (from);
  /* TODO: Crappy way to communicate with the Cache bubble
     scheduler. Do it nicely in the future. */
  ((int (*) (struct marcel_topo_level *)) marcel_bubble_cache_sched.priv) (from);
  ma_resched_existing_threads (from);
  ma_bubble_unlock_all (bubble, from);  

  return 0;
}

static int
memory_sched_submit (marcel_entity_t *e) {
  MA_BUG_ON (e->type != MA_BUBBLE_ENTITY);
  bubble_sched_debug ("Memory: Submitting entities!\n");
  return ma_memory_sched_submit (ma_bubble_entity (e), marcel_topo_level (0, 0));
}

static int
memory_sched_shake () {
  int ret = 0, shake = 0;
  marcel_bubble_t *holding_bubble = ma_bubble_holder (ma_entity_task (marcel_self ())->init_holder);
  marcel_entity_t *e;
  MA_BUG_ON (holding_bubble == NULL);

  ma_bubble_synthesize_stats (holding_bubble);

  ma_local_bh_disable ();
  ma_preempt_disable ();
 
  for_each_entity_held_in_bubble (e, holding_bubble)
  {
    struct marcel_topo_level *parent_level = ma_get_parent_rq (e)->topolevel;
    struct marcel_topo_level *favorite_location = ma_favourite_location (e);

    if (parent_level && favorite_location) {
      if (ma_topo_lower_ancestor (parent_level, favorite_location) != favorite_location) {
	shake = 1;
	break;
      }
    } 
  }
  
  ma_preempt_enable_no_resched ();
  ma_local_bh_enable ();

  if (shake) {
    ma_bubble_move_top_and_submit (&marcel_root_bubble);
    ret = 1;
  }

  return ret;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER (memory,
  .start = memory_sched_start,
  .submit = memory_sched_submit,
  .shake = memory_sched_shake,
);

#else /* MARCEL_MAMI_ENABLED */

static int
warning_start (void) {
  marcel_printf ("WARNING: You're currently trying to use the memory bubble scheduler, but as the enable_mami option is not activated in the current flavor, the bubble scheduler won't do anything! Please activate enable_mami in your flavor.\n");
  return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER (memory,
  .start = warning_start,
  .submit = (void*)warning_start,
  .shake = (void*)warning_start,
);

#endif /* MARCEL_MAMI_ENABLED */
#endif /* MA__BUBBLES */
