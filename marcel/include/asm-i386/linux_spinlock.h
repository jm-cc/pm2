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


#ifndef __ASM_I386_LINUX_SPINLOCK_H__
#define __ASM_I386_LINUX_SPINLOCK_H__


#include "tbx_compiler.h"
#include "marcel_utils.h"


/** Public macros **/
#ifdef X86_ARCH
#define MA_SPINB "b"
#define MA_SPINb "b"
#define MA_SPINT char
#endif

#ifdef MA__LWPS
#define MA_SPIN_LOCK_UNLOCKED { 1 }
#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)
#endif				/* MA__LWPS */


/** Public data types **/
#ifdef MA__LWPS
/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */
typedef struct {
	volatile unsigned int lock;
} ma_spinlock_t;
#endif				/* MA__LWPS */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#ifdef MA__LWPS
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */
#define ma_spin_is_locked(x)	(*(volatile signed MA_SPINT *)(&(x)->lock) <= 0)
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))
#define ma_spin_lock_string \
	"\n1:\t" \
	"lock ; dec"MA_SPINB" %0\n\t" \
	"jns 3f\n" \
	"2:\t" \
	"rep;nop\n\t" \
	"cmp"MA_SPINB" $0,%0\n\t" \
	"jle 2b\n\t" \
	"jmp 1b\n\t" \
	"3:\n"
#endif				/* MA__LWPS */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_LINUX_SPINLOCK_H__ **/
