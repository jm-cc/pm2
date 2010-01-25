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


#ifndef __ASM_GENERIC_INLINEFUNCTIONS_LINUX_ATOMIC_H__
#define __ASM_GENERIC_INLINEFUNCTIONS_LINUX_ATOMIC_H__


#include "asm/linux_atomic.h"
#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#include "asm/marcel_compareexchange.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
static __tbx_inline__ void ma_atomic_add(int i, ma_atomic_t *v)
{
#define __rien
	MA_ATOMIC_ADD_RETURN(__rien);
#undef __rien
}

static __tbx_inline__ int ma_atomic_add_and_test(int i, ma_atomic_t *v)
{
	MA_ATOMIC_ADD_RETURN(repl == 0);
}

static __tbx_inline__ int ma_atomic_add_negative(int i, ma_atomic_t *v)
{
	MA_ATOMIC_ADD_RETURN(repl < 0);
}

static __tbx_inline__ int ma_atomic_add_return(int i, ma_atomic_t *v);
static __tbx_inline__ int ma_atomic_add_return(int i, ma_atomic_t *v)
{
	MA_ATOMIC_ADD_RETURN(repl);
}

static __tbx_inline__ int ma_atomic_xchg(int old, int repl, ma_atomic_t *v);
static __tbx_inline__ int ma_atomic_xchg(int old, int repl, ma_atomic_t *v)
{
#ifdef MA_HAVE_COMPAREEXCHANGE
	return pm2_compareexchange(&v->counter,old,repl,sizeof(v->counter));
#else
	int cur;
	ma_spin_lock_softirq(&v->lock);
	cur = ma_atomic_read(v);
	if (cur == old)
		ma_atomic_set(v, repl);
	ma_spin_unlock_softirq(&v->lock);
	return cur;
#endif
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_INLINEFUNCTIONS_LINUX_ATOMIC_H__ **/
