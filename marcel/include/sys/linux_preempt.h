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


#ifndef __SYS_LINUX_PREEMPT_H__
#define __SYS_LINUX_PREEMPT_H__


#include "tbx_compiler.h"
#include "marcel_config.h"
#include "asm/linux_hardirq.h"
#include "marcel_deviate.h"
#include "sys/linux_thread_info.h"
#include "sys/marcel_compiler.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#ifdef MARCEL_DEBUG_SPINLOCK
#define ma_record_preempt_backtrace() do { \
	if (ma_init_done[MA_INIT_SCHEDULER]) \
		SELF_GETMEM(preempt_backtrace_size) = __TBX_RECORD_SOME_TRACE(SELF_GETMEM(preempt_backtrace), TBX_BACKTRACE_DEPTH); \
} while(0)

#define ma_show_preempt_backtrace() do { \
	if (SELF_GETMEM(preempt_backtrace_size)) \
		__TBX_PRINT_SOME_TRACE(SELF_GETMEM(preempt_backtrace), SELF_GETMEM(preempt_backtrace_size)); \
} while(0)
#else
#define ma_record_preempt_backtrace() (void)0
#define ma_show_preempt_backtrace() (void)0
#endif

#define ma_preempt_count() (SELF_GETMEM(preempt_count))

#define ma_preempt_count_inc() \
do { \
	if (++ma_preempt_count() == 1) \
		ma_record_preempt_backtrace(); \
} while (0)

#define ma_preempt_count_dec() \
do { \
        ma_preempt_count()--; \
	if (ma_preempt_count() & MA_PREEMPT_BUGMASK) { \
		ma_show_preempt_backtrace(); \
		MA_BUG_ON(1); \
	} \
} while (0)

#define ma_preempt_disable() \
do { \
        ma_preempt_count_inc(); \
        ma_barrier(); \
} while (0)

#define ma_check_work() \
do { \
	if (!(SELF_GETMEM(state) & ~MA_TASK_INTERRUPTIBLE) \
			&& HAS_DEVIATE_WORK(MARCEL_SELF)) \
		ma_do_work(MARCEL_SELF); \
} while (0)

#define ma_preempt_enable_no_resched() \
do { \
        ma_barrier(); \
	if (ma_last_preempt()) \
        	ma_check_work(); \
        ma_preempt_count_dec(); \
} while (0)

#define ma_preempt_check_resched(irq) \
do { \
        if (ma_preempt_count()==irq && ma_thread_preemptible() && tbx_unlikely(ma_get_need_resched())) \
                ma_preempt_schedule(irq); \
} while (0)

#define ma_preempt_enable() \
do { \
        ma_preempt_enable_no_resched(); \
        ma_preempt_check_resched(0); \
} while (0)

#define ma_preempt_enable_in_deviation() \
do { \
        ma_barrier(); \
        ma_preempt_count_dec(); \
        ma_preempt_check_resched(0); \
} while (0)


#define MA_PREEMPT_ACTIVE  (1UL << 25)


/** Internal functions **/
void ma_preempt_schedule(int irq);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_PREEMPT_H__ **/
