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


#ifndef __ASM_ALPHA_LINUX_BITOPS_H__
#define __ASM_ALPHA_LINUX_BITOPS_H__


/*
 * Similar to:
 * include/asm-alpha/bitops.h
 * Copyright 1994, Linus Torvalds.
 */


#include "tbx_compiler.h"
#ifdef OSF_SYS
#include "asm-generic/linux_bitops.h"
#endif


#ifdef __MARCEL_KERNEL__


#ifndef OSF_SYS


/** Internal macros **/
/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 *
 * bit 0 is the LSB of addr; bit 64 is the LSB of (addr+1).
 */
#define ma_smp_mb__before_clear_bit()	ma_smp_mb()
#define ma_smp_mb__after_clear_bit()	ma_smp_mb()

#if !(defined(__alpha_cix__) && defined(__alpha_fix__))
#define ma_fls	ma_generic_fls
#endif

#if defined(__alpha_cix__) && defined(__alpha_fix__)
#define ma_hweight32(x) ma_hweight64((x) & 0xfffffffful)
#define ma_hweight16(x) ma_hweight64((x) & 0xfffful)
#define ma_hweight8(x)  ma_hweight64((x) & 0xfful)
#else
#define ma_hweight32(x) ma_generic_hweight32(x)
#define ma_hweight16(x) ma_generic_hweight16(x)
#define ma_hweight8(x)  ma_generic_hweight8(x)
#endif

/*
 * Find next zero bit in a bitmap reasonably efficiently..
 */
#define ma_find_first_zero_bit(addr, size) \
	ma_find_next_zero_bit((addr), (size), 0)
#define ma_find_first_bit(addr, size) \
	ma_find_next_bit((addr), (size), 0)


/** Internal functions **/
/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 *
 * bit 0 is the LSB of addr; bit 64 is the LSB of (addr+1).
 */
static __tbx_inline__ void
ma_set_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ void
__ma_set_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ void
ma_clear_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ void
__ma_clear_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ void
ma_change_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ void
__ma_change_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
ma_test_and_set_bit(unsigned long nr, volatile void *addr);
static __tbx_inline__ int
__ma_test_and_set_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
ma_test_and_clear_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
__ma_test_and_clear_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
ma_test_and_change_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
__ma_test_and_change_bit(unsigned long nr, volatile void * addr);
static __tbx_inline__ int
ma_test_bit(int nr, const volatile void * addr);
static __tbx_inline__ unsigned long ma_ffz_b(unsigned long x);
static __tbx_inline__ unsigned long ma_ffz(unsigned long word);
static __tbx_inline__ unsigned long __ma_ffs(unsigned long word);
static __tbx_inline__ unsigned long ma_ffs(unsigned long word);
static __tbx_inline__ unsigned long ma_fls(unsigned long word);
static __tbx_inline__ int ma_floor_log2(unsigned long word);
static __tbx_inline__ int ma_ceil_log2(unsigned int word);
static __tbx_inline__ unsigned long ma_hweight64(unsigned long w);
static __tbx_inline__ unsigned long
ma_find_next_zero_bit(void * addr, unsigned long size, unsigned long offset);
static __tbx_inline__ unsigned long
ma_find_next_bit(void * addr, unsigned long size, unsigned long offset);
static __tbx_inline__ unsigned long
ma_sched_find_first_bit(unsigned long b[3]);


#endif /** ! OSF_SYS **/


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_ALPHA_LINUX_BITOPS_H__ **/
