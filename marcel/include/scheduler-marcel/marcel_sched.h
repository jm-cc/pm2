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


#ifndef __MARCEL_SCHED_H__
#define __MARCEL_SCHED_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "scheduler-marcel/marcel_sched_types.h"
#include "scheduler-marcel/linux_runqueues.h"
#include "marcel_attr.h"


/** Public macros **/
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

/* ==== SMP scheduling policies ==== */
/** \brief Reserved code for an invalid scheduling policy */
#define MARCEL_SCHED_INVALID	-1
/** \brief Creates the thread on the global runqueue */
#define MARCEL_SCHED_SHARED      0
/** \brief Creates the thread on another LWP (the next one, usually) */
#define MARCEL_SCHED_OTHER       1
/** \brief Creates the thread on a non-loaded LWP or the same LWP */
#define MARCEL_SCHED_AFFINITY    2
/** \brief Creates the thread on the least loaded LWP */
#define MARCEL_SCHED_BALANCE     3
/** \brief Number of available scheduling policies */
#define __MARCEL_SCHED_AVAILABLE 4


/** Public global variables **/
extern marcel_sched_attr_t marcel_sched_attr_default;


/** Public functions **/
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


/****************************************************************/
/* Structure interne pour une tâche
 */

void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func);

/* ==== scheduling policies ==== */
int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p);
int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p);
int marcel_sched_setscheduler(marcel_t t, int policy, const struct marcel_sched_param *p);
int marcel_sched_getscheduler(marcel_t t);


void marcel_apply_vpset(const marcel_vpset_t *set);

/* ==== scheduler status ==== */

extern TBX_EXTERN void marcel_freeze_sched(void);
extern TBX_EXTERN void marcel_unfreeze_sched(void);


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MARCEL_STATS_ENABLED
#define ma_task_stats_get(t,offset) ma_stats_get(&(t)->as_entity,(offset))
#define ma_task_stats_set(cast,t,offset,val) ma_stats_set(cast, &(t)->as_entity,(offset),val)
#else /* MARCEL_STATS_ENABLED */
#define ma_task_stats_set(cast,t,offset,val)
#endif /* MARCEL_STATS_ENABLED */


/** Internal global variables **/
extern marcel_schedpolicy_func_t ma_user_policy[MARCEL_MAX_USER_SCHED];


/** Internal functions **/
int ma_wake_up_task(marcel_task_t * p);

__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpset_init_rq(const marcel_vpset_t *vpset);

/* unsigned marcel_sched_add_vp(void); */
void *marcel_sched_seed_runner(void *arg);

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

__tbx_inline__ static void
marcel_sched_internal_init_marcel_task(marcel_task_t* t,
		const marcel_attr_t *attr);
__tbx_inline__ static void
marcel_sched_internal_init_marcel_thread(marcel_task_t* t,
		const marcel_attr_t *attr);
__tbx_inline__ static void
marcel_sched_init_thread_seed(marcel_task_t* t,
		const marcel_attr_t *attr);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_SCHED_H__ **/
