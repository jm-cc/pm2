
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
#depend "asm-generic/linux_atomic.h[]"
/*
 * Similar to:
 * include/asm-i386/rwlock.h
 *
 *	Helpers used by both rw spinlocks and rw semaphores.
 *
 *	Based in part on code from semaphore.h and
 *	spinlock.h Copyright 1996 Linus Torvalds.
 *
 *	Copyright 1999 Red Hat, Inc.
 *
 *	Written by Benjamin LaHaise.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#section marcel_macros
#define MA_RW_LOCK_BIAS		 0x01000000

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
typedef ma_atomic_t ma_rwlock_t;

#section marcel_macros
#define MA_RW_LOCK_UNLOCKED MA_ATOMIC_INIT(MA_RW_LOCK_BIAS);

#define ma_rwlock_init(x)	do { ma_atomic_set(x, MA_RW_LOCK_UNLOCKED); } while(0)

#define ma_rwlock_is_locked(x) (ma_atomic_read(x) != MA_RW_LOCK_BIAS)

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

extern void __ma_read_lock_failed(ma_rwlock_t *rw);
static inline void _ma_raw_read_lock(ma_rwlock_t *rw)
{
	if (ma_atomic_add_negative(-1,rw))
		__ma_read_lock_failed(rw);
}

extern void __ma_write_lock_failed(ma_rwlock_t *rw);
static inline void _ma_raw_write_lock(ma_rwlock_t *rw)
{
	if (!ma_atomic_sub_and_test(MA_RW_LOCK_BIAS,rw))
		__ma_write_lock_failed(rw);
}

#section marcel_macros
#define _ma_raw_read_unlock(rw) ma_atomic_inc(rw)
#define _ma_raw_write_unlock(rw) ma_atomic_add(MA_RW_LOCK_BIAIS,rw);

#section marcel_inline
static inline int _ma_raw_write_trylock(ma_rwlock_t *rw)
{
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS,rw))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS,rw);
	return 0;
}

