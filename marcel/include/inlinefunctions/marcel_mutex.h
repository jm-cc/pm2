/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 the PM2 team (see AUTHORS file)
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

/* Low level mutex lock implementation based on fastlock */


#ifndef __INLINEFUNCTIONS_MARCEL_MUTEX_H__
#define __INLINEFUNCTIONS_MARCEL_MUTEX_H__


#include "marcel_config.h"
#include "sys/marcel_fastlock.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


static __tbx_inline__ void __marcel_lock(struct _marcel_fastlock *lock, marcel_t self)
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(ma_atomic_add_negative(-1, &lock->__status))) { // try to take the lock
                ma_fastlock_acquire(lock);
                __marcel_lock_wait(lock, self, 0);
		ma_fastlock_release(lock);
        }

        MARCEL_LOG("getting lock %p in task %p\n", lock, self);
        MARCEL_LOG_OUT();
}

static __tbx_inline__ void __lpt_lock(struct _lpt_fastlock *lock, marcel_t self)
{
        if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
                __lpt_init_lock(lock);
        __marcel_lock(LPT_FASTLOCK_2_MA_FASTLOCK(lock), self);
}


static __tbx_inline__ void __marcel_unlock(struct _marcel_fastlock *lock)
{
	blockcell *first;
	marcel_t task;

        MARCEL_LOG_IN();

        if (ma_atomic_inc_return(&lock->__status) <= 0) {
		ma_fastlock_acquire(lock);

		while (1) {
			first = MA_MARCEL_FASTLOCK_WAIT_LIST(lock);
			if (tbx_likely(first)) {
				first->blocked = tbx_false;
				__marcel_unregister_lock_spinlocked(lock, first);
				task = first->task; /* first can be freed in __marcel_lock_*_wait: 
						       if the fastlock is release */
				ma_fastlock_release(lock);
				ma_wake_up_thread(task);
				break;
			} else {
				ma_fastlock_release(lock);
				ma_cpu_relax();
				ma_fastlock_acquire(lock);
			}
		}
        }

	MARCEL_LOG_OUT();
}

static __tbx_inline__ void __lpt_unlock(struct _lpt_fastlock *lock)
{
        if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
                __lpt_init_lock(lock);
        __marcel_unlock(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}


static __tbx_inline__ int __marcel_trylock(struct _marcel_fastlock *lock)
{
        MARCEL_LOG_IN();
        if (tbx_unlikely(1 == ma_atomic_xchg(1, 0, &lock->__status)))
                MARCEL_LOG_RETURN(1); // lock taken                                                                           
        MARCEL_LOG_RETURN(0);
}

static __tbx_inline__ int __lpt_trylock(struct _lpt_fastlock *lock)
{
        if (tbx_unlikely(NULL == LPT_FASTLOCK_2_MA_FASTLOCK(lock)))
                __lpt_init_lock(lock);
        return __marcel_trylock(LPT_FASTLOCK_2_MA_FASTLOCK(lock));
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /**  __INLINEFUNCTIONS_MARCEL_MUTEX_H__ **/
