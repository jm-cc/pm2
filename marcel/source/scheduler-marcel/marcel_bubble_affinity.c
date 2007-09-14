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

#ifdef MA__BUBBLES

#if 1
#define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#else
#define debug(fmt, ...) (void)0
#endif

static struct list_head submitted_entities;
static ma_rwlock_t submit_rwlock = MA_RW_LOCK_UNLOCKED;

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
	  marcel_task_t *t = ma_task_entity(e[k]);
	  debug("seed, ");
	}
      else 
	debug("ovni!, ");
    }
  debug("end)\n");
}

/* Soumission d'un ensemble d'entités sur un topo_level */
static void 
__sched_submit(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;

  __debug_show_entities("__sched_submit", e, ne, l);
  
  for (i = 0; i < ne; i++) 
    {	      
      int state = ma_get_entity(e[i]);
      ma_put_entity(e[i], &l[0]->sched.hold, state);
    }
}

/* Ressemble à __sched_submit, mais avec des modifs pour le vol de travail */
static void 
__sched_submit_to_upper_level(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;
  int nb_running = 0;
  int running = 0;
  marcel_entity_t *ee;

  if (ne == 1)
    {
      if ((e[0]->type == MA_BUBBLE_ENTITY && (((ma_bubble_entity(e[0]))->hold.nr_ready) < l[0]->arity))
	  || (e[0]->type != MA_BUBBLE_ENTITY))
	return;
    }

  __debug_show_entities("__sched_submit_to_upper_level", e, ne, l);
  
  for (i = 0; i < ne; i++) 
    {	
      int state = ma_get_entity(e[i]);
      ma_put_entity(e[i], &l[0]->sched.hold, state);
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
	ret += ma_count_all_entities_on_rq(&father[0]->children[i]->sched);
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

/* Distribution d'un ensemble d'entités en fonction de la charge des niveaux d'en dessous */
static void __distribute_entities(struct marcel_topo_level **l, marcel_entity_t *e[], int ne, load_indicator_t *load_manager) 
{                      
  int i;    
  int arity = l[0]->arity;
  
  for (i = 0; i < ne; i++)                         
    {
      if (e[i]->type == MA_BUBBLE_ENTITY)                     
        {
          marcel_bubble_t *b = ma_bubble_entity(e[i]);
          if (!b->hold.nr_ready) /* On ne prend pas de bulle vide */         
            continue;                     
        }
 
      int state = ma_get_entity(e[i]);
      int nbthreads = ma_entity_load(e[i]);
      
      load_manager[0].load += nbthreads;
      ma_put_entity(e[i], &load_manager[0].l->sched.hold, state);
      /* BEURK! Tri par insertion à la place */ 
      qsort(load_manager, arity, sizeof(load_manager[0]), &rq_load_compar);                         
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
  int ret = 1;
  int nvp = marcel_vpmask_weight(&l[0]->vpset);
  int arity = l[0]->arity;
  int per_item_entities = nvp / arity;
  int i, entities_per_level[arity];

  /* On regarde d'abord si il y a des entités déjà placées sur les rq, 
   * et s'il y en a assez, auquel cas on renvoie 1 */
  for (i = 0; i < arity; i++)
    if (load_manager[i].load < per_item_entities)
      ret = 0;
  
  if (ret)
    return ret;

  for (i = 0; i < arity; i++)
    entities_per_level[i] = load_manager[i].load;

  qsort(entities_per_level, arity, sizeof(int), &int_compar);
  
  for (i = 0; i < ne; i++)
    {	
      entities_per_level[0] += ma_entity_load(e[i]); 
      /* TODO: Tri par insertion boudiou ! */
      qsort(entities_per_level, arity, sizeof(int), &int_compar);
    } 

  for (i = 0; i < arity; i++)
    if (entities_per_level[i] < per_item_entities)
      ret = 0;
  
  return ret;
}

static void 
__marcel_bubble_affinity(struct marcel_topo_level **l) 
{  
  int ne;
  int nvp = marcel_vpmask_weight(&l[0]->vpset); // Nombre de processeurs à arroser
  int arity = l[0]->arity;
  int k;

  if (!arity)
    {
      debug("done: !arity\n");
      return;
    }
  
  ne = ma_count_entities_on_rq(&l[0]->sched);
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
  ma_get_entities_from_rq(&l[0]->sched, e);

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
	  
	  /* On commence par compter le nombre de sous-entités */
	  unsigned new_ne = 0;
	  int i, j;
	  int bubble_has_exploded = 0;
	  int nb = 0;
	  for (i = 0; i < ne; i++) 
	    {
	      /* TODO: C'est ici qu'il faudra choisir la bulle a exploser, en fonction de son épaisseur ou autre */
	      if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) 
		{
		  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
		  marcel_entity_t *ee;
		  
		  if (bb->hold.nr_ready) /* Si la bulle n'est pas vide */ 
		    for_each_entity_scheduled_in_bubble_begin(ee,bb)
		      new_ne++;
		      bubble_has_exploded = 1; /* On a percé une bulle, c'est peut-être suffisant! */
		    for_each_entity_scheduled_in_bubble_end()
		           
		    debug("counting: nr_ready: %ld, new_ne: %d\n", bb->hold.nr_ready, new_ne);
		}
	      else
		continue; /* On ne touche pas à ce qui est déjà sur la runqueue */
	    }

	  if (!bubble_has_exploded) /* On n'a rien trouvé à crever, pas la peine d'aller plus loin */
	    {
	      debug( "done: !bubble_has_exploded\n");
	      return;
	    }

	  if (!new_ne) /* Il n'y a pas d'entités, on s'arrête */
	    {
	      debug( "done: !new_ne\n");
	      return;
	    }
      
	  /* On créé la liste auxiliaire de toutes les entités */
	  marcel_entity_t *new_e[new_ne], *ee;
	  j = 0;
	  bubble_has_exploded = 0;
	  for (i = 0; i < ne; i++) 
	    {
	      /* TODO: Là encore élire la bulle à exploser en fonction de son épaisseur, ou d'autres paramètres */
	      if (e[i]->type == MA_BUBBLE_ENTITY && !bubble_has_exploded) 
		{
		  marcel_bubble_t *bb = ma_bubble_entity(e[i]);
		  if (bb->hold.nr_ready) /* Si la bulle n'est pas vide */
		    for_each_entity_scheduled_in_bubble_begin(ee,bb)
		      new_e[j++] = ee;
		      bubble_has_exploded = 1;
		    for_each_entity_scheduled_in_bubble_end()
		}
	      else
		continue; /* new_e ne contient que les entités extraites de la bulle éclatée */
	    }
  
	  __sched_submit(new_e, new_ne, l);
	  return __marcel_bubble_affinity(l);
	}
    }
  else /* ne > nvp */ 
    {
      /* On retarde au maximum l'éclatement des bulles */
      debug("plus d'entites que de vps, on retarde au maximum l'eclatement des bulles.\n");
      __distribute_entities(l, e, ne, load_manager);
    }
  
  if (arity)
    {
      int i;
      
      for (i = 0; i < arity; i++)
	__marcel_bubble_affinity(&l[0]->children[i]);
    }
}

void 
marcel_bubble_affinity(marcel_bubble_t *b, struct marcel_topo_level *l) 
{
  unsigned vp;
  marcel_entity_t *e = &b->sched;

  debug("adresse de la marcel_root_bubble: %p \n", &marcel_root_bubble);
 
  marcel_bubble_deactivate_idle_scheduler();

  ma_bubble_synthesize_stats(b);
  ma_preempt_disable();
  ma_local_bh_disable();

  ma_bubble_lock_all(b, l);
  __ma_bubble_gather(b, b);
  __sched_submit(&e, 1, &l);
  __marcel_bubble_affinity(&l);
  
  /* resched existing threads */
  marcel_vpmask_foreach_begin(vp,&l->vpset)
    ma_lwp_t lwp = ma_vp_lwp[vp];
  ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
  marcel_vpmask_foreach_end()
  
  ma_bubble_unlock_all(b, l);  

  ma_preempt_enable_no_resched();
  ma_local_bh_enable();
  
  marcel_bubble_activate_idle_scheduler();
}

int
affinity_sched_init()
{
  INIT_LIST_HEAD(&submitted_entities);
  return 0;
}

int
affinity_sched_exit()
{
  return 0;
}

int
affinity_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  debug("===========[repartition avec affinity]===========\n");
  marcel_bubble_affinity(ma_bubble_entity(e), l);

  ma_write_lock(&submit_rwlock);
  list_add_tail(&e->next, &submitted_entities);
  ma_write_unlock(&submit_rwlock);

  return 0;
}

static int 
see(struct marcel_topo_level *level, int up_power, struct marcel_topo_level *registered_level)
{
  ma_runqueue_t *rq = &level->sched;
  struct marcel_topo_level *father = level->father;
  struct marcel_topo_level *top = marcel_topo_level(0,0);

  int allthreads, nvp;
  int ents = ma_count_all_entities_on_rq(rq);
  int nbentities = ma_count_entities_on_rq(rq);

  if ((registered_level && (father->level < registered_level->level))
      || !registered_level)
    registered_level = father;

  nvp = marcel_vpmask_weight(&father->vpset);
  marcel_threadslist(0, NULL, &allthreads, 0);

  if (!nbentities)
    return 0;

  marcel_entity_t *e[nbentities];
  ma_get_entities_from_rq(rq, e);
  __sched_submit_to_upper_level(e, nbentities, &registered_level);
  //__sched_submit(e, nbentities, &registered_level);

  if (ents < nvp)
    return 0;
  else
    return 1;
}

static int 
see_down(struct marcel_topo_level *level, 
	 struct marcel_topo_level *me, 
	 struct marcel_topo_level *registered_level) 
{
  ma_runqueue_t *rq = &level->sched;
  int power = 0;
  int i = 0, n = level->arity;
  int found = 0;

  //struct marcel_topo_level *top = marcel_topo_level(0,0);
  //if (me)
  //if (me->father == top) 
  //  return 0;
      
  debug("see_down from %d %d\n", level->type, level->number);
  
  if (rq)
    power = marcel_vpmask_weight(&rq->vpset);
  
  if (me) 
    {
      /* si l'appelant fait partie des fils, l'éviter */
      n--;
      i = (me->index + 1) % level->arity;
    }
  
  while (n--) 
    {
      /* examiner les fils */
      if (see(level->children[i], power, registered_level) || see_down(level->children[i], NULL, registered_level))
	found = 1;
      
      i = (i+1) % level->arity;
    }
  
  debug("down done\n");
  
  if (found)
    return 1;
  else
    return 0;
}

static struct marcel_topo_level * 
see_up(struct marcel_topo_level *level) 
{
  struct marcel_topo_level *father = level->father;
  struct marcel_topo_level *registered_level = father;

  debug("see_up from %d %d\n", level->type, level->number);
  if (!father)
    return NULL;

  ma_gather_all_bubbles_on_rq(&father->sched);
  
  /* regarder vers le haut, c'est d'abord regarder en bas */
  if (see_down(father, level, registered_level))
    if (registered_level)
      return registered_level;
  
  /* puis chez le père */
  registered_level = see_up(father);
  if (registered_level)
    return registered_level;
    
  debug("up done\n");
  return NULL;
}

static int
__ma_rearrange_entities(struct marcel_topo_level *from)
{
  struct marcel_topo_level *registered_level = see_up(from);
  
  if (registered_level)
    {  
      __marcel_bubble_affinity(&registered_level);
      registered_level = NULL;
      
      return 1;
    }

  return 0;
}

static int
__ma_local_steal_entities(struct marcel_topo_level *empty_level)
{
  struct marcel_topo_level *father = empty_level->father;
  if (!father) return 0;
  int arity = father->arity;
  int i, k;

  ma_runqueue_t *empty_level_rq = &empty_level->sched;

  for (i = 0; i < arity; i++)
    {
      struct marcel_topo_level *current = father->children[i];
      if (current == empty_level)
	continue;
	
      ma_runqueue_t *rq = &current->sched;
      int nbentities = ma_count_entities_on_rq(rq);
      
      if (nbentities > 1)
	{
	  marcel_entity_t *e[nbentities];
	  ma_get_entities_from_rq(rq, e);
	  
	  for (k = 0; k < nbentities; k++)
	    {
	      if (e[k]->type == MA_BUBBLE_ENTITY)
		{
		  marcel_bubble_t *b = ma_bubble_entity(e[k]);
		  marcel_entity_t *ee;
		  int running = 0;
		  
		  if (!ma_bubble_hold_stats_get(b, ma_stats_nbrunning_offset))
		    {
		      int state = ma_get_entity(e[k]);
		      ma_put_entity(e[k], &empty_level_rq->hold, state);
		      
		      return 1;
		    }
		}
	    }
	}
    }
  
  return 0;
}

int
affinity_sched_vp_is_idle(unsigned vp)
{
  struct marcel_topo_level *me = &marcel_topo_vp_level[vp];
  struct marcel_topo_level *top = marcel_topo_level(0,0);
  marcel_entity_t *e;
  debug("========[vp idle : %u]========\n", vp);

  ma_read_lock(&submit_rwlock);
  list_for_each_entry(e, &submitted_entities, next)
    {
      if (e->type == MA_BUBBLE_ENTITY)
	{
	  marcel_bubble_t *b = ma_bubble_entity(e);
	  debug("synthesize de la bulle %p\n", b);
	  ma_bubble_synthesize_stats(b);
	}
    }
  ma_read_unlock(&submit_rwlock);

  marcel_bubble_deactivate_idle_scheduler();
  ma_bubble_lock_all(&marcel_root_bubble, top);

  /* Vol -furtif- de bulle sur les niveaux frères */
  if (__ma_local_steal_entities(me))
    {
      debug("le vol furtif a fonctionne!\n");
      ma_bubble_unlock_all(&marcel_root_bubble, top);
      marcel_bubble_activate_idle_scheduler();

      return 1;
    }
  /* Remontée des entités à la racine d'un sous-arbre pour re-répartition */
  else if (__ma_rearrange_entities(me))
    {
      debug("le vol furtif n'a pas suffit, il a fallu repartir a nouveau\n");
      ma_bubble_unlock_all(&marcel_root_bubble, top);
      marcel_bubble_activate_idle_scheduler();

      return 1;
    }
  
  debug("aucun des deux vols n'a fonctionne\n");
  ma_bubble_unlock_all(&marcel_root_bubble, top);
  marcel_bubble_activate_idle_scheduler();

  return 0;
}

struct ma_bubble_sched_struct marcel_bubble_affinity_sched = {
  .init = affinity_sched_init,
  .exit = affinity_sched_exit,
  .submit = affinity_sched_submit,
  .vp_is_idle = affinity_sched_vp_is_idle,
  //.vp_is_idle = NULL,
};

#endif /* MA__BUBBLES */
