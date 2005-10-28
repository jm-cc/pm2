
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
 * include/asm-i386/spinlock.h
 */
#depend "asm/linux_rwlock.h[]"
#include "tbx_compiler.h"

#ifdef MA__LWPS

#section types
/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

typedef struct {
	volatile unsigned int lock;
} ma_spinlock_t;

#section macros
#define MA_SPIN_LOCK_UNLOCKED { 1 }

#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)

#section marcel_macros
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#define ma_spin_is_locked(x)	(*(volatile signed char *)(&(x)->lock) <= 0)
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define ma_spin_lock_string \
	"\n1:\t" \
	"lock ; decb %0\n\t" \
	"js 2f\n" \
	MA_LOCK_SECTION_START("") \
	"2:\t" \
	"rep;nop\n\t" \
	"cmpb $0,%0\n\t" \
	"jle 2b\n\t" \
	"jmp 1b\n" \
	MA_LOCK_SECTION_END

#section marcel_inline
/*
 * This works. Despite all the confusion.
 * (except on PPro SMP or if we are using OOSTORE)
 * (PPro errata 66, 92)
 */
 
#if !defined(CONFIG_X86_OOSTORE) && !defined(CONFIG_X86_PPRO_FENCE)

#define ma_spin_unlock_string \
	"movb $1,%0" \
		:"=m" (lock->lock) : : "memory"


static __tbx_inline__ void _ma_raw_spin_unlock(ma_spinlock_t *lock)
{
	__asm__ __volatile__(
		ma_spin_unlock_string
	);
}

#else

#define ma_spin_unlock_string \
	"xchgb %b0, %1" \
		:"=q" (oldval), "=m" (lock->lock) \
		:"0" (oldval) : "memory"

static __tbx_inline__ void _ma_raw_spin_unlock(ma_spinlock_t *lock)
{
	char oldval = 1;
	__asm__ __volatile__(
		ma_spin_unlock_string
	);
}

#endif

static __tbx_inline__ int _ma_raw_spin_trylock(ma_spinlock_t *lock)
{
	char oldval;
	__asm__ __volatile__(
		"xchgb %b0,%1"
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

#section marcel_structures
#section marcel_functions
#section functions
#section common
#endif /* MA__LWPS */
