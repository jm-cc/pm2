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


#ifndef __INLINEFUNCTIONS_ASM_IA64_LINUX_BITOPS_H__
#define __INLINEFUNCTIONS_ASM_IA64_LINUX_BITOPS_H__


/*
 * Similar to:
 * include/asm-ia64/bitops.h
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * 02/06/02 find_next_bit() and find_first_bit() added from Erich Focht's ia64 O(1)
 *	    scheduler patch
 */


#include "asm/linux_bitops.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 *
 * The address must be (at least) "long" aligned.
 * Note that there are driver (e.g., eepro100) which use these operations to operate on
 * hw-defined data-structures, so we can't easily change these operations to force a
 * bigger alignment.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */
static __tbx_inline__ void
ma_set_bit (int nr, volatile void *addr)
{
	__ma_u32 bit, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	bit = 1 << (nr & 31);
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old | bit;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
}

/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static __tbx_inline__ void
__ma_set_bit (int nr, volatile void *addr)
{
	*((volatile __ma_u32 *) addr + (nr >> 5)) |= (1 << (nr & 31));
}

/*
 * clear_bit() has "acquire" semantics.
 */
static __tbx_inline__ void
ma_clear_bit (int nr, volatile void *addr)
{
	__ma_u32 mask, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	mask = ~(1 << (nr & 31));
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old & mask;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
}

/**
 * __clear_bit - Clears a bit in memory (non-atomic version)
 */
/* #section marcel_functions
 *static __tbx_inline__ void
 *__ma_clear_bit (int nr, volatile void *addr);
 *#section marcel_inline
 *static __tbx_inline__ void
 *__ma_clear_bit (int nr, volatile void *addr)
 *{
 *	volatile __ma_u32 *p = (volatile __ma_u32 *) addr + (nr >> 5);
 *	__ma_u32 m = 1 << (nr & 31);
 *	*p &= ~m;
 *}
 */

/**
 * change_bit - Toggle a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * change_bit() is atomic and may not be reordered.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __tbx_inline__ void
ma_change_bit (int nr, volatile void *addr)
{
	__ma_u32 bit, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	bit = (1 << (nr & 31));
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old ^ bit;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
}

/**
 * __change_bit - Toggle a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike change_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static __tbx_inline__ void
__ma_change_bit (int nr, volatile void *addr)
{
	*((volatile __ma_u32 *) addr + (nr >> 5)) ^= (1 << (nr & 31));
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int
ma_test_and_set_bit (int nr, volatile void *addr)
{
	__ma_u32 bit, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	bit = 1 << (nr & 31);
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old | bit;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
	return (old & bit) != 0;
}

/**
 * __test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.  
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static __tbx_inline__ int
__ma_test_and_set_bit (int nr, volatile void *addr)
{
	__ma_u32 *p = (__ma_u32 *) addr + (nr >> 5);
	__ma_u32 m = 1 << (nr & 31);
	int oldbitset = (*p & m) != 0;

	*p |= m;
	return oldbitset;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int
ma_test_and_clear_bit (int nr, volatile void *addr)
{
	__ma_u32 mask, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	mask = ~(1 << (nr & 31));
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old & mask;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
	return (old & ~mask) != 0;
}

/**
 * __test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from

 *
 * This operation is non-atomic and can be reordered.  
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static __tbx_inline__ int
__ma_test_and_clear_bit(int nr, volatile void * addr)
{
	__ma_u32 *p = (__ma_u32 *) addr + (nr >> 5);
	__ma_u32 m = 1 << (nr & 31);
	int oldbitset = *p & m;

	*p &= ~m;
	return oldbitset;
}

/**
 * test_and_change_bit - Change a bit and return its new value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int
ma_test_and_change_bit (int nr, volatile void *addr)
{
	__ma_u32 bit, old, repl;
	volatile __ma_u32 *m;
	MA_CMPXCHG_BUGCHECK_DECL

	m = (volatile __ma_u32 *) addr + (nr >> 5);
	bit = (1 << (nr & 31));
	do {
		MA_CMPXCHG_BUGCHECK(m);
		old = *m;
		repl = old ^ bit;
	} while (ma_cmpxchg_acq(m, old, repl) != old);
	return (old & bit) != 0;
}

/*
 * WARNING: non atomic version.
 */
static __tbx_inline__ int
__ma_test_and_change_bit (int nr, volatile void *addr)
{
	__ma_u32 old, bit = (1 << (nr & 31));
	__ma_u32 *m = (__ma_u32 *) addr + (nr >> 5);

	old = *m;
	*m = old ^ bit;
	return (old & bit) != 0;
}

static __tbx_inline__ int
ma_test_bit (int nr, const volatile void *addr)
{
	return 1 & (((const volatile __ma_u32 *) addr)[nr >> 5] >> (nr & 31));
}

/**
 * ffz - find the first zero bit in a long word
 * @x: The long word to find the bit in
 *
 * Returns the bit-number (0..63) of the first (least significant) zero bit.  Undefined if
 * no zero exists, so code should check against ~0UL first...
 */
static __tbx_inline__ unsigned long
ma_ffz (unsigned long x)
{
	unsigned long result;
	result = ma_ia64_popcnt(x & (~x - 1));
	return result;
}

/**
 * __ffs - find first bit in word.
 * @x: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __tbx_inline__ unsigned long
__ma_ffs (unsigned long x)
{
	unsigned long result;
	result = ma_ia64_popcnt((x-1) & ~x);
	return result;
}

/*
 * find_last_zero_bit - find the last zero bit in a 64 bit quantity
 * @x: The value to search
 * undefined for x == 0
 */
static __tbx_inline__ unsigned long
ma_ia64_fls (unsigned long x)
{
	long double d = x;
	long exp;
#ifdef __INTEL_COMPILER
	exp = ia64_getf_exp(d);
#else
	__asm__ ("getf.exp %0=%1" : "=r"(exp) : "f"(d));
#endif
	//	exp = ma_ia64_getf_exp(d);
	return exp - 0xffff;
}

static __tbx_inline__ int
ma_fls (int x)
{
	return ma_ia64_fls((unsigned int) x)+1;
}

/*
 * ffs: find first bit set. This is defined the same way as the libc and compiler builtin
 * ffs routines, therefore differs in spirit from the above ffz (man ffs): it operates on
 * "int" values only and the result value is the bit number + 1.  ffs(0) is defined to
 * return zero.
 */
static __tbx_inline__ unsigned long
ma_hweight64 (unsigned long x)
{
	unsigned long result;
#ifdef __INTEL_COMPILER
	result = ia64_popcnt(x);
#else
	__asm__ ("popcnt %0=%1" : "=r" (result) : "r" (x));
#endif
     //	result = ma_ia64_popcnt(x);
	return result;
}

static __tbx_inline__ int
ma_sched_find_first_bit (unsigned long *b)
{
	if (tbx_unlikely(b[0]))
		return __ma_ffs(b[0]);
#if MA_BITMAP_BITS > 64
	if (tbx_unlikely(b[1]))
		return 64 + __ma_ffs(b[1]);
#endif
#if MA_BITMAP_BITS > 128
	return __ma_ffs(b[2]) + 128;
#endif
	MA_BUG();
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_IA64_LINUX_BITOPS_H__ **/
