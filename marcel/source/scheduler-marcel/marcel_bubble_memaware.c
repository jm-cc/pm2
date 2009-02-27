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

/*
 *
 * Algorithme MemAware
 *
 */

#ifdef MA__BUBBLES
#ifdef MA__NUMA_MEMORY

int ma_bubble_memaware_checkload = 1;
int ma_bubble_memaware_nodelevel = -1;
static int ma_bubble_memaware_vplevel = -1;
static marcel_entity_t * ma_bubble_memaware_to_spread[256];
static int ma_bubble_memaware_num_to_spread = 0;
static ma_spinlock_t ma_bubble_memaware_to_spread_lock = MA_SPIN_LOCK_UNLOCKED;
/* Nombre de vols, spreads et riens */
static int ma_bubble_memaware_nbsteals = 0;
static int ma_bubble_memaware_nbspreads = 0;
static int ma_bubble_memaware_cspread = 0;
static int ma_bubble_memaware_nbnots = 0;
static unsigned long ma_bubble_memaware_lastmix[32];
static ma_spinlock_t ma_bubble_memaware_remix_lock = MA_SPIN_LOCK_UNLOCKED;
static int ma_bubble_memaware_want[MA_NR_LWPS];

/* Regarde si une entite est deja contenue dans une autre */
#ifdef PM2DEBUG
static int ma_check_inentity(marcel_entity_t *entity, marcel_entity_t *inentity)
{
  marcel_entity_t *downentity;

  if (inentity == entity)
    return 1;
  if (inentity->type != MA_BUBBLE_ENTITY)
    return 0;
  else {
    MA_BUG_ON(inentity->type != MA_BUBBLE_ENTITY);
    for_each_entity_scheduled_in_bubble_begin(downentity,ma_bubble_entity(inentity)) {
      if (ma_check_inentity(entity, downentity))
        return 1;
    }
    for_each_entity_scheduled_in_bubble_end();
  }
  return 0;
}
#endif

/* Place une entite dans un tableau de spread et fait attention que l'entite
   n'est pas deje  presente quelquepart au sein d'une autre entite */
void ma_put_in_spread(marcel_entity_t *entity, struct marcel_topo_level *level)
{
  ma_spin_lock(&ma_bubble_memaware_to_spread_lock);

#ifdef PM2DEBUG
  int i, already=0;
  for (i = 0 ; i < ma_bubble_memaware_num_to_spread ; i++) {
    already = ma_check_inentity(entity, ma_bubble_memaware_to_spread[i]);
    MA_BUG_ON(already);
  }
  if (already)
#endif
    {
      ma_bubble_memaware_to_spread[ma_bubble_memaware_num_to_spread] = entity;
      ma_bubble_memaware_num_to_spread++;
      if (level)
        ma_stats_set (long, entity, ma_stats_last_vp_offset, level->number * marcel_vpset_weight(&level->vpset));
    }
  ma_spin_unlock(&ma_bubble_memaware_to_spread_lock);
}

static void ma_clear_spread(void)
{
  int i;
  ma_spin_lock(&ma_bubble_memaware_to_spread_lock);
  for (i = 0 ; i < ma_bubble_memaware_num_to_spread ; i++) {
    if (ma_bubble_memaware_to_spread[i]->type == MA_BUBBLE_ENTITY)
      ma_bubble_unlock_all_but_levels(ma_bubble_entity(ma_bubble_memaware_to_spread[i]));
  }
  ma_bubble_memaware_num_to_spread = 0;
  ma_spin_unlock(&ma_bubble_memaware_to_spread_lock);
}

/* Permet de remonter les entites d'un niveau ainsi que des sous-niveau de celui-ci. *
 * On place aussi les entites remontees dans un tableau de SPREAD */
static void ma_lifton_entities(struct marcel_topo_level *where, struct marcel_topo_level *level, int recurse)
{
  ma_runqueue_t *rqlevel = &level->rq;
  int i = 0, n = level->arity;
  int j = 0;
  marcel_entity_t *entity;

  /* on regarde ce qui s'execute sur le niveau */
  list_for_each_entry(entity, &rqlevel->as_holder.sched_list, sched_list) {
    /* on rassemble la bulle */
    if (entity->type == MA_BUBBLE_ENTITY && entity != &marcel_root_bubble.as_entity) {
      ma_bubble_lock_all_but_levels(ma_bubble_entity(entity));
      __ma_bubble_gather(ma_bubble_entity(entity), ma_bubble_entity(entity));
    }
    ma_put_in_spread(entity, level);
  }
  j = 0;
  /* parcours sur chaque fils */
  while (n--) {
    ma_lifton_entities(where, level->children[i], recurse + 1);
    i = (i+1) % level->arity;
  }
}

/******************** Voir si équilibré **********************/

/* Compter les entités dans une bulle */
static void ma_count_in_bubble(marcel_bubble_t *bubble, int *number)
{
  marcel_entity_t *downentity;

  //  bubble_sched_debug("bubble entity %p\n", &bubble->sched);
  ma_holder_lock(&bubble->as_holder);
  for_each_entity_scheduled_in_bubble_begin(downentity,bubble) {
    if (downentity->type == MA_BUBBLE_ENTITY)
      ma_count_in_bubble(ma_bubble_entity(downentity),number);
    else {
      bubble_sched_debug("task entity %p\n", downentity);
      (*number) ++;
    }
  }
  for_each_entity_scheduled_in_bubble_end();
  ma_holder_unlock(&bubble->as_holder);
}

/* Travail équilibré sous un niveau */
static void ma_work_is_balanced_down(struct marcel_topo_level *me, int *number, int *sum)
{
  /* Calcul du nombre d'entités sur me */
  ma_runqueue_t *rq =&me->rq;
  ma_holder_t *h = &rq->as_holder;
  marcel_entity_t *e;

  bubble_sched_debug("down me %p\n",me);
  ma_holder_rawlock(h);
  list_for_each_entry(e, &h->sched_list, sched_list) {
    /* charge totale */
    if (e->type == MA_BUBBLE_ENTITY)
      ma_bubble_synthesize_stats(ma_bubble_entity(e));
    (*sum) += (ma_bubble_memaware_checkload ? ma_entity_load(e) : MA_DEFAULT_LOAD *  ma_count_threads_in_entity(e));
    /* nb threads */
    if (e->type == MA_BUBBLE_ENTITY) {
      ma_count_in_bubble(ma_bubble_entity(e),number);
    }
    else {
      (*number) ++;
    }
  }
  ma_holder_rawunlock(h);
  /* Plus le nombre d'entités en dessous */
  if (me->arity) {
    int i;
    for (i = 0 ; i < me->arity ; i++)
      ma_work_is_balanced_down(me->children[i], number, sum);
  }
}

/* Rapport de charge entre voleur et vole */
#define MA_MORE_LOAD 8
/* Difference de threads */
#define MA_MORE_THREADS 8

/* Test d'équilibrage */
static int ma_work_is_balanced_up(int vp, struct marcel_topo_level *me, int menumber, int mesum, struct marcel_topo_level **uplevel)
{
  struct marcel_topo_level *father = me->father;
  int nbthreads;
  int i=0,n=father->arity;
  int sum = 0;
  int number = 0;
  int meweight = marcel_vpset_weight(&me->vpset);
  int weight = 0;

  bubble_sched_debug("vp %d, up me %p, menumber %d\n",vp,me,menumber);
  marcel_threadslist(0, NULL, &nbthreads, NOT_BLOCKED_ONLY);
  //int nvp = marcel_vpset_weight(&marcel_topo_level(0,0)->vpset);
  //marcel_fprintf(stderr,"nbthreads %d, nvp %d\n", nbthreads, nvp);
  //float average = (float)nbthreads/nvp;

  if (!father)
    return 1;

  if (me) {
    /* si l'appelant fait partie des fils, l'éviter */
    n--;
    i = (me->index + 1) % father->arity;
  }

  while (n--) {
    /* on regarde récursivement dans les frères */
    ma_work_is_balanced_down(father->children[i],&number,&sum);
    /* on change de frère */
    i = (i+1) % father->arity;
    weight += marcel_vpset_weight(&father->children[i]->vpset);
  }

  bubble_sched_debug("ma_work_ibup : father %p, menumber %d, mesum %d, number %d, sum %d\n",
                     father, menumber, mesum, number, sum);
  /* Test de l'équilibrage : difference de threads + rapport de charge */

  if (MA_MORE_THREADS + menumber < number
      && MA_MORE_THREADS * menumber * weight< number * meweight
      && weight * MA_MORE_LOAD * mesum < sum * meweight) {
    /* Ce n'est pas équilibré, respreader ici */
    *uplevel = father;
    return 0;
  }
  else {
    /* on regarde si c'est équilibré plus haut */
    return ma_work_is_balanced_up(vp, father, menumber + number, mesum + sum, uplevel);
  }
}

/************************************************************/

/* Nombre de threads par rapport aux nombre de vps pour voler */
#define MA_RATIO_THREADS_VPS 1.2

/* Fonction de redistribution du travail */
static int __memaware(unsigned vp) {
  int gotwork = 0;
  struct marcel_topo_level *me = &marcel_topo_vp_level[vp];

  ma_spin_lock(&ma_idle_scheduler_lock);
  if (ma_atomic_read (&ma_idle_scheduler)) {
    /* On teste ici l'equilibrage pour savoir
       si on vole ou si on respread localement */
    struct marcel_topo_level *uplevel = NULL;
    int balanced = ma_work_is_balanced_up(vp, me, 0, 0, &uplevel);

    if (!balanced && !ma_bubble_memaware_cspread) {
      bubble_sched_debug("spread\n");
      ma_bubble_memaware_nbspreads++;
      ma_topo_lock_levels(uplevel);

      /* On remonte les entités, on les verrouille et on respread */
      ma_lifton_entities(uplevel, uplevel, 0);
      marcel_bubble_mspread_entities(ma_bubble_memaware_to_spread, ma_bubble_memaware_num_to_spread, &uplevel, 1);

      ma_clear_spread();
      ma_topo_unlock_levels(uplevel);

      ma_bubble_memaware_cspread = 3;
      gotwork = 1;
    }
    else { /* Vol de travail */
      int allthreads, nvp;

      ma_bubble_memaware_cspread --;
      bubble_sched_debug("steal\n");
      nvp = marcel_vpset_weight(&marcel_topo_level(0,0)->vpset);
      marcel_threadslist(0, NULL, &allthreads, NOT_BLOCKED_ONLY);

      if (MA_RATIO_THREADS_VPS * allthreads >= nvp) {
        ma_bubble_memaware_nbsteals ++;
        bubble_sched_debug("msteal numero %d\n", ma_bubble_memaware_nbsteals);
        gotwork = marcel_bubble_msteal_see_up(me);
      }
      else {
        ma_bubble_memaware_nbnots ++;
        bubble_sched_debug("no mspread, no msteal : %d\n", ma_bubble_memaware_nbnots);
      }
    }
  }
  ma_spin_unlock(&ma_idle_scheduler_lock);
  return gotwork;
}

/****************************************************/
int marcel_start_idle_memaware(void)
{
  ma_activate_idle_scheduler ();
  return 0;
}

int marcel_stop_idle_memaware(void)
{
  ma_deactivate_idle_scheduler ();
  return 0;
}

/* Permet de quantifier les charges */
void marcel_start_checkload(void)
{
  ma_bubble_memaware_checkload = 1;
}

void marcel_stop_checkload(void)
{
  ma_bubble_memaware_checkload = 0;
}

/* gerer un timer pour chaque vp */
#define MA_TIME_REMIX_AGAIN 50
/* timer en fonction du nombre de non-vol */
#define MA_COEF_NOT 0.2

/* La fonction qui decide si on redistribue du travail */
static int memaware(marcel_bubble_sched_t *self, unsigned vp)
{
  _ma_raw_spin_lock(&ma_bubble_memaware_remix_lock);

  if (!ma_bubble_memaware_want[vp]) {
    /* First try, or someone did the job for us, abort */
    ma_bubble_memaware_want[vp] = 1;
    _ma_raw_spin_unlock(&ma_bubble_memaware_remix_lock);
    return 1;
  }

  /* Si non, on lance mix */
  unsigned long now = marcel_clock();

  int vpnum = (int)vp;
  if (now - ma_bubble_memaware_lastmix[vpnum] > (MA_COEF_NOT * ma_bubble_memaware_nbnots + 1)*MA_TIME_REMIX_AGAIN) {
    if (!__memaware(vp)) {
      /* pas de travail au moment start */
      ma_bubble_memaware_lastmix[vpnum] = marcel_clock();
    }
  }
  else {
    /* faire d'autre chose pendant l'attente
       car cherche trop souvent du travail */
  }
  /* tell others that we did the job */
  memset(ma_bubble_memaware_want, 0, marcel_nbvps()*sizeof(*ma_bubble_memaware_want));
  _ma_raw_spin_unlock(&ma_bubble_memaware_remix_lock);
  return 1;
}

/*************************** Fonctions de debut **************************/

/* Debogage : permet de lister la structure de bulles */
void marcel_see_bubble(marcel_bubble_t *bubble, int recurse, int number)
{
  marcel_entity_t *downentity;
  int downnumber = 0;

  bubble_sched_debug("entity %d %d -> %p\n", recurse, number, &bubble->as_entity);
  for_each_entity_scheduled_in_bubble_begin(downentity,bubble) {
    if (downentity->type == MA_BUBBLE_ENTITY)
      marcel_see_bubble(ma_bubble_entity(downentity), recurse + 1, downnumber++);
    else
      bubble_sched_debug("entity %d %d -> %p\n", recurse + 1, downnumber++, downentity);
  }
  for_each_entity_scheduled_in_bubble_end();
}

#ifdef PM2DEBUG
/* Debuggage : permet de lister les levels */
static void ma_see_level(struct marcel_topo_level *level, int recurse)
{
  bubble_sched_debug("level %d %d -> %p\n", recurse, level->number, level);
  int i;
  for ( i = 0 ; i < level->arity ; i++) {
    ma_see_level(level->children[i],recurse + 1);
  }
}
#endif /* PM2DEBUG */

static int memaware_sched_submit(marcel_bubble_sched_t *self, marcel_entity_t *e)
{
  bubble_sched_debug("memaware_sched_submit\n");
  marcel_stop_idle_memaware();
#ifdef PM2DEBUG
  ma_see_level(marcel_topo_level(0,0),0);
#endif
  //marcel_see_bubble(ma_bubble_entity(e), 0, 1);
  if (marcel_topo_node_level != NULL)
    ma_bubble_memaware_nodelevel = marcel_topo_node_level[0].level;
  else
    ma_bubble_memaware_nodelevel = -1;
  if (marcel_topo_vp_level != NULL)
    ma_bubble_memaware_vplevel = marcel_topo_vp_level[0].level;
  else
    ma_bubble_memaware_vplevel = 100;
  marcel_bubble_mspread(ma_bubble_entity(e),marcel_topo_level(0,0));
  //marcel_start_idle_memaware();
  return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER (memaware,
  .submit = memaware_sched_submit,
  .vp_is_idle = memaware,
);

#else /* !MA__NUMA_MEMORY */

/* Define an empty scheduler.  */
MARCEL_DEFINE_BUBBLE_SCHEDULER (memaware);

#endif /* !MA__NUMA_MEMORY */
#endif /* BUBBLES */
