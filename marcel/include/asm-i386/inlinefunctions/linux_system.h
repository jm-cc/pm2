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


#ifndef __ASM_I386_INLINEFUNCTIONS_LINUX_SYSTEM_H__
#define __ASM_I386_INLINEFUNCTIONS_LINUX_SYSTEM_H__


#include "asm/linux_system.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
/*
 * The semantics of XCHGCMP8B are a bit strange, this is why
 * there is a loop and the loading of %%eax and %%edx has to
 * be inside. This inlines well in most cases, the cached
 * cost is around ~38 cycles. (in the future we might want
 * to do an SIMD/3DNOW!/MMX/FPU 64-bit store here, but that
 * might have an implicit FPU-save as a cost, so it's not
 * clear which path to go.)
 *
 * cmpxchg8b must be used with the lock prefix here to allow
 * the instruction to be executed atomically, see page 3-102
 * of the instruction set reference 24319102.pdf. We need
 * the reader side to see the coherent 64bit value.
 */
static __tbx_inline__ unsigned long __ma_xchg(unsigned long x, volatile void *ptr,
					      int size)
{
	switch (size) {
	case 1:
	      __asm__ __volatile__("xchgb %b0,%1":"=q"(x)
	      :		     "m"(*__ma_xg(ptr)), "0"(x)
	      :		     "memory");
		break;
	case 2:
	      __asm__ __volatile__("xchgw %w0,%1":"=r"(x)
	      :		     "m"(*__ma_xg(ptr)), "0"(x)
	      :		     "memory");
		break;
	case 4:
	      __asm__ __volatile__("xchgl %0,%1":"=r"(x)
	      :		     "m"(*__ma_xg(ptr)), "0"(x)
	      :		     "memory");
		break;
	default:
		abort();
	}
	return x;
}

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
static __tbx_inline__ unsigned long TBX_NOINST __ma_cmpxchg(volatile void *ptr,
							    unsigned long old,
							    unsigned long repl, int size)
{
	unsigned long prev;
	switch (size) {
	case 1:
	      __asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgb %b1,%2":"=a"(prev)
	      :		     "q"(repl), "m"(*__ma_xg(ptr)),
				     "0"(old)
	      :		     "memory");
		return prev;
	case 2:
	      __asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgw %w1,%2":"=a"(prev)
	      :		     "q"(repl), "m"(*__ma_xg(ptr)),
				     "0"(old)
	      :		     "memory");
		return prev;
	case 4:
	      __asm__ __volatile__(MA_LOCK_PREFIX "cmpxchgl %1,%2":"=a"(prev)
	      :		     "q"(repl), "m"(*__ma_xg(ptr)),
				     "0"(old)
	      :		     "memory");
		return prev;
	default:
		abort();
	}
	return old;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_LINUX_SYSTEM_H__ **/
