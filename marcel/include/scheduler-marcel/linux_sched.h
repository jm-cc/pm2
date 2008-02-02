
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

/** \file
 * \brief Linux scheduler
 */
#section common
#include "tbx_compiler.h"
/*
 * Similar to:
 * include/linux/interrupt.h
 */

#section marcel_variables
#depend "sys/marcel_lwp.h[marcel_macros]"
#depend "asm-generic/linux_perlwp.h[marcel_macros]"
#depend "marcel_descr.h[types]"
extern int nr_threads;
/* extern int last_tid; */

#section marcel_functions
/* extern int ma_nr_threads(void); */
extern unsigned long ma_nr_ready(void);
/* extern unsigned long ma_nr_uninterruptible(void); */
/* extern unsigned long nr_iowait(void); */

/*
 * Scheduling policies
 */
/* #define SCHED_NORMAL		0 */
/* #define SCHED_FIFO		1 */
/* #define SCHED_RR		2 */

/* struct sched_param { */
/* 	int sched_priority; */
/* }; */

#section marcel_variables
#depend "asm/linux_rwlock.h[marcel_types]"
/*
 * This serializes "schedule()" and also protects
 * the run-queue from deletions/modifications (but
 * _adding_ to the beginning of the run-queue has
 * a separate lock).
 */
extern ma_rwlock_t tasklist_lock;

/* void io_schedule(void); */
/* long io_schedule_timeout(long timeout); */

/* extern void update_process_times(int user); */
/* extern void update_one_process(marcel_task_t *p, unsigned long user, */
/* 			       unsigned long system, int cpu); */
extern TBX_EXTERN void ma_scheduler_tick(int user_tick, int system);
/* extern unsigned long cache_decay_ticks; */

#section marcel_macros
#include <limits.h>
#define	MA_MAX_SCHEDULE_TIMEOUT	LONG_MAX

#section marcel_functions
void ma_resched_task(marcel_task_t *p, int vp, ma_lwp_t lwp);
asmlinkage TBX_EXTERN int ma_schedule(void);
asmlinkage void ma_schedule_tail(marcel_task_t *prev);

#section marcel_functions
extern int ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync);
extern int FASTCALL(ma_wake_up_state(marcel_task_t * tsk, unsigned int state));
extern TBX_EXTERN int FASTCALL(ma_wake_up_thread(marcel_task_t * tsk));
extern int FASTCALL(ma_wake_up_thread_async(marcel_task_t * tsk));
#ifdef MA__LWPS
 extern void ma_kick_process(marcel_task_t * tsk);
#else
 static __tbx_inline__ void ma_kick_process(marcel_task_t *tsk) { }
#endif

#section functions
extern void marcel_wake_up_created_thread(marcel_task_t * tsk);

#section marcel_functions

int ma_sched_change_prio(marcel_t t, int prio);

#section marcel_functions
#ifdef MA__LWPS
extern void ma_wait_task_inactive(marcel_task_t * p);
#else
#define ma_wait_task_inactive(p)	do { } while (0)
#endif

#section marcel_inline
#depend "linux_thread_info.h[marcel_inline]"
/* Protects ->fs, ->files, ->mm, and synchronises with wait4().
 * Nests both inside and outside of read_lock(&tasklist_lock).
 * It must not be nested with write_lock_irq(&tasklist_lock),
 * neither inside nor outside.
 */
#if 0
static __tbx_inline__ void ma_task_lock(marcel_task_t *p)
{
	ma_spin_lock(&p->alloc_lock);
}

static __tbx_inline__ void ma_task_unlock(marcel_task_t *p)
{
	ma_spin_unlock(&p->alloc_lock);
}
#endif 

extern TBX_EXTERN void __ma_cond_resched(void);
static __tbx_inline__ void ma_cond_resched(void)
{
	if (ma_get_need_resched())
		__ma_cond_resched();
}

/*
 * cond_resched_lock() - if a reschedule is pending, drop the given lock,
 * call schedule, and on return reacquire the lock.
 *
 * This works OK both with and without CONFIG_PREEMPT.  We do strange low-level
 * operations here to prevent schedule() from being called twice (once via
 * spin_unlock(), once by hand).
 */
static __tbx_inline__ void ma_cond_resched_lock(ma_spinlock_t * lock)
{
	if (ma_get_need_resched()) {
		_ma_raw_spin_unlock(lock);
		ma_preempt_enable_no_resched();
		__ma_cond_resched();
		ma_spin_lock(lock);
	}
}

/*
 * Wrappers for p->thread_info->cpu access. No-op on UP.
 */
static __tbx_inline__ ma_lwp_t ma_task_lwp(marcel_task_t *p)
{
	return GET_LWP(p);
}

static __tbx_inline__ void ma_set_task_lwp(marcel_task_t *p, ma_lwp_t lwp)
{
	SET_LWP(p, lwp);
}


#section marcel_functions
int marcel_idle_lwp(ma_lwp_t lwp);
int marcel_task_prio(marcel_task_t *p);
int marcel_task_curr(marcel_task_t *p);
