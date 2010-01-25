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


#ifndef __INLINEFUNCTIONS_LINUX_SPINLOCK_H__
#define __INLINEFUNCTIONS_LINUX_SPINLOCK_H__


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/*
 * Taking a spin lock usually also disable preemption (since else other
 * spinners would wait for our being rescheduled): ma_*_lock/unlock
 *
 * If a spin lock is to be taken from a bottom half or softirq as well, these
 * have to be disabled as well: ma_*_lock/unlock_softirq/bh
 *
 * When releasing a spin lock, we need to check whether we should have been
 * preempted in the meanwhile, and thus the unlock functions may call
 * schedule(). If we are to call ma_schedule() shortly (e.g. in a
 * INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING loop), then we can use the
 * *_no_resched version to avoid calling ma_schedule() twice.
 */


#include "linux_spinlock.h"
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#include "asm/linux_spinlock.h"
#endif

/*
 *  bit-based spin_lock()
 *
 * Don't use this unless you really need to: spin_lock() and spin_unlock()
 * are significantly faster.
 */
static __tbx_inline__ void ma_bit_spin_lock(int bitnum, unsigned long *addr)
{
	/*
	 * Assuming the lock is uncontended, this never enters
	 * the body of the outer loop. If it is contended, then
	 * within the inner loop a non-atomic test is used to
	 * busywait with less bus contention for a good time to
	 * attempt to acquire the lock bit.
	 */
	ma_preempt_disable();
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	while (ma_test_and_set_bit(bitnum, addr)) {
		while (ma_test_bit(bitnum, addr))
			ma_cpu_relax();
	}
#endif
}

/*
 * Return true if it was acquired
 */
static __tbx_inline__ int ma_bit_spin_trylock(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	int ret;

	ma_preempt_disable();
	ret = !ma_test_and_set_bit(bitnum, addr);
	if (!ret)
		ma_preempt_enable();
	return ret;
#else
	ma_preempt_disable();
	return 1;
#endif
}

/*
 *  bit-based spin_unlock()
 */
static __tbx_inline__ void ma_bit_spin_unlock(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	MA_BUG_ON(!ma_test_bit(bitnum, addr));
	ma_smp_mb__before_clear_bit();
	ma_clear_bit(bitnum, addr);
#endif
	ma_preempt_enable();
}

/*
 * Return true if the lock is held.
 */
static __tbx_inline__ int ma_bit_spin_is_locked(int bitnum, unsigned long *addr)
{
#if defined(MA__LWPS) || defined(MA_DEBUG_SPINLOCK)
	return ma_test_bit(bitnum, addr);
#else
	return ma_preempt_count();
#endif
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_LINUX_SPINLOCK_H__ **/
