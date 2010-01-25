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


#ifndef __ASM_PPC_LINUX_SYSTEM_H__
#define __ASM_PPC_LINUX_SYSTEM_H__


/*
 * Similar to:
 * include/asm-ia64/system.h
 *
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


#include "tbx_compiler.h"


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define ma_xchg(ptr,x)                                                          \
  ({                                                                         \
     __typeof__(*(ptr)) _x_ = (x);                                           \
     (__typeof__(*(ptr))) __xchg((ptr), (unsigned long)_x_, sizeof(*(ptr))); \
  })


#define xchg_local(ptr,x)                                                    \
  ({                                                                         \
     __typeof__(*(ptr)) _x_ = (x);                                           \
     (__typeof__(*(ptr))) __xchg_local((ptr),                                \
                (unsigned long)_x_, sizeof(*(ptr)));                         \
  })


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

#define ma_mb()		__asm__ __volatile__ ("sync" : : : "memory")
#define ma_rmb()	__asm__ __volatile__ ("sync" : : : "memory")
#define ma_wmb()	__asm__ __volatile__ ("eieio" : : : "memory")
#define ma_read_barrier_depends()	do { } while(0)

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	do { } while(0)
#endif

/*
 * XXX check on these---I suspect what Linus really wants here is
 * acquire vs release semantics but we can't discuss this stuff with
 * Linus just yet.  Grrr...
 */
#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

#define ma_cpu_relax() ma_barrier()


/** Internal inline functions **/
/*
 * taken from Linux (include/asm-powerpc/system.h)
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
static __tbx_inline__ unsigned long
__xchg_u32(volatile void *p, unsigned long val) ;
static __tbx_inline__ unsigned long
__xchg_u32_local(volatile void *p, unsigned long val) ;
static __tbx_inline__ unsigned long
__xchg(volatile void *ptr, unsigned long x, unsigned int size) ;
static __tbx_inline__ unsigned long
__xchg_local(volatile void *ptr, unsigned long x, unsigned int size) ;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_PPC_LINUX_SYSTEM_H__ **/
