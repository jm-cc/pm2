
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*! \mainpage The Marcel API Documentation
 *
 * \section intro_sec Introduction
 * <div class="sec">
 * Marcel is a thread library that was originaly developped to meet
 * the needs of the PM2 multithreaded environment. Marcel provides a
 * POSIX-compliant interface and a set of original extensions. It can
 * also be compiled to provide ABI-compabiblity with NTPL threads
 * under Linux, so that multithreaded applications can use Marcel
 * without being recompiled.
 * <P>Marcel features a two-level thread scheduler (also called N:M
 * scheduler) that achieves the performance of a user-level thread
 * package while being able to exploit multiprocessor machines. The
 * architecture of Marcel was carefully designed to support a high
 * number of threads and to efficiently exploit hierarchical
 * architectures (e.g. multi-core chips, NUMA machines).</P>
 * <P>More information about Marcel can be found at
 *  found at http://runtime.bordeaux.inria.fr/marcel/.</P>
 * </div>
 */

/** \file
 * \brief Marcel scheduler
 */
#section default [no-include-in-main,no-include-in-master-section]

#section common
#include "tbx_compiler.h"


/****************************************************************/
/* Attributs
 */

#section types
#depend "[structures]"
typedef struct marcel_sched_attr marcel_sched_attr_t;

#section structures
#depend "scheduler/marcel_holder.h[types]"
struct marcel_sched_attr {
	int sched_policy;
	ma_holder_t *natural_holder;
	tbx_bool_t inheritholder;
};

#section variables
extern marcel_sched_attr_t marcel_sched_attr_default;

#section macros
#define MARCEL_SCHED_ATTR_INITIALIZER { \
	.sched_policy = MARCEL_SCHED_DEFAULT, \
	.natural_holder = NULL, \
	.inheritholder = tbx_false, \
}

#define MARCEL_SCHED_ATTR_DESTROYER { \
   .sched_policy = -1, \
	.natural_holder = NULL, \
	.inheritholder = tbx_false, \
}
#section functions
int marcel_sched_attr_init(marcel_sched_attr_t *attr);
#define marcel_sched_attr_destroy(attr_ptr)	0


int marcel_sched_attr_setnaturalholder(marcel_sched_attr_t *attr, ma_holder_t *h) __THROW;
/** Sets the natural holder for created thread */
int marcel_attr_setnaturalholder(marcel_attr_t *attr, ma_holder_t *h) __THROW;
#define marcel_attr_setnaturalholder(attr,holder) marcel_sched_attr_setnaturalholder(&(attr)->sched,holder)

int marcel_sched_attr_getnaturalholder(__const marcel_sched_attr_t *attr, ma_holder_t **h) __THROW;
/** Gets the natural holder for created thread */
int marcel_attr_getnaturalholder(__const marcel_attr_t *attr, ma_holder_t **h) __THROW;
#define marcel_attr_getnaturalholder(attr,holder) marcel_sched_attr_getnaturalholder(&(attr)->sched,holder)


int marcel_sched_attr_setnaturalrq(marcel_sched_attr_t *attr, ma_runqueue_t *rq) __THROW;
#define marcel_sched_attr_setnaturalrq(attr, rq) marcel_sched_attr_setnaturalholder(attr, &(rq)->as_holder)
/** Sets the natural runqueue for created thread */
int marcel_attr_setnaturalrq(marcel_attr_t *attr, ma_runqueue_t *rq) __THROW;
#define marcel_attr_setnaturalrq(attr,rq) marcel_sched_attr_setnaturalrq(&(attr)->sched,rq)

int marcel_sched_attr_getnaturalrq(__const marcel_sched_attr_t *attr, ma_runqueue_t **rq) __THROW;
/** Gets the initial runqueue for created thread */
int marcel_attr_getnaturalrq(__const marcel_attr_t *attr, ma_runqueue_t **rq) __THROW;
#define marcel_attr_getinitrq(attr,rq) marcel_sched_attr_getinitrq(&(attr)->sched,rq)


#ifdef MA__BUBBLES
int marcel_sched_attr_setnaturalbubble(marcel_sched_attr_t *attr, marcel_bubble_t *bubble) __THROW;
#define marcel_sched_attr_setnaturalbubble(attr, bubble) marcel_sched_attr_setnaturalholder(attr, &(bubble)->as_holder)
/** Sets the natural bubble for created thread */
int marcel_attr_setnaturalbubble(marcel_attr_t *attr, marcel_bubble_t *bubble) __THROW;
#define marcel_attr_setnaturalbubble(attr,bubble) marcel_sched_attr_setnaturalbubble(&(attr)->sched,bubble)
int marcel_sched_attr_getnaturalbubble(__const marcel_sched_attr_t *attr, marcel_bubble_t **bubble) __THROW;
/** Gets the natural bubble for created thread */
int marcel_attr_getnaturalbubble(__const marcel_attr_t *attr, marcel_bubble_t **bubble) __THROW;
#define marcel_attr_getnaturalbubble(attr,bubble) marcel_sched_attr_getnaturalbubble(&(attr)->sched,bubble)
#endif

int marcel_sched_attr_setinheritholder(marcel_sched_attr_t *attr, int yes) __THROW;
/** Makes created thread inherits holder from its parent */
int marcel_attr_setinheritholder(marcel_attr_t *attr, int yes) __THROW;
#define marcel_attr_setinheritholder(attr,yes) marcel_sched_attr_setinheritholder(&(attr)->sched,yes)
int marcel_sched_attr_getinheritholder(__const marcel_sched_attr_t *attr, int *yes) __THROW;
/** Whether created thread inherits holder from its parent */
int marcel_attr_getinheritholder(__const marcel_attr_t *attr, int *yes) __THROW;
#define marcel_attr_getinheritholder(attr,yes) marcel_sched_attr_getinheritholder(&(attr)->sched,yes)

int marcel_sched_attr_setschedpolicy(marcel_sched_attr_t *attr, int policy) __THROW;
/** Sets the scheduling policy */
int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy) __THROW;
#define marcel_attr_setschedpolicy(attr,policy) marcel_sched_attr_setschedpolicy(&(attr)->sched,policy)
int marcel_sched_attr_getschedpolicy(__const marcel_sched_attr_t * __restrict attr,
                               int * __restrict policy) __THROW;
/** Gets the scheduling policy */
int marcel_attr_getschedpolicy(__const marcel_attr_t * __restrict attr,
                               int * __restrict policy) __THROW;
#define marcel_attr_getschedpolicy(attr,policy) marcel_sched_attr_getschedpolicy(&(attr)->sched,policy)




#section functions
#ifdef MA__LWPS
unsigned marcel_add_lwp(void);
#else
#define marcel_add_lwp() (0)
#endif

/****************************************************************/
/* Structure interne pour une tâche
 */

#section marcel_types
#depend "scheduler/marcel_holder.h[marcel_structures]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "scheduler/marcel_bubble_sched.h[structures]"

#section marcel_functions
int ma_wake_up_task(marcel_task_t * p);

#section marcel_functions
#depend "scheduler/linux_runqueues.h[marcel_types]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpset_init_rq(const marcel_vpset_t *vpset);

#section marcel_inline
#depend "scheduler/linux_runqueues.h[marcel_types]"
#depend "sys/marcel_lwp.h[marcel_inline]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpset_init_rq(const marcel_vpset_t *vpset)
{
	if (tbx_unlikely(marcel_vpset_isfull(vpset)))
		return &ma_main_runqueue;
	else if (tbx_unlikely(marcel_vpset_iszero(vpset)))
		return &ma_dontsched_runqueue;
	else {
		struct marcel_topo_level *level;
		unsigned int first_vp = marcel_vpset_first(vpset);
		MA_BUG_ON(first_vp >= marcel_nbvps() && first_vp>=marcel_nballvps());
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

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_task(marcel_task_t* t,
		const marcel_attr_t *attr);
#section sched_marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
#depend "marcel_topology.h[marcel_structures]"
#depend "scheduler/linux_runqueues.h[marcel_variables]"
#depend "scheduler/marcel_holder.h[marcel_macros]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "[marcel_variables]"
static __tbx_inline__ ma_runqueue_t *
marcel_sched_select_runqueue(marcel_task_t* t,
		const marcel_attr_t *attr) {
	ma_runqueue_t *rq;
#ifdef MA__LWPS
	register marcel_lwp_t *cur_lwp = MA_LWP_SELF;
#  ifdef MA__BUBBLES
	marcel_bubble_t *b;
#  endif
#endif
	if (attr->topo_level) {
		MA_BUG_ON(!marcel_vpset_isincluded(&attr->vpset, &attr->topo_level->vpset));
		return &attr->topo_level->rq;
	}
	if (!marcel_vpset_isfull(&attr->vpset))
		return marcel_sched_vpset_init_rq(&attr->vpset);
	rq = NULL;
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
	b = &SELF_GETMEM(bubble);
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
			ma_runqueue_t *rq;
			ma_bubble_detach(b);
			rq = ma_to_rq_holder(h);
			if (!rq)
				rq = &ma_main_runqueue;
			b->as_entity.sched_holder = &rq->as_holder;
			ma_holder_lock_softirq(&rq->as_holder);
			ma_put_entity(&b->as_entity, &rq->as_holder, MA_ENTITY_READY);
			ma_holder_unlock_softirq(&rq->as_holder);
		}
	}
	t->as_entity.sched_holder=NULL;
	marcel_bubble_insertentity(b, &t->as_entity);
	if (b->as_entity.sched_level >= MARCEL_LEVEL_KEEPCLOSED)
		/* keep this thread inside the bubble */
		return NULL;
#  endif
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
				if (rq2->as_holder.nr_ready < THREAD_THRESHOLD_LOW) {
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
			unsigned best = ma_lwp_vprq(cur_lwp)->as_holder.nr_ready;
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
				if (rq2->as_holder.nr_ready < best) {
					rq = rq2;
					best = rq2->as_holder.nr_ready;
				}
			}
			break;
		}
		default: {
			unsigned num = ma_user_policy[t->as_entity.sched_policy - __MARCEL_SCHED_AVAILABLE](t, ma_vpnum(cur_lwp));
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
marcel_sched_internal_init_marcel_task(marcel_task_t* t,
		const marcel_attr_t *attr)
{
	ma_holder_t *h = NULL;
	LOG_IN();
	if (attr->sched.natural_holder) {
		h = attr->sched.natural_holder;
#ifdef MA__BUBBLES
	} else if (attr->sched.inheritholder) {
		/* TODO: on suppose ici que la bulle est éclatée et qu'elle ne sera pas refermée d'ici qu'on wake_up_created ce thread */
		h = ma_task_natural_holder(MARCEL_SELF);
#endif
	}
	t->as_entity.sched_policy = attr->sched.sched_policy;
	t->as_entity.run_holder=NULL;
	t->as_entity.natural_holder=NULL;
	t->as_entity.run_holder_data=NULL;
#ifdef MA__BUBBLES
	INIT_LIST_HEAD(&t->as_entity.bubble_entity_list);
#endif
	if (h) {
		t->as_entity.sched_holder = h;
#ifdef MA__BUBBLES
		t->as_entity.natural_holder = h;
#endif
	} else {
		ma_runqueue_t *rq = marcel_sched_select_runqueue(t, attr);
		if (rq) {
			t->as_entity.sched_holder = &rq->as_holder;
			MA_BUG_ON(!rq->name[0]);
		}
	}
	INIT_LIST_HEAD(&t->as_entity.run_list);
	t->as_entity.prio=attr->__schedparam.__sched_priority;
	PROF_EVENT2(sched_setprio,ma_task_entity(&t->as_entity),t->as_entity.prio);
	/* TODO: only for the spread scheduler */
#ifdef MA__LWPS
	t->as_entity.sched_level=MARCEL_LEVEL_DEFAULT;
#endif

#ifdef MARCEL_STATS_ENABLED
	ma_stats_reset(&t->as_entity);
	ma_task_stats_set(long, t, marcel_stats_load_offset, MA_TASK_NOT_COUNTED_IN_RUNNING(t) ? 0L : 1L);
	ma_task_stats_set(long, t, ma_stats_nbrunning_offset, 0);
	ma_task_stats_set(long, t, ma_stats_nbready_offset, 0);
#ifdef MARCEL_MAMI_ENABLED
	{
	  unsigned node;
	  for (node = 0; node < marcel_nbnodes; node++) {
	    ((long *) ma_task_stats_get (t, ma_stats_memnode_offset))[node] = 0;
	  }
	}
#endif /* MARCEL_MAMI_ENABLED */
#endif /* MARCEL_STATS_ENABLED */
#ifdef MARCEL_MAMI_ENABLED
	ma_spin_lock_init(&t->as_entity.memory_areas_lock);
	INIT_LIST_HEAD(&t->as_entity.memory_areas);
#endif /* MARCEL_MAMI_ENABLED */
	if (ma_holder_type(t->as_entity.sched_holder) == MA_RUNQUEUE_HOLDER)
		sched_debug("%p(%s)'s holder is %s (prio %d)\n", t, t->name, ma_rq_holder(t->as_entity.sched_holder)->name, t->as_entity.prio);
	else
		sched_debug("%p(%s)'s holder is bubble %p (prio %d)\n", t, t->name, ma_bubble_holder(t->as_entity.sched_holder), t->as_entity.prio);
	LOG_OUT();
}

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t,
		const marcel_attr_t *attr);
#section sched_marcel_inline
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t,
		const marcel_attr_t *attr)
{
	LOG_IN();
	t->as_entity.type = MA_THREAD_ENTITY;
	marcel_sched_internal_init_marcel_task(t, attr);
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, t, ma_stats_nbthreads_offset, 1);
	ma_task_stats_set(long, t, ma_stats_nbthreadseeds_offset, 0);
#endif /* MARCEL_STATS_ENABLED */

#ifdef MA__BUBBLES
	/* bulle non initialisée */
	t->bubble.as_entity.natural_holder = NULL;
#endif
	ma_atomic_init(&t->as_entity.time_slice,MARCEL_TASK_TIMESLICE);
	LOG_OUT();
}

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_init_thread_seed(marcel_task_t* t,
		const marcel_attr_t *attr);
#section sched_marcel_inline
__tbx_inline__ static void 
marcel_sched_init_thread_seed(marcel_task_t* t,
		const marcel_attr_t *attr)
{
	LOG_IN();
	t->as_entity.type = MA_THREAD_SEED_ENTITY;
	marcel_sched_internal_init_marcel_task(t, attr);
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, t, ma_stats_nbthreads_offset, 0);
	ma_task_stats_set(long, t, ma_stats_nbthreadseeds_offset, 1);
#endif /* MARCEL_STATS_ENABLED */
	
	LOG_OUT();
}

#section marcel_functions
/* unsigned marcel_sched_add_vp(void); */
void *marcel_sched_seed_runner(void *arg);

#section marcel_macros
#section macros
/* ==== SMP scheduling policies ==== */

#define MARCEL_SCHED_INVALID	-1
/** Creates the thread on the global runqueue */
#define MARCEL_SCHED_SHARED      0
/** Creates the thread on another LWP (the next one, usually) */
#define MARCEL_SCHED_OTHER       1
/** Creates the thread on a non-loaded LWP or the same LWP */
#define MARCEL_SCHED_AFFINITY    2
/** Creates the thread on the least loaded LWP */
#define MARCEL_SCHED_BALANCE     3
#define __MARCEL_SCHED_AVAILABLE 4

#section types
#depend "marcel_threads.h[types]"
typedef unsigned (*marcel_schedpolicy_func_t)(marcel_t pid,
						   unsigned current_lwp);
#section marcel_variables
extern marcel_schedpolicy_func_t ma_user_policy[MARCEL_MAX_USER_SCHED];
#section functions
void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func);

/* ==== scheduling policies ==== */
#section types
struct marcel_sched_param {
	int __sched_priority;
};
#ifndef sched_priority
#define sched_priority __sched_priority
#endif

#section functions
int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p);
int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p);
int marcel_sched_setscheduler(marcel_t t, int policy, const struct marcel_sched_param *p);
int marcel_sched_getscheduler(marcel_t t);

#section marcel_macros
#ifdef MARCEL_STATS_ENABLED
#define ma_task_stats_get(t,offset) ma_stats_get(&(t)->as_entity,(offset))
#define ma_task_stats_set(cast,t,offset,val) ma_stats_set(cast, &(t)->as_entity,(offset),val)
#else /* MARCEL_STATS_ENABLED */
#define ma_task_stats_set(cast,t,offset,val)
#endif /* MARCEL_STATS_ENABLED */

/* ==== SMP scheduling directives ==== */
#section functions

void marcel_apply_vpset(const marcel_vpset_t *set);

/* ==== scheduler status ==== */

extern TBX_EXTERN void marcel_freeze_sched(void);
extern TBX_EXTERN void marcel_unfreeze_sched(void);

#section marcel_functions
extern void ma_freeze_thread(marcel_task_t * tsk);
extern void ma_unfreeze_thread(marcel_task_t * tsk);

/* ==== miscelaneous private defs ==== */

#ifdef MA__DEBUG
extern void marcel_breakpoint(void);
#endif

marcel_t marcel_give_hand_from_upcall_new(marcel_t cur, marcel_lwp_t *lwp);

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Création d'un nouveau thread                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#section marcel_functions
static __tbx_setjmp_inline__
int marcel_sched_internal_create(marcel_task_t *cur, 
					       marcel_task_t *new_task,
					       __const marcel_attr_t *attr,
					       __const int dont_schedule,
					       __const unsigned long base_stack);
int marcel_sched_internal_create_dontstart(marcel_task_t *cur, 
					       marcel_task_t *new_task,
					       __const marcel_attr_t *attr,
					       __const int dont_schedule,
					       __const unsigned long base_stack);

int marcel_sched_internal_create_start(marcel_task_t *cur, 
					       marcel_task_t *new_task,
					       __const marcel_attr_t *attr,
					       __const int dont_schedule,
					       __const unsigned long base_stack);

#section marcel_inline
static __tbx_setjmp_inline__
int marcel_sched_internal_create(marcel_task_t *cur, marcel_task_t *new_task,
				 __const marcel_attr_t *attr,
				 __const int dont_schedule,
				 __const unsigned long base_stack)
{ 
#ifdef MA__BUBBLES
	ma_holder_t *bh;
#endif
	LOG_IN();
#ifdef MA__BUBBLES
	if ((bh=ma_task_natural_holder(new_task)) && bh->type != MA_BUBBLE_HOLDER)
		bh = NULL;
#endif

	/* On est encore dans le père. On doit démarrer le fils */
	if(
	        /* On ne peut pas céder la main maintenant */
		(ma_in_atomic())
		/* ou bien on ne veut pas  */
		|| !ma_thread_preemptible()
		|| dont_schedule
#ifdef MA__LWPS
		/* On ne peut pas placer ce thread sur le LWP courant */
		|| (!marcel_vpset_isset(&THREAD_GETMEM(new_task,vpset),ma_vpnum(MA_LWP_SELF)))
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
		)
		LOG_RETURN(marcel_sched_internal_create_dontstart(cur, new_task, attr, dont_schedule, base_stack));
	else
		LOG_RETURN(marcel_sched_internal_create_start(cur, new_task, attr, dont_schedule, base_stack));
}

#section marcel_structures
struct ma_lwp_usage_stat {
	unsigned long long user;
	unsigned long long nice;
	//unsigned long long system;
	unsigned long long softirq;
	unsigned long long irq;
	unsigned long long idle;
	//unsigned long long iowait;
};
