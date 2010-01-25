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


#ifndef __ASM_I386_INLINEFUNCTIONS_LINUX_SPINLOCK_H__
#define __ASM_I386_INLINEFUNCTIONS_LINUX_SPINLOCK_H__


#include "asm/linux_spinlock.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
#ifdef MA__LWPS

/*
 * This works. Despite all the confusion.
 * (except on PPro SMP or if we are using OOSTORE)
 * (PPro errata 66, 92)
 */
 
#if !defined(CONFIG_X86_OOSTORE) && !defined(CONFIG_X86_PPRO_FENCE)

#define ma_spin_unlock_string \
	"mov"MA_SPINB" $1,%0" \
		:"=m" (lock->lock) : : "memory"


static __tbx_inline__ void _ma_raw_spin_unlock(ma_spinlock_t *lock)
{
	__asm__ __volatile__(
		ma_spin_unlock_string
	);
}

#else

#define ma_spin_unlock_string \
	"xchg"MA_SPINB" %"MA_SPINb"0, %1" \
		:"=q" (oldval), "=m" (lock->lock) \
		:"0" (oldval) : "memory"

static __tbx_inline__ void _ma_raw_spin_unlock(ma_spinlock_t *lock)
{
	MA_SPINT oldval = 1;
	__asm__ __volatile__(
		ma_spin_unlock_string
	);
}

#endif

static __tbx_inline__ int _ma_raw_spin_trylock(ma_spinlock_t *lock)
{
	MA_SPINT oldval;
	__asm__ __volatile__(
		"xchg"MA_SPINB" %"MA_SPINb"0,%1"
		:"=q" (oldval), "=m" (lock->lock)
		:"0" (0) : "memory");
	return oldval > 0;
}

static __tbx_inline__ void _ma_raw_spin_lock(ma_spinlock_t *lock)
{
	__asm__ __volatile__(
		ma_spin_lock_string
		:"=m" (lock->lock) : : "memory");
}

#endif /* MA__LWPS */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_LINUX_SPINLOCK_H__ **/
