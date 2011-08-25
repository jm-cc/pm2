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


#ifndef __ASM_IA64_LINUX_ATOMIC_H__
#define __ASM_IA64_LINUX_ATOMIC_H__


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


#include "tbx_compiler.h"
#include "asm/linux_types.h"


/** Public data types **/
typedef struct {
	volatile int32_t counter;
} ma_atomic_t;
typedef struct {
	volatile int64_t counter;
} ma_atomic64_t;


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_ATOMIC_INIT(i)	{ (i) }
#define MA_ATOMIC64_INIT(i)	{ (i) }
#define ma_atomic_read(v)	((v)->counter)
#define ma_atomic64_read(v)	((v)->counter)
#define ma_atomic_set(v,i)	(((v)->counter) = (i))
#define ma_atomic64_set(v,i)	(((v)->counter) = (i))
#define ma_atomic_init(v,i)	ma_atomic_set((v),(i))
#define ma_atomic64_init(v,i)	ma_atomic64_set((v),(i))
#define ma_atomic_add_return(i,v)						\
({									\
	int __ia64_aar_i = (i);						\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_aar_i ==  1) || (__ia64_aar_i ==   4)		\
	     || (__ia64_aar_i ==  8) || (__ia64_aar_i ==  16)		\
	     || (__ia64_aar_i == -1) || (__ia64_aar_i ==  -4)		\
	     || (__ia64_aar_i == -8) || (__ia64_aar_i == -16)))		\
		? ma_ia64_fetch_and_add(__ia64_aar_i, &(v)->counter)	\
		: ma_ia64_atomic_add(__ia64_aar_i, v);			\
})
#define ma_atomic64_add_return(i,v)					\
({									\
	long __ia64_aar_i = (i);					\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_aar_i ==  1) || (__ia64_aar_i ==   4)		\
	     || (__ia64_aar_i ==  8) || (__ia64_aar_i ==  16)		\
	     || (__ia64_aar_i == -1) || (__ia64_aar_i ==  -4)		\
	     || (__ia64_aar_i == -8) || (__ia64_aar_i == -16)))		\
		? ma_ia64_fetch_and_add(__ia64_aar_i, &(v)->counter)	\
		: ma_ia64_atomic64_add(__ia64_aar_i, v);			\
})
#define ma_atomic_sub_return(i,v)						\
({									\
	int __ia64_asr_i = (i);						\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_asr_i ==   1) || (__ia64_asr_i ==   4)		\
	     || (__ia64_asr_i ==   8) || (__ia64_asr_i ==  16)		\
	     || (__ia64_asr_i ==  -1) || (__ia64_asr_i ==  -4)		\
	     || (__ia64_asr_i ==  -8) || (__ia64_asr_i == -16)))	\
		? ma_ia64_fetch_and_add(-__ia64_asr_i, &(v)->counter)	\
		: ma_ia64_atomic_sub(__ia64_asr_i, v);			\
})
#define ma_atomic64_sub_return(i,v)					\
({									\
	long __ia64_asr_i = (i);					\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_asr_i ==   1) || (__ia64_asr_i ==   4)		\
	     || (__ia64_asr_i ==   8) || (__ia64_asr_i ==  16)		\
	     || (__ia64_asr_i ==  -1) || (__ia64_asr_i ==  -4)		\
	     || (__ia64_asr_i ==  -8) || (__ia64_asr_i == -16)))	\
		? ma_ia64_fetch_and_add(-__ia64_asr_i, &(v)->counter)	\
		: ma_ia64_atomic64_sub(__ia64_asr_i, v);			\
})
#define ma_atomic_dec_return(v)		ma_atomic_sub_return(1, (v))
#define ma_atomic_inc_return(v)		ma_atomic_add_return(1, (v))
#define ma_atomic64_dec_return(v)	ma_atomic64_sub_return(1, (v))
#define ma_atomic64_inc_return(v)	ma_atomic64_add_return(1, (v))
#define ma_atomic_sub_and_test(i,v)	(ma_atomic_sub_return((i), (v)) == 0)
#define ma_atomic_dec_and_test(v)	(ma_atomic_sub_return(1, (v)) == 0)
#define ma_atomic_inc_and_test(v)	(ma_atomic_add_return(1, (v)) != 0)
#define ma_atomic64_sub_and_test(i,v)	(ma_atomic64_sub_return((i), (v)) == 0)
#define ma_atomic64_dec_and_test(v)	(ma_atomic64_sub_return(1, (v)) == 0)
#define ma_atomic64_inc_and_test(v)	(ma_atomic64_add_return(1, (v)) != 0)
#define ma_atomic_add(i,v)		ma_atomic_add_return((i), (v))
#define ma_atomic_sub(i,v)		ma_atomic_sub_return((i), (v))
#define ma_atomic_inc(v)		ma_atomic_add(1, (v))
#define ma_atomic_dec(v)		ma_atomic_sub(1, (v))
#define ma_atomic64_add(i,v)		ma_atomic64_add_return((i), (v))
#define ma_atomic64_sub(i,v)		ma_atomic64_sub_return((i), (v))
#define ma_atomic64_inc(v)		ma_atomic64_add(1, (v))
#define ma_atomic64_dec(v)		ma_atomic64_sub(1, (v))
#define ma_atomic_xchg(o,r,v)           ma_cmpxchg(&(v)->counter,o,r)
#define ma_atomic64_xchg(o,r,v)         ma_cmpxchg(&(v)->counter,o,r)

/* Atomic operations are already serializing */
#define ma_smp_mb__before_atomic_dec()	ma_barrier()
#define ma_smp_mb__after_atomic_dec()	ma_barrier()
#define ma_smp_mb__before_atomic_inc()	ma_barrier()
#define ma_smp_mb__after_atomic_inc()	ma_barrier()


/** Internal functions **/
static __tbx_inline__ int ma_ia64_atomic_add(int i, ma_atomic_t * v);

static __tbx_inline__ int ma_ia64_atomic64_add(int64_t i, ma_atomic64_t * v);
static __tbx_inline__ int ma_ia64_atomic_sub(int i, ma_atomic_t * v);
static __tbx_inline__ int ma_ia64_atomic64_sub(int64_t i, ma_atomic64_t * v);
static __tbx_inline__ int ma_atomic_add_negative(int i, ma_atomic_t * v);
static __tbx_inline__ int ma_atomic64_add_negative(int64_t i, ma_atomic64_t * v);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_LINUX_ATOMIC_H__ **/
