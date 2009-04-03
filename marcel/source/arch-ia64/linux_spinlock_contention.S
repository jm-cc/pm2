
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

/* Come from arch/ia64/kernel/head.S */

#include "sys/marcel_flags.h"
#include "asmmacro.h"

#ifdef MA__LWPS

//#define MA_IA64_PSR_I_BIT          14

	/*
	 * This routine handles spinlock contention.  It uses a non-standard calling
	 * convention to avoid converting leaf routines into interior routines.  Because
	 * of this special convention, there are several restrictions:
	 *
	 * - do not use gp relative variables, this code is called from the kernel
	 *   and from modules, r1 is undefined.
	 * - do not use stacked registers, the caller owns them.
	 * - do not use the scratch stack space, the caller owns it.
	 * - do not use any registers other than the ones listed below
	 *
	 * Inputs:
	 *   ar.pfs - saved CFM of caller
	 *   ar.ccv - 0 (and available for use)
	 *   r27    - flags from spin_lock_irqsave or 0.  Must be preserved.
	 *   r28    - available for use.
	 *   r29    - available for use.
	 *   r30    - available for use.
	 *   r31    - address of lock, available for use.
	 *   b6     - return address
	 *   p14    - available for use.
	 *   p15    - used to track flag status.
	 *
	 * If you patch this code to use more registers, do not forget to update
	 * the clobber lists for spin_lock() in include/asm-ia64/spinlock.h.
	 */

#if __GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3)

GLOBAL_ENTRY(ma_ia64_spinlock_contention_pre3_4)
	.hidden ma_ia64_spinlock_contention_pre3_4
	.prologue
	.save ar.pfs, r0	// this code effectively has a zero frame size
	.save rp, r28
	.body
	nop 0
#if 0
	tbit.nz p15,p0=r27,IA64_PSR_I_BIT
#endif
	.restore sp		// pop existing prologue after next insn
	mov b6 = r28
	.prologue
	.save ar.pfs, r0
	.altrp b6
	.body
	;;
#if 0
(p15)	ssm psr.i		// reenable interrupts if they were on
				// DavidM says that srlz.d is slow and is not required in this case
#endif
.wait:
	// exponential backoff, kdb, lockmeter etc. go in here
	hint @pause
	ld4 r30=[r31]		// don't use ld4.bias; if it's contended, we won't write the word
	nop 0
	;;
	cmp4.ne p14,p0=r30,r0
(p14)	br.cond.sptk.few .wait
#if 0
(p15)	rsm psr.i		// disable interrupts if we reenabled them
#endif
	br.cond.sptk.few b6	// lock is now free, try to acquire
END(ma_ia64_spinlock_contention_pre3_4)

#else

GLOBAL_ENTRY(ma_ia64_spinlock_contention)
//	.hidden ma_ia64_spinlock_contention
	.prologue
	.altrp b6
	.body
#if 0
	tbit.nz p15,p0=r27,MA_IA64_PSR_I_BIT
	;;
#endif
.wait:
#if 0
(p15)	ssm psr.i		// reenable interrupts if they were on
				// DavidM says that srlz.d is slow and is not required in this case
#endif
.wait2:
	// exponential backoff, kdb, lockmeter etc. go in here
	hint @pause
	ld4 r30=[r31]		// don't use ld4.bias; if it's contended, we won't write the word
	;;
	cmp4.ne p14,p0=r30,r0
	mov r30 = 1
(p14)	br.cond.sptk.few .wait2
#if 0
(p15)	rsm psr.i		// disable interrupts if we reenabled them
#endif
	;;
	cmpxchg4.acq r30=[r31], r30, ar.ccv
	;;
	cmp4.ne p14,p0=r0,r30
(p14)	br.cond.sptk.few .wait

	br.ret.sptk.many b6	// lock is now taken
END(ma_ia64_spinlock_contention)

#endif

#endif /* MA__LWPS */
