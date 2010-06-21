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


#ifndef __ASM_I386_INLINEFUNCTIONS_LINUX_RWLOCK_H__
#define __ASM_I386_INLINEFUNCTIONS_LINUX_RWLOCK_H__


/*
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


#include "asm/linux_rwlock.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
/*
 * On x86, we implement read-write locks as a 32-bit counter
 * with the high bit (sign) being the "contended" bit.
 *
 * The __tbx_inline__ assembly is non-obvious. Think about it.
 *
 * Changed to use the same technique as rw semaphores.  See
 * semaphore.h for details.  -ben
 */
/* the spinlock helpers are in arch/i386/kernel/semaphore.c */
#ifdef MA__LWPS
static __tbx_inline__ void _ma_raw_read_lock(ma_rwlock_t * rw)
{
	__ma_build_read_lock(rw, "__ma_read_lock_failed");
}

static __tbx_inline__ void _ma_raw_write_lock(ma_rwlock_t * rw)
{
	__ma_build_write_lock(rw, "__ma_write_lock_failed");
}

static __tbx_inline__ int _ma_raw_write_trylock(ma_rwlock_t * lock)
{
	ma_atomic_t *count = (ma_atomic_t *) lock;
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS, count))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS, count);
	return 0;
}
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_LINUX_RWLOCK_H__ **/
