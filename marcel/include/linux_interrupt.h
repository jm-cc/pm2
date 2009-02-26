
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

#section common
#include "tbx_compiler.h"
/*
 * Similar to:
 * include/linux/interrupt.h
 */

#section marcel_macros
/* SoftIRQ primitives.  */
#define ma_local_bh_disable() \
	do { \
		if ((ma_preempt_count() += MA_SOFTIRQ_OFFSET) \
				== MA_SOFTIRQ_OFFSET) \
			ma_record_preempt_backtrace(); \
		ma_barrier();  \
	} while (0)

/* Actual lowlevel bottom-halves enabling. Doesn't check for pending bottom halves */
#define __ma_local_bh_enable() \
	do { ma_barrier(); ma_preempt_count() -= MA_SOFTIRQ_OFFSET; } while (0)

#section marcel_functions
/* Enable bottom-halves, check for pending bottom halves, but do not try to
 * reschedule, i.e. the caller promises that he will call schedule shortly. */
extern void TBX_EXTERN ma_local_bh_enable_no_resched(void);
/* Enable bottom-halves, check for pending bottom halves, and possibly
 * reschedule */
extern void TBX_EXTERN ma_local_bh_enable(void);

#section marcel_types
enum {
	MA_TIMER_HARDIRQ=0,
	MA_HI_SOFTIRQ,
	MA_TIMER_SOFTIRQ,
	MA_TASKLET_SOFTIRQ,
	MA_SIGNAL_SOFTIRQ
};

#section marcel_structures
struct ma_softirq_action {
	void (*action)(struct ma_softirq_action *);
	void *data;
};

#section marcel_functions
asmlinkage TBX_EXTERN void ma_do_softirq(void);
extern TBX_EXTERN void ma_open_softirq(int nr, void (*action)(struct ma_softirq_action*), void *data);
extern TBX_EXTERN void FASTCALL(ma_raise_softirq_from_hardirq(unsigned int nr));
extern TBX_EXTERN void FASTCALL(ma_raise_softirq_bhoff(unsigned int nr));
extern TBX_EXTERN void FASTCALL(ma_raise_softirq(unsigned int nr));

#depend "sys/marcel_lwp.h[marcel_structures]"
#depend "marcel_topology.h[marcel_variables]"
#depend "asm/linux_bitops.h[marcel_inline]"
static __tbx_inline__ void __ma_raise_softirq_vp(unsigned int nr, unsigned vp) {
	ma_set_bit(nr, &ma_softirq_pending_vp(vp));
}

#ifndef ma_invoke_softirq
#  define ma_invoke_softirq() ma_do_softirq()
#endif

#section common
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

#section marcel_structures
#depend "asm/linux_atomic.h[marcel_types]"
#include "marcel_topology.h"
struct ma_tasklet_struct {
	struct ma_tasklet_struct *next;
	unsigned long state;
	ma_atomic_t count;
	void (*func)(unsigned long);
	unsigned long data;
#ifdef MARCEL_REMOTE_TASKLETS
	marcel_vpset_t vp_set;
	struct list_head associated_bubble;
	unsigned priority;
#endif
	int preempt;	/* do send a signal to force quick tasklet scheduling */
};

#section marcel_macros
#ifdef MARCEL_REMOTE_TASKLETS
#  define MA_REMOTE_TASKLET_INIT		.vp_set=MARCEL_VPSET_FULL,
#else /* MARCEL_REMOTE_TASKLETS */
#  define MA_REMOTE_TASKLET_INIT
#endif /* MARCEL_REMOTE_TASKLETS */

#define MA_TASKLET_INIT_COUNT(name, _func, _data, _count, _preempt) { \
		.next=NULL, \
		.state=0, \
		.count=MA_ATOMIC_INIT(_count), \
                .preempt=_preempt, \
		.func=_func, \
		.data=_data, MA_REMOTE_TASKLET_INIT \
	}

#define MA_TASKLET_INIT(name, _func, _data, _preempt) \
	MA_TASKLET_INIT_COUNT(name, _func, _data, 0, _preempt)
#define MA_DECLARE_TASKLET(name, _func, _data, _preempt) \
	struct ma_tasklet_struct name = MA_TASKLET_INIT(name, _func, _data, _preempt)

#define MA_TASKLET_INIT_DISABLED(name, _func, _data, _preempt) \
	MA_TASKLET_INIT_COUNT(name, _func, _data, 1, _preempt)
#define MA_DECLARE_TASKLET_DISABLED(name, _func, _data, _preempt) \
	struct ma_tasklet_struct name = MA_TASKLET_INIT_DISABLED(name, _func, _data, _preempt)

#section marcel_types
enum {
	MA_TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	MA_TASKLET_STATE_RUN	/* Tasklet is running (SMP only) */
};

#section marcel_inline
#depend "asm/linux_bitops.h[]"
#depend "asm/linux_bitops.h[marcel_macros]"
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

#section marcel_functions
extern TBX_EXTERN void FASTCALL(__ma_tasklet_schedule(struct ma_tasklet_struct *t));

#section marcel_inline

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
		marcel_kthread_kill(lwp->pid, MARCEL_TIMER_USERSIGNAL);
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

#section marcel_functions
extern TBX_EXTERN void FASTCALL(__ma_tasklet_hi_schedule(struct ma_tasklet_struct *t));

#section marcel_inline
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

#section marcel_inline
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

#section marcel_functions
extern TBX_EXTERN void ma_tasklet_kill(struct ma_tasklet_struct *t);
extern void tasklet_kill_immediate(struct ma_tasklet_struct *t, ma_lwp_t lwp);
extern TBX_EXTERN void ma_tasklet_init(struct ma_tasklet_struct *t,
			void (*func)(unsigned long), unsigned long data, int preempt);


