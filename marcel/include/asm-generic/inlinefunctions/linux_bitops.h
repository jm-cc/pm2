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


#ifndef __ASM_GENERIC_INLINEFUNCTIONS_LINUX_BITOPS_H__
#define __ASM_GENERIC_INLINEFUNCTIONS_LINUX_BITOPS_H__


#include "asm/linux_bitops.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assembly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */

static __tbx_inline__ void ma_set_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(|,)

static __tbx_inline__ void __ma_set_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(|,)

static __tbx_inline__ void ma_clear_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(& ~,)

static __tbx_inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(& ~,)

static __tbx_inline__ void ma_change_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(^,)

static __tbx_inline__ void __ma_change_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(^,)

static __tbx_inline__ int ma_constant_test_bit(int nr, const volatile unsigned long *addr)
{
        return ((1UL << (nr % MA_BITS_PER_LONG)) & (addr[nr / MA_BITS_PER_LONG])) != 0;
}
static __tbx_inline__ int ma_variable_test_bit(int nr, const volatile unsigned long * addr)
{
	unsigned long	mask;

	addr += nr / MA_BITS_PER_LONG;
	mask = 1UL << (nr % MA_BITS_PER_LONG);
	return ((mask & *addr) != 0);
}
	
static __tbx_inline__ int ma_test_and_set_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(|,(old & mask) != 0)

static __tbx_inline__ int __ma_test_and_set_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(|,(old & mask) != 0)

static __tbx_inline__ int ma_test_and_clear_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(& ~,(old & mask) != 0)

static __tbx_inline__ int __ma_test_and_clear_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(& ~,(old & mask) != 0)

static __tbx_inline__ int ma_test_and_change_bit(int nr, volatile unsigned long * addr)
	ATOMIC_BITOPT_RETURN(^,(old & mask) != 0)

static __tbx_inline__ int __ma_test_and_change_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(^,(old & mask) != 0)


static __tbx_inline__ int ma_sched_find_first_bit(const unsigned long *b)
{
	int i;
	for (i=0;i<MAP_BITMAP_BITS/MA_BITS_PER_LONG;i++) {
		if (tbx_unlikely(b[i]))
			return ma___ffs(b[i]) + MA_BITS_PER_LONG*i;
	}
	if (i*MA_BITS_PER_LONG==MA_BITMAP_BITS)
		abort();
	return ma___ffs(b[i]) + MA_BITS_PER_LONG*i;
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_INLINEFUNCTIONS_LINUX_BITOPS_H__ **/
