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

#section marcel_types
enum lock_mode { UNLOCKED_MODE, LOCKED_MODE };
enum counting_mode { ITERATIVE_MODE, RECURSIVE_MODE };

#section marcel_functions
#depend "scheduler/marcel_holder.h[types]"
/* Calcule récursivement la charge d'une entité. Nécessite un appel préalable
   à la méthode ma_bubble_synthesize_stats(). */
long ma_entity_load(marcel_entity_t *);

/* Renvoie 1 si l'entité considérée est une graine ou une bulle ne contenant 
   que des graines */
unsigned ma_is_a_seed(marcel_entity_t *);

/* Gestion de ma_idle_scheduler */
void marcel_bubble_activate_idle_scheduler();
void marcel_bubble_deactivate_idle_scheduler();

/* Fontions de comparaison de charges */
int ma_decreasing_order_entity_load_compar(const void *_e1, const void *_e2);
int ma_increasing_order_entity_load_compar(const void *_e1, const void *_e2);

/* Fontions de comparaison d'affinity */
int decreasing_order_entity_attraction_compar(const void *_e1, const void *_e2);
int increasing_order_entity_attraction_compar(const void *_e1, const void *_e2);

/* Fonctions de comparaison des deux */
int decreasing_order_entity_both_compar(const void *_e1, const void *_e2);
int increasing_order_entity_both_compar(const void *_e1, const void *_e2);

/* Fonctions du nombre de threads */
int ma_decreasing_order_threads_compar(const void *_e1, const void *_e2);
int ma_increasing_order_threads_compar(const void *_e1, const void *_e2);

/* Gestion des entités sur une runqueue */
int ma_count_entities_on_rq(ma_runqueue_t *rq, enum counting_mode recursive);
/* \e rq must be already locked because else entities may die under your hand.  */
int ma_get_entities_from_rq(ma_runqueue_t *rq, marcel_entity_t *e[], int ne);
int ma_gather_all_bubbles_on_rq(ma_runqueue_t *rq);
void ma_resched_existing_threads(struct marcel_topo_level *l);
int ma_count_threads_in_entity(marcel_entity_t *entity);

#section marcel_macros

/* Itère sur toutes les entités qui sont ordonnancées directement dans cette bubble */
#define for_each_entity_scheduled_in_bubble_begin(e,b) \
	list_for_each_entry(e,&b->heldentities, bubble_entity_list) { \
      if(e->sched_holder)\
        if (e->sched_holder->type == MA_BUBBLE_HOLDER) { \
           /* scheduling holder of e is a bubble, that must be a ancestry of b */

#define for_each_entity_scheduled_in_bubble_end() \
      } \
   }

/* Itère sur toutes les entités contenues dans cette bubble */
#define for_each_entity_held_in_bubble(e,b) \
   list_for_each_entry(e,&b->heldentities, bubble_entity_list)

/* Iterates over every entity scheduled on the runqueue r */
#define for_each_entity_scheduled_on_runqueue(e,r) \
  list_for_each_entry(e, &rq->hold.sched_list, sched_list)
