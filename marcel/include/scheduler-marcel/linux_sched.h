
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
#depend "asm/linux_rwlock.h[marcel_types]"
extern int nr_threads;

/*
 * This serializes "schedule()" and also protects
 * the run-queue from deletions/modifications (but
 * _adding_ to the beginning of the run-queue has
 * a separate lock).
 */
extern ma_rwlock_t tasklist_lock;
extern TBX_EXTERN void ma_scheduler_tick(int user_tick, int system);

#section marcel_macros
#include <limits.h>
#define	MA_MAX_SCHEDULE_TIMEOUT	LONG_MAX

#section marcel_functions
extern unsigned long ma_nr_ready(void);
asmlinkage TBX_EXTERN int ma_schedule(void);
asmlinkage void ma_schedule_tail(marcel_task_t *prev);
extern int ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync);
extern int FASTCALL(ma_wake_up_state(marcel_task_t * tsk, unsigned int state));
extern TBX_EXTERN int FASTCALL(ma_wake_up_thread(marcel_task_t * tsk));
extern int FASTCALL(ma_wake_up_thread_async(marcel_task_t * tsk));
#ifdef MA__LWPS
extern void ma_kick_process(marcel_task_t * tsk);
#else
static __tbx_inline__ void ma_kick_process(marcel_task_t *tsk) { }
#endif
int ma_sched_change_prio(marcel_t t, int prio);
#ifdef MA__LWPS
extern void ma_wait_task_inactive(marcel_task_t * p);
#else
#  define ma_wait_task_inactive(p)	do { } while (0)
#endif
int marcel_idle_lwp(ma_lwp_t lwp);
int marcel_task_prio(marcel_task_t *p);
int marcel_task_curr(marcel_task_t *p);

#section marcel_inline
#depend "linux_thread_info.h[marcel_inline]"
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

#section functions
extern void marcel_wake_up_created_thread(marcel_task_t * tsk);
extern void ma_resched_task(marcel_task_t *p, int vp, ma_lwp_t lwp);

