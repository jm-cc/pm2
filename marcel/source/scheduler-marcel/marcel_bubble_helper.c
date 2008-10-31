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
#include <string.h>
#include <math.h>

#ifdef MA__BUBBLES

volatile unsigned long init_phase = 1;

long ma_entity_load(marcel_entity_t *e) 
{
  switch (e->type) {
    case MA_BUBBLE_ENTITY:
      return *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), marcel_stats_load_offset);
    case MA_THREAD_ENTITY:
      return *(long*)ma_task_stats_get(ma_task_entity(e), marcel_stats_load_offset);
    default:
      MA_BUG ();
  }
}

unsigned ma_is_a_seed(marcel_entity_t *e)
{
  long nb_seeds, nb_threads;
  
  if (e->type == MA_BUBBLE_ENTITY)
    {
      marcel_bubble_t *b = ma_bubble_entity(e);
      ma_bubble_synthesize_stats(b);
      nb_seeds = *(long*)ma_bubble_hold_stats_get(b, ma_stats_nbthreadseeds_offset);
      nb_threads = *(long*)ma_bubble_hold_stats_get(b, ma_stats_nbthreads_offset);

      if (nb_seeds >= nb_threads - 1)
	return 1;
      else
	return 0;
    }
  else if (e->type == MA_THREAD_SEED_ENTITY)
    return 1;
  else
    return 0;
}

unsigned ma_entity_is_running (marcel_entity_t *e) {
  if (e->type == MA_BUBBLE_ENTITY) {
    marcel_entity_t *ee;
    for_each_entity_held_in_bubble (ee, ma_bubble_entity (e)) {
      if ((ee->type != MA_BUBBLE_ENTITY) && MA_TASK_IS_RUNNING (ma_task_entity (ee)))
	return 1;
    }
  } else {
    if (MA_TASK_IS_RUNNING (ma_task_entity (e)))
      return 1;
  }
  return 0;
}

/* This function is called by bubble schedulers to determine the
  "cache friendly" runqueues where entities should be attracted to, if
  possible. */
long 
ma_favourite_vp (marcel_entity_t *e) {
  return e->type == MA_BUBBLE_ENTITY ?     
    *(long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_last_vp_offset) :
    *(long *) ma_stats_get (e, ma_stats_last_vp_offset);
}

/* This function returns the last time the entity passed in argument
   was running. */
long 
ma_last_ran (marcel_entity_t *e) {
  return e->type == MA_BUBBLE_ENTITY ?     
    *(long *) ma_bubble_hold_stats_get (ma_bubble_entity (e), ma_stats_last_ran_offset) :
    *(long *) ma_stats_get (e, ma_stats_last_ran_offset);
}

int ma_decreasing_order_entity_load_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t*const*) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t*const*) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	return l2 - l1; /* decreasing order */
}

int ma_increasing_order_entity_load_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t*const*) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t*const*) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	return l1 - l2; /* increasing order */
}

unsigned int 
ma_count_entities_on_rq (ma_runqueue_t *rq) {
  marcel_entity_t *ee;
  unsigned int ne = 0;
  
  for_each_entity_scheduled_on_runqueue (ee, rq) {
    if (ee->type == MA_BUBBLE_ENTITY) { 
      if (!(ma_bubble_entity (ee))->as_holder.nr_ready) {
	continue;
      }
    }       
    ne++;
  }
  
  return ne;
}

unsigned int
ma_load_on_rq (ma_runqueue_t *rq) {
  marcel_entity_t *ee;
  unsigned int load = 0;
  
  for_each_entity_scheduled_on_runqueue (ee, rq) {
    load += ma_entity_load (ee);
  }
  
  return load;
}

int ma_get_entities_from_rq(ma_runqueue_t *rq, marcel_entity_t *e[], int ne)
{
  marcel_entity_t *ee;
  int i = 0;

  for_each_entity_scheduled_on_runqueue(ee, rq)
    {
      if (ee->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *bb = ma_bubble_entity(ee);
	  if (bb->as_holder.nr_ready)
	    e[i++] = ee;
	}
      else
	e[i++] = ee;
    }
   
  return i;
}

int
ma_gather_all_bubbles_on_rq(ma_runqueue_t *rq)
{
  marcel_entity_t *e;
  int ret = 0;
  
  for_each_entity_scheduled_on_runqueue(e, rq)
    {
      if (e->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *b = ma_bubble_entity(e);
	  __ma_bubble_gather(b, b);
	  ret = 1;
	}
    }
  
  return ret;
}

#ifdef MA__NUMA_MEMORY
int decreasing_order_entity_attraction_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;

	/* ne pas considerer les frequences dacces pour le calcul du volume */
	int weight_coef = 0;
	/* considerer seulement des zones au moins de frequence medium */
	enum pinfo_weight access_min = MEDIUM_WEIGHT;
	
	long a1 = ma_compute_total_attraction(e1,weight_coef,access_min,1,NULL);
	long a2 = ma_compute_total_attraction(e2,weight_coef,access_min,1,NULL);

	return a2 - a1; /* decreasing order */
}

int increasing_order_entity_attraction_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;

	/* ne pas considerer les frequences dacces pour le calcul du volume */
	int weight_coef = 0;
	/* considerer seulement des zones au moins de frequence medium */
	enum pinfo_weight access_min = MEDIUM_WEIGHT;
	
	long a1 = ma_compute_total_attraction(e1,weight_coef,access_min,1,NULL);
	long a2 = ma_compute_total_attraction(e2,weight_coef,access_min,1,NULL);

	return a1 - a2; /* increasing order */
}

/* ordre en sommant la charge et du volume memoire a deplacer */
int decreasing_order_entity_both_compar(const void *_e1, const void *_e2)
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	
	/* ne pas considerer les frequences dacces pour le calcul du volume */
	int weight_coef = 0;
	/* considerer seulement des zones au moins de frequence medium */
	enum pinfo_weight access_min = MEDIUM_WEIGHT;
	
	long v1 = MA_VOLUME_COEF * ma_compute_total_attraction(e1,weight_coef,access_min,1,NULL);
	long v2 = MA_VOLUME_COEF * ma_compute_total_attraction(e2,weight_coef,access_min,1,NULL);

	return (MA_LOAD_COEF * log10((double)l2) + MA_VOLUME_COEF * log10((double)v2)) - (MA_LOAD_COEF * log10((double)l1) + MA_VOLUME_COEF * log10((double)v1)); /* decreasing order */
}

int increasing_order_entity_both_compar(const void *_e1, const void *_e2)
{
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = ma_entity_load(e1);
	long l2 = ma_entity_load(e2);
	
	/* ne pas considerer les frequences dacces pour le calcul du volume */
	int weight_coef = 0;
	/* considerer seulement des zones au moins de frequence medium */
	enum pinfo_weight access_min = MEDIUM_WEIGHT;
	
	long v1 = MA_VOLUME_COEF * ma_compute_total_attraction(e1,weight_coef,access_min,1,NULL);
	long v2 = MA_VOLUME_COEF * ma_compute_total_attraction(e2,weight_coef,access_min,1,NULL);

	return (MA_LOAD_COEF * l1 + MA_VOLUME_COEF * v1) - (MA_LOAD_COEF * l2 + MA_VOLUME_COEF * v2); /* increasing order */
}
#endif /* MA__NUMA_MEMORY */

void
ma_resched_existing_threads(struct marcel_topo_level *l)
{
	unsigned vp;

	marcel_vpset_foreach_begin(vp,&l->vpset)
		ma_lwp_t lwp = ma_vp_lwp[vp];
	ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
	marcel_vpset_foreach_end()
}

/* Lock the entity ! */
int ma_count_threads_in_entity(marcel_entity_t *entity)
{
	int nb = 0;	
	/* entities in bubble */
	if (entity->type == MA_BUBBLE_ENTITY)
	{
		marcel_entity_t *downentity;
		for_each_entity_held_in_bubble(downentity,ma_bubble_entity(entity))
			nb += ma_count_threads_in_entity(downentity);
	}
	else
		nb = 1;
	return nb;
}

int ma_decreasing_order_threads_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t*const*) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t*const*) _e2;
	long l1 =  ma_count_threads_in_entity(e1);
	long l2 =  ma_count_threads_in_entity(e2);
	return l2 - l1; /* decreasing order */
}

int ma_increasing_order_threads_compar(const void *_e1, const void *_e2) 
{
	marcel_entity_t *e1 = *(marcel_entity_t*const*) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t*const*) _e2;
	long l1 =  ma_count_threads_in_entity(e1);
	long l2 =  ma_count_threads_in_entity(e2);
	return l1 - l2; /* increasing order */
}

int
ma_burst_bubble (marcel_bubble_t *bubble) {
  int extracted_entities = 0;
  marcel_entity_t *e;

  MA_BUG_ON (!bubble); 
  
  if ((&bubble->as_entity)->sched_holder->type == MA_RUNQUEUE_HOLDER) {
    ma_holder_t *target_holder = (&bubble->as_entity)->sched_holder;
    for_each_entity_scheduled_in_bubble_begin (e, bubble)
      ma_move_entity (e, target_holder);
      extracted_entities++;
    for_each_entity_scheduled_in_bubble_end ()
  }
  
  return extracted_entities;
}

/* Debug function that prints information about the _ne_ entities
   stored in _e_ */
void
ma_debug_show_entities(const char *func_name TBX_UNUSED, marcel_entity_t *e[], int ne) {
  int k;
  bubble_sched_debug("in %s: There are %d entities: adresses :\n(", func_name, ne);
  for (k = 0; k < ne; k++)
    bubble_sched_debug("[%p, %d]", e[k], e[k]->type);
  bubble_sched_debug("end)\n");
  bubble_sched_debug("names :\n(");
  for (k = 0; k < ne; k++) {
    if (e[k]->type == MA_BUBBLE_ENTITY) {
      bubble_sched_debug("bubble, ");
    } else if (e[k]->type == MA_THREAD_ENTITY) {
      bubble_sched_debug("%s, ", ma_task_entity(e[k])->name);
    } else if (e[k]->type == MA_THREAD_SEED_ENTITY) {
      bubble_sched_debug("thread_seed, ");
    } else { 
      bubble_sched_debug("unknown!, ");
    }
  }
  bubble_sched_debug("end)\n");
}

#endif /* MA__BUBBLES */
