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


#ifndef __INLINE_FUNCTIONS_LINUX_SCHED_H__
#define __INLINE_FUNCTIONS_LINUX_SCHED_H__


#include "scheduler/linux_sched.h"
#include "sys/marcel_sched_generic.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
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

/* tries to find a LWP where the task p can be scheduled */
static __tbx_inline__ ma_lwp_t ma_find_lwp_for_task(marcel_task_t *p, ma_holder_t *h)
{
	marcel_lwp_t *chosen;
        ma_runqueue_t *rq;

	chosen = NULL;

        /* try_to_resched may be called while ma_list_lwp has not been added any item yet */
        if (tbx_unlikely(! ma_any_lwp()))
                return chosen;

	rq = ma_to_rq_holder(h);
	if (tbx_unlikely(! rq))
                return chosen;

#ifdef MA__LWPS
        if (rq->lwp) {
		/* the rq is an LWP rq, we dont have any other choice */
                if (TASK_CURR_PREEMPT(p, rq->lwp) > 0)
			chosen = rq->lwp;
        } else {
                int vp;
		int max_preempt, preempt;
                marcel_lwp_t *lwp;
		
		max_preempt = 0;
		marcel_vpset_foreach_begin(vp, &rq->vpset) {
			/** lwp could be NULL if the previous lwp which ran on this vp
			 *  is a blocked lwp:
			 *  - call ma_lwp_block()
			 *  - call ma_clr_lwp_vpnum()
			 *
			 * Signal was risen but a spare does not take the VP yet
			 * (ma_lwp_wait_active())                                      */
                        lwp = ma_get_lwp_by_vpnum(vp);
			if (tbx_likely(lwp)) {
				preempt = TASK_CURR_PREEMPT(p, lwp);
				if (preempt > max_preempt) {
					max_preempt = preempt;
					chosen = lwp;
				}
			}
		} marcel_vpset_foreach_end(vp, &rq->vpset);
	}
#else
        if (TASK_CURR_PREEMPT(p, MA_LWP_SELF) > 0)
                chosen = MA_LWP_SELF;
#endif

	return chosen;
}

/***
 * ma_awake_task - wake up a thread
 * @p: the to-be-woken-up thread
 * @state: the mask of task states that can be woken
 *
 * Put it on the run-queue if it's not already there. The "current"
 * thread is always on the run-queue (except when the actual
 * re-schedule is in progress), and as such you're allowed to do
 * the simpler "current->state = TASK_RUNNING" to mark yourself
 * runnable without the overhead of this.
 *
 * returns failure only if the task is already active.
 */
static __tbx_inline__ int ma_awake_task(marcel_task_t * p, unsigned int state, ma_holder_t **h)
{
	if (p->state & state) {
		/* on s'occupe de la réveiller */
		PROF_EVENT2(sched_thread_wake, p, p->state);
#ifdef MARCEL_STATS_ENABLED
		ma_task_stats_set(long, p, ma_stats_nbready_offset, 1);
#endif
		p->state = MA_TASK_RUNNING;
		if (MA_TASK_IS_BLOCKED(p)) { /* not running or runnable */
			PROF_EVENT(sched_thread_wake_unblock);
			/*
			 * Fast-migrate the task if it's not running or runnable
			 * currently. Do not violate hard affinity.
			 */
#ifdef PM2_DEV
#warning TODO
#endif
			/* Attention ici: h peut être une bulle, auquel cas
			 * account_ready_or_running_entity peut lâcher la bulle pour verrouiller
			 * une runqueue */
			ma_set_ready_holder(&p->as_entity, *h);
			ma_enqueue_entity(&p->as_entity, *h);
			/*
			 * Sync wakeups (i.e. those types of wakeups where the waker
			 * has indicated that it will leave the CPU in short order)
			 * don't trigger a preemption, if the woken up task will run on
			 * this cpu. (in this case the 'I will reschedule' promise of
			 * the waker guarantees that the freshly woken up task is going
			 * to be considered on this CPU.)
			 */
		}

		return 1;
	}

	return 0;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_SCHED_H__ **/
