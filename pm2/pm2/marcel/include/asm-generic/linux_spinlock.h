
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

#error "to write..."

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


static inline void _ma_raw_spin_unlock(ma_spinlock_t *lock)
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

static inline void _ma_raw_spin_unlock(ma_spinlock_t *lock)
{
	char oldval = 1;
	__asm__ __volatile__(
		ma_spin_unlock_string
	);
}

#endif

static inline int _ma_raw_spin_trylock(ma_spinlock_t *lock)
{
	char oldval;
	__asm__ __volatile__(
		"xchgb %b0,%1"
		:"=q" (oldval), "=m" (lock->lock)
		:"0" (0) : "memory");
	return oldval > 0;
}

static inline void _ma_raw_spin_lock(ma_spinlock_t *lock)
{
	__asm__ __volatile__(
		ma_spin_lock_string
		:"=m" (lock->lock) : : "memory");
}

#section marcel_types
/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 */
typedef struct {
	volatile unsigned int lock;
} ma_rwlock_t;

#section marcel_macros
#define MA_RW_LOCK_UNLOCKED (ma_rwlock_t) { MA_RW_LOCK_BIAS }

#define ma_rwlock_init(x)	do { *(x) = MA_RW_LOCK_UNLOCKED; } while(0)

#define ma_rwlock_is_locked(x) ((x)->lock != MA_RW_LOCK_BIAS)

#section marcel_inline
/*
 * On x86, we implement read-write locks as a 32-bit counter
 * with the high bit (sign) being the "contended" bit.
 *
 * The inline assembly is non-obvious. Think about it.
 *
 * Changed to use the same technique as rw semaphores.  See
 * semaphore.h for details.  -ben
 */
/* the spinlock helpers are in arch/i386/kernel/semaphore.c */

static inline void _ma_raw_read_lock(ma_rwlock_t *rw)
{
	__ma_build_read_lock(rw, "__read_lock_failed");
}

static inline void _ma_raw_write_lock(ma_rwlock_t *rw)
{
	__ma_build_write_lock(rw, "__write_lock_failed");
}

#section marcel_macros
#define _ma_raw_read_unlock(rw)		asm volatile("lock ; incl %0" :"=m" ((rw)->lock) : : "memory")
#define _ma_raw_write_unlock(rw)	asm volatile("lock ; addl $" MA_RW_LOCK_BIAS_STR ",%0":"=m" ((rw)->lock) : : "memory")

#section marcel_inline
static inline int _ma_raw_write_trylock(ma_rwlock_t *lock)
{
	ma_atomic_t *count = (ma_atomic_t *)lock;
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS, count))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS, count);
	return 0;
}

#section common
#endif /* MA__LWPS */
