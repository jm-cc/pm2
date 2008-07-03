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

#ifdef MA__BUBBLES

#define MA_FAILED_STEAL_COOLDOWN 1000
#define MA_SUCCEEDED_STEAL_COOLDOWN 100

static unsigned long last_failed_steal = 0;
static unsigned long last_succeeded_steal = 0;
static ma_atomic_t succeeded_steals = MA_ATOMIC_INIT(0);
static ma_atomic_t failed_steals = MA_ATOMIC_INIT(0);

/* Submits a set of entities on a marcel_topo_level */
static void
__sched_submit (marcel_entity_t *e[], int ne, struct marcel_topo_level *l) {
  int i;
  bubble_sched_debug("Submitting entities on runqueue %p:\n", &l->rq);
  ma_debug_show_entities("__sched_submit", e, ne);
  for (i = 0; i < ne; i++) {	      
    if (e[i]) {
      int state = ma_get_entity (e[i]);
      ma_put_entity(e[i], &l->rq.as_holder, state);
    }
  }
}

struct load_indicator {
  int load;
  struct marcel_topo_level *l; 
};

typedef struct load_indicator load_indicator_t;

static int 
rq_load_compar(const void *_li1, const void *_li2) {
  load_indicator_t *li1 = (load_indicator_t *) _li1;
  load_indicator_t *li2 = (load_indicator_t *) _li2;
  long l1 = li1->load;
  long l2 = li2->load;
  return l1 - l2;
}

static int
load_from_children(struct marcel_topo_level *father) {
  int arity = father->arity;
  int ret = 0;
  int i;
  
  if (arity)
    for (i = 0; i < arity; i++) {
      ret += load_from_children(father->children[i]);
      ret += ma_count_entities_on_rq(&father->children[i]->rq, RECURSIVE_MODE);
    } else {
    ret += ma_count_entities_on_rq(&father->rq, RECURSIVE_MODE);
  }
  return ret;
}

static void
initialize_load_manager(load_indicator_t *load_manager, struct marcel_topo_level *from) {
  int arity = from->arity;
  int k;
  
  for (k = 0; k < arity; k++) { 
    load_manager[k].load = load_from_children(from->children[k]);
    load_manager[k].l = from->children[k];
  }
}

static int
__find_index(load_indicator_t *load_manager, int begin, int end) {
  int value = load_manager[0].load;
  if (begin >= end - 1)
    return begin;
  int pivot = (begin + end) / 2;
  if (value > load_manager[pivot].load)
    return __find_index(load_manager, pivot, end);
  else
    return __find_index(load_manager, begin, pivot);
}

/* Inserts an element in a sorted load_manager */
static void
__rearrange_load_manager(load_indicator_t *load_manager, int arity) {
  load_indicator_t tmp;
  tmp.load = load_manager[0].load;
  tmp.l = load_manager[0].l;
  int index = __find_index(load_manager, 0, arity);
  memmove(&load_manager[0], &load_manager[1], index * sizeof(struct load_indicator));
  load_manager[index].load = tmp.load;
  load_manager[index].l = tmp.l;
}

/* Distributes a set of entities regarding the load of the underlying
   levels */
static void 
__distribute_entities(struct marcel_topo_level *l, marcel_entity_t *e[], int ne, load_indicator_t *load_manager) {                      
  int i;    
  int arity = l->arity;

  if (!arity) {
    bubble_sched_debug("__distribute_entities: done !arity\n");
    return;
  }
  
  qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);

  for (i = 0; i < ne; i++) {
    if (e[i]) {
      if (e[i]->type == MA_BUBBLE_ENTITY) {
	marcel_bubble_t *b = ma_bubble_entity(e[i]);
	if (!b->as_holder.nr_ready) { /* We don't pick empty bubbles */         
	  bubble_sched_debug("bubble %p is empty\n",b);
	  continue;                     
	}
      }
      
      int state = ma_get_entity(e[i]);
      int nbthreads = ma_entity_load(e[i]);
      
      load_manager[0].load += nbthreads;
      ma_put_entity(e[i], &load_manager[0].l->rq.as_holder, state);
      bubble_sched_debug("%p on %s\n",e[i],load_manager[0].l->rq.name);
      __rearrange_load_manager(load_manager, arity);
    }
  }
}

static int 
int_compar(const void *_e1, const void *_e2) {
  return *(int *)_e2 - *(int *)_e1;
}

/* Checks wether enough entities are already positionned on
   the considered runqueues */
static int
__has_enough_entities(struct marcel_topo_level *l, 
		      marcel_entity_t *e[], 
		      int ne, 
		      load_indicator_t *load_manager) {
  int ret = 1, prev_state = 1;
  int nvp = marcel_vpset_weight(&l->vpset);
  int arity = l->arity;
  int per_item_entities = nvp / arity;
  int i, entities_per_level[arity];

  if (ne < arity)
    return 0;

  for (i = 0; i < arity; i++)
    if (load_manager[i].load < per_item_entities)
      prev_state = 0;
  
  if (prev_state)
    return prev_state;
  
  for (i = 0; i < arity; i++)
    entities_per_level[i] = load_manager[i].load;
  
  qsort(entities_per_level, arity, sizeof(int), &int_compar);
  
  for (i = 0; i < ne; i++) {
    int k, tmp;
    entities_per_level[0] += ma_entity_load(e[i]); 
    
    for (k = 0; k < arity - 1; k++) {
      if (entities_per_level[k] > entities_per_level[k + 1]) {
	tmp = entities_per_level[k + 1];
	entities_per_level[k + 1] = entities_per_level[k];
	entities_per_level[k] = tmp;
      }
      else
	continue;
    } 
  } 
  
  for (i = 0; i < arity; i++)
    if (entities_per_level[i] < per_item_entities)
      ret = 0;
  
  return ret;
}

static 
void __marcel_bubble_affinity (struct marcel_topo_level *l) {  
  int ne;
  int nvp = marcel_vpset_weight(&l->vpset);
  int arity = l->arity;
  int i, k;
  
  if (!arity) {
    bubble_sched_debug("done: !arity\n");
    return;
  }
  
  bubble_sched_debug("count in __marcel_bubble__affinity\n");
  ne = ma_count_entities_on_rq(&l->rq, ITERATIVE_MODE);
  bubble_sched_debug("ne = %d on runqueue %p\n", ne, &l->rq);  

  if (!ne) {
    for (k = 0; k < arity; k++)
      __marcel_bubble_affinity(l->children[k]);
    return;
  }

  bubble_sched_debug("affinity found %d entities to distribute.\n", ne);
  
  load_indicator_t load_manager[arity];
  initialize_load_manager(load_manager, l);
  
  qsort(load_manager, arity, sizeof(load_manager[0]), &rq_load_compar);

  marcel_entity_t *e[ne];
  bubble_sched_debug("get in __marcel_bubble_affinity\n");
  ma_get_entities_from_rq(&l->rq, e, ne);

  bubble_sched_debug("Entities were taken from runqueue %p:\n", &l->rq);
  ma_debug_show_entities("__marcel_bubble_affinity", e, ne);

  if (ne < nvp) {
    if (__has_enough_entities(l, e, ne, load_manager))
      __distribute_entities(l, e, ne, load_manager);
    else {
      /* We really have to explode at least one bubble */
      bubble_sched_debug("We have to explode bubbles...\n");
      
      qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);
      
      /* Let's start by counting sub-entities */
      unsigned new_ne = 0;
      int j;
      int bubble_has_exploded = 0;
      
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
	    for_each_entity_scheduled_in_bubble_begin(ee,bb)
	      new_ne++;
	    for_each_entity_scheduled_in_bubble_end()
	    bubble_has_exploded = 1; /* We exploded one bubble,
					it may be enough ! */
	    bubble_sched_debug("counting: nr_ready: %ld, new_ne: %d\n", bb->as_holder.nr_ready, new_ne);
	    break;
	  }
	} 
      }
      
      if (!bubble_has_exploded) {
	__distribute_entities(l, e, ne, load_manager);
	for (k = 0; k < arity; k++)
	  __marcel_bubble_affinity(l->children[k]);
	return;
      }
	  
      if (!new_ne) {
	bubble_sched_debug( "done: !new_ne\n");
	return;
      }
      
      /* Now we can create the auxiliary list that will contain
	 the new entities */
      marcel_entity_t *new_e[new_ne], *ee;
      j = 0;
      bubble_has_exploded = 0;
      for (i = 0; i < ne; i++) {
	/* TODO: Once again could be a good moment to choose which
	   bubble to explode.*/
	if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) {
	  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
	  if (bb->as_holder.nr_ready) { 
	    bubble_sched_debug("exploding bubble %p\n", bb);
	    for_each_entity_scheduled_in_bubble_begin(ee,bb)
	      new_e[j++] = ee;
	    for_each_entity_scheduled_in_bubble_end()
	    bubble_has_exploded = 1;
	    break;
	  }
	}
      }
      MA_BUG_ON(new_ne != j);
      
      __sched_submit (new_e, new_ne, l);
      return __marcel_bubble_affinity(l);
    }
  } else { /* ne >= nvp */ 
    /* We can delay bubble explosion ! */
    bubble_sched_debug("more entities (%d) than vps (%d), delaying bubble explosion...\n", ne, nvp);
    __distribute_entities(l, e, ne, load_manager);
  }
  
  /* Keep distributing on the underlying levels */
  for (i = 0; i < arity; i++)
    __marcel_bubble_affinity(l->children[i]);
}

void 
marcel_bubble_affinity(marcel_bubble_t *b, struct marcel_topo_level *l) {
  marcel_entity_t *e = &b->as_entity;
  
  bubble_sched_debug("marcel_root_bubble: %p \n", &marcel_root_bubble);
  
  ma_bubble_synthesize_stats(b);
  ma_preempt_disable();
  ma_local_bh_disable();
  
  ma_bubble_lock_all(b, l);
  __ma_bubble_gather(b, b);
  __sched_submit(&e, 1, l);
  __marcel_bubble_affinity(l);
  ma_resched_existing_threads(l);
  ma_bubble_unlock_all(b, l);  

  ma_preempt_enable_no_resched();
  ma_local_bh_enable();
}

static int
affinity_sched_init() {
  last_succeeded_steal = 0;
  last_failed_steal = 0;
  ma_atomic_init(&succeeded_steals, 0);
  ma_atomic_init(&failed_steals, 0);
  return 0;
}

static int
affinity_sched_exit() {
  bubble_sched_debug("Succeeded steals : %d, failed steals : %d\n", 
		     ma_atomic_read(&succeeded_steals), 
		     ma_atomic_read(&failed_steals));
  return 0;
}

static marcel_bubble_t *b = &marcel_root_bubble;

static int
affinity_sched_submit (marcel_entity_t *e) {
  struct marcel_topo_level *l = marcel_topo_level (0,0);
  b = ma_bubble_entity (e);
  if (!ma_atomic_read (&ma_init))
    marcel_bubble_affinity (b, l);
  else 
    __sched_submit (&e, 1, l);
  
  return 0;
}

/* This function moves _entity_to_steal_ to the _starving_rq_
   runqueue, while moving everything that needs to be moved to avoid
   locking issues. */
static int
steal (marcel_entity_t *entity_to_steal, ma_runqueue_t *common_rq, ma_runqueue_t *starving_rq) {
  int i, nb_ancestors = 0, max_ancestors = 128, nvp = marcel_vpset_weight (&common_rq->vpset);
  marcel_entity_t **ancestors = calloc (max_ancestors, sizeof(marcel_entity_t));
  marcel_entity_t *e;
  
  /* Before moving the target entity, we have to move up some of its
     ancestors to avoid locking problems. */
  for (e = &ma_bubble_holder(entity_to_steal->init_holder)->as_entity;
       e->init_holder; 
       e = &ma_bubble_holder(e->init_holder)->as_entity) {
    /* In here, we try to find these ancestors */
    if (e->sched_holder->type == MA_RUNQUEUE_HOLDER)
      if ((e->sched_holder == &common_rq->as_holder) 
	  || (marcel_vpset_weight (&(ma_rq_holder(e->sched_holder))->vpset) > nvp))
	break;
    ancestors[nb_ancestors] = e;
    nb_ancestors++;
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

  free (ancestors);
  return 1;
}


static ma_runqueue_t *
get_parent_rq (marcel_entity_t *e) {
  if (e) {
    ma_holder_t *sh = e->sched_holder;
    if (sh && (sh->type == MA_RUNQUEUE_HOLDER))
      return ma_to_rq_holder(sh);
    
    marcel_entity_t *upper_e = &ma_bubble_holder(sh)->as_entity;
    MA_BUG_ON (upper_e->sched_holder->type != MA_RUNQUEUE_HOLDER);
    return ma_to_rq_holder (upper_e->sched_holder);
  }
  return NULL;
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
  /* We try to avoid currently running entities. */
  if (!ma_entity_is_running (*tested_entity)) {
    long load = ma_entity_load (*tested_entity);
    if ((*tested_entity)->type == MA_BUBBLE_ENTITY) {
      if (load > *greater) {
	*greater = load;
	*bestbb = *tested_entity;
	found = 1;
      }
    } else {
      /* If we don't find any bubble to steal, we may steal a
	 thread. In that case, we try to steal the higher thread we
	 find in the entities hierarchy. This kind of behaviour favors
	 load balancing of OpenMP nested applications for example. */
      *thread_to_steal = (*thread_to_steal == NULL) ? *tested_entity : *thread_to_steal;
    }
  }
  return found;
}

/* This function browses the content of the _hold_ holder, in order to
   find something to steal (a bubble if possible) to the _source_
   topo_level. It recursively look at bubbles content too. 

   If we didn't find any bubble to steal, we resign to steal
   _thread_to_steal_, which can be seen as the guy you always choose
   last when forming a soccer team with your pals. */
static int
browse_and_steal(ma_holder_t *hold, void *args) {
  marcel_entity_t *e, *bestbb = NULL; 
  marcel_entity_t *thread_to_steal = (*(struct browse_and_steal_args *)args).thread_to_steal;
  int greater = 0, available_entities = 0;
  struct marcel_topo_level *top = marcel_topo_level(0,0);
  struct marcel_topo_level *source = (*(struct browse_and_steal_args *)args).source;  

  /* We don't want to browse runqueues and bubbles the same
     way. (i.e., we want to browse directly included entities first,
     to steal the upper bubble we find.) */
  if (hold->type == MA_BUBBLE_HOLDER) {
    for_each_entity_scheduled_in_bubble_begin (e, ma_bubble_holder(hold)) 
      is_entity_worth_stealing (&greater, &bestbb, &thread_to_steal, &e);
      available_entities++;
    for_each_entity_scheduled_in_bubble_end () 
  } else {
    list_for_each_entry(e, &hold->sched_list, sched_list) {
      is_entity_worth_stealing (&greater, &bestbb, &thread_to_steal, &e);
      available_entities++;
    }
  }
  ma_runqueue_t *common_rq = NULL, *rq = NULL;
  if (bestbb)
    rq = get_parent_rq(bestbb);
  else if (thread_to_steal)
    rq = get_parent_rq(thread_to_steal);

  if (rq) {
    PROF_EVENTSTR(sched_status, "stealing subtree");
    for (common_rq = rq->father; common_rq != &top->rq; common_rq = common_rq->father) {
      /* This only works because _source_ is the level vp_is_idle() is
	 called from (i.e. it's one of the leaves in the topo_level
	 tree). */
      if (ma_rq_covers(common_rq, source->number))
	break;
    }
  }
 
  if (bestbb) {
    if (available_entities > 1) 
      return steal(bestbb, common_rq, &source->rq);
    else {
      /* Browse the bubble */
      struct browse_and_steal_args new_args = {
	.source = source,
	.thread_to_steal = thread_to_steal,
      };
      return browse_and_steal(&ma_bubble_entity(bestbb)->as_holder, &new_args);
    }
  } else if (thread_to_steal && (available_entities > 1)) {
    /* There are no more bubbles to browse... Let's steal the bad
       soccer player... */
    return steal(thread_to_steal, common_rq, &source->rq);
  }

  return 0;
}

static int
affinity_steal(unsigned from_vp) {
  struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp];
  struct marcel_topo_level *father = me->father;
  struct marcel_topo_level *top = marcel_topo_level(0,0);
  int arity;
  
  int n, smthg_to_steal = 0;
  int nvp = marcel_vpset_weight(&top->vpset);
  
  if ((last_failed_steal && ((marcel_clock() - last_failed_steal) < MA_FAILED_STEAL_COOLDOWN)) 
      || (last_succeeded_steal && ((marcel_clock() - last_failed_steal) < MA_SUCCEEDED_STEAL_COOLDOWN)))
    return 0;
  
  ma_write_lock(&ma_idle_scheduler_lock);
  if (!ma_idle_scheduler) {
    ma_write_unlock(&ma_idle_scheduler_lock);
    return 0;
  }
  ma_idle_scheduler = 0;
  
  marcel_threadslist(0,NULL,&n,NOT_BLOCKED_ONLY);

  if (n < nvp) {
    ma_idle_scheduler = 1;
    ma_write_unlock(&ma_idle_scheduler_lock);
    return 0;
  }
  
  bubble_sched_debug("===[Processor %u is idle]===\n", from_vp);

  if (father)
    arity = father->arity;

  ma_preempt_disable();
  ma_local_bh_disable();  
  ma_bubble_synthesize_stats(&marcel_root_bubble);
  ma_bubble_lock_all(&marcel_root_bubble, marcel_topo_level(0,0));
  struct browse_and_steal_args args = {
    .source = me,
    .thread_to_steal = NULL,
  };
  smthg_to_steal = ma_topo_level_browse (me, browse_and_steal, &args);
  ma_bubble_unlock_all(&marcel_root_bubble, marcel_topo_level(0,0));
  ma_resched_existing_threads(me);    
  ma_preempt_enable_no_resched();
  ma_local_bh_enable();
  ma_idle_scheduler = 1;
  ma_write_unlock(&ma_idle_scheduler_lock);
  
  if (smthg_to_steal) { 
    bubble_sched_debug("We successfuly stole one or several entities !\n");
    last_succeeded_steal = marcel_clock();
    ma_atomic_inc(&succeeded_steals);
    return 1;
  }

  last_failed_steal = marcel_clock();
  bubble_sched_debug("We didn't manage to steal anything !\n");
  ma_atomic_inc(&failed_steals);
  return 0;
}

struct ma_bubble_sched_struct marcel_bubble_affinity_sched = {
  .init = affinity_sched_init,
  .exit = affinity_sched_exit,
  .submit = affinity_sched_submit,
  //.vp_is_idle = affinity_steal,
};

#endif /* MA__BUBBLES */
