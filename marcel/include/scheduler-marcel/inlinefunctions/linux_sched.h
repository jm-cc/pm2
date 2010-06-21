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


#ifndef __LINUX_SCHED_H__
#define __LINUX_SCHED_H__


#include "scheduler-marcel/linux_sched.h"
#include "marcel_sched_generic.h"


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


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_SCHED_H__ **/
