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


#ifndef __ASM_IA64_LINUX_BITOPS_H__
#define __ASM_IA64_LINUX_BITOPS_H__


/*
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *
 * 02/06/02 find_next_bit() and find_first_bit() added from Erich Focht's ia64 O(1)
 *	    scheduler patch
 */


#include "asm/linux_intrinsics.h"


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
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
#define ma_ffs(x)	ma_generic_ffs(x)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */
#define ma_hweight32(x) ma_hweight64 ((x) & 0xfffffffful)
#define ma_hweight16(x) ma_hweight64 ((x) & 0xfffful)
#define ma_hweight8(x)  ma_hweight64 ((x) & 0xfful)

#define ma_find_next_zero_bit(addr, size, offset) \
			__ma_find_next_zero_bit((addr), (size), (offset))
#define ma_find_next_bit(addr, size, offset) \
			__ma_find_next_bit((addr), (size), (offset))
/*
 * The optimizer actually does good code for this case..
 */
#define ma_find_first_zero_bit(addr, size) ma_find_next_zero_bit((addr), (size), 0)

#define ma_find_first_bit(addr, size) ma_find_next_bit((addr), (size), 0)

#define __ma_clear_bit(nr, addr)	ma_clear_bit(nr, addr)


/** Internal functions **/
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
ma_set_bit (int nr, volatile void *addr);
static __tbx_inline__ void
__ma_set_bit (int nr, volatile void *addr);
static __tbx_inline__ void
ma_clear_bit (int nr, volatile void *addr);
static __tbx_inline__ void
ma_change_bit (int nr, volatile void *addr);
static __tbx_inline__ void
__ma_change_bit (int nr, volatile void *addr);
static __tbx_inline__ int
ma_test_and_set_bit (int nr, volatile void *addr);
static __tbx_inline__ int
__ma_test_and_set_bit (int nr, volatile void *addr);
static __tbx_inline__ int
ma_test_and_clear_bit (int nr, volatile void *addr);
static __tbx_inline__ int
__ma_test_and_clear_bit(int nr, volatile void * addr);
static __tbx_inline__ int
ma_test_and_change_bit (int nr, volatile void *addr);
static __tbx_inline__ int
__ma_test_and_change_bit (int nr, volatile void *addr);
static __tbx_inline__ int
ma_test_bit (int nr, const volatile void *addr);
static __tbx_inline__ unsigned long
ma_ffz (unsigned long x);
static __tbx_inline__ unsigned long
__ma_ffs (unsigned long x);
static __tbx_inline__ unsigned long
ma_ia64_fls (unsigned long x);
static __tbx_inline__ int
ma_fls (int x);
static __tbx_inline__ unsigned long
ma_hweight64 (unsigned long x);
extern int __ma_find_next_zero_bit (void *addr, unsigned long size,
				    unsigned long offset);
extern int __ma_find_next_bit(const void *addr, unsigned long size,
			      unsigned long offset);

static __tbx_inline__ int
ma_sched_find_first_bit (unsigned long *b);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_LINUX_BITOPS_H__ **/
