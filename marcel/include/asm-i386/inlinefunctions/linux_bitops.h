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


#ifndef __ASM_I386_INLINEFUNCTIONS_LINUX_BITOPS_H__
#define __ASM_I386_INLINEFUNCTIONS_LINUX_BITOPS_H__


#include "asm/linux_bitops.h"
#include "asm/linux_atomic.h"
#include "tbx_compiler.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */
static __tbx_inline__ void ma_set_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__( MA_LOCK_PREFIX
		"btsl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}


static __tbx_inline__ void __ma_set_bit(int nr, volatile unsigned long * addr)
{
	__asm__(
		"btsl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

static __tbx_inline__ void ma_clear_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__( MA_LOCK_PREFIX
		"btrl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

static __tbx_inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__(
		"btrl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}
static __tbx_inline__ void __ma_change_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__(
		"btcl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

static __tbx_inline__ void ma_change_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__( MA_LOCK_PREFIX
		"btcl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

static __tbx_inline__ int ma_test_and_set_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

static __tbx_inline__ int __ma_test_and_set_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__(
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr));
	return oldbit;
}

static __tbx_inline__ int ma_test_and_clear_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

static __tbx_inline__ int __ma_test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	int oldbit;

	__asm__(
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr));
	return oldbit;
}

/* WARNING: non atomic and it can be reordered! */
static __tbx_inline__ int __ma_test_and_change_bit(int nr, volatile unsigned long *addr)
{
	int oldbit;

	__asm__ __volatile__(
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

static __tbx_inline__ int ma_test_and_change_bit(int nr, volatile unsigned long* addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

static __tbx_inline__ int ma_constant_test_bit(int nr, const volatile unsigned long *addr)
{
	return ((1UL << (nr & 31)) & (addr[nr >> 5])) != 0;
}

static __tbx_inline__ int ma_variable_test_bit(int nr, const volatile unsigned long * addr)
{
	int oldbit;

	__asm__ __volatile__(
		"btl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit)
		:"m" (*addr),"dIr" (nr));
	return oldbit;
}

/* #if 0 */
/* static __tbx_inline__ int ma_find_first_zero_bit(const unsigned long *addr, unsigned size) */
/* { */
/* 	int d0, d1, d2; */
/* 	int res; */

/* 	if (!size) */
/* 		return 0; */
/* 	/\* This looks at memory. Mark it volatile to tell gcc not to move it around *\/ */
/* 	__asm__ __volatile__( */
/* 		"movl $-1,%%eax\n\t" */
/* 		"xorl %%edx,%%edx\n\t" */
/* 		"repe; scasl\n\t" */
/* 		"je 1f\n\t" */
/* 		"xorl -4(%%edi),%%eax\n\t" */
/* 		"subl $4,%%edi\n\t" */
/* 		"bsfl %%eax,%%edx\n" */
/* 		"1:\tsubl %%ebx,%%edi\n\t" */
/* 		"shll $3,%%edi\n\t" */
/* 		"addl %%edi,%%edx" */
/* 		:"=d" (res), "=&c" (d0), "=&D" (d1), "=&a" (d2) */
/* 		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr)); */
/* 	return res; */
/* } */
/* #endif */

/* #if 0 */
/* static __tbx_inline__ int ma_find_first_bit(const unsigned long *addr, unsigned size) */
/* { */
/* 	int d0, d1; */
/* 	int res; */

/* 	/\* This looks at memory. Mark it volatile to tell gcc not to move it around *\/ */
/* 	__asm__ __volatile__( */
/* 		"xorl %%eax,%%eax\n\t" */
/* 		"repe; scasl\n\t" */
/* 		"jz 1f\n\t" */
/* 		"leal -4(%%edi),%%edi\n\t" */
/* 		"bsfl (%%edi),%%eax\n" */
/* 		"1:\tsubl %%ebx,%%edi\n\t" */
/* 		"shll $3,%%edi\n\t" */
/* 		"addl %%edi,%%eax" */
/* 		:"=a" (res), "=&c" (d0), "=&D" (d1) */
/* 		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr)); */
/* 	return res; */
/* } */
/* #endif */

/* #if 0 */
/* static __tbx_inline__ int ma_find_next_zero_bit(const unsigned long *addr, int size, int offset) */
/* { */
/* 	unsigned long * p = ((unsigned long *) addr) + (offset >> 5); */
/* 	int set = 0, bit = offset & 31, res; */
	
/* 	if (bit) { */
/* 		/\* */
/* 		 * Look for zero in the first 32 bits. */
/* 		 *\/ */
/* 		__asm__("bsfl %1,%0\n\t" */
/* 			"jne 1f\n\t" */
/* 			"movl $32, %0\n" */
/* 			"1:" */
/* 			: "=r" (set) */
/* 			: "r" (~(*p >> bit))); */
/* 		if (set < (32 - bit)) */
/* 			return set + offset; */
/* 		set = 32 - bit; */
/* 		p++; */
/* 	} */
/* 	/\* */
/* 	 * No zero yet, search remaining full bytes for a zero */
/* 	 *\/ */
/* 	res = ma_find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr)); */
/* 	return (offset + set + res); */
/* } */
/* #endif */

/* #if 0 */
/* static __tbx_inline__ int ma_find_next_bit(const unsigned long *addr, int size, int offset) */
/* { */
/* 	const unsigned long *p = addr + (offset >> 5); */
/* 	int set = 0, bit = offset & 31, res; */

/* 	if (bit) { */
/* 		/\* */
/* 		 * Look for nonzero in the first 32 bits: */
/* 		 *\/ */
/* 		__asm__("bsfl %1,%0\n\t" */
/* 			"jne 1f\n\t" */
/* 			"movl $32, %0\n" */
/* 			"1:" */
/* 			: "=r" (set) */
/* 			: "r" (*p >> bit)); */
/* 		if (set < (32 - bit)) */
/* 			return set + offset; */
/* 		set = 32 - bit; */
/* 		p++; */
/* 	} */
/* 	/\* */
/* 	 * No set bit yet, search remaining full words for a bit */
/* 	 *\/ */
/* 	res = ma_find_first_bit (p, size - 32 * (p - addr)); */
/* 	return (offset + set + res); */
/* } */
/* #endif */

static __tbx_inline__ unsigned long ma_ffz(unsigned long word)
{
#ifdef X86_64_ARCH
	__asm__("bsfq %1,%0"
#else
	__asm__("bsfl %1,%0"
#endif
		:"=r" (word)
		:"r" (~word));
	return word;
}

static __tbx_inline__ unsigned long __ma_ffs(unsigned long word)
{
#ifdef X86_64_ARCH
	__asm__("bsfq %1,%0"
#else
	__asm__("bsfl %1,%0"
#endif
		:"=r" (word)
		:"rm" (word));
	return word;
}

static __tbx_inline__ int ma_sched_find_first_bit(const unsigned long *b)
{
	if (tbx_unlikely(b[0]))
		return __ma_ffs(b[0]);
#ifdef X86_64_ARCH
	if (tbx_unlikely(b[1]))
		return __ma_ffs(b[1]) + 64;
	return __ma_ffs(b[2]) + 128;
#else
	if (tbx_unlikely(b[1]))
		return __ma_ffs(b[1]) + 32;
	if (tbx_unlikely(b[2]))
		return __ma_ffs(b[2]) + 64;
	if (b[3])
		return __ma_ffs(b[3]) + 96;
	return __ma_ffs(b[4]) + 128;
#endif
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_LINUX_BITOPS_H__ **/
