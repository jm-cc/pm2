/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008, 2009 "the PM2 team" (see AUTHORS file)
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
#include <math.h>
#include <string.h>
#include <time.h>

#ifdef MA__BUBBLES

/** \brief A cache bubble scheduler (inherits from
 * `marcel_bubble_sched_t').  */
struct marcel_bubble_cache_sched {
	marcel_bubble_sched_t scheduler;

  /** \brief Whether work stealing is enabled.  */
	tbx_bool_t work_stealing;
};


#define MA_CACHE_BSCHED_NEEDS_DEBUGGING_FUNCTIONS 0

#define MA_FAILED_STEAL_COOLDOWN 1000
#define MA_SUCCEEDED_STEAL_COOLDOWN 100

#define MA_CACHE_FAVORS_LOAD_BALANCING 0

#define MA_CACHE_ACTS_AS_COMPACT 0

/* Submits a set of entities on a marcel_topo_level */
static void
ma_cache_sched_submit(marcel_entity_t * e[], unsigned int ne, struct marcel_topo_level *l)
{
	unsigned int i;
	MARCEL_SCHED_LOG("Submitting entities on runqueue %s (%p):\n",
			 l->rq.as_holder.name, &l->rq);
	ma_debug_show_entities("ma_cache_sched_submit", e, ne);
	for (i = 0; i < ne; i++) {
		if (e[i]) {
			int state = ma_get_entity(e[i]);
			ma_put_entity(e[i], &l->rq.as_holder, state);
		}
	}
}

static int decreasing_int_compar(const void *_e1, const void *_e2)
{
	return *(const int *) _e1 - *(const int *) _e2;
}

/* This function translates a vp index into a from->children
   index. (i.e., sometimes you're not distributing on vp-level
   runqueues, so you have to know which child topo_level covers the
   _favourite_vp_ vp.) */
static int
ma_translate_favorite_vp(int favorite_vp, marcel_entity_t * e,
			 struct marcel_topo_level *from)
{
	unsigned int i;
	int translated_index = MA_VPSTATS_NO_LAST_VP;
	if (favorite_vp != MA_VPSTATS_NO_LAST_VP) {
		if (favorite_vp == MA_VPSTATS_CONFLICT) {
			/* If the considered entity is a bubble that contains conflicting
			   last_vp informations, we try to attract this bubble to the last
			   topo_level it was scheduled on. */
			MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
			struct marcel_topo_level *last_topo_level =
			    *(struct marcel_topo_level **) ma_stats_get(e,
									ma_stats_last_topo_level_offset);
			if (last_topo_level) {
				for (i = 0; i < from->arity; i++) {
					if (marcel_vpset_isincluded
					    (&from->children[i]->vpset,
					     &last_topo_level->vpset)) {
						translated_index = i;
						break;
					}
				}
			}
		} else {
			for (i = 0; i < from->arity; i++) {
				if (marcel_vpset_isset
				    (&from->children[i]->vpset,
				     (unsigned int) favorite_vp)) {
					translated_index = i;
					break;
				}
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
ma_strict_cache_distribute(marcel_entity_t * e,
			   int favorite_vp,
			   ma_distribution_t * distribution,
			   ma_distribution_t * load_balancing_entities)
{
	if (favorite_vp == MA_VPSTATS_NO_LAST_VP) {
		ma_distribution_add(e, load_balancing_entities, 0);
	} else {
		if (!distribution[favorite_vp].nb_entities) {
			ma_distribution_add_tail(e, &distribution[favorite_vp]);
		} else {
			unsigned int i, done = 0;
			long last_ran = ma_last_ran(e);
			for (i = 0; i < distribution[favorite_vp].nb_entities; i++) {
				if (last_ran >
				    ma_last_ran(distribution[favorite_vp].entities[i])) {
					ma_distribution_add(e, &distribution[favorite_vp],
							    i);
					done = 1;
					break;
				}
			}
			if (!done) {
				ma_distribution_add_tail(e, &distribution[favorite_vp]);
			}
		}
	}
}

/* Greedily distributes entities stored in _load_balancing_entities_
   over the _arity_ levels described in _distribution_.*/
static void
ma_spread_load_balancing_entities(ma_distribution_t * distribution,
				  ma_distribution_t * load_balancing_entities,
				  unsigned int arity,
				  int entities_are_the_same,
				  unsigned int entities_per_level)
{
	if (entities_are_the_same) {
		/* Distribute according to threads indexes */
		/* This can help with false sharing issues on OpenMP work
		   shares. */
		unsigned int level;
		for (level = 0; level < arity; level++) {
			while (distribution[level].total_load < entities_per_level
			       && load_balancing_entities->nb_entities) {
				ma_distribution_add_tail(ma_distribution_remove_tail
							 (load_balancing_entities),
							 &distribution[level]);
			}
		}
		/* Put the remaining entities consecutively */
		while (load_balancing_entities->nb_entities)
			ma_distribution_add_tail(ma_distribution_remove_tail
						 (load_balancing_entities),
						 &distribution[arity - 1]);
	} else {
		/* Sort the entities before distributing them. */
		qsort(load_balancing_entities->entities,
		      load_balancing_entities->nb_entities,
		      sizeof(load_balancing_entities->entities[0]),
		      &ma_increasing_order_entity_load_compar);

		/* Greedy distribute the entities. */
		while (load_balancing_entities->nb_entities) {
			unsigned int target =
			    ma_distribution_least_loaded_index(distribution, arity);
			ma_distribution_add_tail(ma_distribution_remove_tail
						 (load_balancing_entities),
						 &distribution[target]);
		}
	}
}

/* Even the load by taking entities from the most loaded levels and
   put them on the least loaded levels, until each level holds more than
   _load_per_level_ entities. */
static int
ma_global_load_balance(ma_distribution_t * distribution, unsigned int arity,
		       unsigned int load_per_level)
{
	int least_loaded = ma_distribution_least_loaded_index(distribution, arity);
	int most_loaded = ma_distribution_most_loaded_index(distribution, arity);

	/* If the most loaded level is also the least one, it means that
	   every underlying level already has the same amount of work, and
	   we can't do anything else.  Furthermore, we want to avoid
	   entering an endless balancing oscillation, so load balancing
	   terminates when the most loaded level's number of entities is
	   that of the least loaded plus 1.  */

	while ((distribution[least_loaded].nb_entities + 1 <
		distribution[most_loaded].nb_entities)
	       && (distribution[least_loaded].total_load < load_per_level)) {
		ma_distribution_add_tail(ma_distribution_remove_tail
					 (&distribution[most_loaded]),
					 &distribution[least_loaded]);
		least_loaded = ma_distribution_least_loaded_index(distribution, arity);
		most_loaded = ma_distribution_most_loaded_index(distribution, arity);
	}
	return 0;
}

static int ma_entities_are_the_same(marcel_entity_t * e[], unsigned int ne)
{
	unsigned int i;
	unsigned int type = e[0]->type;
	int load = ma_entity_load(e[0]);

	for (i = 0; i < ne; i++) {
		if (e[i]->type != type || ma_entity_load(e[i]) != load)
			return 0;
	}

	return 1;
}


/* Distributes a set of entities regarding cache affinities */
static int
ma_cache_distribute_entities_cache(struct marcel_topo_level *l,
				   marcel_entity_t * e[],
				   unsigned int ne, ma_distribution_t * distribution)
{
	unsigned int i, arity = l->arity, entities_per_level =
	    marcel_vpset_weight(&l->vpset) / arity;
	ma_distribution_t load_balancing_entities;
	ma_distribution_init(&load_balancing_entities, (struct marcel_topo_level *) NULL,
			     0, ne);

	/* First, we add each entity stored in _e[]_ to the
	   _distribution_ structure, by putting it into the array
	   corresponding to the level it's attracted to. */
	for (i = 0; i < ne; i++) {
		long last_vp = ma_favourite_vp(e[i]), last_vp_index = -1;
		last_vp_index = ma_translate_favorite_vp(last_vp, e[i], l);

		ma_strict_cache_distribute(e[i],
					   last_vp_index,
					   distribution, &load_balancing_entities);
	}

	int entities_are_the_same = ma_entities_are_the_same(e, ne);

	/* Then, we greedily distribute the unconstrained entities to balance
	   the load. */
	ma_spread_load_balancing_entities(distribution,
					  &load_balancing_entities,
					  arity, entities_are_the_same, ne / arity);

	/* At this point, we verify if every underlying topo_level will be
	   occupied. If not, we even the load by taking entities from the
	   most loaded levels to the least loaded ones. */
	ma_global_load_balance(distribution, arity, entities_per_level);

	/* We now have a satisfying logical distribution, let's physically
	   move entities according it. */
	ma_apply_distribution(distribution, arity);

	ma_distribution_destroy(&load_balancing_entities, 1);

	return 0;
}

#define MA_CACHE_DISTRIBUTES_WITHOUT_BURSTING 0UL
#define MA_CACHE_NEEDS_TO_BURST 1UL
#define MA_CACHE_CANNOT_BURST 2UL
#define MA_CACHE_FAMINE 3UL

/* Checks wether enough entities are already positionned on
   the considered runqueues */
static unsigned int
ma_cache_has_enough_entities(struct marcel_topo_level *l,
			     marcel_entity_t * e[],
			     unsigned int ne, const ma_distribution_t * distribution)
{
	unsigned int i, prev_state = 1, nvp = marcel_vpset_weight(&l->vpset);
	unsigned int arity = l->arity, per_item_entities =
	    nvp / arity, entities_per_level[arity];
	unsigned int total_load = 0, ret = MA_CACHE_DISTRIBUTES_WITHOUT_BURSTING;

	/* First, check if enough entities are already scheduled over the
	   underlying levels. */
	for (i = 0; i < arity; i++) {
		if (distribution[i].total_load < per_item_entities) {
			prev_state = 0;
		}
	}

	if (prev_state) {
		return ret;
	}
#if MA_CACHE_ACTS_AS_COMPACT
	int nthreads = 0;
	for (i = 0; i < ne; i++)
		nthreads += ma_count_threads_in_entity(e[i]);

	if (nthreads < nvp)
		return MA_CACHE_FAMINE;
#endif

	/* If not, initialize the entities_per_level array with the load of
	   all entities scheduled in each topology subtree. */
	for (i = 0; i < arity; i++) {
		entities_per_level[i] = distribution[i].total_load;
	}

	/* Make sure that entities_per_level[0] always points to the
	   least-loaded level. */
	qsort(entities_per_level, arity, sizeof(int), &decreasing_int_compar);

	for (i = 0; i < ne; i++) {
		unsigned int k, tmp;
		/* Put the bigger entity on the least-loaded level.
		 * We assume the _e_ array is sorted. */
		entities_per_level[0] += ma_entity_load(e[i]);
		total_load += ma_entity_load(e[i]);

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

	if (total_load < arity)
		/* There is no need to explode anything, as the considered
		   entities do not hold enough work to occupy every underlying
		   level. */
		ret = MA_CACHE_CANNOT_BURST;
	else {
		/* Eventually checks if every underlying core will be occupied this
		   way. */
		for (i = 0; i < arity; i++) {
			if (entities_per_level[i] < per_item_entities) {
				/* If not, tell the scheduler to explode a bubble. */
				ret = MA_CACHE_NEEDS_TO_BURST;
			}
		}
	}

	return ret;
}

/* Explode one bubble from the _e_ set of entities. */
static
unsigned int ma_cache_burst_one_bubble_from(struct marcel_topo_level *from,
					    marcel_entity_t ** e, unsigned int ne)
{
	/* Let's start by counting the sub-entities. */
	unsigned int i, j, new_ne = 0, bubble_has_exploded = 0;
	marcel_entity_t *ee;

	for (i = 0; i < ne; i++) {
		if (!e[i])
			continue;

		if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) {
			marcel_bubble_t *bb = ma_bubble_entity(e[i]);
			if (bb->as_holder.nb_ready_entities && (ma_entity_load(e[i]) != 1)) {
				/* If the bubble is not empty, and contains more than one thread. */
				for_each_entity_scheduled_in_bubble_begin(ee, bb)
				    new_ne++;
				for_each_entity_scheduled_in_bubble_end()
				    bubble_has_exploded = 1;
				break;
			}
		}
	}

	if (!new_ne || !bubble_has_exploded)
		return 0;

	/* Now we can create the auxiliary list that will contain
	   the new entities */
	marcel_entity_t *new_e[new_ne];
	j = 0;
	bubble_has_exploded = 0;
	for (i = 0; i < ne; i++) {
		if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) {
			marcel_bubble_t *bb = ma_bubble_entity(e[i]);
			if (bb->as_holder.nb_ready_entities && (ma_entity_load(e[i]) != 1)) {
				MARCEL_SCHED_LOG("exploding bubble %p\n", bb);
				for_each_entity_scheduled_in_bubble_begin(ee, bb)
				    new_e[j++] = ee;
				for_each_entity_scheduled_in_bubble_end()
				    bubble_has_exploded = 1;
				break;
			}
		}
	}
	MA_BUG_ON(new_ne != j);

	ma_cache_sched_submit(new_e, new_ne, from);

	return bubble_has_exploded;
}

static
void ma_cache_distribute_entities_compact(struct marcel_topo_level *l)
{
	unsigned int i;
	unsigned int ne = ma_count_entities_on_rq(&l->rq);

	marcel_entity_t *e[ne];
	ma_get_entities_from_rq(&l->rq, e, ne);

	int nb_cores_to_occupy = 0;
	int occupied_cores = 0;

	for (i = 0; i < ne; i++)
		nb_cores_to_occupy += ma_count_threads_in_entity(e[i]);

	ma_distribution_t distribution[l->arity];
	ma_distribution_init(distribution, l, l->arity, nb_cores_to_occupy);

	int per_level_cores = marcel_vpset_weight(&distribution[0].level->vpset);
	int assigned_load = 0;
	int current_level = 0;

	while (occupied_cores < nb_cores_to_occupy) {
		occupied_cores = 0;

		ne = ma_count_entities_on_rq(&l->rq);
		marcel_entity_t *new_e[ne];
		ma_get_entities_from_rq(&l->rq, new_e, ne);

		for (i = 0; i < ne; i++) {
			ma_distribution_add(new_e[i], &distribution[current_level], 0);
			assigned_load += ma_count_threads_in_entity(new_e[i]);

			if ((int) (distribution[current_level].total_load) >=
			    per_level_cores) {
				current_level++;
				occupied_cores +=
				    assigned_load >
				    per_level_cores ? per_level_cores : assigned_load;
				assigned_load = 0;
			}
		}

		occupied_cores += assigned_load;

		if (occupied_cores < nb_cores_to_occupy) {
#ifdef PM2_BUG_ON
			int burst = ma_cache_burst_one_bubble_from(l, new_e, ne);
			MA_BUG_ON(!burst);
#endif

			for (i = 0; i < l->arity; i++)
				ma_distribution_clean(&distribution[i]);

			current_level = 0;
		} else
			ma_apply_distribution(distribution, l->arity);
	}
}

static
void ma_cache_distribute_from(struct marcel_topo_level *l)
{
	unsigned int ne, nvp = marcel_vpset_weight(&l->vpset);
	unsigned int arity = l->arity;
	unsigned int i, k;

	if (!arity) {
		MARCEL_SCHED_LOG("done: !arity\n");
		return;
	}

	ne = ma_count_entities_on_rq(&l->rq);
	MARCEL_SCHED_LOG("ne = %d on runqueue %s (%p)\n", ne, l->rq.as_holder.name,
			 &l->rq);

	if (!ne) {
		for (k = 0; k < arity; k++)
			ma_cache_distribute_from(l->children[k]);
		return;
	}

	MARCEL_SCHED_LOG("cache found %d entities to distribute.\n", ne);

	ma_distribution_t distribution[arity];
	ma_distribution_init(distribution, l, arity, ne);

	marcel_entity_t *e[ne];
	ma_get_entities_from_rq(&l->rq, e, ne);

	MARCEL_SCHED_LOG("Entities were taken from runqueue %s (%p):\n",
			 l->rq.as_holder.name, &l->rq);
	ma_debug_show_entities("ma_cache_distribute_from", e, ne);

	qsort(e, ne, sizeof(e[0]), &ma_increasing_order_entity_load_compar);

	if (ne < nvp) {
		unsigned int state = ma_cache_has_enough_entities(l, e, ne, distribution);

		switch (state) {

		case MA_CACHE_DISTRIBUTES_WITHOUT_BURSTING:
			ma_cache_distribute_entities_cache(l, e, ne, distribution);
			break;

		case MA_CACHE_NEEDS_TO_BURST:
			{
				unsigned int bubble_has_exploded =
				    ma_cache_burst_one_bubble_from(l, e, ne);
				if (bubble_has_exploded) {
					ma_distribution_destroy(distribution, arity);
					return ma_cache_distribute_from(l);
				} else {
					MARCEL_SCHED_LOG
					    ("Didn't find any bubble to explode...\n");
					if (ne >= arity) {
						MARCEL_SCHED_LOG
						    ("... at least, distribute over the children of level %s.\n",
						     l->rq.as_holder.name);
						ma_cache_distribute_entities_cache(l, e,
										   ne,
										   distribution);
					} else {
						MARCEL_SCHED_LOG
						    ("... so leave the entities on level %s.\n",
						     l->rq.as_holder.name);
					}
				}
				break;
			}

		case MA_CACHE_CANNOT_BURST:
			break;

		case MA_CACHE_FAMINE:
			ma_cache_distribute_entities_compact(l);
			break;

		default:
			MA_BUG();
		}
	} else {		/* ne >= nvp */
		/* We can delay bubble explosion ! */
		MARCEL_SCHED_LOG
		    ("more entities (%d) than vps (%d), delaying bubble explosion...\n",
		     ne, nvp);
		ma_cache_distribute_entities_cache(l, e, ne, distribution);
	}

	/* Keep distributing on the underlying levels */
	for (i = 0; i < arity; i++)
		ma_cache_distribute_from(l->children[i]);

	ma_distribution_destroy(distribution, arity);
}

static void marcel_bubble_cache(struct marcel_topo_level *l)
{
	MARCEL_SCHED_LOG("Cache: Distributing entities from topo_level %s.\n",
			 l->rq.as_holder.name);

	ma_cache_distribute_from(l);
	__ma_resched_topo_level(l);

	MARCEL_SCHED_LOG("Cache: Distribution from topo_level %s is now over.\n",
			 l->rq.as_holder.name);
}

static void ma_bubble_load_balance(unsigned int level)
{
	unsigned int x_index;
	struct marcel_topo_level *topo_lvl;

	for (x_index = 0; x_index < marcel_topo_level_nbitems[level]; x_index++) {
		unsigned int child_id, best_load = 0, most_loaded_child_id = 0;
		topo_lvl = marcel_topo_level(level, x_index);

		for (child_id = 0; child_id < topo_lvl->arity; child_id++) {
			unsigned int i, my_load = 0, ne =
			    ma_count_entities_on_rq(&topo_lvl->children[child_id]->rq);
			marcel_entity_t *e[ne];
			ma_get_entities_from_rq(&topo_lvl->children[child_id]->rq, e, ne);

			for (i = 0; i < ne; i++)
				my_load += ma_entity_load(e[i]);

			if (my_load > best_load) {
				best_load = my_load;
				most_loaded_child_id = child_id;
			}
		}

		unsigned int i, ne =
		    ma_count_entities_on_rq(&topo_lvl->children[most_loaded_child_id]->
					    rq);
		marcel_entity_t *e[ne];
		ma_get_entities_from_rq(&topo_lvl->children[most_loaded_child_id]->rq, e,
					ne);

		for (i = 0; i < ne; i++) {
			e[i]->settled = 1;
			ma_move_entity(e[i], &topo_lvl->rq.as_holder);
		}
	}
}

static void marcel_bubble_load_balance(struct marcel_topo_level *from)
{
	struct marcel_topo_level *topo_lvl =
	    marcel_topo_level(marcel_topo_nblevels - 2, 0);

	while (topo_lvl) {
		if (topo_lvl->level < from->level)
			break;

		ma_bubble_load_balance(topo_lvl->level);
		topo_lvl = topo_lvl->father;
	}
}

static int cache_sched_init(marcel_bubble_sched_t * self TBX_UNUSED)
{
	return 0;
}

static int cache_sched_exit(marcel_bubble_sched_t * self TBX_UNUSED)
{
	return 0;
}

static int
cache_sched_rawsubmit(marcel_bubble_sched_t * self TBX_UNUSED, marcel_entity_t * e)
{
	MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);

	if (e->settled)
		return 0;

	struct marcel_topo_level *from = ma_get_parent_rq(e)->topolevel;
	marcel_bubble_cache(from);

	if (MA_CACHE_FAVORS_LOAD_BALANCING)
		marcel_bubble_load_balance(from);

	/* Remember the distribution we've just applied. */
	ma_bubble_snapshot(from);

	return 0;
}

static int
cache_sched_submit(marcel_bubble_sched_t * self TBX_UNUSED, marcel_entity_t * e)
{
	MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);

	if (e->settled)
		return 0;

	struct marcel_topo_level *from = ma_get_parent_rq(e)->topolevel;
	marcel_bubble_t *b = ma_bubble_entity(e);

	/* Update the marcel statistics on any entity contained in b. */
	ma_bubble_synthesize_stats(b);

	ma_topo_lock_all(from);

	marcel_bubble_cache(from);

	if (MA_CACHE_FAVORS_LOAD_BALANCING)
		marcel_bubble_load_balance(from);

	/* Remember the distribution we've just applied. */
	ma_bubble_snapshot(from);

	ma_topo_unlock_all(from);

	return 0;
}



/* Work stealing.  */

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
static int is_blocked(marcel_entity_t * e)
{
	if (e->type == MA_BUBBLE_ENTITY) {
		marcel_entity_t *inner_e;
		for_each_entity_held_in_bubble(inner_e, ma_bubble_entity(e)) {
			if ((inner_e->type != MA_BUBBLE_ENTITY)
			    && !MA_TASK_IS_BLOCKED(ma_task_entity(inner_e))) {
				/* We found a non-blocked thread! */
				return 0;
			}
		}
	} else {
		if (!MA_TASK_IS_BLOCKED(ma_task_entity(e))) {
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
is_entity_worth_stealing(int *greater,
			 marcel_entity_t ** bestbb,
			 marcel_entity_t ** thread_to_steal,
			 marcel_entity_t ** tested_entity)
{
	int found = 0;
	long load = ma_entity_load(*tested_entity);

	/* We don't want to pick blocked entities, otherwise the starving vp
	   will still be idle. */
	if (!is_blocked(*tested_entity)) {
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
			if (*thread_to_steal == NULL) {
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
static int browse_and_steal(ma_holder_t * hold, void *args)
{
	marcel_entity_t *e, *bestbb = NULL;
	int greater = 0, available_entities = 0;
	struct marcel_topo_level *source =
	    (*(struct browse_and_steal_args *) args).source;
	marcel_entity_t *thread_to_steal =
	    (*(struct browse_and_steal_args *) args).thread_to_steal;

	/* We don't want to browse runqueues and bubbles the same
	   way. (i.e., we want to browse directly included entities first,
	   to steal the upper bubble we find.) */
	if (hold->type == MA_BUBBLE_HOLDER) {
		for_each_entity_scheduled_in_bubble_begin(e, ma_bubble_holder(hold))
		    if (is_entity_worth_stealing(&greater, &bestbb, &thread_to_steal, &e)) {
			available_entities++;
		}
		for_each_entity_scheduled_in_bubble_end()
	} else {
		tbx_fast_list_for_each_entry(e, &hold->ready_entities,
					     ready_entities_item) {
			if (is_entity_worth_stealing
			    (&greater, &bestbb, &thread_to_steal, &e)) {
				available_entities++;
			}
		}
	}

	if (bestbb) {
		if (available_entities > 1) {
			return ma_bsched_steal(bestbb, source);
		} else {
			/* Browse the bubble */
			struct browse_and_steal_args new_args = {
				.source = source,
				.thread_to_steal = thread_to_steal,
			};

			return browse_and_steal(&ma_bubble_entity(bestbb)->as_holder,
						&new_args);
		}
	} else if (thread_to_steal && (available_entities > 1)) {
		/* There are no more bubbles to browse... Let's steal the bad
		   soccer player... */
		return ma_bsched_steal(thread_to_steal, source);
	}

	return 0;
}

static int cache_steal(marcel_bubble_sched_t * sched, unsigned int from_vp)
{
	marcel_bubble_cache_sched_t *self = (marcel_bubble_cache_sched_t *) sched;
	struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp];
	int smthg_to_steal = 0;
	int n;

	if (!self->work_stealing)
		/* Work stealing is disabled.  */
		return 0;

	if (!ma_idle_scheduler_is_running()) {
		MARCEL_SCHED_LOG
		    ("===[Processor %u is idle but ma_idle_scheduler is off! (%d)]===\n",
		     from_vp, ma_idle_scheduler_is_running());
		return 0;
	}

	ma_deactivate_idle_scheduler();
	marcel_threadslist(0, NULL, &n, NOT_BLOCKED_ONLY);

	MARCEL_SCHED_LOG("===[Processor %u is idle]===\n", from_vp);

	ma_local_bh_disable();
	ma_preempt_disable();

	ma_bubble_synthesize_stats(&marcel_root_bubble);
	ma_bubble_lock_all(&marcel_root_bubble, marcel_topo_level(0, 0));
	struct browse_and_steal_args args = {
		.source = me,
		.thread_to_steal = NULL,
	};
	/* We try to steal an interesting bubble from one of our neighbour
	   runqueues (i.e. from our numa node) */
	smthg_to_steal = ma_topo_level_browse(me,
					      ma_get_topo_type_depth(MARCEL_LEVEL_NODE),
					      browse_and_steal, &args);
	ma_bubble_unlock_all(&marcel_root_bubble, marcel_topo_level(0, 0));

	ma_resched_topo_level(me);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
	ma_activate_idle_scheduler();

	if (smthg_to_steal > 0) {
		MARCEL_SCHED_LOG("We successfuly stole one or several entities !\n");
		return 1;
	}

	MARCEL_SCHED_LOG("We didn't manage to steal anything !\n");
	return 0;
}


/* Instantiation.  */

int
marcel_bubble_cache_sched_init(struct marcel_bubble_cache_sched *scheduler,
			       struct marcel_topo_level *root_level,
			       tbx_bool_t work_stealing)
{
	memset(scheduler, 0, sizeof(*scheduler));

	scheduler->scheduler.klass = &marcel_bubble_cache_sched_class;

	scheduler->scheduler.init = cache_sched_init;
	scheduler->scheduler.exit = cache_sched_exit;
	scheduler->scheduler.submit = cache_sched_submit;
	scheduler->scheduler.rawsubmit = cache_sched_rawsubmit;
	scheduler->scheduler.vp_is_idle = cache_steal;
	scheduler->scheduler.root_level = root_level;

	scheduler->work_stealing = work_stealing;

	return 0;
}

/* Create a bubble scheduler with default parameters.  */
static int make_default_scheduler(marcel_bubble_sched_t * scheduler)
{
	return marcel_bubble_cache_sched_init((marcel_bubble_cache_sched_t *) scheduler,
					      marcel_topo_level(0, 0), tbx_false);
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS(cache, make_default_scheduler);

#endif				/* MA__BUBBLES */

/*
   Local Variables:
   tab-width: 8
   End:
 */
