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

#include <marcel.h>

#ifdef MA__BUBBLES
#ifdef MARCEL_MAMI_ENABLED

#define MA_MEMORY_BSCHED_USE_WORK_STEALING 0
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

/* This function returns the "favorite location" for the considered
   entity.

   For now, we assume that the favorite location of:
     - a thread is the topology level corresponding to the numa node
       where the thread allocated the greatest amount of memory.

     - a bubble is the topology level corresponding to the common
       ancestor of all the favorite locations of the contained entities.
*/
static marcel_topo_level_t *
ma_memory_favorite_level (marcel_entity_t *e) {
  unsigned int node, best_node = 0;
  int first_node = -1, last_node = 0;
  marcel_topo_level_t *favorite_level = NULL;
  long *mem_stats = (e->type == MA_BUBBLE_ENTITY) ?
    (long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_memnode_offset)
    : (long *) ma_stats_get (e, ma_stats_memnode_offset);

  for (node = 0; node < marcel_nbnodes; node++) {
    best_node = (mem_stats[node] > mem_stats[best_node]) ? node : best_node;
    first_node = (first_node == -1 && mem_stats[node]) ? node : first_node;
    last_node = mem_stats[node] ? node : last_node;
  }

  if (!mem_stats[best_node])
    return NULL;

  switch (e->type) {

  case MA_THREAD_ENTITY:
    favorite_level = &marcel_topo_node_level[best_node];
    break;

  case MA_BUBBLE_ENTITY:
    favorite_level = ma_topo_common_ancestor (&marcel_topo_node_level[first_node],
					     &marcel_topo_node_level[last_node]);
    break;

  default:
    MA_BUG ();
  }

  return favorite_level;
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

  /* We _strictly for now_ move each entity to their favorite
     location. */
  /* TODO: This offers an ideal distribution in terms of thread/memory
     location, but this could completely unbalance the
     architecture. We should definitely have some work stealing
     primitives to fill the blanks, by moving threads that have not
     been executed yet for example, and so benefit from the first
     touch allocation policy. */
  for (i = 0; i < ne; i++) {
    struct marcel_topo_level *favorite_location = ma_memory_favorite_level (e[i]);
    if (favorite_location) {
      if (favorite_location != from) {
	/* The e[i] entity hasn't reached its favorite location yet,
	   let's put it there. */
	ma_move_entity (e[i], &favorite_location->rq.as_holder);
      } else {
	/* We're already on the right location. */
	if (favorite_location->type != MARCEL_LEVEL_NODE) {
	  if (e[i]->type == MA_BUBBLE_ENTITY) {
	    ma_burst_bubble (ma_bubble_entity (e[i]));
	    return ma_memory_schedule_from (from);
	  }
	}
      }
    }
    /* If the considered entity has no favorite location, just leave
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
    struct marcel_topo_level *favorite_location = ma_memory_favorite_level (e);

    if (parent_level && favorite_location) {
      if (ma_topo_common_ancestor (parent_level, favorite_location) != favorite_location) {
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

#if MA_MEMORY_BSCHED_USE_WORK_STEALING
struct memory_goodness_hints {

  /* The topo_level we're trying to steal for. */
  struct marcel_topo_level *from;

  /* The number of the node that holds _from_. */
  int from_node;

  /* The best entity we have found yet. */
  marcel_entity_t *best_entity;

  /* The _best_entity_'s score. */
  int best_score;

  /* Let the scheduler know we're calling goodness () for the
     first time. */
  int first_call;
};

/* TODO: Tune them! */
#define MA_MEMORY_SCHED_MAX_SCORE 500

#define MA_MEMORY_SCHED_REMOTENESS_PENALTY -10

#define MA_MEMORY_SCHED_LOAD_BONUS 10
#define MA_MEMORY_SCHED_LAST_VP_BONUS 10
#define MA_MEMORY_SCHED_LOCAL_MEM_BONUS 10

static int
memory_sched_compute_entity_score (marcel_entity_t *current_e,
				   struct memory_goodness_hints *hints) {
  int computed_score;
  int topo_max_depth = marcel_topo_vp_level[0].level;
  long current_e_last_vp;
  long *mem_stats;
  struct marcel_topo_level *current_level = ma_get_parent_rq (current_e)->topolevel;

  /* Initialize the current score. */
  computed_score = MA_MEMORY_SCHED_MAX_SCORE;

  /* Apply a penalty based on how far from _hints->from_ is located
     the entity we consider. */
  computed_score += (topo_max_depth - ma_topo_common_ancestor (hints->from, current_level)->level) * MA_MEMORY_SCHED_REMOTENESS_PENALTY;

  /* Apply a bonus according to how much threads the _current_e_
     entity is holding. */
  computed_score += ma_entity_load (current_e) * MA_MEMORY_SCHED_LOAD_BONUS;

  /* Apply a bonus if _current_e_ was last executed from the
     _hints_from_ topo_level. */
  current_e_last_vp = *(long *) ma_stats_get (current_e, ma_stats_last_vp_offset);
  computed_score += (&marcel_topo_vp_level[current_e_last_vp] == hints->from) ? MA_MEMORY_SCHED_LAST_VP_BONUS : 0;

  /* Apply a bonus if _current_e_ accesses data allocated on the node
     that holds _hints->from_. */
  mem_stats = (current_e->type == MA_BUBBLE_ENTITY) ?
    (long *) ma_bubble_hold_stats_get (ma_bubble_entity (current_e), ma_stats_memnode_offset)
    : (long *) ma_stats_get (current_e, ma_stats_memnode_offset);
  computed_score += mem_stats[hints->from_node] * MA_MEMORY_SCHED_LOCAL_MEM_BONUS;

  if (hints->first_call) {
    hints->first_call = 0;
    hints->best_score = computed_score;
    hints->best_entity = current_e;
  }

  if (hints->best_score < computed_score) {
    hints->best_score = computed_score;
    hints->best_entity = current_e;
  }

  return computed_score;
}

static void
say_hello (marcel_entity_t *e, int current_score) {
  marcel_printf ("Looking at entity %s%s (%p), score = %i\n",
		 (e->type == MA_BUBBLE_ENTITY) ? "bubble" : "thread ",
		 (e->type == MA_BUBBLE_ENTITY) ? "" : ma_task_entity (e)->name,
		 e,
		 current_score);
}

static int
goodness (ma_holder_t *hold, void *args) {
  struct memory_goodness_hints *hints = (struct memory_goodness_hints *)args;
  marcel_entity_t *e;
  int current_score;

  list_for_each_entry (e, &hold->sched_list, sched_list) {
    if (hold->nr_ready > 1)
      current_score = memory_sched_compute_entity_score (e, hints);
    else
      current_score = -1;
    say_hello (e, current_score);
    if (e->type == MA_BUBBLE_ENTITY)
      goodness (&ma_bubble_entity (e)->as_holder, hints);
  }

  return 0;
}

static int
memory_sched_steal (unsigned from_vp) {
  int ret = 0;
  struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp];

  struct memory_goodness_hints hints = {
    .from = me,
    .best_entity = NULL,
    .first_call = 1,
  };

  ma_topo_level_browse (me,
			MARCEL_LEVEL_MACHINE,
			goodness,
			&hints);

  marcel_printf ("[Work Stealing] Best entity = %p (score = %i)\n", hints.best_entity, hints.best_score);

  if (hints.best_entity)
    /* Steal the entity for real! */
    ma_bsched_steal (hints.best_entity, me);

  return ret;
}
#endif /* MA_MEMORY_BSCHED_USE_WORK_STEALING */

MARCEL_DEFINE_BUBBLE_SCHEDULER (memory,
  .start = memory_sched_start,
  .submit = memory_sched_submit,
  .shake = memory_sched_shake,
#if MA_MEMORY_BSCHED_USE_WORK_STEALING
  .vp_is_idle = memory_sched_steal,
#endif
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
