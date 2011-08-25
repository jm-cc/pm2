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


#ifndef __INLINEFUNCTIONS_ASM_PPC64_LINUX_SYSTEM_H__
#define __INLINEFUNCTIONS_ASM_PPC64_LINUX_SYSTEM_H__


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


#include "asm/linux_system.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
/*
 * taken from Linux (include/asm-powerpc/system.h)
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
static __tbx_inline__ unsigned long __xchg_u32(volatile void *p, unsigned long val)
{
	unsigned long prev;

	__asm__ __volatile__("lwsync	\n"
			     "1:     lwarx   %0,0,%2 \n" "       stwcx.  %3,0,%2 \n\
        bne-    1b" "\n\tisync\n":"=&r"(prev), "+m"(*(volatile unsigned int *) p)
			     :"r"(p), "r"(val)
			     :"cc", "memory");

	return prev;
}



static __tbx_inline__ unsigned long __xchg_u32_local(volatile void *p, unsigned long val)
{
	unsigned long prev;

	__asm__ __volatile__("1:     lwarx   %0,0,%2 \n" "       stwcx.  %3,0,%2 \n\
        bne-    1b":"=&r"(prev), "+m"(*(volatile unsigned int *) p)
			     :"r"(p), "r"(val)
			     :"cc", "memory");

	return prev;
}


static __tbx_inline__ unsigned long __xchg_u64(volatile void *p, unsigned long val)
{
	unsigned long prev;

	__asm__ __volatile__("lwsync	\n"
			     "1:     ldarx   %0,0,%2 \n" "       stdcx.  %3,0,%2 \n\
        bne-    1b" "\n\tisync\n":"=&r"(prev), "+m"(*(volatile unsigned long *) p)
			     :"r"(p), "r"(val)
			     :"cc", "memory");

	return prev;
}


static __tbx_inline__ unsigned long __xchg_u64_local(volatile void *p, unsigned long val)
{
	unsigned long prev;

	__asm__ __volatile__("1:     ldarx   %0,0,%2 \n" "       stdcx.  %3,0,%2 \n\
        bne-    1b":"=&r"(prev), "+m"(*(volatile unsigned long *) p)
			     :"r"(p), "r"(val)
			     :"cc", "memory");

	return prev;
}


static __tbx_inline__ unsigned long
__xchg(volatile void *ptr, unsigned long x, unsigned int size)
{
	switch (size) {
	case 4:
		return __xchg_u32(ptr, x);
	case 8:
		return __xchg_u64(ptr, x);
	default:
		abort();
	}
	return x;
}

static __tbx_inline__ unsigned long
__xchg_local(volatile void *ptr, unsigned long x, unsigned int size)
{
	switch (size) {
	case 4:
		return __xchg_u32_local(ptr, x);
	case 8:
		return __xchg_u64_local(ptr, x);
	default:
		abort();
	}
	return x;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_PPC64_LINUX_SYSTEM_H__ **/
