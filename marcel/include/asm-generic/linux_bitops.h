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


#ifndef __ASM_GENERIC_LINUX_BITOPS_H__
#define __ASM_GENERIC_LINUX_BITOPS_H__


#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#include "asm/marcel_compareexchange.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define ATOMIC_BITOPT_RETURN(op,retexpr) \
{ \
	unsigned long	mask, old, repl, ret; \
	\
	addr += nr / MA_BITS_PER_LONG; \
	mask = 1UL << (nr % MA_BITS_PER_LONG); \
	old = *addr; \
	while (1) { \
		repl = old op mask; \
		ret = pm2_compareexchange(addr,old,repl,sizeof(*addr)); \
		if (tbx_likely(ret == old)) \
			return retexpr; \
		old = ret; \
	} \
}
#define BITOPT_RETURN(op,retexpr) \
{ \
	unsigned long	mask, old; \
	\
	addr += nr / MA_BITS_PER_LONG; \
	mask = 1UL << (nr % MA_BITS_PER_LONG); \
	old = *addr; \
	*addr = old op mask; \
	return retexpr; \
}

//static __tbx_inline__ int ma__test_bit(int nr, const unsigned long * addr);
#define ma_test_bit(nr,addr) \
	(__builtin_constant_p(nr) ? \
	 ma_constant_test_bit((nr),(addr)) : \
	 ma_variable_test_bit((nr),(addr)))

/*
 * fls: find last bit set.
 */
#define ma_fls(x) ma_generic_fls(x)

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
#define ma_ffs(x) ma_generic_ffs(x)
#define ma___ffs(x) (ma_generic_ffs(x)-1)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */
#define ma_hweight64(x) ma_generic_hweight64(x)
#define ma_hweight32(x) ma_generic_hweight32(x)
#define ma_hweight16(x) ma_generic_hweight16(x)
#define ma_hweight8(x) ma_generic_hweight8(x)

#define ma_smp_mb__before_clear_bit()   ma_barrier()
#define ma_smp_mb__after_clear_bit()    ma_barrier()


/** Internal functions **/
static __tbx_inline__ void ma_set_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ void __ma_set_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ void ma_clear_bit(int nr, volatile unsigned long * addr) ;
static __tbx_inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ void ma_change_bit(int nr, volatile unsigned long * addr) ;
static __tbx_inline__ void __ma_change_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int ma_test_and_set_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int __ma_test_and_set_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int ma_test_and_clear_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int __ma_test_and_clear_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int ma_test_and_change_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int __ma_test_and_change_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ int ma_sched_find_first_bit(const unsigned long *b);
static __tbx_inline__ int ma_constant_test_bit(int nr, const volatile unsigned long *addr) ;
static __tbx_inline__ int ma_variable_test_bit(int nr, const volatile unsigned long * addr) ;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_LINUX_BITOPS_H__ **/
