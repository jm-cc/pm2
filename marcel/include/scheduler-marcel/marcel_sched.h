
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

#section default [no-include-in-main,no-include-in-master-section]

#section common
#include "tbx_compiler.h"
#section sched_marcel_functions [no-depend-previous]

#section sched_marcel_inline

/****************************************************************/
/* Protections
 */
#section common
#ifndef MARCEL_SCHED_INTERNAL_INCLUDE
#error This file should not be directely included. Use "marcel_sched.h" instead
#endif
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
	ma_holder_t *init_holder;
	tbx_bool_t inheritholder;
};

#section variables
extern marcel_sched_attr_t marcel_sched_attr_default;

#section macros
#define MARCEL_SCHED_ATTR_INITIALIZER { \
	.sched_policy = MARCEL_SCHED_DEFAULT, \
	.init_holder = NULL, \
	.inheritholder = tbx_false, \
}

#define MARCEL_SCHED_ATTR_DESTROYER { \
   .sched_policy = -1, \
	.init_holder = NULL, \
	.inheritholder = tbx_false, \
}
#section functions
int marcel_sched_attr_init(marcel_sched_attr_t *attr);
#define marcel_sched_attr_destroy(attr_ptr)	0

int marcel_sched_attr_setinitholder(marcel_sched_attr_t *attr, ma_holder_t *h) __THROW;
int marcel_sched_attr_getinitholder(__const marcel_sched_attr_t *attr, ma_holder_t **h) __THROW;

int marcel_sched_attr_setinitrq(marcel_sched_attr_t *attr, ma_runqueue_t *rq) __THROW;
#define marcel_sched_attr_setinitrq(attr, rq) marcel_sched_attr_setinitholder(attr, &(rq)->hold)
int marcel_sched_attr_getinitrq(__const marcel_sched_attr_t *attr, ma_runqueue_t **rq) __THROW;

#ifdef MA__BUBBLES
int marcel_sched_attr_setinitbubble(marcel_sched_attr_t *attr, marcel_bubble_t *bubble) __THROW;
#define marcel_sched_attr_setinitbubble(attr, bubble) marcel_sched_attr_setinitholder(attr, &(bubble)->hold)
int marcel_sched_attr_getinitbubble(__const marcel_sched_attr_t *attr, marcel_bubble_t **bubble) __THROW;
#endif

int marcel_sched_attr_setinheritholder(marcel_sched_attr_t *attr, int yes) __THROW;
int marcel_sched_attr_getinheritholder(__const marcel_sched_attr_t *attr, int *yes) __THROW;

int marcel_sched_attr_setschedpolicy(marcel_sched_attr_t *attr, int policy) __THROW;
int marcel_sched_attr_getschedpolicy(__const marcel_sched_attr_t * __restrict attr,
                               int * __restrict policy) __THROW;

#section macros
#define marcel_attr_setinitrq(attr,rq) marcel_sched_attr_setinitrq(&(attr)->sched,rq)
#define marcel_attr_getinitrq(attr,rq) marcel_sched_attr_getinitrq(&(attr)->sched,rq)
#define marcel_attr_setinitholder(attr,holder) marcel_sched_attr_setinitholder(&(attr)->sched,holder)
#define marcel_attr_getinitholder(attr,holder) marcel_sched_attr_getinitholder(&(attr)->sched,holder)
#ifdef MA__BUBBLES
#define marcel_attr_setinitbubble(attr,bubble) marcel_sched_attr_setinitbubble(&(attr)->sched,bubble)
#define marcel_attr_getinitbubble(attr,bubble) marcel_sched_attr_getinitbubble(&(attr)->sched,bubble)
#endif

#define marcel_attr_setinheritholder(attr,yes) marcel_sched_attr_setinheritholder(&(attr)->sched,yes)
#define marcel_attr_getinheritholder(attr,yes) marcel_sched_attr_getinheritholder(&(attr)->sched,yes)

#define marcel_attr_setschedpolicy(attr,policy) marcel_sched_attr_setschedpolicy(&(attr)->sched,policy)
#define marcel_attr_getschedpolicy(attr,policy) marcel_sched_attr_getschedpolicy(&(attr)->sched,policy)

#section functions
#ifdef MA__LWPS
unsigned marcel_add_lwp(void);
#else
#define marcel_add_lwp() (0)
#endif

/****************************************************************/
/* Structure interne pour une t�che
 */

#section marcel_types
#depend "scheduler/marcel_holder.h[marcel_structures]"
typedef struct {
	struct ma_sched_entity entity;
	unsigned long timestamp, last_ran;
	marcel_bubble_t bubble;
} marcel_sched_internal_task_t;

#section marcel_functions
int ma_wake_up_task(marcel_task_t * p);

#section marcel_functions
#depend "scheduler/linux_runqueues.h[marcel_types]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpmask_init_rq(const marcel_vpmask_t *mask);

#section marcel_inline
#depend "scheduler/linux_runqueues.h[marcel_types]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpmask_init_rq(const marcel_vpmask_t *mask)
{
	if (tbx_unlikely(*mask==MARCEL_VPMASK_EMPTY))
		return &ma_main_runqueue;
	else if (tbx_unlikely(*mask==MARCEL_VPMASK_FULL))
		return &ma_dontsched_runqueue;
	else {
		int first_vp;
		first_vp=ma_ffs(~*mask)-1;
		/* pour l'instant, on ne g�re qu'un vp activ� */
		MA_BUG_ON(*mask!=MARCEL_VPMASK_ALL_BUT_VP(first_vp));
		/* on peut arriver sur un lwp suppl�mentaire, il faudrait un autre compteur que nbvps */
		/* MA_BUG_ON(first_vp && first_vp>=marcel_nbvps()); */
		return &marcel_topo_vp_level[first_vp].sched;
	}
}

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t,
		marcel_sched_internal_task_t *internal,
		const marcel_attr_t *attr);
#section sched_marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
#depend "marcel_topology.h[marcel_structures]"
#depend "scheduler/linux_runqueues.h[marcel_variables]"
#depend "scheduler/marcel_holder.h[marcel_macros]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "[marcel_variables]"
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t, 
		marcel_sched_internal_task_t *internal,
		const marcel_attr_t *attr)
{
	ma_holder_t *h = NULL;
	DEFINE_CUR_LWP(register, =, LWP_SELF);
	LOG_IN();
	internal->entity.type = MA_TASK_ENTITY;
	if (attr->sched.init_holder) {
		h = attr->sched.init_holder;
#ifdef MA__BUBBLES
	} else if (attr->sched.inheritholder) {
		/* TODO: on suppose ici que la bulle est �clat�e et qu'elle ne sera pas referm�e d'ici qu'on wake_up_created ce thread */
		h = ma_task_init_holder(MARCEL_SELF);
#endif
	}
	internal->entity.sched_policy = attr->sched.sched_policy;
	internal->entity.run_holder=NULL;
	internal->entity.holder_data=NULL;
#ifdef MA__BUBBLES
	INIT_LIST_HEAD(&internal->entity.bubble_entity_list);
#endif
	if (h) {
		internal->entity.sched_holder =
#ifdef MA__BUBBLES
			internal->entity.init_holder =
#endif
			h;
	} else do {
#ifdef MA__BUBBLES
		marcel_bubble_t *b = &SELF_GETMEM(sched).internal.bubble;
		if (!b->sched.init_holder) {
			marcel_bubble_init(b);
			h = ma_task_init_holder(MARCEL_SELF);
			if (!h)
				h = &ma_main_runqueue.hold;
			b->sched.init_holder = h;
			if (h->type != MA_RUNQUEUE_HOLDER) {
				marcel_bubble_t *bb = ma_bubble_holder(h);
				b->sched.sched_level = bb->sched.sched_level + 1;
				marcel_bubble_insertbubble(bb, b);
			}
#ifdef MARCEL_BUBBLE_STEAL
			if (b->sched.sched_level == MARCEL_LEVEL_KEEPCLOSED) {
				ma_runqueue_t *rq;
				marcel_bubble_detach(b);
				rq = ma_to_rq_holder(h);
				if (!rq)
					rq = &ma_main_runqueue;
				b->sched.sched_holder = &rq->hold;
				ma_put_entity(&b->sched, &rq->hold, MA_ENTITY_BLOCKED);
				PROF_EVENT2(bubble_sched_switchrq, b, rq);
			}
#endif
		}
		marcel_bubble_insertentity(b, &internal->entity);
#ifdef MARCEL_BUBBLE_STEAL
		if (b->sched.sched_level >= MARCEL_LEVEL_KEEPCLOSED)
			/* keep this thread inside the bubble */
			break;
#endif
#endif
#ifndef MARCEL_BUBBLE_EXPLODE
		ma_runqueue_t *rq;
		if (attr->vpmask != MARCEL_VPMASK_EMPTY)
			rq = marcel_sched_vpmask_init_rq(&attr->vpmask);
		else {
#ifdef MA__LWPS
		rq = NULL;
		switch (internal->entity.sched_policy) {
			case MARCEL_SCHED_SHARED:
				rq = &ma_main_runqueue;
				break;
/* TODO: vpmask ? */
			case MARCEL_SCHED_OTHER: {
				struct marcel_topo_level *vp;
				for_vp_from(vp, LWP_NUMBER(cur_lwp)) {
					rq = &vp->sched;
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
				for_vp_from(vp, LWP_NUMBER(cur_lwp)) {
					rq2 = &vp->sched;
					if (rq2->hold.nr_running < THREAD_THRESHOLD_LOW) {
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
				unsigned best = ma_lwp_vprq(cur_lwp)->hold.nr_running;
				struct marcel_topo_level *vp;
				ma_runqueue_t *rq2;
				rq = ma_lwp_vprq(cur_lwp);
				for_vp_from(vp, LWP_NUMBER(cur_lwp)) {
					rq2 = &vp->sched;
					if (rq2->hold.nr_running < best) {
						rq = rq2;
						best = rq2->hold.nr_running;
					}
				}
				break;
			}
			default: {
				unsigned num = ma_user_policy[internal->entity.sched_policy - __MARCEL_SCHED_AVAILABLE](t, LWP_NUMBER(cur_lwp));
				rq = ma_lwp_vprq(GET_LWP_BY_NUM(num));
				break;
			}
		}
		if (!rq)
#endif
			rq = &ma_main_runqueue;
		}
		internal->entity.sched_holder = &rq->hold;
		MA_BUG_ON(!rq->name[0]);
#endif
	} while (0);
	INIT_LIST_HEAD(&internal->entity.run_list);
	internal->entity.prio=attr->__schedparam.__sched_priority;
	PROF_EVENT2(sched_setprio,ma_task_entity(&internal->entity),internal->entity.prio);
	/* timestamp, last_ran */
	ma_atomic_init(&internal->entity.time_slice,MARCEL_TASK_TIMESLICE);
#ifdef MA__LWPS
	internal->entity.sched_level=MARCEL_LEVEL_DEFAULT;
#endif
	if (ma_holder_type(internal->entity.sched_holder) == MA_RUNQUEUE_HOLDER)
		sched_debug("%p(%s)'s holder is %s (prio %d)\n", t, t->name, ma_rq_holder(internal->entity.sched_holder)->name, internal->entity.prio);
	else
		sched_debug("%p(%s)'s holder is bubble %p (prio %d)\n", t, t->name, ma_bubble_holder(internal->entity.sched_holder), internal->entity.prio);
#ifdef MA__BUBBLES
	/* bulle non initialis�e */
	internal->bubble.sched.init_holder = NULL;
#endif
	LOG_OUT();
}



#section marcel_functions
/* unsigned marcel_sched_add_vp(void); */

#section marcel_macros
#section macros
/* ==== SMP scheduling policies ==== */

#define MARCEL_SCHED_INVALID	-1
#define MARCEL_SCHED_SHARED      0
#define MARCEL_SCHED_OTHER       1
#define MARCEL_SCHED_AFFINITY    2
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

/* ==== SMP scheduling directives ==== */

void marcel_change_vpmask(marcel_vpmask_t *mask);

/* ==== scheduler status ==== */

unsigned marcel_activethreads(void);
unsigned marcel_sleepingthreads(void);
unsigned marcel_blockedthreads(void);
unsigned marcel_frozenthreads(void);


extern MARCEL_PROTECTED void marcel_freeze_sched(void);
extern MARCEL_PROTECTED void marcel_unfreeze_sched(void);

#section marcel_functions
extern void ma_freeze_thread(marcel_task_t * tsk);
extern void ma_unfreeze_thread(marcel_task_t * tsk);

/* ==== miscelaneous private defs ==== */

#ifdef MA__DEBUG
extern void marcel_breakpoint();
#endif

int __marcel_tempo_give_hand(unsigned long timeout, tbx_bool_t *blocked, marcel_sem_t *s);

marcel_t marcel_unchain_task_and_find_next(marcel_t t, 
						     marcel_t find_next);
void marcel_insert_task(marcel_t t);
marcel_t marcel_radical_next_task(void);
marcel_t marcel_give_hand_from_upcall_new(marcel_t cur, marcel_lwp_t *lwp);

int marcel_check_sleeping(void);

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Cr�ation d'un nouveau thread                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#section sched_marcel_functions
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

#section sched_marcel_inline
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
	if ((bh=ma_task_init_holder(new_task)) && bh->type != MA_BUBBLE_HOLDER)
		bh = NULL;
#endif

	/* On est encore dans le p�re. On doit d�marrer le fils */
	if(
	        /* On ne peut pas c�der la main maintenant */
		(ma_in_atomic())
		/* ou bien on ne veut pas  */
		|| !ma_thread_preemptible()
		/* On n'a pas encore de scheduler... */
		|| dont_schedule
#ifdef MA__LWPS
		/* On ne peut pas placer ce thread sur le LWP courant */
		|| (!lwp_isset(LWP_NUMBER(LWP_SELF), THREAD_GETMEM(new_task, sched.lwps_allowed)))
		/* Si la politique est du type 'placer sur le LWP le moins
		   charg�', alors on ne peut pas placer ce thread sur le LWP
		   courant */
		|| (new_task->sched.internal.entity.sched_policy != MARCEL_SCHED_OTHER)
#endif
#ifdef MA__BUBBLES
		/* on est plac� dans une bulle (on ne sait donc pas si l'on a
		   le droit d'�tre ordonnanc� tout de suite)
		   XXX: �a veut dire qu'on n'a pas de cr�ation rapide avec les
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
	unsigned long long system;
	unsigned long long softirq;
	unsigned long long irq;
	unsigned long long idle;
	unsigned long long iowait;
};
