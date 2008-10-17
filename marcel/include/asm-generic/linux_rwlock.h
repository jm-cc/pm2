
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

#section macros
#section marcel_macros
#ifdef MA__LWPS
#define MA_RW_LOCK_BIAS		 0x01000000
#endif

#section marcel_types
#depend "asm/linux_atomic.h[]"
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
#ifdef MA__LWPS
typedef ma_atomic_t ma_rwlock_t;
#endif

#section marcel_macros
#depend "asm/linux_atomic.h[]"
#ifdef MA__LWPS
#define MA_RW_LOCK_UNLOCKED MA_ATOMIC_INIT(MA_RW_LOCK_BIAS);

#define ma_rwlock_init(x)	do { ma_atomic_set(x, MA_RW_LOCK_BIAS); } while(0)

#define ma_rwlock_is_locked(x) (ma_atomic_read(x) != MA_RW_LOCK_BIAS)
#endif

#section marcel_inline
#depend "asm/linux_atomic.h[]"
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

#ifdef MA__LWPS
extern TBX_EXTERN void __ma_read_lock_failed(ma_rwlock_t *rw);
static __tbx_inline__ void _ma_raw_read_lock(ma_rwlock_t *rw)
{
	if (ma_atomic_add_negative(-1,rw))
		__ma_read_lock_failed(rw);
}

extern TBX_EXTERN void __ma_write_lock_failed(ma_rwlock_t *rw);
static __tbx_inline__ void _ma_raw_write_lock(ma_rwlock_t *rw)
{
	if (!ma_atomic_sub_and_test(MA_RW_LOCK_BIAS,rw))
		__ma_write_lock_failed(rw);
}
#endif

#section marcel_macros
#depend "asm/linux_atomic.h[]"
#ifdef MA__LWPS
#define _ma_raw_read_unlock(rw) ma_atomic_inc(rw)
#define _ma_raw_write_unlock(rw) ma_atomic_add(MA_RW_LOCK_BIAS,rw);
#endif

#section marcel_inline
#depend "asm/linux_atomic.h[]"
#ifdef MA__LWPS
static __tbx_inline__ int _ma_raw_write_trylock(ma_rwlock_t *rw)
{
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS,rw))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS,rw);
	return 0;
}
#endif
