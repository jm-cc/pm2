
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

#define __ma_local_bh_enable() \
		do { ma_barrier(); ma_preempt_count() -= MA_SOFTIRQ_OFFSET; } while (0)

#section marcel_functions
extern void MARCEL_PROTECTED ma_local_bh_enable(void);

/* PLEASE, avoid to allocate new softirqs, if you need not _really_ high
   frequency threaded job scheduling. For almost all the purposes
   tasklets are more than enough. F.e. all serial device BHs et
   al. should be converted to tasklets, not to softirqs.
 */

#section marcel_types
enum
{
	MA_TIMER_HARDIRQ=0,
	MA_HI_SOFTIRQ,
	MA_TIMER_SOFTIRQ,
	MA_TASKLET_SOFTIRQ,
	MA_SIGNAL_SOFTIRQ
};

#section marcel_structures
/* softirq mask and active fields moved to irq_cpustat_t in
 * asm/hardirq.h to get better cache usage.  KAO
 */

struct ma_softirq_action
{
	void	(*action)(struct ma_softirq_action *);
	void	*data;
};

#section marcel_functions
asmlinkage MARCEL_PROTECTED void ma_do_softirq(void);
extern MARCEL_PROTECTED void ma_open_softirq(int nr, void (*action)(struct ma_softirq_action*), void *data);
extern void ma_softirq_init(void);
/* #define __ma_raise_softirq_irqoff(nr) do { ma_local_softirq_pending() |= 1UL << (nr); } while (0) */
extern MARCEL_PROTECTED void FASTCALL(ma_raise_softirq_from_hardirq(unsigned int nr));
extern MARCEL_PROTECTED void FASTCALL(ma_raise_softirq_bhoff(unsigned int nr));
extern MARCEL_PROTECTED void FASTCALL(ma_raise_softirq(unsigned int nr));

#ifndef ma_invoke_softirq
#define ma_invoke_softirq() ma_do_softirq()
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
struct ma_tasklet_struct
{
	struct ma_tasklet_struct *next;
	unsigned long state;
	ma_atomic_t count;
	void (*func)(unsigned long);
	unsigned long data;
};

#section marcel_macros
#define MA_TASKLET_INIT(name, _func, _data) \
  { .next=NULL, .state=0, .count=MA_ATOMIC_INIT(0), .func=_func, .data=_data }
#define MA_DECLARE_TASKLET(name, _func, _data) \
struct ma_tasklet_struct name = MA_TASKLET_INIT(name, _func, _data)

#define MA_TASKLET_INIT_DISABLED(name, _func, _data) \
  { .next=NULL, .state=0, .count=MA_ATOMIC_INIT(1), .func=_func, .data=_data }
#define MA_DECLARE_TASKLET_DISABLED(name, _func, _data) \
struct ma_tasklet_struct name = MA_TASKLET_INIT_DISABLED(name, _func, _data)

#section marcel_types
enum
{
	MA_TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	MA_TASKLET_STATE_RUN	/* Tasklet is running (SMP only) */
};

#section marcel_inline
#depend "asm/linux_bitops.h[]"
#depend "asm/linux_bitops.h[marcel_macros]"
#ifdef MA__LWPS
static __tbx_inline__ int ma_tasklet_trylock(struct ma_tasklet_struct *t)
{
	return !ma_test_and_set_bit(MA_TASKLET_STATE_RUN, &(t)->state);
}

static __tbx_inline__ void ma_tasklet_unlock(struct ma_tasklet_struct *t)
{
	ma_smp_mb__before_clear_bit(); 
	ma_clear_bit(MA_TASKLET_STATE_RUN, &(t)->state);
}

static __tbx_inline__ void ma_tasklet_unlock_wait(struct ma_tasklet_struct *t)
{
	while (ma_test_bit(MA_TASKLET_STATE_RUN, &(t)->state)) { ma_barrier(); }
}
#else
#define ma_tasklet_trylock(t) 1
#define ma_tasklet_unlock_wait(t) do { } while (0)
#define ma_tasklet_unlock(t) do { } while (0)
#endif

#section marcel_functions
extern MARCEL_PROTECTED void FASTCALL(__ma_tasklet_schedule(struct ma_tasklet_struct *t));

#section marcel_inline
static __tbx_inline__ void ma_tasklet_schedule(struct ma_tasklet_struct *t)
{
	if (!ma_test_and_set_bit(MA_TASKLET_STATE_SCHED, &t->state))
		__ma_tasklet_schedule(t);
}

#section marcel_functions
extern MARCEL_PROTECTED void FASTCALL(__ma_tasklet_hi_schedule(struct ma_tasklet_struct *t));

#section marcel_inline
static __tbx_inline__ void ma_tasklet_hi_schedule(struct ma_tasklet_struct *t)
{
	if (!ma_test_and_set_bit(MA_TASKLET_STATE_SCHED, &t->state))
		__ma_tasklet_hi_schedule(t);
}


#section marcel_inline
static __tbx_inline__ void ma_tasklet_disable_nosync(struct ma_tasklet_struct *t)
{
	ma_atomic_inc(&t->count);
	ma_smp_mb__after_atomic_inc();
}

static __tbx_inline__ void ma_tasklet_disable(struct ma_tasklet_struct *t)
{
	ma_tasklet_disable_nosync(t);
	ma_tasklet_unlock_wait(t);
	ma_smp_mb();
}

static __tbx_inline__ void ma_tasklet_enable(struct ma_tasklet_struct *t)
{
	ma_smp_mb__before_atomic_dec();
	ma_atomic_dec(&t->count);
}

static __tbx_inline__ void ma_tasklet_hi_enable(struct ma_tasklet_struct *t)
{
	ma_smp_mb__before_atomic_dec();
	ma_atomic_dec(&t->count);
}

#section marcel_functions
extern MARCEL_PROTECTED void ma_tasklet_kill(struct ma_tasklet_struct *t);
extern void tasklet_kill_immediate(struct ma_tasklet_struct *t, ma_lwp_t lwp);
extern MARCEL_PROTECTED void ma_tasklet_init(struct ma_tasklet_struct *t,
			 void (*func)(unsigned long), unsigned long data);


