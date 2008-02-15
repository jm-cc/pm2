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
#include <math.h>
#include <string.h>
#include <time.h>

#ifdef MA__BUBBLES

#if 0
#define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#else
#define debug(fmt, ...) (void)0
#endif

#define MA_VP_IS_IDLE_COOLDOWN 1000

static struct list_head submitted_entities;
static ma_rwlock_t submit_rwlock = MA_RW_LOCK_UNLOCKED;
static unsigned long last_failed_steal = 0;
volatile unsigned long init_mode = 1;
volatile unsigned long succeeded_steals;
volatile unsigned long failed_steals;

/* Fonction de debug qui affiche la liste des adresses et des noms des entites trouvees sur une runqueue */
static void
__debug_show_entities(const char *func_name, marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int k;

  debug("dans %s: j'ai trouve %d entites, sur la runqueue %p, dont les adresses sont:\n(", func_name, ne, &l[0]->sched);
  for (k = 0; k < ne; k++)
    debug("[%p, %d]", e[k], e[k]->type);
  debug("end)\n");
  
  debug("et dont les noms sont:\n(");
  for (k = 0; k < ne; k++)
    {
      if (e[k]->type == MA_BUBBLE_ENTITY)
	{
	  debug("bulle, ");
	}
      else if (e[k]->type == MA_THREAD_ENTITY)
	{
	  marcel_task_t *t = ma_task_entity(e[k]);
	  debug("%s, ", t->name);
	}
      else if (e[k]->type == MA_THREAD_SEED_ENTITY)
	{
	  debug("seed, ");
	}
      else 
	debug("ovni!, ");
    }
  debug("end)\n");
}

/* Soumission d'un ensemble d'entit�s sur un topo_level */
static void
__sched_submit(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;

  __debug_show_entities("__sched_submit", e, ne, l);
  
  for (i = 0; i < ne; i++) 
    {	      
      if (e[i])
	{
	  int state = ma_get_entity(e[i]);
	  ma_put_entity(e[i], &l[0]->sched.hold, state);
	}
    }
}

struct load_indicator
{
  int load;
  struct marcel_topo_level *l; 
};

typedef struct load_indicator load_indicator_t;

static int 
rq_load_compar(const void *_li1, const void *_li2) 
{
  load_indicator_t *li1 = (load_indicator_t *) _li1;
  load_indicator_t *li2 = (load_indicator_t *) _li2;
  long l1 = li1->load;
  long l2 = li2->load;
  return l1 - l2;
}

static int
load_from_children(struct marcel_topo_level **father)
{
  int arity = father[0]->arity;
  int ret = 0;
  int i;

  if (arity)
    for (i = 0; i < arity; i++) 
      {
	ret += load_from_children(&father[0]->children[i]);
	debug("count dans load_from_children\n");
	ret += ma_count_entities_on_rq(&father[0]->children[i]->sched, RECURSIVE_MODE);
      }
  
  return ret;
}

static void
initialize_load_manager(load_indicator_t *load_manager, struct marcel_topo_level **from)
{
  int arity = from[0]->arity;
  int k;

  for (k = 0; k < arity; k++)
    { 
      load_manager[k].load = load_from_children(&from[0]->children[k]);
      load_manager[k].l = from[0]->children[k];
    }
}

static int
__find_index(load_indicator_t *load_manager, int begin, int end)
{
  int value = load_manager[0].load;
  
  if (begin >= end - 1)
    return begin;

  int pivot = (begin + end) / 2;

  if (value > load_manager[pivot].load)
    return __find_index(load_manager, pivot, end);
  else
    return __find_index(load_manager, begin, pivot);
}

/* Insertion d'un �l�ment dans un load_manager d�j� tri�*/
static void
__rearrange_load_manager(load_indicator_t *load_manager, int arity)
{
  load_indicator_t tmp;
  tmp.load = load_manager[0].load;
  tmp.l = load_manager[0].l;

  int index = __find_index(load_manager, 0, arity);
  memmove(&load_manager[0], &load_manager[1], index * sizeof(struct load_indicator));
  load_manager[index].load = tmp.load;
  load_manager[index].l = tmp.l;
}

/* Distribution d'un ensemble d'entit�s en fonction de la charge des niveaux d'en dessous */
static void 
__distribute_entities(struct marcel_topo_level **l, marcel_entity_t *e[], int ne, load_indicator_t *load_manager) 
{                      
  int i;    
  int arity = l[0]->arity;

  if (!arity) 
    {
      debug("__distribute_entities: done !arity\n");
      return;
    }
  
  qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);

  for (i = 0; i < ne; i++)                         
    {
      if (e[i])
	{
	  if (e[i]->type == MA_BUBBLE_ENTITY)                     
	    {
	      marcel_bubble_t *b = ma_bubble_entity(e[i]);
	      if (!b->hold.nr_ready) { /* On ne prend pas de bulle vide */         
		debug("bulle %p est vide\n",b);
		continue;                     
	      }
	    }
 
	  int state = ma_get_entity(e[i]);
	  int nbthreads = ma_entity_load(e[i]);
	  
	  load_manager[0].load += nbthreads;
	  ma_put_entity(e[i], &load_manager[0].l->sched.hold, state);
	  debug("%p sur %s\n",e[i],load_manager[0].l->sched.name);
	  __rearrange_load_manager(load_manager, arity);
	}
    }
}

static int 
int_compar(const void *_e1, const void *_e2) 
{
  int *e1 = (int *) _e1;
  int *e2 = (int *) _e2;
  
  return *e2 - *e1;
}

static int
__has_enough_entities(struct marcel_topo_level **l, 
		      marcel_entity_t *e[], 
		      int ne, 
		      load_indicator_t *load_manager)
{
  int ret = 1, prev_state = 1;
  int nvp = marcel_vpset_weight(&l[0]->vpset);
  int arity = l[0]->arity;
  int per_item_entities = nvp / arity;
  int i, entities_per_level[arity];

  /* On regarde d'abord si il y a des entit�s d�j� plac�es sur les rq, 
   * et s'il y en a assez, auquel cas on renvoie 1 */
  for (i = 0; i < arity; i++)
    if (load_manager[i].load < per_item_entities)
      prev_state = 0;
  
  if (prev_state)
    return prev_state;

  for (i = 0; i < arity; i++)
    entities_per_level[i] = load_manager[i].load;

  qsort(entities_per_level, arity, sizeof(int), &int_compar);
  
  for (i = 0; i < ne; i++)
    {	
      int k, tmp;
      entities_per_level[0] += ma_entity_load(e[i]); 

      for (k = 0; k < arity - 1; k++)
	{
	  if (entities_per_level[k] > entities_per_level[k + 1])
	    {
	      /* Swap des deux �l�ments */
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
void __marcel_bubble_affinity(struct marcel_topo_level **l) 
{  
  int ne;
  int nvp = marcel_vpset_weight(&l[0]->vpset); // Nombre de processeurs � arroser
  int arity = l[0]->arity;
  int i, k;

  if (!arity)
    {
      debug("done: !arity\n");
      return;
    }
  
  debug("count dans __marcel_bubble__affinity\n");
  ne = ma_count_entities_on_rq(&l[0]->sched, ITERATIVE_MODE);
  debug("ne = %d on runqueue %p\n", ne, &l[0]->sched);  

  if (!ne)
    {
      for (k = 0; k < arity; k++)
	__marcel_bubble_affinity(&l[0]->children[k]);
      
      return;
    }

  debug("affinity a trouve %d entites a repartir\n", ne);
  
  load_indicator_t load_manager[arity];
  initialize_load_manager(load_manager, l);
  
  qsort(load_manager, arity, sizeof(load_manager[0]), &rq_load_compar);

  marcel_entity_t *e[ne];
  debug("get dans __marcel_bubble_affinity\n");
  ma_get_entities_from_rq(&l[0]->sched, e, ne);

  __debug_show_entities("__marcel_bubble_affinity", e, ne, l);

  if (ne < nvp) 
    {
      if (ne >= arity && __has_enough_entities(l, e, ne, load_manager))
	__distribute_entities(l, e, ne, load_manager);
      else
	{
	  /* Il faut vraiment percer des bulles, sous peine de famine */
	  debug("il faut vraiment percer des bulles, sous peine de famine.\n");
	  
	  qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);
	  
	  /* On commence par compter le nombre de sous-entit�s */
	  unsigned new_ne = 0;
	  int j;
	  int bubble_has_exploded = 0;
	  
	  for (i = 0; i < ne; i++) 
	    {
	      if (!e[i])
		continue;
	      
	      /* TODO: C'est ici qu'il faudra choisir la bulle a exploser, en fonction de son �paisseur ou autre */
	      if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) 
		{
		  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
		  marcel_entity_t *ee;
		  
		  if (bb->hold.nr_ready) /* Si la bulle n'est pas vide */ 
		    for_each_entity_scheduled_in_bubble_begin(ee,bb)
		      new_ne++;
		  bubble_has_exploded = 1; /* On a perc� une bulle, c'est peut-�tre suffisant! */
		  for_each_entity_scheduled_in_bubble_end()
		    
		    debug("counting: nr_ready: %ld, new_ne: %d\n", bb->hold.nr_ready, new_ne);
		}
	      else
		continue; /* On ne touche pas � ce qui est d�j� sur la runqueue */
	    }
	  
	  if (!bubble_has_exploded) /* On n'a rien trouv� � crever, pas la peine d'aller plus loin */
	    {
	      if (ne >= arity)
		{
		  __distribute_entities(l, e, ne, load_manager);
		  for (k = 0; k < arity; k++)
		    __marcel_bubble_affinity(&l[0]->children[k]);
		}
	    }
	  
	  if (!new_ne) /* Il n'y a pas d'entit�s, on s'arr�te */
	    {
	      debug( "done: !new_ne\n");
	      return;
	    }
	  
	  /* On cr�� la liste auxiliaire de toutes les entit�s */
	  marcel_entity_t *new_e[new_ne], *ee;
	  j = 0;
	  bubble_has_exploded = 0;
	  for (i = 0; i < ne; i++) 
	    {
	      /* TODO: L� encore �lire la bulle � exploser en fonction de son �paisseur, ou d'autres param�tres */
	      if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) 
		{
		  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
		  if (bb->hold.nr_ready) { /* Si la bulle n'est pas vide */
		    debug("exploding bubble %p\n", bb);
		    for_each_entity_scheduled_in_bubble_begin(ee,bb)
		      new_e[j++] = ee;
		    bubble_has_exploded = 1;
		    for_each_entity_scheduled_in_bubble_end()
		      }
		}
	      else
		continue; /* new_e ne contient que les entit�s extraites de la bulle �clat�e */
	    }
	  MA_BUG_ON(new_ne != j);
	  
	  __sched_submit(new_e, new_ne, l);
	  return __marcel_bubble_affinity(l);
	}
    }
  else /* ne >= nvp */ 
    {
      /* On retarde au maximum l'�clatement des bulles */
      debug("plus d'entites (%d) que de vps (%d), on retarde au maximum l'eclatement des bulles.\n", ne, nvp);
      __distribute_entities(l, e, ne, load_manager);
    }

  /* On poursuit la repartition sur les niveaux d'en dessous */
  for (i = 0; i < arity; i++)
    __marcel_bubble_affinity(&l[0]->children[i]);
}

void 
marcel_bubble_affinity(marcel_bubble_t *b, struct marcel_topo_level *l) 
{
  unsigned vp;
  marcel_entity_t *e = &b->sched;
  
  debug("adresse de la marcel_root_bubble: %p \n", &marcel_root_bubble);
  
  ma_bubble_synthesize_stats(b);
  ma_preempt_disable();
  ma_local_bh_disable();
  
  ma_bubble_lock_all(b, l);
  __ma_bubble_gather(b, b);
  __sched_submit(&e, 1, &l);
  __marcel_bubble_affinity(&l);
  ma_resched_existing_threads(l);
  marcel_vpset_foreach_begin(vp,&l->vpset)
    ma_lwp_t lwp = ma_vp_lwp[vp];
    ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
  marcel_vpset_foreach_end()
  ma_bubble_unlock_all(b, l);  

  ma_preempt_enable_no_resched();
  ma_local_bh_enable();
}

int
affinity_sched_init()
{
  INIT_LIST_HEAD(&submitted_entities);
  last_failed_steal = 0;
  succeeded_steals = 0;
  failed_steals = 0;
  return 0;
}

int
affinity_sched_exit()
{
  fprintf(stderr, "Succeeded steals : %lu, failed steals : %lu\n", succeeded_steals, failed_steals);
  return 0;
}

static marcel_bubble_t *b = &marcel_root_bubble;

int
affinity_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l =  marcel_topo_level(0,0);
  b = ma_bubble_entity(e);
  fprintf(stderr, "submit : %d\n", ma_atomic_read(&ma_init));
  if (!ma_atomic_read(&ma_init))
      marcel_bubble_affinity(b, l);
  else 
    __sched_submit(&e, 1, &l);

  ma_write_lock(&submit_rwlock);
  list_add_tail(&e->next, &submitted_entities);
  ma_write_unlock(&submit_rwlock);
  return 0;
}

marcel_entity_t *
ma_get_upper_ancestor(marcel_entity_t *e, ma_runqueue_t *rq)
{
  marcel_entity_t *upper_entity, *chosen_entity = e;
  int nvp = marcel_vpset_weight(&rq->vpset);

  for (upper_entity = e; 
       upper_entity->init_holder != NULL; 
       upper_entity = &ma_bubble_holder(upper_entity->init_holder)->sched) 
    {      
      if (upper_entity->sched_holder->type == MA_RUNQUEUE_HOLDER) 
	{
	  ma_runqueue_t *current_rq = ma_rq_holder(upper_entity->sched_holder);
	  if (current_rq == rq || marcel_vpset_weight(&current_rq->vpset) > nvp) 
	    break;
	}
      chosen_entity = upper_entity;
    }
  return chosen_entity;                                 
}

int
ma_redistribute(marcel_entity_t *e, ma_runqueue_t *common_rq)
{
  marcel_entity_t *upper_entity = ma_get_upper_ancestor(e, common_rq);
  debug("ma_redistribute : upper_entity = %p\n", upper_entity);
  if (upper_entity->type == MA_BUBBLE_ENTITY)
    {
      marcel_bubble_t *upper_bb = ma_bubble_entity(upper_entity);
      __ma_bubble_gather(upper_bb, upper_bb);
      int state = ma_get_entity(upper_entity);
      ma_put_entity(upper_entity, &common_rq->hold, state);
      struct marcel_topo_level *root_lvl = tbx_container_of(common_rq, struct marcel_topo_level, sched);
      __marcel_bubble_affinity(&root_lvl);

      return 1;
    }
  return 0;
}


ma_runqueue_t *get_parent_rq(marcel_entity_t *e)  
{
  if (e) {
    if (e->sched_holder && (e->sched_holder->type == MA_RUNQUEUE_HOLDER))
      return ma_to_rq_holder(e->sched_holder);
    
    /* Si on n'est pas sur une runqueue, et qu'on n'a pas de pere, pb... */
    MA_BUG_ON(!e->init_holder);

    marcel_entity_t *upentity;
    upentity = &ma_bubble_holder(e->init_holder)->sched;
    return get_parent_rq(upentity);
  }

  return NULL;
}

int
browse_bubble_and_steal(ma_holder_t *hold, unsigned from_vp)
{
  marcel_entity_t *bestbb = NULL;
  int greater = 0;
  int cpt = 0, available_threads = 0;
  marcel_entity_t *e; 
  struct marcel_topo_level *top = marcel_topo_level(0,0);

  list_for_each_entry(e, &hold->sched_list, sched_list) 
    {
      if (e->type == MA_BUBBLE_ENTITY) 
	{
	  long nbthreads = *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), ma_stats_nbthreads_offset);
	  long nbthreadseeds = *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), ma_stats_nbthreadseeds_offset);
      
	  if (nbthreads + nbthreadseeds > greater) 
	    {
	      greater = nbthreads + nbthreadseeds;
	      bestbb = e;
	    }
	  cpt++;
	}
      else
	available_threads++;
    }
  
  marcel_entity_t *stealable_threads[available_threads];
  if (!bestbb && available_threads)
    {
      int pos = 0;
      list_for_each_entry(e, &hold->sched_list, sched_list) 
	{
	  if (e->type != MA_BUBBLE_ENTITY)
	    {
	      stealable_threads[pos] = e;
	      pos++;
	    }
	} 
    }  

  ma_runqueue_t *common_rq = NULL;
  ma_runqueue_t *rq = NULL;
  if (bestbb)
    rq = get_parent_rq(bestbb);
  else if (available_threads)
    rq = get_parent_rq(stealable_threads[0]);

  if(rq)
    {
      PROF_EVENTSTR(sched_status, "stealing subtree");
      for (common_rq = rq->father; common_rq != &top->sched; common_rq = common_rq->father) 
	{
	  if (ma_rq_covers(common_rq, from_vp))
	    break;
	}
    }
 
  if (bestbb && cpt) 
    return ma_redistribute(bestbb, common_rq);
  else if (bestbb && !available_threads) 
    { 
      /* Parcourir la bulle */
      marcel_bubble_t *b = ma_bubble_entity(bestbb);
      return browse_bubble_and_steal(&b->hold, from_vp);
    }
  else if (available_threads)
    /* Voler des threads a la place */
    return ma_redistribute(stealable_threads[0], common_rq);
  
  return 0;
}

static int 
see(struct marcel_topo_level *level, 
    unsigned from_vp)
{
  ma_runqueue_t *rq = &level->sched;
  return browse_bubble_and_steal(&rq->hold, from_vp);
}

static int 
see_down(struct marcel_topo_level *level, 
	 struct marcel_topo_level *me, 
	 unsigned from_vp) 
{
  if (!level) {
    debug("down done");
    return 0;
  }

   int i = 0, n = level->arity;

  debug("see_down from %d %d\n", level->type, level->number);
  
  if (me) {
    /* si l'appelant fait partie des fils, l'�viter */
    n--;
    i = (me->index + 1) % level->arity;
  }
  
  /* Si on a des fils... */
  if (level->arity) {
    while (n--) {
      /* ... les examiner */
      if (see(level->children[i], from_vp) || see_down(level->children[i], NULL, from_vp))
	return 1;
      
      i = (i+1) % level->arity;
    }
  }
  
  debug("down done\n");
  
  return 0;
}

int 
see_up(struct marcel_topo_level *level, unsigned from_vp) 
{
  struct marcel_topo_level *father = level->father;

  debug("see_up from %d %d\n", level->type, level->number);

  if (!father)
    {
      debug("up done\n");
      return 0;
    }
  
  /* regarder vers le haut, c'est d'abord regarder en bas */
  if (see_down(father, level, from_vp))
    return 1;
  /* puis chez le p�re */
  else
    return see_up(father, from_vp);
}

/* D�place toutes les entit�s contenues sur chacunes des listes pr�sentes 
 * dans l'ensemble from vers la liste to, et renvoie le nombre d'entit�s 
 * d�plac�es */
int
move_entities(struct list_head *from, struct marcel_topo_level *to)
{
  ma_runqueue_t *rq;
  int ret = 0;

  list_for_each_entry(rq, from, next)
    {
      int nb = ma_count_entities_on_rq(rq, ITERATIVE_MODE);
      if (nb)
	{
	  marcel_entity_t *entities[nb];
	  ma_get_entities_from_rq(rq, entities, nb);
	  __sched_submit(entities, nb, &to);
	  ret += nb;
	}
    }
  
  return ret;
}

int
affinity_steal(unsigned from_vp)
{
  struct marcel_topo_level *me = &marcel_topo_vp_level[from_vp];
  struct marcel_topo_level *father = me->father;
  struct marcel_topo_level *top = marcel_topo_level(0,0);
  int arity;

  int n, smthg_to_steal = 0;
  int nvp = marcel_vpset_weight(&top->vpset);
  
  /* Si le dernier steal a echou� trop r�cemment, on attend un certain
     laps de temps avant de retenter */
  if (last_failed_steal 
      && ((marcel_clock() - last_failed_steal) < MA_VP_IS_IDLE_COOLDOWN))
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
  
  debug("===[Le processeur %u est idle]===\n", from_vp);

  if (father)
    arity = father->arity;

  ma_preempt_disable();
  ma_local_bh_disable();  
  ma_bubble_synthesize_stats(&marcel_root_bubble);
  ma_bubble_lock_all(&marcel_root_bubble, marcel_topo_level(0,0));
  smthg_to_steal = see_up(me, from_vp);
  ma_bubble_unlock_all(&marcel_root_bubble, marcel_topo_level(0,0));
  ma_resched_existing_threads(me);    
  ma_preempt_enable_no_resched();
  ma_local_bh_enable();
  ma_idle_scheduler = 1;
  ma_write_unlock(&ma_idle_scheduler_lock);
  
  if (smthg_to_steal) { 
    debug("Le vol a reussi !\n");
    succeeded_steals++;
    return 1;
  }

  last_failed_steal = marcel_clock();
  debug("Le vol a echoue !\n");
  failed_steals++;
  return 0;
}

struct ma_bubble_sched_struct marcel_bubble_affinity_sched = {
  .init = affinity_sched_init,
  .exit = affinity_sched_exit,
  .submit = affinity_sched_submit,
  //.vp_is_idle = affinity_steal,
};

#endif /* MA__BUBBLES */
