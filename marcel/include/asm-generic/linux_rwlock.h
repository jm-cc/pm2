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


#ifndef __ASM_GENERIC_LINUX_RWLOCK_H__
#define __ASM_GENERIC_LINUX_RWLOCK_H__


/** Public macros **/
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


#ifdef __MARCEL_KERNEL__
#include "asm-generic/linux_rwlock.h"
#include "asm/linux_atomic.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MA__LWPS
#define MA_RW_LOCK_BIAS		 0x01000000
#endif

#ifdef MA__LWPS
#define MA_RW_LOCK_UNLOCKED MA_ATOMIC_INIT(MA_RW_LOCK_BIAS)

#define ma_rwlock_init(x)	do { ma_atomic_init(x, MA_RW_LOCK_BIAS); } while(0)

#define ma_rwlock_is_locked(x) (ma_atomic_read(x) != MA_RW_LOCK_BIAS)
#endif

#ifdef MA__LWPS
#define _ma_raw_read_unlock(rw) ma_atomic_inc(rw)
#define _ma_raw_write_unlock(rw) ma_atomic_add(MA_RW_LOCK_BIAS,rw);
#endif


/** Internal data types **/
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


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_LINUX_RWLOCK_H__ **/
