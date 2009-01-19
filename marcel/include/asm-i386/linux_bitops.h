
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
 * include/asm-i386/bitops.h
 * Copyright 1992, Linus Torvalds.
 */

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

#section marcel_functions
/**
 * ma_set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __ma_set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __tbx_inline__ void ma_set_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ void ma_set_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__( MA_LOCK_PREFIX
		"btsl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}


#section marcel_functions
/**
 * __ma_set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike ma_set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static __tbx_inline__ void __ma_set_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ void __ma_set_bit(int nr, volatile unsigned long * addr)
{
	__asm__(
		"btsl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

#section marcel_functions
/**
 * ma_clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * ma_clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call ma_smp_mb__before_clear_bit() and/or ma_smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __tbx_inline__ void ma_clear_bit(int nr, volatile unsigned long * addr);
static __tbx_inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
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
#section marcel_macros
#define ma_smp_mb__before_clear_bit()	ma_barrier()
#define ma_smp_mb__after_clear_bit()	ma_barrier()

#section marcel_functions
/**
 * __ma_change_bit - Toggle a bit in memory
 * @nr: the bit to change
 * @addr: the address to start counting from
 *
 * Unlike ma_change_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static __tbx_inline__ void __ma_change_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ void __ma_change_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__(
		"btcl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

#section marcel_functions
/**
 * ma_change_bit - Toggle a bit in memory
 * @nr: Bit to change
 * @addr: Address to start counting from
 *
 * ma_change_bit() is atomic and may not be reordered.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __tbx_inline__ void ma_change_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ void ma_change_bit(int nr, volatile unsigned long * addr)
{
	__asm__ __volatile__( MA_LOCK_PREFIX
		"btcl %1,%0"
		:"=m" (*addr)
		:"dIr" (nr));
}

#section marcel_functions
/**
 * ma_test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int ma_test_and_set_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ int ma_test_and_set_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

#section marcel_functions
/**
 * __ma_test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.  
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static __tbx_inline__ int __ma_test_and_set_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ int __ma_test_and_set_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__(
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr));
	return oldbit;
}

#section marcel_functions
/**
 * ma_test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int ma_test_and_clear_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __tbx_inline__ int ma_test_and_clear_bit(int nr, volatile unsigned long * addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

#section marcel_functions
/**
 * __ma_test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.  
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static __tbx_inline__ int __ma_test_and_clear_bit(int nr, volatile unsigned long *addr);
static __tbx_inline__ int __ma_test_and_change_bit(int nr, volatile unsigned long *addr);
#section marcel_inline
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

#section marcel_functions
/**
 * ma_test_and_change_bit - Change a bit and return its new value
 * @nr: Bit to change
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __tbx_inline__ int ma_test_and_change_bit(int nr, volatile unsigned long* addr);
#section marcel_inline
static __tbx_inline__ int ma_test_and_change_bit(int nr, volatile unsigned long* addr)
{
	int oldbit;

	__asm__ __volatile__( MA_LOCK_PREFIX
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (*addr)
		:"dIr" (nr) : "memory");
	return oldbit;
}

#section marcel_functions
#if 0 /* Fool kernel-doc since it doesn't do macros yet */
/**
 * ma_test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static int ma_test_bit(int nr, const volatile void * addr);
#endif

static __tbx_inline__ int ma_constant_test_bit(int nr, const volatile unsigned long *addr);
static __tbx_inline__ int ma_variable_test_bit(int nr, const volatile unsigned long * addr);
#section marcel_inline
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

#section marcel_macros
#define ma_test_bit(nr,addr) \
(__builtin_constant_p(nr) ? \
 ma_constant_test_bit((nr),(addr)) : \
 ma_variable_test_bit((nr),(addr)))

#section marcel_functions
/**
 * ma_find_first_zero_bit - find the first zero bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit-number of the first zero bit, not the number of the byte
 * containing a bit.
 */
#if 0
static __tbx_inline__ int ma_find_first_zero_bit(const unsigned long *addr, unsigned size);
#endif
#section marcel_inline
#if 0
static __tbx_inline__ int ma_find_first_zero_bit(const unsigned long *addr, unsigned size)
{
	int d0, d1, d2;
	int res;

	if (!size)
		return 0;
	/* This looks at memory. Mark it volatile to tell gcc not to move it around */
	__asm__ __volatile__(
		"movl $-1,%%eax\n\t"
		"xorl %%edx,%%edx\n\t"
		"repe; scasl\n\t"
		"je 1f\n\t"
		"xorl -4(%%edi),%%eax\n\t"
		"subl $4,%%edi\n\t"
		"bsfl %%eax,%%edx\n"
		"1:\tsubl %%ebx,%%edi\n\t"
		"shll $3,%%edi\n\t"
		"addl %%edi,%%edx"
		:"=d" (res), "=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr));
	return res;
}
#endif

#section marcel_functions
/**
 * ma_find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit-number of the first set bit, not the number of the byte
 * containing a bit.
 */
#if 0
static __tbx_inline__ int ma_find_first_bit(const unsigned long *addr, unsigned size);
#endif
#section marcel_inline
#if 0
static __tbx_inline__ int ma_find_first_bit(const unsigned long *addr, unsigned size)
{
	int d0, d1;
	int res;

	/* This looks at memory. Mark it volatile to tell gcc not to move it around */
	__asm__ __volatile__(
		"xorl %%eax,%%eax\n\t"
		"repe; scasl\n\t"
		"jz 1f\n\t"
		"leal -4(%%edi),%%edi\n\t"
		"bsfl (%%edi),%%eax\n"
		"1:\tsubl %%ebx,%%edi\n\t"
		"shll $3,%%edi\n\t"
		"addl %%edi,%%eax"
		:"=a" (res), "=&c" (d0), "=&D" (d1)
		:"1" ((size + 31) >> 5), "2" (addr), "b" (addr));
	return res;
}
#endif

#section marcel_functions
/**
 * ma_find_next_zero_bit - find the first zero bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
#if 0
static __tbx_inline__ int ma_find_next_zero_bit(const unsigned long *addr, int size, int offset);
#endif
#section marcel_inline
#if 0
static __tbx_inline__ int ma_find_next_zero_bit(const unsigned long *addr, int size, int offset)
{
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;
	
	if (bit) {
		/*
		 * Look for zero in the first 32 bits.
		 */
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (~(*p >> bit)));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = ma_find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}
#endif

#section marcel_functions
/**
 * ma_find_next_bit - find the first set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
#if 0
static __tbx_inline__ int ma_find_next_bit(const unsigned long *addr, int size, int offset);
#endif
#section marcel_inline
#if 0
static __tbx_inline__ int ma_find_next_bit(const unsigned long *addr, int size, int offset)
{
	const unsigned long *p = addr + (offset >> 5);
	int set = 0, bit = offset & 31, res;

	if (bit) {
		/*
		 * Look for nonzero in the first 32 bits:
		 */
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (*p >> bit));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No set bit yet, search remaining full words for a bit
	 */
	res = ma_find_first_bit (p, size - 32 * (p - addr));
	return (offset + set + res);
}
#endif

#section marcel_functions
/**
 * ma_ffz - find first zero in word.
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
static __tbx_inline__ unsigned long ma_ffz(unsigned long word);
#section marcel_inline
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

#section marcel_functions
/**
 * __ma_ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __tbx_inline__ unsigned long __ma_ffs(unsigned long word);
#section marcel_inline
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

#section marcel_macros
/*
 * fls: find last bit set.
 */

#define ma_fls(x) ma_generic_fls(x)

#section marcel_functions
/*
 * Every architecture must define this function. It's the fastest
 * way of searching a 140-bit bitmap where the first 100 bits are
 * unlikely to be set. It's guaranteed that at least one of the 140
 * bits is cleared.
 */
static __tbx_inline__ int ma_sched_find_first_bit(const unsigned long *b);
#section marcel_inline
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

#section marcel_functions
/**
 * ma_ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ma_ffz (man ffs).
 */
static __tbx_inline__ unsigned long ma_ffs(unsigned long x);
#section marcel_inline
static __tbx_inline__ unsigned long ma_ffs(unsigned long x)
{
	unsigned long r;

	__asm__("bsfq %1,%0\n\t"
		"jnz 1f\n\t"
		"movq $-1,%0\n"
		"1:" : "=r" (r) : "rm" (x));
	return r+1;
}

#section marcel_macros
/**
 * ma_hweightN - returns the hamming weight of a N-bit word
 * @x: the word to weigh
 *
 * The Hamming Weight of a number is the total number of bits set in it.
 */

#define ma_hweight64(x) ma_generic_hweight64(x)
#define ma_hweight32(x) ma_generic_hweight32(x)
#define ma_hweight16(x) ma_generic_hweight16(x)
#define ma_hweight8(x) ma_generic_hweight8(x)

