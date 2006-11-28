
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

#define switchrq(e,rq) \
	PROF_EVENT2(bubble_sched_switchrq, \
	e->type == MA_TASK_ENTITY? \
	(void*)ma_task_entity(e): \
	(void*)ma_bubble_entity(e), \
	rq)

/******************************************************************************
 *
 * Répartiteur de bulles.
 *
 * Suppose que la hiérarchie est rassemblée.
 *
 */

#include <math.h>
static inline long entity_load(marcel_entity_t *e) {
	if (e->type == MA_BUBBLE_ENTITY)
		return *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e), marcel_stats_load_offset);
	else
		return *(long*)ma_task_stats_get(ma_task_entity(e), marcel_stats_load_offset);
}

static int load_compar(const void *_e1, const void *_e2) {
	marcel_entity_t *e1 = *(marcel_entity_t**) _e1;
	marcel_entity_t *e2 = *(marcel_entity_t**) _e2;
	long l1 = entity_load(e1);
	long l2 = entity_load(e2);
	return l2 - l1; /* decreasing order */
}
static void __marcel_bubble_spread(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl) {
	/* TODO: give imbalance in recursion */
	/* TODO: XXX: lock holders! */
	int i;
	fprintf(stderr,"spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		fprintf(stderr," %p", e[i]);
	fprintf(stderr," at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		fprintf(stderr," %s", l[i]->sched.name);
	fprintf(stderr,"\n");

	/* TODO: tune */
	if (ne < nl) {
		/* less entities than items, recurse into entities before items */
		unsigned new_ne = 0;
		unsigned recursed = 0;
		int i, j;
		fprintf(stderr,"%d < %d, recurse into entities\n", ne, nl);

		/* first count */
		for (i=0; i<ne; i++) {
			if (e[i]->type == MA_BUBBLE_ENTITY) {
				unsigned nb = ma_bubble_entity(e[i])->nbentities;
				if (nb) {
					recursed = 1;
					new_ne += nb;
				}
			} else
				new_ne += 1;
		}
		if (recursed) {
			fprintf(stderr,"%d sub-entities\n", new_ne);
			/* now establish the list */
			marcel_entity_t *new_e[new_ne], *ee;
			j = 0;
			for (i=0; i<ne; i++) {
				if (e[i]->type == MA_BUBBLE_ENTITY) {
					marcel_bubble_t *bb = ma_bubble_entity(e[i]);
					list_for_each_entry(ee, &bb->heldentities, bubble_entity_list)
						new_e[j++] = ee;
				} else
					new_e[j++] = e[i];
			}
			/* and recurse */
			__marcel_bubble_spread(new_e, new_ne, l, nl);
		} else {
			/* Grmpf, really not enough parallelism, only
			 * use part of the machine */
			MA_BUG_ON(ne > nl);
			fprintf(stderr,"Not enough parallelism, use only %d item%s\n", ne, ne>1?"s":"");
			__marcel_bubble_spread(&e[0], ne, l, ne);
		}
	}

	else 

	{
		/* More entities than items, distribute */
		int i, j, n, nitems, nentities;
		unsigned long totload = 0, load;
		float per_item_load, bonus = 0, partload;

		/* compute total load */
		for (i=0; i<ne; i++) {
			if (e[i]->type == MA_BUBBLE_ENTITY)
				totload += *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e[i]), marcel_stats_load_offset);
			else
				totload += *(long*)ma_task_stats_get(ma_task_entity(e[i]), marcel_stats_load_offset);
		}
		per_item_load = (float)totload / nl;
		fprintf(stderr,"load %lu = %d*%.2f\n", totload, nl, per_item_load);

		/* Sort entities by load */
		qsort(e, ne, sizeof(e[0]), &load_compar);

		/* Start with heaviest, put on the first item */
		i = 0;
		n = 0;
		while(i<ne) {
			/* when entities' load is 0, just leave them here */
			if (entity_load(e[i]) == 0) {
				fprintf(stderr,"remaining entities are 0-loaded, just leave them here\n");
				int state;
				for (j=i; j<ne; j++) {
					ma_runqueue_t *rq;
					state = ma_get_entity(e[j]);
					if (l[0]->father)
						rq = &l[0]->father->sched;
					else
						rq = &marcel_machine_level[0].sched;
					ma_put_entity(e[j], &rq->hold, state);
					switchrq(e[j], rq);
				}
				break;
			}

			/* start with some bonus or malus */
			partload = -bonus;
			fprintf(stderr,"starting at %s with load %.2f\n", l[n]->sched.name, partload);
			for (j=i; j<ne; j++) {
				load = entity_load(e[j]);
				partload += load;
				fprintf(stderr,"adding entity %p(%lu) -> %.2f\n", e[j], load, partload);
				/* accept 10% imbalance: TODO: tune */
				if (partload > per_item_load * 9 / 10) {
					nentities = j-i+1;
					break;
				}
			}
			if (j == ne)
				nentities = ne-i;

			nitems = (partload + per_item_load / 10) / per_item_load;
			if (!nitems)
				nitems = 1;
			bonus = nitems * per_item_load - partload;
			fprintf(stderr,"Ok, %d item%s and remaining %.2f load\n", nitems, nitems>1?"s":"", bonus);

			if (nitems == 1) {
				/* one item selected, put that there */
				fprintf(stderr,"Ok, put %s on %s\n", nentities>1?"them":"it", l[n]->sched.name);
				int state;
				for (j=i; j<i+nentities; j++) {
					state = ma_get_entity(e[j]);
					ma_put_entity(e[j], &l[n]->sched.hold, state);
					switchrq(e[j], &l[n]->sched);
				}
				if (l[n]->arity) {
					fprintf(stderr,"and recurse\n");
					__marcel_bubble_spread(&e[i], nentities, l[n]->sons, l[n]->arity);
				}
			} else {
				/* several items, just recurse */
				__marcel_bubble_spread(&e[i], nentities, &l[n], nitems);
			}
			n += nitems;
			i += nentities;
			/* TODO: if n>nl, put the rest ! (should only happen if floating point operations are not exact) */
			MA_BUG_ON(n>nl);
		}
	}
	fprintf(stderr,"spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		fprintf(stderr," %p", e[i]);
	fprintf(stderr," at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		fprintf(stderr," %s", l[i]->sched.name);
	fprintf(stderr," done.\n");

}

/* Rassembleur de la hiérarchie de la bulle */
static void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble) {
	marcel_entity_t *e;
	ma_holder_rawlock(&b->hold);
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		int state;
		ma_holder_t *h = NULL;

		if (e->type == MA_BUBBLE_ENTITY)
			__ma_bubble_gather(ma_bubble_entity(e), rootbubble);

		if (e->sched_holder == &b->hold || e->sched_holder == &rootbubble->hold)
			/* déjà rassemblé */
			continue;

		if (e->run_holder != &b->hold && e->run_holder != &rootbubble->hold)
			/* verrouiller le conteneur actuel de e */
			/* TODO: en principe, devrait être "plus bas" */
			h = ma_entity_holder_rawlock(e);

		state = ma_get_entity(e);
		fprintf(stderr,"putting back %p in bubble %p(%p)\n", e, b, &b->hold);
		ma_put_entity(e, &b->hold, state);
		PROF_EVENT2(bubble_sched_goingback, e, b);

		if (h)
			ma_entity_holder_rawunlock(h);
	}
	ma_holder_rawunlock(&b->hold);
}
void ma_bubble_gather(marcel_bubble_t *b) {
	ma_preempt_disable();
	ma_local_bh_disable();
	__ma_bubble_gather(b, b);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}
void marcel_bubble_spread(marcel_bubble_t *b, struct marcel_topo_level *l) {
	marcel_entity_t *e = &b->sched;
	ma_bubble_synthesize_stats(b);
	ma_preempt_disable();
	ma_local_bh_disable();
	__ma_bubble_gather(b, b);
	ma_holder_rawlock(&b->hold);
	__marcel_bubble_spread(&e, 1, &l, 1);
	ma_holder_rawunlock(&b->hold);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

