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


#ifndef __ASM_X86_64_LINUX_SYSTEM_H__
#define __ASM_X86_64_LINUX_SYSTEM_H__


#include "tbx_compiler.h"


/*
 * System defines. Note that this is included both from .c and .S
 * files, so it does only defines, not any C code.  This is based
 * on information published in the Processor Abstraction Layer
 * and the System Abstraction Layer manual.
 *
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   ma_wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   ma_rmb():	Like wmb(), but for reads.
 *   ma_mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "ma_mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */

#define ma_mb()		__asm__ __volatile__("mfence":::"memory")
/*
 * Some non-Intel clones support out of order store. wmb() ceases to be a
 * nop for these.
 */
#define ma_rmb()	__asm__ __volatile__("lfence":::"memory")
#define ma_wmb()	__asm__ __volatile__("sfence":::"memory")
#define ma_read_barrier_depends()	do { } while(0)

#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))__ma_cmpxchg((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
# define ma_set_mb(var, value) do { (void) ma_xchg(&(var), (value)); } while (0)
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	do { } while(0)
# define ma_set_mb(var, value) do { (var) = (value); ma_barrier(); } while (0)
#endif

#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

#define ma_cpu_relax() asm volatile("rep; nop" ::: "memory")


/** Internal functions **/
/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
#define __ma_xg(x) ((volatile long *)(x))
#define ma_xchg(ptr,v) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

static __tbx_inline__ unsigned long __ma_xchg(unsigned long x, volatile void * ptr, int size) ;


#ifdef MA__LWPS
#define MA_LOCK_PREFIX "lock ; "
#else
#define MA_LOCK_PREFIX ""
#endif

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
static __tbx_inline__ unsigned long TBX_NOINST __ma_cmpxchg(volatile void *ptr, unsigned long old,
							    unsigned long repl, int size) ;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_X86_64_LINUX_SYSTEM_H__ **/
