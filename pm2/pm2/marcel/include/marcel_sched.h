
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

#ifndef MARCEL_SCHED_EST_DEF
#define MARCEL_SCHED_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/marcel_macros.h"
#include "sys/marcel_work.h"

/* ==== Starting and Shuting down the scheduler ==== */

void marcel_sched_init(unsigned nb_lwp);
void marcel_sched_start(void);
void marcel_sched_shutdown(void);


/* ==== get current thread or LWP id ==== */

static __inline__ marcel_t marcel_self()
{
  return __marcel_self();
}

static __inline__ unsigned marcel_current_vp() __attribute__ ((unused));
static __inline__ unsigned marcel_current_vp()
{
  DEFINE_CUR_LWP(, =, GET_LWP(marcel_self()));

  return cur_lwp->number;
}


/* ==== explicit preemption ==== */

void marcel_yield(void);

#ifdef MARCEL_KERNEL
void ma__marcel_yield(void);
#endif

#ifdef MARCEL_RT
void ma__marcel_find_and_yield_to_rt_task(void);
#endif

/* ==== SMP scheduling policies ==== */

#define MARCEL_MAX_USER_SCHED    16

#define MARCEL_SCHED_FIXED(vp)   (vp)
#define MARCEL_SCHED_OTHER       (-1)
#define MARCEL_SCHED_AFFINITY    (-2)
#define MARCEL_SCHED_BALANCE     (-3)
#define __MARCEL_SCHED_AVAILABLE (-4)

typedef __lwp_t *(*marcel_schedpolicy_func_t)(marcel_t pid,
					      __lwp_t *current_lwp);
void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func);

/* ==== SMP scheduling directives ==== */

// Primitives & macros de construction de "masques" de processeurs
// virtuels. 

// ATTENTION : le placement d'un thread est autorise sur un 'vp' si le
// bit correspondant est a _ZERO_ dans le masque (similitude avec
// sigset_t pour la gestion des masques de signaux).

#define MARCEL_VPMASK_EMPTY          ((marcel_vpmask_t)0)
#define MARCEL_VPMASK_FULL           ((marcel_vpmask_t)-1)
#define MARCEL_VPMASK_ONLY_VP(vp)    ((marcel_vpmask_t)(1U << (vp)))
#define MARCEL_VPMASK_ALL_BUT_VP(vp) ((marcel_vpmask_t)(~(1U << (vp))))

#define marcel_vpmask_init(m)        marcel_vpmask_empty(m)

static __inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask) __attribute__ ((unused));
static __inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask)
{
  *mask = 0;
}

static __inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask) __attribute__ ((unused));
static __inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask)
{
  *mask = -1;
}

static __inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp) __attribute__ ((unused));
static __inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
  *mask |= 1U << vp;
}

static __inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp) __attribute__ ((unused));
static __inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp)
{
  *mask = 1U << vp;
}

static __inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp) __attribute__ ((unused));
static __inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
  *mask &= ~(1U << vp);
}

static __inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp) __attribute__ ((unused));
static __inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp)
{
  *mask = ~(1U << vp);
}

static __inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t *mask,
						unsigned vp) __attribute__ ((unused));
static __inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t *mask,
						unsigned vp)
{
  return 1 & ((*mask) >> vp);
}


/* ==== `sleep' functions ==== */

void marcel_delay(unsigned long millisecs);


/* ==== scheduler status ==== */

unsigned marcel_nbvps(void);
unsigned marcel_nbthreads(void);
unsigned marcel_activethreads(void);
unsigned marcel_sleepingthreads(void);
unsigned marcel_blockedthreads(void);
unsigned marcel_frozenthreads(void);
unsigned long marcel_createdthreads(void);


/* ==== snapshot ==== */

typedef void (*snapshot_func_t)(marcel_t pid);

void marcel_snapshot(snapshot_func_t f);

#define ALL_THREADS		 0
#define MIGRATABLE_ONLY		 1
#define NOT_MIGRATABLE_ONLY	 2
#define DETACHED_ONLY		 4
#define NOT_DETACHED_ONLY	 8
#define BLOCKED_ONLY		16
#define NOT_BLOCKED_ONLY	32
#define SLEEPING_ONLY           64
#define NOT_SLEEPING_ONLY      128

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which);


/* ==== Scheduler locking ==== */

#ifdef MARCEL_RT
extern unsigned __rt_task_exist;
#endif

/* Must be called each time a LWP is about to access its local task
   queue. */
static __inline__ void ma_sched_lock(__lwp_t *lwp) __attribute__ ((unused));
static __inline__ void ma_sched_lock(__lwp_t *lwp)
{
  marcel_lock_acquire(&(SCHED_DATA(lwp).sched_queue_lock));
}

/* Must be called when a LWP is not modifying the local queue any
   more. */
static __inline__ void ma_sched_unlock(__lwp_t *lwp) __attribute__ ((unused));
static __inline__ void ma_sched_unlock(__lwp_t *lwp)
{
  marcel_lock_release(&(SCHED_DATA(lwp).sched_queue_lock));
}

static __inline__ void ma_lock_task(void) __attribute__ ((unused));
static __inline__ void ma_lock_task(void)
{
  atomic_inc(&GET_LWP(marcel_self())->_locked);
}

static __inline__ void ma_unlock_task(void) __attribute__ ((unused));
static __inline__ void ma_unlock_task(void)
{
  volatile atomic_t *locked = &GET_LWP(marcel_self())->_locked;

#ifdef MARCEL_RT
  if(__rt_task_exist &&
     atomic_read(locked) == 1 &&
     !MA_TASK_REAL_TIME(marcel_self()))
    ma__marcel_find_and_yield_to_rt_task();
#endif

#ifdef MA__WORK
  if ((atomic_read(locked) == 1) 
      && (marcel_self()->has_work || marcel_global_work)) {
    do_work(marcel_self());
  }
#endif
  atomic_dec(locked);

#if defined(PM2DEBUG) && defined(MA__ACTIVATION)
  pm2debug_flush();
#endif
}

static __inline__ void unlock_task_for_debug(void) __attribute__ ((unused));
static __inline__ void unlock_task_for_debug(void)
{
  atomic_dec(&GET_LWP(marcel_self())->_locked);
}

static __inline__ int locked(void) __attribute__ ((unused));
static __inline__ int locked(void)
{
  return atomic_read(&GET_LWP(marcel_self())->_locked);
}

extern volatile atomic_t __preemption_disabled;

static __inline__ void disable_preemption(void) __attribute__ ((unused));
static __inline__ void disable_preemption(void)
{
  atomic_inc(&__preemption_disabled);
}

static __inline__ void enable_preemption(void) __attribute__ ((unused));
static __inline__ void enable_preemption(void)
{
  atomic_dec(&__preemption_disabled);
}

static __inline__ unsigned int preemption_enabled(void) __attribute__ ((unused));
static __inline__ unsigned int preemption_enabled(void)
{
  return atomic_read(&__preemption_disabled) == 0;
}

#ifdef DEBUG_LOCK_TASK
#define unlock_task() \
   (ma_unlock_task(), \
   lock_task_debug("\t=> unlock --%i\n", locked()))
#define lock_task() \
   (lock_task_debug("\t=> lock %i++\n", locked()), \
   ma_lock_task())
#else
#define unlock_task() ma_unlock_task()
#define lock_task() ma_lock_task()
#endif

#ifdef DEBUG_SCHED_LOCK
#define sched_lock(lwp) \
   (sched_lock_debug("sched_lock: try\n"), \
   ma_sched_lock(lwp),                     \
   sched_lock_debug("sched_lock: success\n"))
#define sched_unlock(lwp) \
   (sched_lock_debug("sched_unlock\n"), \
   ma_sched_unlock(lwp))
#else
#define sched_lock(lwp) ma_sched_lock(lwp)
#define sched_unlock(lwp) ma_sched_unlock(lwp)
#endif

/* ==== miscelaneous private defs ==== */

#ifdef MA__DEBUG
extern void breakpoint();
#endif

_PRIVATE_ void marcel_one_task_less(marcel_t pid);
_PRIVATE_ void marcel_one_more_task(marcel_t pid);

_PRIVATE_ void marcel_give_hand(boolean *blocked, marcel_lock_t *lock);
_PRIVATE_ void marcel_tempo_give_hand(unsigned long timeout, boolean *blocked, marcel_sem_t *s);
_PRIVATE_ void marcel_wake_task(marcel_t t, boolean *blocked);
_PRIVATE_ marcel_t marcel_unchain_task_and_find_next(marcel_t t, 
						     marcel_t find_next);
_PRIVATE_ void marcel_insert_task(marcel_t t);
_PRIVATE_ marcel_t marcel_radical_next_task(void);

#ifdef USE_VIRTUAL_TIMER

#define MARCEL_TIMER_SIGNAL   SIGVTALRM
#define MARCEL_ITIMER_TYPE    ITIMER_VIRTUAL

#else

#define MARCEL_TIMER_SIGNAL   SIGALRM
#define MARCEL_ITIMER_TYPE    ITIMER_REAL

#endif

void marcel_update_time(marcel_t cur);

#endif
