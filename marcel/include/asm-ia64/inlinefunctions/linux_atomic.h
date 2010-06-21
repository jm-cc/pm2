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


#ifndef __INLINEFUNCTIONS_ASM_IA64_LINUX_ATOMIC_H__
#define __INLINEFUNCTIONS_ASM_IA64_LINUX_ATOMIC_H__


/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 *
 * NOTE: don't mess with the types below!  The "unsigned long" and
 * "int" types were carefully placed so as to ensure proper operation
 * of the macros.
 *
 * Copyright (C) 1998, 1999, 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

/*
 * On IA-64, counter must always be volatile to ensure that that the
 * memory accesses are ordered.
 */


#include "asm/linux_atomic.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
static __tbx_inline__ int ma_ia64_atomic_add(int i, ma_atomic_t * v)
{
	int old, repl;
	MA_CMPXCHG_BUGCHECK_DECL
	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		repl = old + i;
	} while (ma_ia64_cmpxchg(acq, v, old, repl, sizeof(ma_atomic_t)) != old);
	return repl;
}

static __tbx_inline__ int ma_ia64_atomic64_add(int64_t i, ma_atomic64_t * v)
{
	int64_t old, repl;
	MA_CMPXCHG_BUGCHECK_DECL
	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		repl = old + i;
	} while (ma_ia64_cmpxchg(acq, v, old, repl, sizeof(ma_atomic_t)) != old);
	return repl;
}

static __tbx_inline__ int ma_ia64_atomic_sub(int i, ma_atomic_t * v)
{
	int old, repl;
	MA_CMPXCHG_BUGCHECK_DECL
	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		repl = old - i;
	} while (ma_ia64_cmpxchg(acq, v, old, repl, sizeof(ma_atomic_t)) != old);
	return repl;
}

static __tbx_inline__ int ma_ia64_atomic64_sub(int64_t i, ma_atomic64_t * v)
{
	int64_t old, repl;
	MA_CMPXCHG_BUGCHECK_DECL
	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		repl = old - i;
	} while (ma_ia64_cmpxchg(acq, v, old, repl, sizeof(ma_atomic_t)) != old);
	return repl;
}

/*
 * Atomically add I to V and return TRUE if the resulting value is
 * negative.
 */
static __tbx_inline__ int ma_atomic_add_negative(int i, ma_atomic_t * v)
{
	return ma_atomic_add_return(i, v) < 0;
}

static __tbx_inline__ int ma_atomic64_add_negative(int64_t i, ma_atomic64_t * v)
{
	return ma_atomic64_add_return(i, v) < 0;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_IA64_LINUX_ATOMIC_H__ **/
