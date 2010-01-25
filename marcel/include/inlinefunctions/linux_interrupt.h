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


#ifndef __INLINEFUNCTIONS_LINUX_INTERRUPT_H__
#define __INLINEFUNCTIONS_LINUX_INTERRUPT_H__


/* Tasklets --- multithreaded analogue of BHs.

   Main feature differing them of generic softirqs: tasklet
   is running only on one CPU simultaneously.

   Main feature differing them of BHs: different tasklets
   may be run simultaneously on different CPUs.

   Properties:
   * If tasklet_schedule() is called, then tasklet is guaranteed
     to be executed on some cpu at least once after this.
   * If the tasklet is already scheduled, but its excecution is still not
     started, it will be executed only once.
   * If this tasklet is already running on another CPU (or schedule is called
     from tasklet itself), it is rescheduled for later.
   * Tasklet is strictly serialized wrt itself, but not
     wrt another tasklets. If client needs some intertask synchronization,
     he makes it with spinlocks.
 */


#ifdef __MARCEL_KERNEL__
#include "asm/linux_bitops.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
#if defined(MA__LWPS) && defined (MARCEL_REMOTE_TASKLETS)
static __tbx_inline__ void ma_remote_tasklet_set_vpset(struct ma_tasklet_struct *t, marcel_vpset_t * vpset) {
       t->vp_set=*vpset;
}
#  define ma_remote_tasklet_init(lock)		ma_spin_lock_init(lock)
#  define ma_remote_tasklet_lock(lock)		ma_spin_lock(lock)
#  define ma_remote_tasklet_unlock(lock)	ma_spin_unlock(lock)
#else
#  define ma_remote_tasklet_set_vpset(t,vpset)	do { } while (0)
#  define ma_remote_tasklet_init(lock)		do { } while (0)
#  define ma_remote_tasklet_lock(lock)		do { } while (0)
#  define ma_remote_tasklet_unlock(lock)	do { } while (0)
#endif

#ifdef MA__LWPS
static __tbx_inline__ int ma_tasklet_trylock(struct ma_tasklet_struct *t) {
	return !ma_test_and_set_bit(MA_TASKLET_STATE_RUN, &(t)->state);
}

/* returns !=0 if somebody else added SCHED while we were running, in
 * which case it's up to us to call ma_tasklet_schedule() again */
/*************************************************************************************
 * static __tbx_inline__ int ma_tasklet_unlock(struct ma_tasklet_struct *t) {        *
 * 	unsigned long old_state;                                                     *
 * 	ma_smp_mb__before_clear_bit();                                               *
 * 	old_state = ma_xchg(&(t)->state, 0);                                         *
 * 	return old_state & (1<<MA_TASKLET_STATE_SCHED);                              *
 * }                                                                                 *
 * #else                                                                             *
 *************************************************************************************/
static __tbx_inline__ void ma_tasklet_unlock(struct ma_tasklet_struct *t) {
        ma_smp_mb__before_clear_bit();
        ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
}

static __tbx_inline__ void ma_tasklet_unlock_wait(struct ma_tasklet_struct *t) {
	while (ma_test_bit(MA_TASKLET_STATE_RUN, &(t)->state)) { ma_barrier(); }
}
#else
#  define ma_tasklet_trylock(t)			(1)
#  define ma_tasklet_unlock_wait(t)		do { } while (0)
#  define ma_tasklet_unlock(t)			((void)0)
#endif


#ifdef MARCEL_REMOTE_TASKLETS
static __tbx_inline__ void __ma_tasklet_remote_schedule(struct ma_tasklet_struct *t, unsigned vpnum) {
	struct marcel_topo_vpdata * const vp = ma_topo_vpdata_by_vpnum(vpnum);

	ma_remote_tasklet_lock(&vp->tasklet_lock);
	
	t->next = vp->tasklet_vec.list;
	vp->tasklet_vec.list = t;
	__ma_raise_softirq_vp(MA_TASKLET_SOFTIRQ, vpnum);
	
	if (t->preempt) {
		ma_lwp_t lwp = ma_get_lwp_by_vpnum(vpnum);
		//DISP("sending signal to vp %d", (int)vpnum);
		MA_LWP_RESCHED(lwp);
	}

	
        ma_remote_tasklet_unlock(&vp->tasklet_lock);
}

static __tbx_inline__ void ma_tasklet_schedule(struct ma_tasklet_struct *t) {
	struct marcel_topo_vpdata * const vp = ma_topo_vpdata_self();
	unsigned long old_state;
	ma_remote_tasklet_lock(&vp->tasklet_lock);
	old_state = ma_xchg(&t->state,(1<<MA_TASKLET_STATE_SCHED)|(1<<MA_TASKLET_STATE_RUN));
        switch (old_state) {
	case 0: {
		unsigned long target_vp_num;
		//DISP("tasklet schedule, t = %p", t);
                /* not running, not scheduled, schedule it here */
		if( marcel_vpset_isset(&(t->vp_set),marcel_current_vp())) {
			ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
			ma_remote_tasklet_unlock(&vp->tasklet_lock);
			__ma_tasklet_schedule(t);
			break;
		}

		/* vp_set doesn't fit. Let's take any vp that match */
		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
		ma_remote_tasklet_unlock(&vp->tasklet_lock);

		target_vp_num = marcel_vpset_first(&t->vp_set);
		MA_BUG_ON (!(target_vp_num+1));

		__ma_tasklet_remote_schedule(t, target_vp_num);
		break;
	}
	case 1<<MA_TASKLET_STATE_SCHED:
		/* not running anywhere, but already scheduled somewhere, so let it run there */
		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
		/* FALLTHROUGH */
	case (1<<MA_TASKLET_STATE_RUN):
		/* already running somewhere and wasn't scheduled yet, let there handle it */
	case (1<<MA_TASKLET_STATE_RUN)|(1<<MA_TASKLET_STATE_SCHED):
		/* already running somewhere and still schedule, let there handle it (again) */
		//DISP("tasklet already scheduled, t = %p", t);
		ma_remote_tasklet_unlock(&vp->tasklet_lock);
		break;
	}
}
#else

/*************************************************************************************************************
 * static __tbx_inline__ void ma_tasklet_schedule(struct ma_tasklet_struct *t) {                             *
 * 	unsigned long old_state;                                                                             *
 * 	old_state = ma_xchg(&t->state,(1<<MA_TASKLET_STATE_SCHED)|(1<<MA_TASKLET_STATE_RUN));                *
 * 	switch (old_state) {                                                                                 *
 * 	case 0:                                                                                              *
 * 		/\* not running, not scheduled, schedule it here *\/                                         *
 * 		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);                                             *
 * 		__ma_tasklet_schedule(t);                                                                    *
 * 		break;                                                                                       *
 * 	case 1<<MA_TASKLET_STATE_SCHED:                                                                      *
 * 		/\* not running anywhere, but already scheduled somewhere, so let it run there *\/           *
 * 		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);                                             *
 * 		/\* FALLTHROUGH *\/                                                                          *
 * 	case (1<<MA_TASKLET_STATE_RUN):                                                                      *
 * 		/\* already running somewhere and wasn't scheduled yet, let there handle it *\/              *
 * 	case (1<<MA_TASKLET_STATE_RUN)|(1<<MA_TASKLET_STATE_SCHED):                                          *
 * 		/\* already running somewhere and still schedule, let there handle it (again) *\/            *
 * 		break;                                                                                       *
 * 	}                                                                                                    *
 * }                                                                                                         *
 *************************************************************************************************************/
static __tbx_inline__ void ma_tasklet_schedule(struct ma_tasklet_struct *t) {
        if(!ma_test_and_set_bit(MA_TASKLET_STATE_SCHED, &t->state))
                __ma_tasklet_schedule(t);
}

#endif /* MARCEL_REMOTE_TASKLETS */

static __tbx_inline__ void ma_tasklet_hi_schedule(struct ma_tasklet_struct *t) {
	unsigned long old_state;
	old_state = ma_xchg(&t->state,(1<<MA_TASKLET_STATE_SCHED)|(1<<MA_TASKLET_STATE_RUN));
	switch (old_state) {
	case 0:
		/* not running, not scheduled, schedule it here */
		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
		__ma_tasklet_hi_schedule(t);
		break;
	case 1<<MA_TASKLET_STATE_SCHED:
		/* not running anywhere, but already scheduled somewhere, so let it run there */
		ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
		/* FALLTHROUGH */
	case (1<<MA_TASKLET_STATE_RUN):
		/* already running somewhere and wasn't scheduled yet, let there handle it */
	case (1<<MA_TASKLET_STATE_RUN)|(1<<MA_TASKLET_STATE_SCHED):
		/* already running somewhere and still schedule, let there handle it (again) */
		break;
	default:
		MA_BUG();
	}
}

static __tbx_inline__ void ma_tasklet_disable_nosync(struct ma_tasklet_struct *t) {
	ma_atomic_inc(&t->count);
	ma_smp_mb__after_atomic_inc();
}

static __tbx_inline__ void ma_tasklet_disable(struct ma_tasklet_struct *t) {
	ma_tasklet_disable_nosync(t);
	ma_tasklet_unlock_wait(t);
	ma_smp_mb();
}

static __tbx_inline__ void ma_tasklet_enable(struct ma_tasklet_struct *t) {
        ma_smp_mb__before_atomic_dec();
	ma_atomic_dec(&t->count);
}

static __tbx_inline__ void ma_tasklet_hi_enable(struct ma_tasklet_struct *t) {
	ma_smp_mb__before_atomic_dec();
	ma_atomic_dec(&t->count);
}

static __tbx_inline__ void __ma_raise_softirq_vp(unsigned int nr, unsigned vp) {
	MA_BUG_ON(nr >= MA_NR_SOFTIRQs);
	ma_set_bit(nr, &ma_softirq_pending_vp(vp));
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_LINUX_INTERRUPT_H__ **/
