

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

#include "marcel.h"
#include "tbx_compiler.h"

#warning "[1;33m<<< [1;37mScheduler [1;32mmarcel[1;37m selected[1;33m >>>[0m"

marcel_sched_attr_t marcel_sched_attr_default = MARCEL_SCHED_ATTR_INITIALIZER;

int marcel_sched_attr_init(marcel_sched_attr_t *attr)
{
  *attr = marcel_sched_attr_default;
  return 0;
}

int marcel_sched_attr_setinitholder(marcel_sched_attr_t *attr, ma_holder_t *h)
{
  attr->init_holder = h;
  return 0;
}

int marcel_sched_attr_getinitholder(__const marcel_sched_attr_t *attr, ma_holder_t **h)
{
  *h = attr->init_holder;
  return 0;
}

int marcel_sched_attr_getinitrq(__const marcel_sched_attr_t *attr, ma_runqueue_t **rq)
{
  ma_holder_t *h = attr->init_holder;
  MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
  *rq = ma_rq_holder(h);
  return 0;
}

#ifdef MA__BUBBLES
int marcel_sched_attr_getinitbubble(__const marcel_sched_attr_t *attr, marcel_bubble_t **b)
{
  ma_holder_t *h = attr->init_holder;
  MA_BUG_ON(h->type != MA_BUBBLE_HOLDER);
  *b = ma_bubble_holder(h);
  return 0;
}
#endif

int marcel_sched_attr_setprio(marcel_sched_attr_t *attr, int prio)
{
  attr->prio = prio;
  return 0;
}

int marcel_sched_attr_getprio(__const marcel_sched_attr_t *attr, int *prio)
{
  *prio = attr->prio;
  return 0;
}

int marcel_sched_attr_setinheritholder(marcel_sched_attr_t *attr, int yes)
{
  attr->inheritholder = yes;
  return 0;
}

int marcel_sched_attr_getinheritholder(__const marcel_sched_attr_t *attr, int *yes)
{
  *yes = attr->inheritholder;
  return 0;
}

static volatile unsigned __active_threads,
  __sleeping_threads,
  __blocked_threads,
  __frozen_threads;


#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
static LIST_HEAD(__waiting_tasks);
#endif
static LIST_HEAD(__delayed_tasks);

/* These two locks must be acquired before accessing the corresponding
   global queue.  They should only encapsulate *non-blocking* code
   sections. */
//static marcel_lock_t __delayed_lock = MARCEL_LOCK_INIT;
//static marcel_lock_t __blocking_lock = MARCEL_LOCK_INIT;

static unsigned next_schedpolicy_available = __MARCEL_SCHED_AVAILABLE;
marcel_schedpolicy_func_t ma_user_policy[MARCEL_MAX_USER_SCHED];
void marcel_schedpolicy_create(int *policy,
			       marcel_schedpolicy_func_t func)
{
  if(next_schedpolicy_available >=
     MARCEL_MAX_USER_SCHED + __MARCEL_SCHED_AVAILABLE)
    MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);

  ma_user_policy[next_schedpolicy_available - __MARCEL_SCHED_AVAILABLE] = func;
  *policy = next_schedpolicy_available++;
}

int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p) {
	return ma_sched_change_prio(t,p->sched_priority);
}

int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p) {
	p->sched_priority=t->sched.internal.prio;
	return 0;
}

int marcel_sched_setscheduler(marcel_t t, int policy, const struct marcel_sched_param *p) {
	t->sched.internal.sched_policy=policy;
	return ma_sched_change_prio(t,p->sched_priority);
}

int marcel_sched_getscheduler(marcel_t t) {
	return t->sched.internal.sched_policy;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Consultation de variables                                      */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

unsigned marcel_activethreads(void)
{
   return __active_threads;
}

unsigned marcel_sleepingthreads(void)
{
  return __sleeping_threads;
}

unsigned marcel_blockedthreads(void)
{
  return __blocked_threads;
}

unsigned marcel_frozenthreads(void)
{
  return __frozen_threads;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                          statistiques                                  */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
/* TODO: Use atomic_inc and atomic_dec when accessing running_tasks! */
#define INC_STATS(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? SCHED_DATA(lwp).running_tasks++ : 0)
#define DEC_STATS(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? SCHED_DATA(lwp).running_tasks-- : 0)

#ifdef MA__ACTIVATION
#define ONE_MORE_ACTIVE_TASK_HOOK() \
  (nb_idle_sleeping ? act_cntl(ACT_CNTL_WAKE_UP,NULL) : 0)
#else
#define ONE_MORE_ACTIVE_TASK_HOOK() ((void)0)
#endif

#define one_more_active_task(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? __active_threads++ : 0), \
  ONE_MORE_ACTIVE_TASK_HOOK(), \
  INC_STATS((t), (lwp)), \
  MTRACE("Activation", (t))


#define one_active_task_less(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? __active_threads-- : 0), \
  DEC_STATS((t), (lwp))

#define one_more_sleeping_task(t) \
  __sleeping_threads++, MTRACE("Sleeping", (t))

#define one_sleeping_task_less(t) \
  __sleeping_threads--

#define one_more_blocked_task(t) \
  __blocked_threads++, MTRACE("Blocking", (t))

#define one_blocked_task_less(t) \
  __blocked_threads--

#define one_more_frozen_task(t) \
  __frozen_threads++, MTRACE("Freezing", (t))

#define one_frozen_task_less(t) \
  __frozen_threads--
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                            SCHEDULER                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

// Pour le debug. ATTENTION : sched_lock/unlock doivent être appelés
// avant et après !
#if 0
static __inline__ void display_sched_queue(marcel_lwp_t *lwp)
{
  marcel_t t;
  int max_iter=10;

  LOG_IN();
  t = SCHED_DATA(lwp).first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of regular tasks on LWP %d:\n", LWP_NUMBER(lwp));
    do {
#ifdef MA__MULTIPLE_RUNNING
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, ext_state %.8x, sf %.8lx)\n",
	     t, t->number, LWP_NUMBER(GET_LWP(t)), t->sched.internal.ext_state, t->flags);
#else
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, fl %.8lx)\n" ,
             t, t->number, LWP_NUMBER(GET_LWP(t)), t->flags);
#endif
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).first && (max_iter-- > 0));
    if (max_iter == -1) {
	    mdebug_sched_q("\t\t\t\tStopping list\n");
    }
    mdebug_sched_q("\t\t\t>\n");
  }

#ifdef MARCEL_RT
  t = SCHED_DATA(lwp).rt_first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of real-time tasks on LWP %d:\n", LWP_NUMBER(lwp));
    do {
#ifdef MA__MULTIPLE_RUNNING
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, ext_state %.8x, sf %.8lx)\n",
	     t, t->number, LWP_NUMBER(GET_LWP(t)), t->sched.internal.ext_state, t->flags);
#else
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, sf %.8lx)\n" ,
             t, t->number, LWP_NUMBER(GET_LWP(t)), t->flags);
#endif
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).rt_first);
    mdebug_sched_q("\t\t\t>\n");
  }
#endif
  LOG_OUT();
}
#endif


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Changement de contexte                                         */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
static int nr_running=0;

#define task_has_lwp(tsk) ((tsk)->sched.internal.cpus_runnable != ~0UL)

static inline void task_set_lwp(marcel_task_t *tsk, marcel_lwp_t *lwp)
{
        tsk->sched.lwp = lwp;
        tsk->sched.internal.lwps_runnable = 1UL << LWP_NUMBER(lwp);
}

static inline void task_release_cpu(marcel_task_t *tsk)
{
        tsk->sched.internal.lwps_runnable = ~0UL;
}

static inline void del_from_runqueue(marcel_task_t * p)
{
        nr_running--;
        //p->sleep_time = jiffies;
        list_del(&p->sched.internal.run_list);
        p->sched.internal.run_list.next = NULL;
}

static inline int task_on_runqueue(marcel_task_t *p)
{
        return (p->sched.internal.run_list.next != NULL);
}
#endif


#ifdef MA__LWPS
unsigned marcel_add_lwp(void) {
	return marcel_lwp_add_vp();
}
#endif
