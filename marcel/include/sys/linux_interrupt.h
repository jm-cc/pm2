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


#ifndef __SYS_LINUX_INTERRUPT_H__
#define __SYS_LINUX_INTERRUPT_H__


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


#include "tbx_fast_list.h"
#include "tbx_compiler.h"
#include "marcel_config.h"
#include "sys/marcel_types.h"
#include "asm/linux_atomic.h"
#include "sys/marcel_lwp.h"
#include "sys/linux_bitops.h"
#include "sys/linux_linkage.h"


/** Public data structures **/
struct marcel_tasklet_struct {
	struct marcel_tasklet_struct *next;
	unsigned long state;
	ma_atomic_t count;
	void (*func) (unsigned long);
	unsigned long data;
#ifdef MARCEL_REMOTE_TASKLETS
	marcel_vpset_t vp_set;
	struct tbx_fast_list_head associated_bubble;
	unsigned priority;
#endif
	int preempt;		/* do send a signal to force quick tasklet scheduling */
};


/** Public macros **/
#ifdef MARCEL_REMOTE_TASKLETS
#  define MARCEL_REMOTE_TASKLET_INIT		.vp_set=MARCEL_VPSET_FULL,
#else				/* MARCEL_REMOTE_TASKLETS */
#  define MARCEL_REMOTE_TASKLET_INIT
#endif				/* MARCEL_REMOTE_TASKLETS */
#define MARCEL_TASKLET_INIT_COUNT(name, _func, _data, _count, _preempt) { \
		.next=NULL, \
		.state=0, \
		.count=MA_ATOMIC_INIT(_count), \
                .preempt=_preempt, \
		.func=_func, \
		.data=_data, \
		MARCEL_REMOTE_TASKLET_INIT \
}
#define MARCEL_TASKLET_INIT(name, _func, _data, _preempt) \
	MARCEL_TASKLET_INIT_COUNT(name, _func, _data, 0, _preempt)
#define MARCEL_DECLARE_TASKLET(name, _func, _data, _preempt) \
	struct marcel_tasklet_struct name = MARCEL_TASKLET_INIT(name, _func, _data, _preempt)
#define MARCEL_TASKLET_INIT_DISABLED(name, _func, _data, _preempt) \
	MARCEL_TASKLET_INIT_COUNT(name, _func, _data, 1, _preempt)
#define MARCEL_DECLARE_TASKLET_DISABLED(name, _func, _data, _preempt) \
	struct marcel_tasklet_struct name = MARCEL_TASKLET_INIT_DISABLED(name, _func, _data, _preempt)


/** Public functions **/
void marcel_tasklet_init(struct marcel_tasklet_struct *t, void (*func) (unsigned long), unsigned long data, int preempt);
void marcel_tasklet_set_vpset(struct marcel_tasklet_struct *t, marcel_vpset_t vp_set);

void marcel_tasklet_disable(void);
void marcel_spin_lock_tasklet_disable(marcel_spinlock_t * lock);
int marcel_spin_trylock_tasklet_disable(marcel_spinlock_t * lock);
void marcel_tasklet_enable(void);
void marcel_spin_unlock_tasklet_enable(marcel_spinlock_t * lock);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
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


/** Internal data types **/
enum {
	MA_TIMER_HARDIRQ = 0,
	MA_HI_SOFTIRQ,
	MA_TIMER_SOFTIRQ,
	MA_TASKLET_SOFTIRQ,
#ifdef MARCEL_SIGNALS_ENABLED
	MA_SIGNAL_SOFTIRQ,
	MA_SIGMASK_SOFTIRQ,
#endif
	MA_NR_SOFTIRQs
};

enum {
	MA_TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	MA_TASKLET_STATE_RUN	/* Tasklet is running (SMP only) */
};


/** Internal data structures **/
struct ma_softirq_action {
	void (*action) (struct ma_softirq_action *);
	void *data;
};


/** Internal inline functions **/
/* Enable bottom-halves, check for pending bottom halves, and possibly
 * reschedule */
static __tbx_inline__ void ma_local_bh_enable(void);
/* Enable bottom-halves, check for pending bottom halves, but do not try to
 * reschedule, i.e. the caller promises that he will call schedule shortly. */
static __tbx_inline__ void ma_local_bh_enable_no_resched(void);


/** Internal functions **/
asmlinkage void ma_do_softirq(void);
extern void ma_open_softirq(int nr, void (*action) (struct ma_softirq_action *),
			    void *data);
extern void FASTCALL(ma_raise_softirq_from_hardirq(unsigned int nr));
extern void FASTCALL(ma_raise_softirq_bhoff(unsigned int nr));
extern void FASTCALL(ma_raise_softirq(unsigned int nr));
extern void FASTCALL(ma_raise_softirq_lwp(ma_lwp_t lwp, unsigned int nr));
extern void FASTCALL(__ma_tasklet_schedule(struct marcel_tasklet_struct *t));
extern void FASTCALL(__ma_tasklet_hi_schedule(struct marcel_tasklet_struct *t));
extern void ma_tasklet_kill(struct marcel_tasklet_struct *t);
extern void tasklet_kill_immediate(struct marcel_tasklet_struct *t, ma_lwp_t lwp);
extern void ma_tasklet_init(struct marcel_tasklet_struct *t,
			    void (*func) (unsigned long), unsigned long data,
			    int preempt);

#ifndef ma_invoke_softirq
#  define ma_invoke_softirq() ma_do_softirq()
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_INTERRUPT_H__ **/
