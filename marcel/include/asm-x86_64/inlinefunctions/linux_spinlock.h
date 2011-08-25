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


#ifndef __ASM_X86_64_INLINEFUNCTIONS_LINUX_SPINLOCK_H__
#define __ASM_X86_64_INLINEFUNCTIONS_LINUX_SPINLOCK_H__


#include "asm/linux_spinlock.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
#ifdef MA__LWPS
static __tbx_inline__ void _ma_raw_spin_unlock(ma_spinlock_t * lock)
{
	__asm__ __volatile__("movl $1,%0"
			     : "=m" (lock->lock) : : "memory");

}

static __tbx_inline__ int _ma_raw_spin_trylock(ma_spinlock_t * lock)
{
	int oldval;
	__asm__ __volatile__("xchgl %0,%1" : "=q"(oldval),
			     "=m"(lock->lock) : "0"(0) : "memory");
	return oldval > 0;
}

static __tbx_inline__ void _ma_raw_spin_lock(ma_spinlock_t * lock)
{
	__asm__ __volatile__("\n1:\t"
			     "lock ; decl %0\n\t"
			     "jns 3f\n"
			     "2:\t"
			     "rep;nop\n\t"
			     "cmpl $0,%0\n\t"
			     "jle 2b\n\t"
			     "jmp 1b\n\t"
			     "3:\n" : "=m"(lock->lock) : : "memory");
}

static __tbx_inline__ int ma_spin_is_locked(ma_spinlock_t * lock)
{
	return lock->lock <= 0;
}
#endif				/* MA__LWPS */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_X86_64_INLINEFUNCTIONS_LINUX_SPINLOCK_H__ **/
