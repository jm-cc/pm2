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


#ifndef __INLINEFUNCTIONS_ASM_IA64_LINUX_SPINLOCK_H__
#define __INLINEFUNCTIONS_ASM_IA64_LINUX_SPINLOCK_H__


/*
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 *
 * This file is used for SMP configurations only.
 */


#include "tbx_compiler.h"
#include "asm/linux_spinlock.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
#ifdef MA__LWPS
/*
 * Try to get the lock.  If we fail to get the lock, make a non-standard call to
 * ia64_spinlock_contention().  We do not use a normal call because that would force all
 * callers of spin_lock() to be non-leaf routines.  Instead, ia64_spinlock_contention() is
 * carefully coded to touch only those registers that spin_lock() marks "clobbered".
 */
#define MA_IA64_SPINLOCK_CLOBBERS "ar.ccv", "ar.pfs", "p14", /*"p15", "r27", */ "r28", "r29", "r30", "b6", "memory"
static __tbx_inline__ void _ma_raw_spin_lock(ma_spinlock_t * lock)
{
	register volatile unsigned int *ptr asm("r31") = &lock->lock;

#if __GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3)
#   ifdef CONFIG_ITANIUM
	/* don't use brl on Itanium... */
	asm volatile ("{\n\t"
		      "  mov ar.ccv = r0\n\t"
		      "  mov r28 = ip\n\t"
		      "  mov r30 = 1;;\n\t"
		      "}\n\t"
		      "cmpxchg4.acq r30 = [%1], r30, ar.ccv\n\t"
		      "movl r29 = ma_ia64_spinlock_contention_pre3_4;;\n\t"
		      "cmp4.ne p14, p0 = r30, r0\n\t" "mov b6 = r29;;\n"
		      //"mov r27=%2\n\t"
		      "(p14) br.cond.spnt.many b6":"=r" (ptr):"r"(ptr)	/*, "r" (flags) */
		      :MA_IA64_SPINLOCK_CLOBBERS);
#   else
	asm volatile ("{\n\t"
		      "  mov ar.ccv = r0\n\t"
		      "  mov r28 = ip\n\t"
		      "  mov r30 = 1;;\n\t"
		      "}\n\t"
		      "cmpxchg4.acq r30 = [%1], r30, ar.ccv;;\n\t"
		      "cmp4.ne p14, p0 = r30, r0\n"
		      //"mov r27=%2\n\t"
		      "(p14) brl.cond.spnt.many ma_ia64_spinlock_contention_pre3_4;;":"=r"
		      (ptr):"r"(ptr) /*, "r" (flags) */ :MA_IA64_SPINLOCK_CLOBBERS);
#   endif			/* CONFIG_MCKINLEY */
#else
#   ifdef CONFIG_ITANIUM
	/* don't use brl on Itanium... */
	/* mis-declare, so we get the entry-point, not it's function descriptor: */
	asm volatile ("mov r30 = 1\n\t"
		      //"mov r27=%2\n\t"
		      "mov ar.ccv = r0;;\n\t"
		      "cmpxchg4.acq r30 = [%0], r30, ar.ccv\n\t"
		      "movl r29 = ma_ia64_spinlock_contention;;\n\t"
		      "cmp4.ne p14, p0 = r30, r0\n\t"
		      "mov b6 = r29;;\n"
		      "(p14) br.call.spnt.many b6 = b6":"=r" (ptr):"r"(ptr)
		      /*, "r" (flags) */
		      :MA_IA64_SPINLOCK_CLOBBERS);
#   else
	asm volatile ("mov r30 = 1\n\t"
		      //"mov r27=%2\n\t"
		      "mov ar.ccv = r0;;\n\t"
		      "cmpxchg4.acq r30 = [%0], r30, ar.ccv;;\n\t"
		      "cmp4.ne p14, p0 = r30, r0\n\t"
		      "(p14) brl.call.spnt.many b6=ma_ia64_spinlock_contention;;":"=r"
		      (ptr):"r"(ptr) /*, "r" (flags) */ :MA_IA64_SPINLOCK_CLOBBERS);
#   endif			/* CONFIG_MCKINLEY */
#endif
}
#endif				/* MA__LWPS */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_IA64_LINUX_SPINLOCK_H__ **/
