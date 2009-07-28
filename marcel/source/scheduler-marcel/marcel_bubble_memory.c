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
#ifdef MM_MAMI_ENABLED

#include "mm_mami.h"

/** \brief A cache bubble scheduler (inherits from
 * `marcel_bubble_sched_t').  */
struct marcel_bubble_memory_sched
{
  marcel_bubble_sched_t scheduler;

  /** \brief Whether work stealing is enabled.  */
  tbx_bool_t work_stealing;

  /** \brief The associated memory manager.  It is used to determine the
      amount of memory associated with a thread on a given node, which then
      directs thread/memory migration decisions.  */
  mami_manager_t *memory_manager;

  /** \brief An associated `cache' scheduler that will be invoked to schedule
      entities within each NUMA node.  */
  marcel_bubble_sched_t *cache_scheduler;
};


#define MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS 0


#if MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS
static void
ma_memory_print_affinities (marcel_bubble_t *bubble) {
  unsigned int i;
  marcel_entity_t *e;

  marcel_fprintf (stderr, "Printing memory affinity hints for bubble %p:\n", bubble);

  for_each_entity_scheduled_in_bubble_begin (e, bubble);

  marcel_fprintf (stderr, "Entity %p (%s): [ ", e, e->type == MA_BUBBLE_ENTITY ? "bubble" : ma_task_entity (e)->name);
  long *nodes = ma_cumulative_stats_get (e, ma_stats_memnode_offset);
  for (i = 0; i < marcel_nbnodes; i++) {
    marcel_fprintf (stderr, "%8ld%s", nodes[i], (i == (marcel_nbnodes - 1)) ? " ]\n" : ", ");
  }

  for_each_entity_scheduled_in_bubble_end ();
}

static void
ma_memory_print_previous_location (marcel_bubble_t *bubble) {
  marcel_entity_t *e;

  marcel_fprintf (stderr, "Printing previous location of threads scheduling in bubble %p:\n", bubble);

  for_each_entity_scheduled_in_bubble_begin (e, bubble);

  marcel_fprintf (stderr, "Entity %p (%s): ", e, e->type == MA_BUBBLE_ENTITY ? "bubble" : ma_task_entity (e)->name);
  long vp = *(long *) ma_cumulative_stats_get (e, ma_stats_last_vp_offset);
  marcel_fprintf (stderr, "%ld\n", vp);

  for_each_entity_scheduled_in_bubble_end ();
}
#endif /* MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS */

void
marcel_bubble_set_memory_manager (marcel_bubble_memory_sched_t *scheduler,
				  mami_manager_t *memory_manager) {
  scheduler->memory_manager = memory_manager;
}

static int
memory_sched_start (marcel_bubble_sched_t *self) {
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
  long *mem_stats = ma_cumulative_stats_get (e, ma_stats_memnode_offset);

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

/* Greedily distributes entities stored in _load_balancing_entities_
   over the _arity_ levels described in _distribution_.*/
static void
ma_memory_spread_load_balancing_entities (ma_distribution_t *distribution,
					  ma_distribution_t *load_balancing_entities,
					  unsigned int arity) {
  while (load_balancing_entities->nb_entities) {
    unsigned int target = ma_distribution_least_loaded_index (distribution, arity);
    ma_distribution_add_tail (ma_distribution_remove_tail (load_balancing_entities), &distribution[target]);
  }
}

/* Compares how much data entities e1 and e2 have subscribed to. */
static int
ma_memory_mem_load_compar (const void *e1, const void *e2) {
  marcel_entity_t **ent1 = (marcel_entity_t **) e1;
  marcel_entity_t **ent2 = (marcel_entity_t **) e2;

  long *mem_stats1 = ma_cumulative_stats_get (*ent1, ma_stats_memnode_offset);
  long *mem_stats2 = ma_cumulative_stats_get (*ent2, ma_stats_memnode_offset);

  unsigned node, best_node = 0;
  for (node = 0; node < marcel_nbnodes; node++) {
    best_node = (mem_stats1[node] > mem_stats1[best_node]) ? node : best_node;
  }

  return (mem_stats1[best_node] <= mem_stats2[best_node]) - (mem_stats1[best_node] >= mem_stats2[best_node]);
}

/* Pick bubbles in _e_ and burst them until the number of entities
   exceeds arity. Sort the _e_ array in order to burst bubbles with
   the fewest allocated data first. */
static int
ma_memory_burst_light_bubbles (marcel_entity_t **e, int ne, int arity) {
  int i, new_ne = ne;
  qsort (e, ne, sizeof (e[0]), ma_memory_mem_load_compar);

  for (i = 0; i < new_ne && new_ne < arity; i++) {
    if (e[i]->type == MA_BUBBLE_ENTITY)
      new_ne += ma_burst_bubble (ma_bubble_entity (e[i])) - 1;
  }

  return new_ne;
}

/* Even the load by taking entities from the most loaded levels and
   put them on the least loaded levels, until each level holds more than
   _load_per_level_ entities. */
static int
ma_memory_global_balance (ma_distribution_t *distribution, unsigned int arity, unsigned int load_per_level) {
  unsigned i;
  int least_loaded = ma_distribution_least_loaded_index (distribution, arity);
  int most_loaded = ma_distribution_most_loaded_index (distribution, arity);

  /* Sort the entities so as to pick the one with the fewest allocated
     data first. */
  for (i = 0; i < arity; i++)
    qsort (distribution[i].entities,
	   distribution[i].nb_entities,
	   sizeof (distribution[i].entities[0]),
	   ma_memory_mem_load_compar);

  while ((distribution[least_loaded].total_load + 1 < distribution[most_loaded].total_load)
	 && (distribution[least_loaded].total_load < load_per_level)) {
    if (distribution[most_loaded].nb_entities > 1) {
      ma_distribution_add_tail (ma_distribution_remove_tail (&distribution[most_loaded]),
				&distribution[least_loaded]);
      least_loaded = ma_distribution_least_loaded_index (distribution, arity);
      most_loaded = ma_distribution_most_loaded_index (distribution, arity);
    } else {
      return 1;
    }
  }
  return 0;
}

static int
ma_memory_apply_memory_distribution (mami_manager_t *memory_manager,
				     ma_distribution_t *distribution,
				     unsigned int arity) {
  unsigned int i, j;

  if (!memory_manager) {
    bubble_sched_debug("`memory' scheduler: lacking a memory manager, nothing done\n");
    return 1;
  }

  for (i = 0; i < arity; i++) {
    for (j = 0; j < distribution[i].nb_entities; j++) {

      switch (distribution[i].entities[j]->type) {

      case MA_BUBBLE_ENTITY:
	mami_bubble_migrate_all (memory_manager, ma_bubble_entity (distribution[i].entities[j]), i);
	break;

      case MA_THREAD_ENTITY:
	mami_task_migrate_all (memory_manager, ma_task_entity (distribution[i].entities[j]), i);
	break;

      default:
	MA_BUG ();
	break;
      }
    }
  }

  return 0;
}

/* Distribute the entities included in _e_ over the children of
   topo_level _from_, considering this hints already set in
   _distribution_. */
static int
ma_memory_distribute (marcel_topo_level_t *from,
		      marcel_entity_t *e[],
		      int ne,
		      ma_distribution_t *distribution) {
  unsigned int i, child, arity = from->arity, entities_per_level = marcel_vpset_weight(&from->vpset) / arity;
  ma_distribution_t load_balancing_entities;
  ma_distribution_init (&load_balancing_entities, (marcel_topo_level_t *)NULL, 0, ne);

  /* Perform a first distribution according to strict memory
     affinity. */
  for (i = 0; i < ne; i++) {
    marcel_topo_level_t *favorite_level = ma_memory_favorite_level (e[i]);
    if (favorite_level) {
      for (child = 0; child < from->arity; child++) {
	if (ma_topo_is_in_subtree (from->children[child], favorite_level))
	  ma_distribution_add_tail (e[i], &distribution[child]);
      }
    } else {
      ma_distribution_add_tail (e[i], &load_balancing_entities);
    }
  }

  /* Even the load by distributing the "load_balancing_entities". */
  ma_memory_spread_load_balancing_entities (distribution, &load_balancing_entities, arity);

  /* Rearrange the distribution until occupying every underlying
     core. */
  ma_memory_global_balance (distribution, arity, entities_per_level);

  /* Physically apply the logical distribution we've just computed. */
  ma_apply_distribution (distribution, arity);

  ma_distribution_destroy (&load_balancing_entities, arity);

  return 0;
}

static int
ma_memory_schedule_from (marcel_bubble_memory_sched_t *scheduler,
			 struct marcel_topo_level *from) {
  unsigned int i, ne, arity = from->arity;

  ne = ma_count_entities_on_rq (&from->rq);

  /* If nothing was found on the current runqueue, let's browse its
     children. */
  if (!ne) {
    for (i = 0; i < arity; i++)
      ma_memory_schedule_from (scheduler, from->children[i]);
    return 0;
  }

  marcel_entity_t *e[ne];
  ma_get_entities_from_rq (&from->rq, e, ne);

  if (ne < arity) {
    /* We need at least one entity per children level. */
    ma_memory_burst_light_bubbles (e, ne, arity);
    return ma_memory_schedule_from (scheduler, from);
  }

  ma_distribution_t distribution[arity];
  ma_distribution_init (distribution, from, arity, ne);

  tbx_bool_t try_again = tbx_false;
  tbx_bool_t node_level_reached = (distribution[0].level->type == MARCEL_LEVEL_NODE) ? tbx_true : tbx_false;

  for (i = 0; i < ne; i++) {
   struct marcel_topo_level *favorite_location = ma_memory_favorite_level (e[i]);
    if (favorite_location && favorite_location == from) {
      /* We're already on the right location, which type is different
	 from MARCEL_LEVEL_NODE. Burst the bubble there to take into
	 account the contained entities' favorite locations. */
      if (e[i]->type == MA_BUBBLE_ENTITY) {
	ma_burst_bubble (ma_bubble_entity (e[i]));
	try_again = tbx_true;
      }
    }
  }

  if (try_again)
    return ma_memory_schedule_from (scheduler, from);

  /* Perform the threads and bubbles distribution. */
  ma_memory_distribute (from, e, ne, distribution);

 if (node_level_reached)
   /* We've just distributed entities over the node-level runqueues,
      we can now migrate memory areas to their new location, described
      in _distribution_.*/
   ma_memory_apply_memory_distribution (scheduler->memory_manager,
					distribution, arity);

  ma_distribution_destroy (distribution, arity);

  /* Recurse over underlying levels */
  for (i = 0; i < arity && !node_level_reached; i++)
    ma_memory_schedule_from (scheduler, from->children[i]);

  return 0;
}

static int
ma_memory_sched_submit (struct marcel_bubble_memory_sched *self,
			marcel_bubble_t *bubble,
			struct marcel_topo_level *from) {
  bubble_sched_debug("marcel_root_bubble: %p \n", &marcel_root_bubble);

  ma_bubble_synthesize_stats (bubble);

#if MA_MEMORY_BSCHED_NEEDS_DEBUGGING_FUNCTIONS
  ma_memory_print_previous_location (bubble);
  ma_memory_print_affinities (bubble);
#endif

  ma_bubble_lock_all (bubble, from);

  /* Only call the Memory scheduler on a node-based computer. */
  if (ma_get_topo_type_depth (MARCEL_LEVEL_NODE) >= 0)
    ma_memory_schedule_from (self, from);

  ma_bubble_unlock_all (bubble, from);

  /* Let the cache scheduler handle the rest of the topology (starting at
     FROM).  XXX: There's a small window during which the bubble hierarchy
     in unlocked.  */
  self->cache_scheduler->submit (self->cache_scheduler,
				 ma_entity_bubble (bubble));

  return 0;
}

static int
memory_sched_submit (marcel_bubble_sched_t *self, marcel_entity_t *e) {
  MA_BUG_ON (e->type != MA_BUBBLE_ENTITY);
  bubble_sched_debug ("Memory: Submitting entities!\n");
  return ma_memory_sched_submit ((struct marcel_bubble_memory_sched *) self,
				 ma_bubble_entity (e), marcel_topo_level (0, 0));
}

static int
memory_sched_shake (marcel_bubble_sched_t *self) {
  int ret = 0, shake = 0;
  marcel_bubble_t *holding_bubble = ma_bubble_holder (ma_entity_task (marcel_self ())->natural_holder);
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


/* Work stealing.  */

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
  mem_stats = ma_cumulative_stats_get (current_e, ma_stats_memnode_offset);
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
		 (e->type == MA_BUBBLE_ENTITY) ? "" : e->name,
		 e,
		 current_score);
}

static int
goodness (ma_holder_t *hold, void *args) {
  struct memory_goodness_hints *hints = (struct memory_goodness_hints *)args;
  marcel_entity_t *e;
  int current_score;

  tbx_fast_list_for_each_entry (e, &hold->ready_entities, ready_entities_item) {
    if (hold->nb_ready_entities > 1)
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
memory_sched_steal (marcel_bubble_sched_t *scheduler, unsigned from_vp) {
  int ret = 0;
  marcel_bubble_memory_sched_t *self;
  struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp];

  struct memory_goodness_hints hints = {
    .from = me,
    .best_entity = NULL,
    .first_call = 1,
  };

  self = (marcel_bubble_memory_sched_t *) scheduler;
  if (!self->work_stealing)
    /* Work stealing is disabled.  */
    return 0;

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

static int
memory_sched_exit (marcel_bubble_sched_t *scheduler) {
  marcel_bubble_memory_sched_t *self;

  self = (marcel_bubble_memory_sched_t *) scheduler;

  if (self->cache_scheduler->exit)
    self->cache_scheduler->exit (self->cache_scheduler);
  marcel_free (self->cache_scheduler);

  scheduler->klass = NULL;

  return 0;
}

int
marcel_bubble_memory_sched_init (marcel_bubble_memory_sched_t *scheduler,
				 mami_manager_t *mami_manager,
				 tbx_bool_t work_stealing) {
  int err;
  marcel_bubble_cache_sched_t *cache_sched;

  memset (scheduler, 0, sizeof (*scheduler));

  cache_sched =
    marcel_malloc (marcel_bubble_sched_instance_size (&marcel_bubble_cache_sched_class),
		   __FILE__, __LINE__);
  if (cache_sched)
    {
      /* Create a `cache' scheduler that will be invoked when `memory' can't
	 do a better job, e.g., within NUMA nodes.  */
      err = marcel_bubble_cache_sched_init (cache_sched, tbx_false);
      if (err)
	marcel_free (cache_sched);
      else
	{
	  scheduler->cache_scheduler = (marcel_bubble_sched_t *) cache_sched;
	  scheduler->work_stealing = work_stealing;
	  scheduler->memory_manager = mami_manager;

	  scheduler->scheduler.klass = &marcel_bubble_memory_sched_class;

	  scheduler->scheduler.start = memory_sched_start;
	  scheduler->scheduler.exit = memory_sched_exit;
	  scheduler->scheduler.submit = memory_sched_submit;
	  scheduler->scheduler.shake = memory_sched_shake;
	  scheduler->scheduler.vp_is_idle = memory_sched_steal;
	}
    }
  else
    err = 1;

  return err;
}

/* Initialize SCHEDULER as a `memory' scheduler with default parameter
   values.  */
static int
make_default_scheduler (marcel_bubble_sched_t *scheduler) {
  /* Note: We can't do much without a memory manager.  */
  return marcel_bubble_memory_sched_init ((marcel_bubble_memory_sched_t *) scheduler,
					  NULL, tbx_false);
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS (memory, make_default_scheduler);

#else /* MM_MAMI_ENABLED */

struct marcel_bubble_memory_sched
{
  marcel_bubble_sched_t scheduler;
};

static int
warning_start (marcel_bubble_sched_t *self) {
  marcel_printf ("WARNING: You're currently trying to use the memory bubble scheduler, but as the enable_mami option is not activated in the current flavor, the bubble scheduler won't do anything! Please activate enable_mami in your flavor.\n");
  return 0;
}

static int
make_default_scheduler (marcel_bubble_sched_t *scheduler) {
  scheduler->klass = &marcel_bubble_memory_sched_class;
  scheduler->start = warning_start;
  scheduler->submit = (void*)warning_start;
  scheduler->shake = (void*)warning_start;
  return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS (memory, make_default_scheduler);

#endif /* MM_MAMI_ENABLED */
#endif /* MA__BUBBLES */

/*
   Local Variables:
   tab-width: 8
   End:
 */
