
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

#ifdef MA__BUBBLES

#if 1
#define _debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#define debug(fmt, ...) fprintf(stderr, "%*s" fmt, recurse, "", ##__VA_ARGS__);
#else
#define _debug(fmt, ...) (void)0
#define debug(fmt, ...) (void)0
#endif

/* spread entities e on topology levels l */
/* e has ne items, l has nl items */
static void __marcel_bubble_spread(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl, int recurse) {
	/* TODO: give imbalance in recursion ? */
	int i;
	float per_item_load;
	unsigned long totload;

	if (!ne) {
		debug("no entity, do nothing");
		return;
	}

	MA_BUG_ON(!nl);

	debug("spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		_debug(" %p", e[i]);
	_debug(" at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		_debug(" %s", l[i]->sched.name);
	_debug("\n");

	if (nl == 1) {
		/* Only one level */
		debug("Ok, just leave %s on %s\n", ne>1?"them":"it", l[0]->sched.name);
		if (l[0]->arity) {
			debug("and recurse in levels\n");
			return __marcel_bubble_spread(e, ne, l[0]->children, l[0]->arity, recurse+1);
		}
		debug("No more possible level recursion, we're done\n");
		return;
	}

	/* compute total load */
	totload = 0;
	for (i=0; i<ne; i++) {
		if (e[i]->type == MA_BUBBLE_ENTITY)
			totload += *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e[i]), marcel_stats_load_offset);
		else
			totload += *(long*)ma_task_stats_get(ma_task_entity(e[i]), marcel_stats_load_offset);
	}
	per_item_load = (float)totload / nl;
	debug("total load %lu = %d*%.2f\n", totload, nl, per_item_load);

	/* Sort entities by load */
	qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);

	/* TODO: tune */
	if (ma_entity_load(e[0]) > per_item_load || ne < nl) {
		/* too big entities, or not enough entities for level items,
		 * recurse into entities before level items */

	/* TODO: être capable de prendre une bulle et une partie des items, plutôt ? */
		unsigned new_ne = 0;
		unsigned recursed = 0;
		int i, j;
		debug("e[0]=%ld > %lf=per_item_load || ne=%d < %d=nl, recurse into entities\n", ma_entity_load(e[0]), per_item_load, ne, nl);

		/* first count sub-entities */
		for (i=0; i<ne; i++) {
			if (e[i]->type == MA_BUBBLE_ENTITY &&
				(ma_entity_load(e[i]) > per_item_load || ne < nl)) {
				unsigned nb = ma_bubble_entity(e[i])->nbentities;
				if (nb) {
					recursed = 1;
					new_ne += nb;
				}
			} else
				new_ne += 1;
		}
		if (recursed) {
			debug("%d sub-entities\n", new_ne);
			/* now establish the list */
			marcel_entity_t *new_e[new_ne], *ee;
			j = 0;
			for (i=0; i<ne; i++) {
				if (e[i]->type == MA_BUBBLE_ENTITY &&
					(ma_entity_load(e[i]) > per_item_load || ne < nl)) {
					marcel_bubble_t *bb = ma_bubble_entity(e[i]);
					list_for_each_entry(ee, &bb->heldentities, bubble_entity_list)
						new_e[j++] = ee;
				} else
					new_e[j++] = e[i];
			}
			debug("recurse into entities\n");
			return __marcel_bubble_spread(new_e, new_ne, l, nl, recurse+1);
		}
		if (ne < nl) {
			/* Grmpf, really not enough parallelism, only
			 * use part of the machine */
			debug("Not enough parallelism, using only %d item%s\n", ne, ne>1?"s":"");
			return __marcel_bubble_spread(&e[0], ne, l, ne, recurse+1);
		}
	}

	/* More entities than items, greedily distribute */
	struct marcel_topo_level *l_l[nl];
	struct list_head l_dist[nl];
	unsigned long l_load[nl];
	int l_n[nl];

	int j, k, m, n, state;

	/* Distribute from heaviest to lightest */
	i = 0;
	n = 0;
	for (i=0; i<nl; i++) {
		l_l[i] = l[i];
		INIT_LIST_HEAD(&l_dist[i]);
		l_load[i] = 0;
		l_n[i] = 0;
	}
	for (i=0; i<ne; i++) {
		debug("entity %p(%ld)\n",e[i],ma_entity_load(e[i]));
		/* when entities' load is very small, just leave them here */
		/* TODO: tune */
		if (ma_entity_load(e[i]) <= per_item_load/30) {
			int state;
			ma_runqueue_t *rq;
			debug("small(%lx), leave it here\n", ma_entity_load(e[i]));
			PROF_EVENTSTR(sched_status, "spread: small, leave it here");
			state = ma_get_entity(e[i]);
			if (l_l[0]->father)
				rq = &l_l[0]->father->sched;
			else
				rq = &marcel_machine_level[0].sched;
			ma_put_entity(e[i], &rq->hold, state);
			continue;
		}

		debug("add to level %s(%ld)",l_l[0]->sched.name,l_load[0]);
		/* Add this entity (heaviest) to least loaded level item */
		PROF_EVENTSTR(sched_status, "spread: add to level");
		list_add_tail(&e[i]->next,&l_dist[0]);
		l_load[0] += ma_entity_load(e[i]);
		l_n[0]++;
		state = ma_get_entity(e[i]);
		ma_put_entity(e[i], &l_l[0]->sched.hold, state);
		_debug(" -> %ld\n",l_load[0]);

		/* And sort */
		if (nl > 1 && l_load[0] > l_load[1]) {
			/* TODO: optimize this */
			j = 1;
			k = nl - 1;
			if (l_load[0] < l_load[k])
			while(1) {
				MA_BUG_ON(l_load[j] >= l_load[0] || l_load[0] >= l_load[k]);
				/* rough guess by assuming linear distribution */
				// TODO: fix it
				//int m = j + (l_load[0] - l_load[j]) * (k - j) / (l_load[k] - l_load[j]);
				if (j == k-1) {
					k = j;
					break;
				}
				m = (j+k)/2;

				debug("trying %d(%ld) between %d(%ld) and %d(%ld)\n", m, l_load[m], j, l_load[j], k, l_load[k]);

				if (l_load[0] == l_load[m])
					break;

				if (l_load[0] < l_load[m])
					k = m;
				else
					j = m;
			}

			debug("inserting level %s(%ld) in place of %s(%ld)\n", l_l[0]->sched.name, l_load[0], l_l[k]->sched.name, l_load[k]);
			{
				unsigned long _l_load;
				struct list_head _l_dist;
				struct marcel_topo_level *_l;
				int _l_n;

				/* Save level 0 */
				INIT_LIST_HEAD(&_l_dist);
				_l_load = l_load[0];
				list_splice_init(&l_dist[0],&_l_dist);
				_l_n = l_n[0];
				_l = l_l[0];

				/* Shift levels */
				for (m=0; m<k; m++) {
					l_load[m] = l_load[m+1];
					list_splice_init(&l_dist[m+1],&l_dist[m]);
					l_n[m] = l_n[m+1];
					l_l[m] = l_l[m+1];
				}

				/* Restore level 0 */
				l_load[k] = _l_load;
				list_splice(&_l_dist,&l_dist[k]);
				l_n[k] = _l_n;
				l_l[k] = _l;
			}
		}
	}
	for (i=0; i<nl; i++) {
		debug("recurse in %s\n",l_l[i]->sched.name);
		marcel_entity_t *ne[l_n[i]];
		marcel_entity_t *e;
		j = 0;
		list_for_each_entry(e,&l_dist[i],next)
			ne[j++] = e;
		MA_BUG_ON(j != l_n[i]);
		if (l_l[i]->arity)
			__marcel_bubble_spread(ne, l_n[i], l_l[i]->children, l_l[i]->arity, recurse+1);
		else
			__marcel_bubble_spread(ne, l_n[i], &l_l[i], 1, recurse+1);
	}
}

void marcel_bubble_spread(marcel_bubble_t *b, struct marcel_topo_level *l) {
	unsigned vp;
	marcel_entity_t *e = &b->sched;
	ma_bubble_synthesize_stats(b);
	ma_preempt_disable();
	ma_local_bh_disable();
	/* XXX: suppose that the bubble is not held out of topo hierarchy under
	 * level l */
	ma_bubble_lock_all(b, l);
	__ma_bubble_gather(b, b);
	__marcel_bubble_spread(&e, 1, &l, 1, 0);
	PROF_EVENTSTR(sched_status, "spread: done");

	/* resched existing threads */
	marcel_vpmask_foreach_begin(vp,&l->vpset)
		ma_lwp_t lwp = ma_vp_lwp[vp];
		ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
	marcel_vpmask_foreach_end()

	ma_bubble_unlock_all(b, l);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

int
spread_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  marcel_bubble_spread(ma_bubble_entity(e), l);

  return 0;
}

struct ma_bubble_sched_struct marcel_bubble_spread_sched = {
  .submit = spread_sched_submit,
};

#endif
