
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

#ifdef MA__BUBBLES
static inline long
sum_load(marcel_entity_t *e[], int ne) {
	long load = 0;
	int i;
	for (i=0; i<ne; i++) {
		if (e[i]->type == MA_BUBBLE_ENTITY)
			load += *(long*)ma_bubble_hold_stats_get(ma_bubble_entity(e[i]), marcel_stats_load_offset);
		else
			load += *(long*)ma_task_stats_get(ma_task_entity(e[i]), marcel_stats_load_offset);
	}
	return load;
}
static inline unsigned
nb_sub_entities(marcel_entity_t *e[], int ne, int nl, float per_item_load, unsigned *recursed) {
	int i;
	unsigned new_ne = 0;
	for (i=0; i<ne; i++) {
		if (e[i]->type == MA_BUBBLE_ENTITY &&
				(ma_entity_load(e[i]) > per_item_load || ne < nl)) {
			unsigned nb = ma_bubble_entity(e[i])->nbentities;
			if (nb) {
				*recursed = 1;
				new_ne += nb;
			}
		} else
			new_ne += 1;
	}
	return new_ne;
}
static inline void
build_list(marcel_entity_t *e[], int ne, int nl, float per_item_load, marcel_entity_t *new_e[], int new_ne) {
	int i, j = 0;
	for (i=0; i<ne; i++) {
		if (e[i]->type == MA_BUBBLE_ENTITY &&
				(ma_entity_load(e[i]) > per_item_load || ne < nl)) {
			marcel_entity_t *ee;
			marcel_bubble_t *bb = ma_bubble_entity(e[i]);
			list_for_each_entry(ee, &bb->heldentities, bubble_entity_list)
				new_e[j++] = ee;
		} else
			new_e[j++] = e[i];
	}
}
static inline int
select_level(unsigned long *l_load, int nl) {
	/* TODO: optimize this */
	int j = 1;
	int k = nl - 1;
	if (l_load[0] >= l_load[k])
		return k;
	while(1) {
		int m;
		MA_BUG_ON(l_load[j] >= l_load[0] || l_load[0] >= l_load[k]);
		/* rough guess by assuming linear distribution */
		// TODO: fix it
		//int m = j + (l_load[0] - l_load[j]) * (k - j) / (l_load[k] - l_load[j]);
		if (j == k-1) {
			k = j;
			break;
		}
		m = (j+k)/2;
		bubble_sched_debug("trying %d(%ld) between %d(%ld) and %d(%ld)\n", m, l_load[m], j, l_load[j], k, l_load[k]);
		if (l_load[0] == l_load[m])
			break;
		if (l_load[0] < l_load[m])
			k = m;
		else
			j = m;
	}
	return k;
}
static inline void
insert_level(struct marcel_topo_level **l_l,
		struct list_head *l_dist,
		unsigned long *l_load,
		int *l_n,
		int k) {
	unsigned long _l_load;
	struct list_head _l_dist;
	struct marcel_topo_level *_l;
	int _l_n;
	int m;

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
/* spread entities e on topology levels l */
/* e has ne items, l has nl items */
static void __marcel_bubble_spread(marcel_entity_t *e[], int ne, struct marcel_topo_level **l, int nl, int recurse) {
	/* TODO: give imbalance in recursion ? */
	int i;
	float per_item_load;
	unsigned long totload;

	if (!ne) {
		bubble_sched_debug("no entity, do nothing");
		return;
	}

	MA_BUG_ON(!nl);

	bubble_sched_debug("spreading entit%s", ne>1?"ies":"y");
	for (i=0; i<ne; i++)
		bubble_sched_debug(" %p", e[i]);
	bubble_sched_debug(" at level%s", nl>1?"s":"");
	for (i=0; i<nl; i++)
		bubble_sched_debug(" %s", l[i]->rq.name);
	bubble_sched_debug("\n");

	if (nl == 1) {
		/* Only one level */
		bubble_sched_debug("Ok, just leave %s on %s\n", ne>1?"them":"it", l[0]->rq.name);
		if (l[0]->arity) {
			bubble_sched_debug("and recurse in levels\n");
			return __marcel_bubble_spread(e, ne, l[0]->children, l[0]->arity, recurse+1);
		}
		bubble_sched_debug("No more possible level recursion, we're done\n");
		return;
	}

#define marcel_stats_load_offset ma_stats_nbready_offset
	/* compute total load */
	totload = sum_load(e, ne);
	per_item_load = (float)totload / nl;
	bubble_sched_debug("total load %lu = %d*%.2f\n", totload, nl, per_item_load);

	/* Sort entities by load */
	qsort(e, ne, sizeof(e[0]), &ma_decreasing_order_entity_load_compar);

	/* TODO: tune */
	if (ma_entity_load(e[0]) > per_item_load || ne < nl) {
		/* too big entities, or not enough entities for level items,
		 * recurse into entities before level items */

	/* TODO: être capable de prendre une bulle et une partie des items, plutôt ? */
		unsigned new_ne = 0;
		unsigned recursed = 0;
		bubble_sched_debug("e[0]=%ld > %lf=per_item_load || ne=%d < %d=nl, recurse into entities\n", ma_entity_load(e[0]), per_item_load, ne, nl);

		/* first count sub-entities */
		new_ne = nb_sub_entities(e, ne, nl, per_item_load, &recursed);
		if (recursed) {
			bubble_sched_debug("%d sub-entities\n", new_ne);
			/* now establish the list */
			marcel_entity_t *new_e[new_ne];
			build_list(e, ne, nl, per_item_load, new_e, new_ne);
			bubble_sched_debug("recurse into entities\n");
			return __marcel_bubble_spread(new_e, new_ne, l, nl, recurse+1);
		}
		if (ne < nl) {
			/* Grmpf, really not enough parallelism, only
			 * use part of the machine */
			bubble_sched_debug("Not enough parallelism, using only %d item%s\n", ne, ne>1?"s":"");
			return __marcel_bubble_spread(&e[0], ne, l, ne, recurse+1);
		}
	}

	/* More entities than items, greedily distribute */
	struct marcel_topo_level *l_l[nl];
	struct list_head l_dist[nl];
	unsigned long l_load[nl];
	int l_n[nl];

	int j, k, n, state;

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
		bubble_sched_debug("entity %p(%ld)\n",e[i],ma_entity_load(e[i]));
		/* when entities' load is very small, just leave them here */
		/* TODO: tune */
		if (ma_entity_load(e[i]) <= per_item_load/30) {
			int state;
			ma_runqueue_t *rq;
			bubble_sched_debug("small(%lx), leave it here\n", ma_entity_load(e[i]));
			PROF_EVENTSTR(sched_status, "spread: small, leave it here");
			state = ma_get_entity(e[i]);
			if (l_l[0]->father)
				rq = &l_l[0]->father->rq;
			else
				rq = &marcel_machine_level[0].rq;
			ma_put_entity(e[i], &rq->as_holder, state);
			continue;
		}
		bubble_sched_debug("add to level %s(%ld)",l_l[0]->rq.name,l_load[0]);
		/* Add this entity (heaviest) to least loaded level item */
		PROF_EVENTSTR(sched_status, "spread: add to level");
		list_add_tail(&e[i]->next,&l_dist[0]);
		l_load[0] += ma_entity_load(e[i]);
		l_n[0]++;
		state = ma_get_entity(e[i]);
		ma_put_entity(e[i], &l_l[0]->rq.as_holder, state);
		bubble_sched_debug(" -> %ld\n",l_load[0]);
		/* And sort */
		if (nl > 1 && l_load[0] > l_load[1]) {
			k = select_level(l_load, nl);
			bubble_sched_debug("inserting level %s(%ld) in place of %s(%ld)\n", l_l[0]->rq.name, l_load[0], l_l[k]->rq.name, l_load[k]);
			insert_level(l_l, l_dist, l_load, l_n, k);
		}
	}
	for (i=0; i<nl; i++) {
		bubble_sched_debug("recurse in %s\n",l_l[i]->rq.name);
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
	marcel_entity_t *e = &b->as_entity;
	ma_bubble_synthesize_stats(b);
	/* XXX: suppose that the bubble is not held out of topo hierarchy under
	 * level l */
	ma_bubble_lock_all(b, l);
	__ma_bubble_gather(b, b);
	__marcel_bubble_spread(&e, 1, &l, 1, 0);
	PROF_EVENTSTR(sched_status, "spread: done");

	/* resched existing threads */
	marcel_vpset_foreach_begin(vp,&l->vpset)
		ma_lwp_t lwp = ma_vp_lwp[vp];
		ma_resched_task(ma_per_lwp(current_thread,lwp),vp,lwp);
	marcel_vpset_foreach_end()

	ma_bubble_unlock_all(b, l);
}

static marcel_bubble_t *b = &marcel_root_bubble;

static int
spread_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  b = ma_bubble_entity(e);
  marcel_bubble_spread(b, l);

  return 0;
}

static int
spread_sched_vp_is_idle(unsigned vp)
{
  if (!vp) return 0;
  if (!ma_idle_scheduler_is_running ()) 
    return 0;

  ma_deactivate_idle_scheduler ();

  int n;
  struct marcel_topo_level *l = &marcel_topo_vp_level[vp];
  while (l->father && !marcel_vpset_isset(&l->vpset, 0))
    l = l->father;
  MA_BUG_ON(!marcel_vpset_isset(&l->vpset, 0));

  marcel_threadslist(0,NULL,&n,NOT_BLOCKED_ONLY);

  if (n < 2*marcel_vpset_weight(&l->vpset)) {
    //bubble_sched_debug("moins de threads que de VPS, on ne fait rien\n");
    ma_activate_idle_scheduler ();
    return 0;
  }
  while (l->father && n >= 2*marcel_vpset_weight(&l->father->vpset)) {
	  l = l->father;
  }
  bubble_sched_debug("%d threads pour %d vps dans %s, on respread là\n", n, marcel_vpset_weight(&l->vpset), l->rq.name);

  bubble_sched_debug("===========[repartition avec spread]===========\n");
  marcel_bubble_spread(&marcel_root_bubble, l);
  ma_activate_idle_scheduler ();
  return 1;
}

struct ma_bubble_sched_struct marcel_bubble_spread_sched = {
  .submit = spread_sched_submit,
  //.vp_is_idle = spread_sched_vp_is_idle,
};

#endif
