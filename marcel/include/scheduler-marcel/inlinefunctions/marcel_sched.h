/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __INLINEFUNCTIONS_MARCEL_SCHED_H__
#define __INLINEFUNCTIONS_MARCEL_SCHED_H__


#include "marcel_debug.h"
#include "scheduler-marcel/marcel_sched.h"
#include <hwloc.h>


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
__tbx_inline__ static ma_runqueue_t *marcel_sched_vpset_init_rq(const marcel_vpset_t *vpset)
{
	if (tbx_unlikely(marcel_vpset_isfull(vpset)))
		return &ma_main_runqueue;
	else if (tbx_unlikely(marcel_vpset_iszero(vpset)))
		return &ma_dontsched_runqueue;
	else {
		struct marcel_topo_level *level;
		unsigned int first_vp = marcel_vpset_first(vpset);
		MA_BUG_ON(first_vp >= marcel_nbvps() && first_vp >= marcel_nballvps());
		level = &marcel_topo_vp_level[first_vp];
		/* start from here up to the root of the tree to find the level vpset
		 * that entirely contains the submitted vpset */
		do {
			if (marcel_vpset_isincluded(&level->vpset, vpset))
				return &level->rq;
			level = level->father;
		} while (level);
		/* even the machine level did not catch the vpset, there must be something wrong */
		MA_BUG();
	}
}

#ifdef MA__NUMA
/* Convert from hwloc cpusets to marcel cpusets */
__tbx_inline__ static void
ma_cpuset_from_hwloc(marcel_vpset_t * mset, hwloc_const_cpuset_t lset)
{
#ifdef MA_HAVE_VPSUBSET
	/* large vpset using an array of unsigned long subsets in both marcel and hwloc */
	int i;
	for (i = 0; i < MA_VPSUBSET_COUNT && i < MA_VPSUBSET_COUNT; i++)
		MA_VPSUBSET_SUBSET(*mset, i) = hwloc_cpuset_to_ith_ulong(lset, i);
#elif MA_BITS_PER_LONG == 32 && MARCEL_NBMAXCPUS > 32
	/* marcel uses unsigned long long mask,
	 * and it's longer than hwloc's unsigned long mask,
	 * use 2 of the latter
	 */
	*mset = (marcel_vpset_t) hwloc_cpuset_to_ith_ulong(lset, 0) | ((marcel_vpset_t)
								       hwloc_cpuset_to_ith_ulong
								       (lset, 1) << 32);
#else
	/* marcel uses int or unsigned long long mask,
	 * and it's smaller or equal-size than hwloc's unsigned long mask,
	 * use 1 of the latter
	 */
	*mset = hwloc_cpuset_to_ith_ulong(lset, 0);
#endif
}

__tbx_inline__ static void
ma_cpuset_to_hwloc(marcel_vpset_t mset, hwloc_cpuset_t *lset)
{
#if (defined(MA_HAVE_VPSUBSET) || (MA_BITS_PER_LONG == 32 && MARCEL_NBMAXCPUS > 32))
	/* large vpset using an array of unsigned long subsets in both marcel and hwloc */
	int vp;
	marcel_vpset_foreach_begin(vp, &mset)
		hwloc_cpuset_set(*lset, vp);
	marcel_vpset_foreach_end()
#else
	/* marcel uses int or unsigned long long mask,
	 * and it's smaller or equal-size than hwloc's unsigned long mask,
	 * use 1 of the latter
	 */
	hwloc_cpuset_from_ith_ulong(*lset, 0, mset);
#endif
}

/** Same as marcel_sched_vpset_init_rq, but using OS cpusets */
__tbx_inline__ static ma_runqueue_t *marcel_sched_cpuset_init_rq(hwloc_const_cpuset_t cpuset)
{
	if (tbx_unlikely(hwloc_cpuset_isfull(cpuset)))
		return &ma_main_runqueue;
	else if (tbx_unlikely(hwloc_cpuset_iszero(cpuset)))
		return &ma_dontsched_runqueue;
	else {
		marcel_vpset_t marcel_cpuset;
		struct marcel_topo_level *level = marcel_machine_level;
		unsigned i;

		ma_cpuset_from_hwloc(&marcel_cpuset, cpuset);
		while (1) {
			for (i = 0; i < level->arity; i++)
				if (marcel_vpset_isincluded
				    (&level->children[i]->cpuset, &marcel_cpuset))
					break;
			if (i == level->arity)
				/* No matching children, father is already best */
				break;
			level = level->children[i];
		}
		return &level->rq;
	}
}
#endif				/* MA__NUMA */

static __tbx_setjmp_inline__
int marcel_sched_internal_create(marcel_task_t * cur, marcel_task_t * new_task,
				 __const marcel_attr_t * attr,
				 __const int dont_schedule,
				 __const unsigned long base_stack)
{
#ifdef MA__BUBBLES
	ma_holder_t *bh;
#endif
	MARCEL_LOG_IN();
#ifdef MA__BUBBLES
	if ((bh = ma_task_natural_holder(new_task)) && bh->type != MA_BUBBLE_HOLDER)
		bh = NULL;
#endif

	/* On est encore dans le père. On doit démarrer le fils */
	if (
		   // On ne peut pas céder la main maintenant
		   (ma_in_atomic())
		   /* ou bien on ne veut pas  */
		   || !ma_thread_preemptible()
		   || dont_schedule
#ifdef MA__LWPS
		   /* On ne peut pas placer ce thread sur le LWP courant */
		   ||
		   (!marcel_vpset_isset
		    (&THREAD_GETMEM(new_task, vpset), ma_vpnum(MA_LWP_SELF)))
		   /* Si la politique est du type 'placer sur le LWP le moins
		      chargé', alors on ne peut pas placer ce thread sur le LWP
		      courant */
		   || (new_task->as_entity.sched_policy == MARCEL_SCHED_OTHER)
		   || (new_task->as_entity.sched_policy == MARCEL_SCHED_BALANCE)
		   /* If the new task is less prioritized than us, don't schedule it yet */
#endif
		   || (new_task->as_entity.prio > SELF_GETMEM(as_entity.prio))
#ifdef MA__BUBBLES
		   /* on est placé dans une bulle (on ne sait donc pas si l'on a
		      le droit d'être ordonnancé tout de suite)
		      XXX: ça veut dire qu'on n'a pas de création rapide avec les
		      bulles si l'on veut strictement respecter cela :( */
		   || bh
#endif
	    ) {
		MARCEL_LOG_RETURN(marcel_sched_internal_create_dontstart
				  (cur, new_task, attr, dont_schedule, base_stack));
	} else {
		MARCEL_LOG_RETURN(marcel_sched_internal_create_start
				  (cur, new_task, attr, dont_schedule, base_stack));
	}
}


static __tbx_inline__ ma_runqueue_t *marcel_sched_select_runqueue(marcel_task_t *
								  t BUBBLE_VAR_UNUSED,
								  const marcel_attr_t *
								  attr)
{
	ma_runqueue_t *rq;
#ifdef MA__LWPS
	register marcel_lwp_t *cur_lwp = MA_LWP_SELF;
#  ifdef MA__BUBBLES
	marcel_bubble_t *b;
#  endif
#endif
	rq = NULL;
	t->as_entity.sched_holder = NULL;
#ifdef MA__LWPS
#  ifdef MA__BUBBLES
	if (SELF_GETMEM(cur_thread_seed)) {
		ma_holder_t *h;
		h = ma_task_natural_holder(MARCEL_SELF);
		if (!h)
			h = &ma_main_runqueue.as_holder;
		t->as_entity.sched_holder = t->as_entity.natural_holder = h;
		rq = ma_to_rq_holder(h);
		if (!rq)
			rq = &ma_main_runqueue;

		return rq;
	}
	if (!MA_TASK_NOT_COUNTED_IN_RUNNING(t)) {
		b = &SELF_GETMEM(default_children_bubble);
		if (!b->as_entity.natural_holder) {
			ma_holder_t *h;
			marcel_bubble_init(b);
			h = ma_task_natural_holder(MARCEL_SELF);
			if (!h)
				h = &ma_main_runqueue.as_holder;
			b->as_entity.natural_holder = h;
			if (h->type != MA_RUNQUEUE_HOLDER) {
				marcel_bubble_t *bb = ma_bubble_holder(h);
				b->as_entity.sched_level = bb->as_entity.sched_level + 1;
				marcel_bubble_insertbubble(bb, b);
			}
			if (b->as_entity.sched_level == MARCEL_LEVEL_KEEPCLOSED) {
				ma_runqueue_t *rq2;
				ma_bubble_detach(b);
				rq2 = ma_to_rq_holder(h);
				if (!rq2)
					rq2 = &ma_main_runqueue;
				b->as_entity.sched_holder = &rq2->as_holder;
				ma_holder_lock_softirq(&rq2->as_holder);
				ma_put_entity(&b->as_entity, &rq2->as_holder,
					      MA_ENTITY_READY);
				ma_holder_unlock_softirq(&rq2->as_holder);
			}
		}
		marcel_bubble_insertentity(b, &t->as_entity);
	}
#  endif			/* MA__BUBBLES */
#endif				/* MA__LWPS */
	if (attr->schedrq) {
#ifdef MA__LWPS
		MA_BUG_ON(!marcel_vpset_isincluded(&attr->vpset, &attr->schedrq->vpset));
#endif				/* MA__LWPS */
		return attr->schedrq;
	}
	if (!marcel_vpset_isfull(&attr->vpset))
		return marcel_sched_vpset_init_rq(&attr->vpset);
#ifdef MA__LWPS
#  ifdef MA__BUBBLES
	if (b->as_entity.sched_level >= MARCEL_LEVEL_KEEPCLOSED)
		/* keep this thread inside the bubble */
		return NULL;
#  endif			/* MA__BUBBLES */
	switch (t->as_entity.sched_policy) {
	case MARCEL_SCHED_SHARED:
		rq = &ma_main_runqueue;
		break;
		/* TODO: vpset ? */
	case MARCEL_SCHED_OTHER: {
			struct marcel_topo_level *vp;
			for_vp_from(vp, ma_vpnum(cur_lwp)) {
				marcel_lwp_t *lwp = ma_get_lwp_by_vpnum(vp->number);
				if (!lwp)
					continue;
				if (!ma_per_lwp(online, lwp))
					continue;
				rq = &vp->rq;
				break;
			}
			break;
	}

	case MARCEL_SCHED_AFFINITY: {
			/* Le cas echeant, retourne un LWP fortement
			   sous-charge (nb de threads running <
			   THREAD_THRESHOLD_LOW), ou alors retourne le
			   LWP "courant". */
			struct marcel_topo_level *vp;
			ma_runqueue_t *rq2;
			for_vp_from(vp, ma_vpnum(cur_lwp)) {
				marcel_lwp_t *lwp = ma_get_lwp_by_vpnum(vp->number);
				if (!lwp)
					continue;
				if (!ma_per_lwp(online, lwp))
					continue;
				rq2 = &vp->rq;
				if (rq2->as_holder.nb_ready_entities <
				    THREAD_THRESHOLD_LOW) {
					rq = rq2;
					break;
				}
			}
			if (!rq)
				rq = ma_lwp_vprq(cur_lwp);
			break;
	}

	case MARCEL_SCHED_BALANCE: {
			/* Retourne le LWP le moins charge (ce qui
			   oblige a parcourir toute la liste) */
			unsigned best = ma_lwp_vprq(cur_lwp)->as_holder.nb_ready_entities;
			struct marcel_topo_level *vp;
			ma_runqueue_t *rq2;
			rq = ma_lwp_vprq(cur_lwp);
			for_vp_from(vp, ma_vpnum(cur_lwp)) {
				marcel_lwp_t *lwp = ma_get_lwp_by_vpnum(vp->number);
				if (!lwp)
					continue;
				if (!ma_per_lwp(online, lwp))
					continue;
				rq2 = &vp->rq;
				if (rq2->as_holder.nb_ready_entities < best) {
					rq = rq2;
					best = rq2->as_holder.nb_ready_entities;
				}
			}
			break;
	}

	default: {
			unsigned num =
			    ma_user_policy[t->as_entity.sched_policy -
					   __MARCEL_SCHED_AVAILABLE] (t,
								      ma_vpnum(cur_lwp));
			rq = ma_lwp_vprq(ma_get_lwp_by_vpnum(num));
			break;
	}

	}
	if (!rq)
#endif
		rq = &ma_main_runqueue;

	return rq;
}

__tbx_inline__ static void
marcel_sched_internal_init_marcel_task(marcel_task_t * t, const marcel_attr_t * attr)
{
	ma_holder_t *h = NULL;
	MARCEL_LOG_IN();
	if (attr->sched.natural_holder) {
		h = attr->sched.natural_holder;
#ifdef MA__BUBBLES
	} else if (attr->sched.inheritholder) {
		/* TODO: on suppose ici que la bulle est éclatée et qu'elle ne sera pas refermée d'ici qu'on wake_up_created ce thread */
		h = ma_task_natural_holder(MARCEL_SELF);
#endif
	}
	t->as_entity.sched_policy = attr->sched.sched_policy;
	t->as_entity.ready_holder = NULL;
	t->as_entity.natural_holder = NULL;
	t->as_entity.ready_holder_data = NULL;
#ifdef MA__BUBBLES
	TBX_INIT_FAST_LIST_HEAD(&t->as_entity.natural_entities_item);
#endif
	if (h) {
		t->as_entity.sched_holder = h;
#ifdef MA__BUBBLES
		t->as_entity.natural_holder = h;
#endif
	} else {
		// TODO: we should probably do it even when a natural holder is given, i.e. merge the h selection above within marcel_sched_select_runqueue
		ma_runqueue_t *rq = marcel_sched_select_runqueue(t, attr);
		if (rq) {
			t->as_entity.sched_holder = &rq->as_holder;
			MA_BUG_ON(!rq->as_holder.name[0]);
		}
	}
	TBX_INIT_FAST_LIST_HEAD(&t->as_entity.cached_entities_item);
	t->as_entity.prio = attr->__schedparam.__sched_priority;
	PROF_EVENT2(sched_setprio, ma_task_entity(&t->as_entity), t->as_entity.prio);
	/* TODO: only for the spread scheduler */
#ifdef MA__LWPS
	t->as_entity.sched_level = MARCEL_LEVEL_DEFAULT;
#endif

#ifdef MARCEL_STATS_ENABLED
	ma_stats_reset(&t->as_entity);
	ma_task_stats_set(long, t, ma_stats_load_offset,
			  MA_TASK_NOT_COUNTED_IN_RUNNING(t) ? 0L : 1L);
	ma_task_stats_set(long, t, ma_stats_nbrunning_offset, 0);
	ma_task_stats_set(long, t, ma_stats_nbready_offset, 0);
#ifdef MARCEL_MAMI_ENABLED
	{
		unsigned node;
		for (node = 0; node < marcel_nbnodes; node++) {
			((long *) ma_task_stats_get(t, ma_stats_memnode_offset))[node] =
			    0;
		}
	}
#endif				/* MARCEL_MAMI_ENABLED */
#endif				/* MARCEL_STATS_ENABLED */
#ifdef MARCEL_MAMI_ENABLED
	marcel_spin_init(&t->as_entity.memory_areas_lock, MARCEL_PROCESS_SHARED);
	TBX_INIT_FAST_LIST_HEAD(&t->as_entity.memory_areas);
#endif				/* MARCEL_MAMI_ENABLED */
	if (ma_holder_type(t->as_entity.sched_holder) == MA_RUNQUEUE_HOLDER)
		MARCEL_SCHED_LOG("%p(%s)'s holder is %s (prio %d)\n", t,
				 t->as_entity.name, t->as_entity.sched_holder->name,
				 t->as_entity.prio);
	else
		MARCEL_SCHED_LOG("%p(%s)'s holder is bubble %p (prio %d)\n", t,
				 t->as_entity.name, t->as_entity.sched_holder->name,
				 t->as_entity.prio);
	MARCEL_LOG_OUT();
}

__tbx_inline__ static void
marcel_sched_internal_init_marcel_thread(marcel_task_t * t, const marcel_attr_t * attr)
{
	MARCEL_LOG_IN();
	t->as_entity.type = MA_THREAD_ENTITY;
	/* disable seed inlining while the seed is being initialized */
	t->cur_thread_seed_runner = (void *) (intptr_t) 1;
	marcel_sched_internal_init_marcel_task(t, attr);
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, t, ma_stats_nbthreads_offset, 1);
	ma_task_stats_set(long, t, ma_stats_nbthreadseeds_offset, 0);
#endif				/* MARCEL_STATS_ENABLED */

#ifdef MA__BUBBLES
	/* bulle non initialisée */
	t->default_children_bubble.as_entity.natural_holder = NULL;
#endif
	ma_atomic_init(&t->as_entity.time_slice, MARCEL_TASK_TIMESLICE);
	MARCEL_LOG_OUT();
}

__tbx_inline__ static void
marcel_sched_init_thread_seed(marcel_task_t * t, const marcel_attr_t * attr)
{
	MARCEL_LOG_IN();
	t->as_entity.type = MA_THREAD_SEED_ENTITY;
	marcel_sched_internal_init_marcel_task(t, attr);
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, t, ma_stats_nbthreads_offset, 0);
	ma_task_stats_set(long, t, ma_stats_nbthreadseeds_offset, 1);
#endif				/* MARCEL_STATS_ENABLED */

	MARCEL_LOG_OUT();
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_SCHED_H__ **/
