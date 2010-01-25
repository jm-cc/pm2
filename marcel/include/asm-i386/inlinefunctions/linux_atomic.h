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


#ifndef __ASM_I386_INLINEFUNCTIONS_LINUX_ATOMIC_H__
#define __ASM_I386_INLINEFUNCTIONS_LINUX_ATOMIC_H__


#include "tbx_compiler.h"
#include "asm/linux_atomic.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
static __tbx_inline__ void ma_atomic_add(int i, ma_atomic_t *v)
{
	__asm__ __volatile__(
		MA_LOCK_PREFIX "addl %1,%0"
		:"+m" (v->counter)
		:"ir" (i));
}

static __tbx_inline__ void ma_atomic_sub(int i, ma_atomic_t *v)
{
	__asm__ __volatile__(
		MA_LOCK_PREFIX "subl %1,%0"
		:"+m" (v->counter)
		:"ir" (i));
}

static __tbx_inline__ int ma_atomic_sub_and_test(int i, ma_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		MA_LOCK_PREFIX "subl %2,%0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		:"ir" (i) : "memory");
	return c;
}

static __tbx_inline__ void ma_atomic_inc(ma_atomic_t *v)
{
	__asm__ __volatile__(
		MA_LOCK_PREFIX "incl %0"
		:"+m" (v->counter));
}

static __tbx_inline__ void ma_atomic_dec(ma_atomic_t *v)
{
	__asm__ __volatile__(
		MA_LOCK_PREFIX "decl %0"
		:"+m" (v->counter));
}

static __tbx_inline__ int ma_atomic_dec_and_test(ma_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		MA_LOCK_PREFIX "decl %0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		: : "memory");
	return c != 0;
}

static __tbx_inline__ int ma_atomic_inc_and_test(ma_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		MA_LOCK_PREFIX "incl %0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		: : "memory");
	return c != 0;
}

static __tbx_inline__ int ma_atomic_add_negative(int i, ma_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		MA_LOCK_PREFIX "addl %2,%0; sets %1"
		:"+m" (v->counter), "=qm" (c)
		:"ir" (i) : "memory");
	return c;
}

/* Same as atomic_add, but return the resulting value after addition */
static __tbx_inline__ int ma_atomic_add_return(int i, ma_atomic_t *v)
{
	int __i;
	/* Modern 486+ processor */
	__i = i;
	__asm__ __volatile__(
		MA_LOCK_PREFIX "xaddl %0, %1;"
		:"+r"(i), "+m" (v->counter)
		: : "memory");
	return i + __i;

}

/* Same as atomic_add, but return the resulting value after substraction */
static __tbx_inline__ int ma_atomic_sub_return(int i, ma_atomic_t *v)
{
	return ma_atomic_add_return(-i,v);
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_LINUX_ATOMIC_H__ **/
