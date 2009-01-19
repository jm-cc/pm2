/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#section common
#include "tbx_compiler.h"
/*
 * Similar to:
 * include/asm-ia64/bitops.h
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * 02/06/02 find_next_bit() and find_first_bit() added from Erich Focht's ia64 O(1)
 *	    scheduler patch
 */

//#include <linux/compiler.h>
//#include <linux/types.h>

#depend "asm/linux_intrinsics.h[marcel_functions]"
#depend "asm/linux_intrinsics.h[marcel_macros]"


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
#section marcel_functions
static __tbx_inline__ void
ma_set_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ void
__ma_set_bit (int nr, volatile void *addr);
#section marcel_inline
static __tbx_inline__ void
__ma_set_bit (int nr, volatile void *addr)
{
	*((volatile __ma_u32 *) addr + (nr >> 5)) |= (1 << (nr & 31));
}

/*
 * clear_bit() has "acquire" semantics.
 */
#section marcel_macros
#define ma_smp_mb__before_clear_bit()	ma_smp_mb()
#define ma_smp_mb__after_clear_bit()	do { /* skip */; } while (0)

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
#section marcel_functions
static __tbx_inline__ void
ma_clear_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ void
ma_change_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ void
__ma_change_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
ma_test_and_set_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
__ma_test_and_set_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
ma_test_and_clear_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
__ma_test_and_clear_bit(int nr, volatile void * addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
ma_test_and_change_bit (int nr, volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ int
__ma_test_and_change_bit (int nr, volatile void *addr);
#section marcel_inline
static __tbx_inline__ int
__ma_test_and_change_bit (int nr, volatile void *addr)
{
	__ma_u32 old, bit = (1 << (nr & 31));
	__ma_u32 *m = (__ma_u32 *) addr + (nr >> 5);

	old = *m;
	*m = old ^ bit;
	return (old & bit) != 0;
}

#section marcel_functions
static __tbx_inline__ int
ma_test_bit (int nr, const volatile void *addr);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ unsigned long
ma_ffz (unsigned long x);
#section marcel_inline
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
#section marcel_functions
static __tbx_inline__ unsigned long
__ma_ffs (unsigned long x);
#section marcel_inline
static __tbx_inline__ unsigned long
__ma_ffs (unsigned long x)
{
	unsigned long result;
	result = ma_ia64_popcnt((x-1) & ~x);
	return result;
}

//#ifdef __KERNEL__

/*
 * find_last_zero_bit - find the last zero bit in a 64 bit quantity
 * @x: The value to search
 * undefined for x == 0
 */
#section marcel_functions
static __tbx_inline__ unsigned long
ma_ia64_fls (unsigned long x);
#section marcel_inline
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

#section marcel_functions
static __tbx_inline__ int
ma_fls (int x);
#section marcel_inline
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
#section marcel_macros
#define ma_ffs(x)	ma_generic_ffs(x)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */
#section marcel_functions
static __tbx_inline__ unsigned long
ma_hweight64 (unsigned long x);
#section marcel_inline
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

#section marcel_macros
#define ma_hweight32(x) ma_hweight64 ((x) & 0xfffffffful)
#define ma_hweight16(x) ma_hweight64 ((x) & 0xfffful)
#define ma_hweight8(x)  ma_hweight64 ((x) & 0xfful)

//#endif /* __KERNEL__ */


#section marcel_functions
extern int __ma_find_next_zero_bit (void *addr, unsigned long size,
				    unsigned long offset);
extern int __ma_find_next_bit(const void *addr, unsigned long size,
			      unsigned long offset);

#section marcel_macros
#define ma_find_next_zero_bit(addr, size, offset) \
			__ma_find_next_zero_bit((addr), (size), (offset))
#define ma_find_next_bit(addr, size, offset) \
			__ma_find_next_bit((addr), (size), (offset))
/*
 * The optimizer actually does good code for this case..
 */
#section marcel_macros
#define ma_find_first_zero_bit(addr, size) ma_find_next_zero_bit((addr), (size), 0)

#define ma_find_first_bit(addr, size) ma_find_next_bit((addr), (size), 0)

//#ifdef __KERNEL__

#define __ma_clear_bit(nr, addr)	ma_clear_bit(nr, addr)

#section marcel_functions
static __tbx_inline__ int
ma_sched_find_first_bit (unsigned long *b);
#section marcel_inline
static __tbx_inline__ int
ma_sched_find_first_bit (unsigned long *b)
{
	if (tbx_unlikely(b[0]))
		return __ma_ffs(b[0]);
	if (tbx_unlikely(b[1]))
		return 64 + __ma_ffs(b[1]);
	return __ma_ffs(b[2]) + 128;
}

//#endif /* __KERNEL__ */

