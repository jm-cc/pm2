/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008 "the PM2 team" (see AUTHORS file)
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
#include <math.h>
#include <string.h>
#include <time.h>
#include <alloca.h>

#ifdef MA__BUBBLES

#define MA_CACHE_BSCHED_USE_WORK_STEALING 1
#define MA_CACHE_BSCHED_NEEDS_DEBUGGING_FUNCTIONS 0

#define MA_FAILED_STEAL_COOLDOWN 1000
#define MA_SUCCEEDED_STEAL_COOLDOWN 100

static unsigned long ma_last_failed_steal = 0;
static unsigned long ma_last_succeeded_steal = 0;
static ma_atomic_t ma_succeeded_steals = MA_ATOMIC_INIT(0);
static ma_atomic_t ma_failed_steals = MA_ATOMIC_INIT(0);

/* Submits a set of entities on a marcel_topo_level */
static void
ma_cache_sched_submit (marcel_entity_t *e[], int ne, struct marcel_topo_level *l) {
  unsigned int i;
  bubble_sched_debug ("Submitting entities on runqueue %p:\n", &l->rq);
  ma_debug_show_entities ("ma_cache_sched_submit", e, ne);
  for (i = 0; i < ne; i++) {	      
    if (e[i]) {
      int state = ma_get_entity (e[i]);
      ma_put_entity (e[i], &l->rq.as_holder, state);
    }
  }
}

static int 
decreasing_int_compar (const void *_e1, const void *_e2) {
  return *(const int *)_e1 - *(const int *)_e2;
}

static unsigned int
ma_load_from_children (struct marcel_topo_level *father) {
  unsigned int arity = father->arity, ret = 0, i;
  
  if (arity) {
    for (i = 0; i < arity; i++) {
      ret += ma_load_from_children (father->children[i]);
    } 
  } 
  ret += ma_load_on_rq (&father->rq);
  return ret;
}

typedef marcel_entity_t * ma_entity_ptr_t;

/* Structure that describes entities attracted to a specific level. */
struct ma_attracting_level {
  /* Array of entities drawn to level _level_ */
  ma_entity_ptr_t *entities;
  /* Number of elements of the _entities_ array */
  unsigned int nb_entities;
  /* Maximum size of entities array */
  unsigned int max_entities;
  /* Total load of entities on the current array */
  unsigned int total_load;
  /* Attraction level for the _entities_ entities */
  struct marcel_topo_level *level;
};
typedef struct ma_attracting_level ma_attracting_level_t;

/* Initialize an array of ma_attracting_level_t describing the relations
   between the _nb_entities_ entities scheduled on level _from_ and
   the underlying runqueues (from->children). */
#define ma_attracting_levels_init(levels, from, arity, _nb_entities)	\
do									\
  {									\
    unsigned int __i;							\
									\
    for (__i = 0; __i < ((arity) ? (arity) : 1); __i++) {		\
      (levels)[__i].entities =						\
	(ma_entity_ptr_t *) alloca ((_nb_entities) *			\
				    (sizeof (*(levels)[0].entities)));	\
      (levels)[__i].nb_entities = 0;					\
      (levels)[__i].level = (from != NULL) ? (from)->children[__i] : NULL; \
      (levels)[__i].max_entities = (_nb_entities);			\
      (levels)[__i].total_load = (from != NULL)				\
	? ma_load_from_children ((from)->children[__i])			\
	: 0;								\
									\
      MA_BUG_ON (!(levels)[__i].entities);				\
    }									\
  }									\
 while (0)

/* Destroy an array of _arity_ elements of ma_attracting_level_t. */
#define ma_attracting_levels_destroy(attracting_levels, arity)	\
  do { } while (0)

/* Add the _e_ entity at the tail of the _attracting_level_->entities
   array. */
static void
ma_attracting_levels_add_tail (marcel_entity_t *e, ma_attracting_level_t *attracting_level) {
  MA_BUG_ON (attracting_level->nb_entities >= attracting_level->max_entities);
  attracting_level->entities[attracting_level->nb_entities] = e;
  attracting_level->nb_entities++;
  attracting_level->total_load += ma_entity_load (e);
}

/* Add the _e_ entity at position _j_ in the
   _attracting_level_->entities array. */
static void
ma_attracting_levels_add (marcel_entity_t *e,
		       ma_attracting_level_t *attracting_level, 
		       unsigned int position) {
  MA_BUG_ON (position >= attracting_level->nb_entities);
  MA_BUG_ON (attracting_level->nb_entities >= attracting_level->max_entities);
  if (attracting_level->entities[position]) {
    unsigned int size = sizeof (attracting_level->entities[0]);
    memmove (&attracting_level->entities[position+1],
	     &attracting_level->entities[position], 
	     (attracting_level->nb_entities - position) * size);
  }
  attracting_level->entities[position] = e;
  attracting_level->nb_entities++;
  attracting_level->total_load += ma_entity_load (e);
}

/* Remove and return the last entity from the
   _attracting_level->entities array. */
static marcel_entity_t *
ma_attracting_levels_remove_tail (ma_attracting_level_t *attracting_level) {
  MA_BUG_ON (!attracting_level->nb_entities);
  marcel_entity_t *removed_entity = attracting_level->entities[attracting_level->nb_entities - 1];
  attracting_level->nb_entities--;
  attracting_level->total_load -= ma_entity_load (removed_entity);
  
  return removed_entity;
}

/* Return the index of the least loaded children topo_level, according
   to the information gathered in _attracting_levels_. */
static unsigned int
ma_attracting_levels_least_loaded_index (ma_attracting_level_t *attracting_levels, unsigned int arity) {
  unsigned int i, res = 0;
  for (i = 1; i < arity; i++) {
    if (attracting_levels[i].total_load < attracting_levels[res].total_load)
      res = i;
  }
  
  return res;
}

/* Return the index of the most loaded children topo_level, according
   to the information gathered in _attracting_levels_. */
static unsigned int
ma_attracting_levels_most_loaded_index (ma_attracting_level_t *attracting_levels, unsigned int arity) {
  unsigned int i, res = 0;
  for (i = 1; i < arity; i++) {
    if (attracting_levels[i].total_load > attracting_levels[res].total_load)
      res = i;
  }
  
  return res;
}

#if MA_CACHE_BSCHED_NEEDS_DEBUGGING_FUNCTIONS
/* Debugging function that prints the address of every entity on each
   attracting level included in _attracting_levels_. */
static void
ma_print_attracting_levels (ma_attracting_level_t *attracting_levels, unsigned int arity) {
  unsigned int i, j;
  for (i = 0; i < arity; i++) {
    fprintf (stderr, "children[%d] = {", i);
    if (!attracting_levels[i].nb_entities)
      fprintf (stderr, " }\n");
    for (j = 0; j < attracting_levels[i].nb_entities; j++) {
      fprintf (stderr, (j != attracting_levels[i].nb_entities - 1) ? " %p, " : " %p }\n", attracting_levels[i].entities[j]);
    }
  }
}
#endif /* MA_CACHE_BSCHED_NEEDS_DEBUGGING_FUNCTIONS */

/* This function translates a vp index into a from->children
   index. (i.e., sometimes you're not distributing on vp-level
   runqueues, so you have to know which child topo_level covers the
   _favourite_vp_ vp.) */
static int
ma_translate_favorite_vp (int favorite_vp, marcel_entity_t *e, struct marcel_topo_level *from) {
  unsigned int i;
  int translated_index = MA_VPSTATS_NO_LAST_VP;
  if (favorite_vp == MA_VPSTATS_CONFLICT) {
    /* If the considered entity is a bubble that contains conflicting
       last_vp informations, we try to attract this bubble to the last
       topo_level it was scheduled on. */
    MA_BUG_ON (e->type != MA_BUBBLE_ENTITY);
    struct marcel_topo_level *last_topo_level = 
      (struct marcel_topo_level *) ma_stats_get (e, ma_stats_last_topo_level_offset);
    if (last_topo_level) {
      for (i = 0; i < from->arity; i++) {
	if (marcel_vpset_isincluded (&from->children[i]->vpset, &last_topo_level->vpset)) {
	  translated_index = i;
	  break;
	}
      }
    }
  } else {
    for (i = 0; i < from->arity; i++) {
      if (marcel_vpset_isset (&from->children[i]->vpset, (unsigned int) favorite_vp)) {
	translated_index = i;
	break;
      }
    }
  }
  return translated_index;
}

/* This function puts the _e_ entity on the right attracting level
   entities array, according to the vp (_favorite_vp_) it's attracted
   to. If _e_ is not attracted anywhere, it's moved to a specific
   attracting level (_load_balancing_entities_) that holds entities
   used for load balancing.*/
static void
ma_strict_cache_distribute (marcel_entity_t *e, 
			 int favorite_vp, 
			 ma_attracting_level_t *attracting_levels, 
			 ma_attracting_level_t *load_balancing_entities) {
  if (favorite_vp == MA_VPSTATS_NO_LAST_VP) {
   ma_attracting_levels_add_tail (e, load_balancing_entities);
  } else {
    if (!attracting_levels[favorite_vp].nb_entities) {
     ma_attracting_levels_add_tail (e, &attracting_levels[favorite_vp]);
    } else {
      unsigned int i, done = 0;
      unsigned long last_ran = ma_last_ran (e);
      for (i = 0; i < attracting_levels[favorite_vp].nb_entities; i++) {
      	if (last_ran > ma_last_ran (attracting_levels[favorite_vp].entities[i])) {
      	  ma_attracting_levels_add (e, &attracting_levels[favorite_vp], i);
      	  done = 1;
	  break;
      	}
      }
      if (!done) {
	ma_attracting_levels_add_tail (e, &attracting_levels[favorite_vp]);
      }
    }
  }
}

/* Greedily distributes entities stored in _load_balancing_entities_
   over the _arity_ levels described in _attracting_levels_.*/
static void
ma_spread_load_balancing_entities (ma_attracting_level_t *attracting_levels, 
				ma_attracting_level_t *load_balancing_entities,
				unsigned int arity) {
  while (load_balancing_entities->nb_entities) {
    unsigned int target = ma_attracting_levels_least_loaded_index (attracting_levels, arity);
    ma_attracting_levels_add_tail (ma_attracting_levels_remove_tail (load_balancing_entities), &attracting_levels[target]);
  }
}

/* Even the load by taking entities from the most loaded levels and
   put them on the least loaded levels, until each level holds more than
   _load_per_level_ entities. */
static int 
ma_global_load_balance (ma_attracting_level_t *attracting_levels, unsigned int arity, unsigned int load_per_level) {
  int least_loaded = ma_attracting_levels_least_loaded_index (attracting_levels, arity);
  int most_loaded = ma_attracting_levels_most_loaded_index (attracting_levels, arity);

  /* If the most loaded level is also the least one, it means that
     every underlying level already has the same amount of work, and we
     can't do anything else.  Furthermore, we want to avoid entering
     an endless balancing oscillation, so load balancing terminates
     when the most loaded level's load is that of the least loaded
     plus 1.  */

  while ((attracting_levels[least_loaded].total_load + 1 < attracting_levels[most_loaded].total_load)
	 && (attracting_levels[least_loaded].total_load < load_per_level)) {
    if (attracting_levels[most_loaded].nb_entities > 1) {
      ma_attracting_levels_add_tail (ma_attracting_levels_remove_tail (&attracting_levels[most_loaded]), 
				  &attracting_levels[least_loaded]);
      least_loaded = ma_attracting_levels_least_loaded_index (attracting_levels, arity);
      most_loaded = ma_attracting_levels_most_loaded_index (attracting_levels, arity);
    } else {
      return 1;
    }
  }
  return 0;
}

/* Physically moves entities according to the logical distribution
   proposed in _attracting_levels_. */
static void
ma_distribute_according_to_attracting_levels (ma_attracting_level_t *attracting_levels, unsigned int arity) {
  unsigned int i, j;
  for (i = 0; i < arity; i++) {
    for (j = 0; j < attracting_levels[i].nb_entities; j++) {
      ma_move_entity (attracting_levels[i].entities[j], &attracting_levels[i].level->rq.as_holder);
    }
  }
}

/* Distributes a set of entities regarding cache affinities */
static int
ma_cache_distribute_entities_cache (struct marcel_topo_level *l, 
				  marcel_entity_t *e[], 
				  int ne,
				  ma_attracting_level_t *attracting_levels) {
  unsigned int i, arity = l->arity, entities_per_level = marcel_vpset_weight(&l->vpset) / arity;
  ma_attracting_level_t load_balancing_entities;
  ma_attracting_levels_init (&load_balancing_entities,
			     (struct marcel_topo_level *)NULL, 0, ne);
  
  /* First, we add each entity stored in _e[]_ to the
     _attracting_levels_ structure, by putting it into the array
     corresponding to the level it's attracted to. */
  for (i = 0; i < ne; i++) {
    long last_vp = ma_favourite_vp (e[i]), last_vp_index = -1; 
    last_vp_index = ma_translate_favorite_vp (last_vp, e[i], l);
        
    ma_strict_cache_distribute (e[i], 
				last_vp_index, 
				attracting_levels, 
				&load_balancing_entities);
  }

  /* Then, we greedily distribute the unconstrained entities to balance
     the load. */
  ma_spread_load_balancing_entities (attracting_levels, 
				     &load_balancing_entities, 
				     arity);

  /* At this point, we verify if every underlying topo_level will be
     occupied. If not, we even the load by taking entities from the
     most loaded levels to the least loaded ones. */
  ma_global_load_balance (attracting_levels, arity, entities_per_level);
  
  /* We now have a satisfying logical distribution, let's physically
     move entities according it. */
  ma_distribute_according_to_attracting_levels (attracting_levels, arity);
  
  ma_attracting_levels_destroy (&load_balancing_entities, 1);

  return 0; 
}

/* Checks wether enough entities are already positionned on
   the considered runqueues */
static int
ma_cache_has_enough_entities (struct marcel_topo_level *l, 
			    marcel_entity_t *e[], 
			    unsigned int ne, 
			    const ma_attracting_level_t *attracting_levels) {
  unsigned int i, ret = 1, prev_state = 1, nvp = marcel_vpset_weight (&l->vpset);
  unsigned int arity = l->arity, per_item_entities = nvp / arity, entities_per_level[arity];

  /* First, check if enough entities are already scheduled over the
     underlying levels. */
  for (i = 0; i < arity; i++) {
    if (attracting_levels[i].total_load < per_item_entities) {
      prev_state = 0;
    }
  }
  
  if (prev_state) {
    return prev_state;
  }  

  /* If not, initialize the entities_per_level array with the load of
     all entities scheduled in each topology subtree. */
  for (i = 0; i < arity; i++) {
    entities_per_level[i] = attracting_levels[i].total_load;
  }

  /* Make sure that entities_per_level[0] always points to the
     least-loaded level. */
  qsort (entities_per_level, arity, sizeof(int), &decreasing_int_compar);
  
  for (i = 0; i < ne; i++) {
    unsigned int k, tmp;
    /* Put the bigger entity on the least-loaded level. 
     * We assume the _e_ array is sorted. */
    entities_per_level[0] += ma_entity_load (e[i]); 

    /* Rearrange the entities_per_level array. */
    for (k = 0; k < arity - 1; k++) {
      if (entities_per_level[k] > entities_per_level[k + 1]) {
	tmp = entities_per_level[k + 1];
	entities_per_level[k + 1] = entities_per_level[k];
	entities_per_level[k] = tmp;
      } else {
	continue;
      }
    } 
  } 
  
  /* Eventually checks if every underlying core will be occupied this
     way. */
  for (i = 0; i < arity; i++) {
    if (entities_per_level[i] < per_item_entities) {
      ret = 0;
    }
  }
  
  return ret;
}

static 
void ma_cache_distribute_from (struct marcel_topo_level *l) {  
  unsigned int ne, nvp = marcel_vpset_weight(&l->vpset);
  unsigned int arity = l->arity;
  unsigned int i, k;
  
  if (!arity) {
    bubble_sched_debug ("done: !arity\n");
    return;
  }
  
  bubble_sched_debug ("count in __marcel_bubble__cache\n");
  ne = ma_count_entities_on_rq (&l->rq);
  bubble_sched_debug ("ne = %d on runqueue %p\n", ne, &l->rq);  

  if (!ne) {
    for (k = 0; k < arity; k++)
      ma_cache_distribute_from (l->children[k]);
    return;
  }

  bubble_sched_debug ("cache found %d entities to distribute.\n", ne);
  
  ma_attracting_level_t attracting_levels[arity];
  ma_attracting_levels_init (attracting_levels, l, arity, ne);
  
  marcel_entity_t *e[ne];
  bubble_sched_debug ("get in ma_cache_distribute_from\n");
  ma_get_entities_from_rq (&l->rq, e, ne);

  bubble_sched_debug ("Entities were taken from runqueue %p:\n", &l->rq);
  ma_debug_show_entities ("ma_cache_distribute_from", e, ne);

  qsort (e, ne, sizeof(e[0]), &ma_increasing_order_entity_load_compar);
   
  if (ne < nvp) {
    if (ma_cache_has_enough_entities (l, e, ne, attracting_levels))
      ma_cache_distribute_entities_cache (l, e, ne, attracting_levels);
    else {
      /* We really have to explode at least one bubble */
      bubble_sched_debug ("We have to explode bubbles...\n");
      
      /* Let's start by counting sub-entities */
      unsigned int j, new_ne = 0, bubble_has_exploded = 0;
      for (i = 0; i < ne; i++) {
	if (!e[i])
	  continue;
	
	/* TODO: We could choose the next bubble to explode here,
	   considering bubble thickness and other parameters. */
	if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) {
	  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
	  marcel_entity_t *ee;
	  if (bb->as_holder.nr_ready && (ma_entity_load(e[i]) != 1)) {
	    /* If the bubble is not empty, and contains more than one thread */ 
	    for_each_entity_scheduled_in_bubble_begin (ee, bb)
	      new_ne++;
	    for_each_entity_scheduled_in_bubble_end ()
	    bubble_has_exploded = 1; /* We exploded one bubble,
					it may be enough ! */
	    bubble_sched_debug ("counting: nr_ready: %ld, new_ne: %d\n", bb->as_holder.nr_ready, new_ne);
	    break;
	  }
	} 
      }
      
      if (!bubble_has_exploded) {
	ma_cache_distribute_entities_cache (l, e, ne, attracting_levels);
	for (k = 0; k < arity; k++)
	  ma_cache_distribute_from (l->children[k]);
	ma_attracting_levels_destroy (attracting_levels, arity);
	return;
      }
	  
      if (!new_ne) {
	bubble_sched_debug ("done: !new_ne\n");
	ma_attracting_levels_destroy (attracting_levels, arity);
	return;
      }
      
      /* Now we can create the auxiliary list that will contain
	 the new entities */
      marcel_entity_t *ee, *new_e[new_ne];
      j = 0;
      bubble_has_exploded = 0;
      for (i = 0; i < ne; i++) {
	/* TODO: Once again could be a good moment to choose which
	   bubble to explode.*/
	if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) {
	  marcel_bubble_t *bb = ma_bubble_entity (e[i]);
	  if (bb->as_holder.nr_ready) { 
	    bubble_sched_debug ("exploding bubble %p\n", bb);
	    for_each_entity_scheduled_in_bubble_begin (ee, bb)
	      new_e[j++] = ee;
	    for_each_entity_scheduled_in_bubble_end ()
	    bubble_has_exploded = 1;
	    break;
	  }
	}
      }
      MA_BUG_ON (new_ne != j);
      
      ma_cache_sched_submit (new_e, new_ne, l);
      ma_attracting_levels_destroy (attracting_levels, arity);
      return ma_cache_distribute_from (l);
    }
  } else { /* ne >= nvp */ 
    /* We can delay bubble explosion ! */
    bubble_sched_debug ("more entities (%d) than vps (%d), delaying bubble explosion...\n", ne, nvp);
    ma_cache_distribute_entities_cache (l, e, ne, attracting_levels);
  }
  
  /* Keep distributing on the underlying levels */
  for (i = 0; i < arity; i++)
    ma_cache_distribute_from (l->children[i]);

  ma_attracting_levels_destroy (attracting_levels, arity);
}

static void
marcel_bubble_cache (marcel_bubble_t *b, struct marcel_topo_level *l) {
  bubble_sched_debug ("marcel_root_bubble: %p \n", &marcel_root_bubble);
  
  ma_bubble_synthesize_stats (b);

  /* Make sure bubbles can't be modified behind our back.  */
  ma_local_bh_disable ();
  ma_preempt_disable ();

  ma_bubble_lock_all (b, l);
  ma_cache_distribute_from (l);
  ma_resched_existing_threads (l);
  /* Remember the distribution we've just applied. */
  ma_bubble_snapshot ();
  ma_bubble_unlock_all (b, l);  

  ma_preempt_enable_no_resched ();
  ma_local_bh_enable ();
}

static int
cache_sched_init (void) {
  ma_last_succeeded_steal = 0;
  ma_last_failed_steal = 0;
  ma_atomic_init (&ma_succeeded_steals, 0);
  ma_atomic_init (&ma_failed_steals, 0);
  return 0;
}

static int
cache_sched_exit (void) {
  bubble_sched_debug ("Succeeded steals : %d, failed steals : %d\n", 
		     ma_atomic_read (&ma_succeeded_steals), 
		     ma_atomic_read (&ma_failed_steals));
  return 0;
}

static int
cache_sched_submit (marcel_entity_t *e) {
  MA_BUG_ON (e->type != MA_BUBBLE_ENTITY);
  bubble_sched_debug ("Cache: Submitting entities!\n");
  marcel_bubble_cache (ma_bubble_entity (e), marcel_topo_level (0, 0));

  return 0;
}

#if MA_CACHE_BSCHED_USE_WORK_STEALING
/* This function moves _entity_to_steal_ to the _starving_rq_
   runqueue, while moving everything that needs to be moved to avoid
   locking issues. */
static int
steal (marcel_entity_t *entity_to_steal, ma_runqueue_t *common_rq, ma_runqueue_t *starving_rq) {
#define MAX_ANCESTORS 128
  int i, nb_ancestors = 0, nvp = marcel_vpset_weight (&common_rq->vpset);
  marcel_entity_t *e, *ancestors[MAX_ANCESTORS];

  /* The main thread doesn't have an `init_holder'.  */
  if (entity_to_steal->init_holder) {
    /* Before moving the target entity, we have to move up some of its
       ancestors to avoid locking problems. */
    for (e = &ma_bubble_holder(entity_to_steal->init_holder)->as_entity;
	 e->init_holder; 
	 e = &ma_bubble_holder(e->init_holder)->as_entity) {
      /* In here, we try to find these ancestors */
      if ((e->sched_holder->type == MA_RUNQUEUE_HOLDER) &&
	  ((e->sched_holder == &common_rq->as_holder) || (marcel_vpset_weight (&(ma_rq_holder(e->sched_holder))->vpset) > nvp))) {
	/* We keep browsing up through the bubbles hierarchy. If we
	   reached a bubble whose scheduling level is >= to the
	   level we need to put ancestors on, our job is over. */
	break;
      } else {
	ancestors[nb_ancestors] = e;
	nb_ancestors++;
      }
    }
  }
  
  if (nb_ancestors) {
    /* Then we burst everyone of them, to let their content where it
       was scheduled, and we move them to the common_rq, covering the
       source and destination runqueue. */
    for (i = nb_ancestors - 1; i > -1; i--) {
      MA_BUG_ON (ancestors[i]->type != MA_BUBBLE_ENTITY);
      ma_burst_bubble (ma_bubble_entity (ancestors[i]));
      ma_move_entity (ancestors[i], &common_rq->as_holder); 
    }
  }

  /* Now that every ancestor is at the right place, we can steal the
     target entity. */
  ma_move_entity (entity_to_steal, &starving_rq->as_holder);

  return 1;
#undef MAX_ANCESTORS
}

/* This structure contains arguments to pass the browse_and_steal
   function. */
struct browse_and_steal_args {
  /* Level we were originally called from (i.e. the one we need to
     feed with stolen entities) */
  struct marcel_topo_level *source;
  /* Thread to be stolen if we can't find better candidates */
  marcel_entity_t *thread_to_steal;
};

/* Returns 1 if the _e_ entity is blocked. If _e_ is a bubble, the
   function returns 1 if the whole set of included entities is
   blocked. */
static int
is_blocked (marcel_entity_t *e) {
  if (e->type == MA_BUBBLE_ENTITY) {
    marcel_entity_t *inner_e;
    for_each_entity_held_in_bubble (inner_e, ma_bubble_entity (e)) {
      if ((inner_e->type != MA_BUBBLE_ENTITY) 
	  && !MA_TASK_IS_BLOCKED (ma_task_entity (inner_e))) {
	/* We found a non-blocked thread! */
	return 0;
      }
    }
  } else {
    if (!MA_TASK_IS_BLOCKED (ma_task_entity (e))) {
      return 0;
    }
  }
  return 1;
}

/* This function is just a way to factorize the code called by
   _browse_and_steal_ when testing if an entity is worth to be
   stolen. It tests if the load of _tested_entity_ is greater than
   _greater_. If so, the considered entity is backed up in
   _bestbb_. */
static int
is_entity_worth_stealing (int *greater, 
	     marcel_entity_t **bestbb, 
	     marcel_entity_t **thread_to_steal, 
	     marcel_entity_t **tested_entity) {
  int found = 0;
  long load = ma_entity_load (*tested_entity);
  
  /* We don't want to pick blocked entities, otherwise the starving vp
     will still be idle. */
  if (!is_blocked (*tested_entity)) {
    /* If we have the choice, pick the most loaded bubble. */
    if ((*tested_entity)->type == MA_BUBBLE_ENTITY) {
      if (load > *greater) {
	*greater = load;
	*bestbb = *tested_entity;
	/* We found a bubble bigger than bestbb. */
	found = 1;
      }
    } else {
      /* If we don't find any bubble to steal, we may steal a
	 thread. In that case, we try to steal the higher thread we
	 find in the entities hierarchy. This kind of behaviour favors
	 load balancing of OpenMP nested applications for example. */
      if (*thread_to_steal ==  NULL) {
	*thread_to_steal = *tested_entity;
      }
      /* We found a non-blocked thread. */
      found = 1;
    }
  }

  return found;
}

/* This function browses the content of the _hold_ holder, in order to
   find something to steal (a bubble if possible) to the _source_
   topo_level. It recursively looks at bubbles content too. 

   If we didn't find any bubble to steal, we resign to steal
   _thread_to_steal_, which can be seen as the guy you always choose
   last when forming a soccer team with your pals. */
static int
browse_and_steal(ma_holder_t *hold, void *args) {
  marcel_entity_t *e, *bestbb = NULL; 
  int greater = 0, available_entities = 0;
  struct marcel_topo_level *top = marcel_topo_level(0,0); 
  struct marcel_topo_level *source = (*(struct browse_and_steal_args *)args).source;  
  marcel_entity_t *thread_to_steal = (*(struct browse_and_steal_args *)args).thread_to_steal;
  
  /* We don't want to browse runqueues and bubbles the same
     way. (i.e., we want to browse directly included entities first,
     to steal the upper bubble we find.) */
  if (hold->type == MA_BUBBLE_HOLDER) {
    for_each_entity_scheduled_in_bubble_begin (e, ma_bubble_holder(hold)) 
      if (is_entity_worth_stealing (&greater, &bestbb, &thread_to_steal, &e)) {
	available_entities++;
      }
    for_each_entity_scheduled_in_bubble_end () 
  } else {
    list_for_each_entry (e, &hold->sched_list, sched_list) {
      if (is_entity_worth_stealing (&greater, &bestbb, &thread_to_steal, &e)) {
	available_entities++;
      }
    }
  }
  
  ma_runqueue_t *common_rq = NULL, *rq = NULL;
  
  if (bestbb) {
    rq = ma_get_parent_rq (bestbb);
  } else if (thread_to_steal) {
    rq = ma_get_parent_rq (thread_to_steal);
  }

  if (rq) {
    PROF_EVENTSTR (sched_status, "stealing subtree");
    for (common_rq = rq->father; common_rq != &top->rq; common_rq = common_rq->father) {
      /* This only works because _source_ is the level vp_is_idle() is
	 called from (i.e. it's one of the leaves in the topo_level
	 tree). */
      if (ma_rq_covers (common_rq, source->number)) {
	break;
      }
    }
  }
 
  if (bestbb) {
    if (available_entities > 1) { 
      return steal (bestbb, common_rq, &source->rq);
    } else {
      /* Browse the bubble */
      struct browse_and_steal_args new_args = {
	.source = source,
	.thread_to_steal = thread_to_steal,
      };
      
      return browse_and_steal (&ma_bubble_entity(bestbb)->as_holder, &new_args);
    }
  } else if (thread_to_steal && (available_entities > 1)) {
    /* There are no more bubbles to browse... Let's steal the bad
       soccer player... */
    return steal (thread_to_steal, common_rq, &source->rq);
  }

  return 0;
}

static int
cache_steal (unsigned int from_vp) {
  struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp], *father = me->father;
  unsigned int arity;
  int smthg_to_steal = 0;
  int n;

#if 0
  if ((ma_last_failed_steal && ((marcel_clock() - ma_last_failed_steal) < MA_FAILED_STEAL_COOLDOWN))
      || (ma_last_succeeded_steal && ((marcel_clock() - ma_last_failed_steal) < MA_SUCCEEDED_STEAL_COOLDOWN)))
    return 0;
#endif
  
  if (!ma_idle_scheduler_is_running ()) {
    bubble_sched_debug ("===[Processor %u is idle but ma_idle_scheduler is off! (%d)]===\n", from_vp, ma_idle_scheduler_is_running ());
    return 0;
  }
  
  ma_deactivate_idle_scheduler ();
  marcel_threadslist (0, NULL, &n, NOT_BLOCKED_ONLY);

  bubble_sched_debug ("===[Processor %u is idle]===\n", from_vp);

  if (father) {
    arity = father->arity;
  }

  ma_local_bh_disable ();  
  ma_preempt_disable ();

  ma_bubble_synthesize_stats (&marcel_root_bubble);
  ma_bubble_lock_all (&marcel_root_bubble, marcel_topo_level (0, 0));
  struct browse_and_steal_args args = {
    .source = me,
    .thread_to_steal = NULL,
  };
  /* We try to steal an interesting bubble from one of our neighbour
     runqueues (i.e. from our numa node) */
  smthg_to_steal = ma_topo_level_browse (me, 
					 ma_get_topo_type_depth (MARCEL_LEVEL_NODE), 
					 browse_and_steal, 
					 &args);
  ma_bubble_unlock_all (&marcel_root_bubble, marcel_topo_level(0,0));
  
  ma_resched_existing_threads (me);    
  ma_preempt_enable_no_resched ();
  ma_local_bh_enable ();
  ma_activate_idle_scheduler ();
  
  if (smthg_to_steal > 0) { 
    bubble_sched_debug ("We successfuly stole one or several entities !\n");
    ma_last_succeeded_steal = marcel_clock ();
    ma_atomic_inc (&ma_succeeded_steals);
    return 1;
  }

  ma_last_failed_steal = marcel_clock ();
  bubble_sched_debug ("We didn't manage to steal anything !\n");
  ma_atomic_inc (&ma_failed_steals);
  return 0;
}
#endif /* MA_CACHE_BSCHED_USE_WORK_STEALING */

MARCEL_DEFINE_BUBBLE_SCHEDULER (cache,
  .init = cache_sched_init,
  .exit = cache_sched_exit,
  .submit = cache_sched_submit,
#if MA_CACHE_BSCHED_USE_WORK_STEALING
  .vp_is_idle = cache_steal,
#endif /* MA_CACHE_BSCHED_USE_WORK_STEALING */
   /* TODO: This definetly is a crappy way to let other bubble
      schedulers use the Cache scheduler distribution algorithm. */
  .priv = ma_cache_distribute_from,
);
#endif /* MA__BUBBLES */
