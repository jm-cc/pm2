
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
/*
 * Similar to:
 * include/linux/preempt.h - macros for accessing and manipulating
 * preempt_count (used for kernel preemption, interrupt count, etc.)
 */

#section marcel_macros

#define ma_preempt_count() (MARCEL_SELF->preempt_count)

#define ma_preempt_count_inc() \
do { \
        ma_preempt_count()++; \
} while (0)

#define ma_preempt_count_dec() \
do { \
        ma_preempt_count()--; \
} while (0)

#section marcel_functions
asmlinkage void ma_preempt_schedule(void);

#section marcel_macros
#define ma_preempt_disable() \
do { \
        ma_preempt_count_inc(); \
        ma_barrier(); \
} while (0)

#ifndef MA__WORK
#define check_work()
#else
#define check_work() \
do { \
	if (MARCEL_SELF->sched.state & (MA_TASK_RUNNING|MA_TASK_INTERRUPTIBLE) \
			&& HAS_DEVIATE_WORK(MARCEL_SELF)) \
		do_work(MARCEL_SELF); \
} while (0)
#endif

#define ma_preempt_enable_no_resched() \
do { \
        ma_barrier(); \
        ma_preempt_count_dec(); \
        check_work(); \
} while (0)

#define ma_preempt_check_resched() \
do { \
        if (tbx_unlikely(ma_test_thread_flag(TIF_NEED_RESCHED))) \
                ma_preempt_schedule(); \
} while (0)

#define ma_preempt_enable() \
do { \
        ma_preempt_enable_no_resched(); \
        ma_preempt_check_resched(); \
} while (0)

#section marcel_macros

#define MA_PREEMPT_ACTIVE  (1UL << 25)

/* Protection pour la réentrance du code de marcel avec les
 * bibliothèques extérieures (glibc par exemple)
 */
#define	ma_enter_lib() ma_preempt_disable()
#define	ma_exit_lib()  ma_preempt_enable()
