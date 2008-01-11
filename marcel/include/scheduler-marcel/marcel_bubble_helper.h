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

#section marcel_functions
#depend "scheduler/marcel_holder.h[types]"
/* Calcule récursivement la charge d'une entité. Nécessite un appel préalable
   à la méthode ma_bubble_synthesize_stats(). */
long ma_entity_load(marcel_entity_t *);

/* Gestion de ma_idle_scheduler */
void marcel_bubble_activate_idle_scheduler();
void marcel_bubble_deactivate_idle_scheduler();

/* Fontions de comparaison de charges */
int ma_decreasing_order_entity_load_compar(const void *_e1, const void *_e2);
int ma_increasing_order_entity_load_compar(const void *_e1, const void *_e2);

/* Gestion des entités sur une runqueue */
int ma_count_entities_on_rq(ma_runqueue_t *rq);
int ma_count_all_entities_on_rq(ma_runqueue_t *rq);
void ma_get_entities_from_rq(ma_runqueue_t *rq, marcel_entity_t *e[]);
int ma_gather_all_bubbles_on_rq(ma_runqueue_t *rq);

#section marcel_macros
#define for_each_entity_scheduled_in_bubble_begin(e,b) \
	list_for_each_entry(e,&b->heldentities, bubble_entity_list) { \
		if (e->sched_holder->type == MA_BUBBLE_HOLDER) {
		/* This entity is scheduled in a bubble, so that must be this bubble */

#define for_each_entity_scheduled_in_bubble_end() \
		} \
	}
