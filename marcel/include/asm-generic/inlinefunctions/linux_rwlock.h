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


#ifndef __ASM_GENERIC_INLINEFUNCTIONS_LINUX_RWLOCK_H__
#define __ASM_GENERIC_INLINEFUNCTIONS_LINUX_RWLOCK_H__


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
#include "tbx_compiler.h"
#include "asm/linux_atomic.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
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

#ifdef MA__LWPS
static __tbx_inline__ int _ma_raw_write_trylock(ma_rwlock_t *rw)
{
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS,rw))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS,rw);
	return 0;
}
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_INLINEFUNCTIONS_LINUX_RWLOCK_H__ **/
