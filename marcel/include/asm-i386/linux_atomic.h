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


#ifndef __ASM_I386_LINUX_ATOMIC_H__
#define __ASM_I386_LINUX_ATOMIC_H__


#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#endif


/** Public data types **/
/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } ma_atomic_t;


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */
#ifdef MA__LWPS
#define MA_LOCK_PREFIX "lock ; "
#else
#define MA_LOCK_PREFIX ""
#endif

#define MA_ATOMIC_INIT(i)	{ (i) }

/**
 * ma_atomic_read - read atomic variable
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically reads the value of @v.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
#define ma_atomic_read(v)		((v)->counter)

/**
 * ma_atomic_set - set atomic variable
 * @v: pointer of type ma_atomic_t
 * @i: required value
 * 
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
#define ma_atomic_set(v,i)		(((v)->counter) = (i))
#define ma_atomic_init(v,i)		ma_atomic_set((v), (i))

#define ma_atomic_inc_return(v)  (ma_atomic_add_return(1,v))
#define ma_atomic_dec_return(v)  (ma_atomic_sub_return(1,v))

#define ma_atomic_xchg(o,r,v) ma_cmpxchg(&(v)->counter,o,r)


/* These are x86-specific, used by some header files */
#define ma_atomic_clear_mask(mask, addr) \
__asm__ __volatile__(MA_LOCK_PREFIX "andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")

#define ma_atomic_set_mask(mask, addr) \
__asm__ __volatile__(MA_LOCK_PREFIX "orl %0,%1" \
: : "r" (mask),"m" (*(addr)) : "memory")

/* Atomic operations are already serializing on x86 */
#define ma_smp_mb__before_atomic_dec()	ma_barrier()
#define ma_smp_mb__after_atomic_dec()	ma_barrier()
#define ma_smp_mb__before_atomic_inc()	ma_barrier()
#define ma_smp_mb__after_atomic_inc()	ma_barrier()


/** Internal functions **/
/**
 * ma_atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically adds @i to @v.  Note that the guaranteed useful range
 * of an ma_atomic_t is only 24 bits.
 */
static __tbx_inline__ void ma_atomic_add(int i, ma_atomic_t *v);
/**
 * ma_atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically subtracts @i from @v.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */
static __tbx_inline__ void ma_atomic_sub(int i, ma_atomic_t *v);
/**
 * ma_atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */
static __tbx_inline__ int ma_atomic_sub_and_test(int i, ma_atomic_t *v);
/**
 * ma_atomic_inc - increment atomic variable
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
static __tbx_inline__ void ma_atomic_inc(ma_atomic_t *v);
/**
 * ma_atomic_dec - decrement atomic variable
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
static __tbx_inline__ void ma_atomic_dec(ma_atomic_t *v);
/**
 * ma_atomic_dec_and_test - decrement and test
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
static __tbx_inline__ int ma_atomic_dec_and_test(ma_atomic_t *v);
/**
 * ma_atomic_inc_and_test - increment and test 
 * @v: pointer of type ma_atomic_t
 * 
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
static __tbx_inline__ int ma_atomic_inc_and_test(ma_atomic_t *v);
/**
 * ma_atomic_add_negative - add and test if negative
 * @v: pointer of type ma_atomic_t
 * @i: integer value to add
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.  Note that the guaranteed
 * useful range of an ma_atomic_t is only 24 bits.
 */ 
static __tbx_inline__ int ma_atomic_add_negative(int i, ma_atomic_t *v);
static __tbx_inline__ int ma_atomic_add_return(int i, ma_atomic_t *v);
static __tbx_inline__ int ma_atomic_sub_return(int i, ma_atomic_t *v);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_LINUX_ATOMIC_H__ **/
