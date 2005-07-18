
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
 * include/linux/bitops.h
 */
#depend "asm/linux_types.h[]"

/*
 * ma_ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

#section marcel_functions
static __tbx_inline__ long ma_generic_ffs(long x);
#section marcel_inline
static __tbx_inline__ long ma_generic_ffs(long x)
{
	int r = 1;

	if (!x)
		return 0;
#if MA_BITS_PER_LONG >= 64
	if (!(x & 0xffffffffUL)) {
		x >>= 32;
		r += 32;
	}
#endif
	if (!(x & 0xffffUL)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/*
 * ma_fls: find last bit set.
 */

#section marcel_functions
static __tbx_inline__ unsigned long ma_generic_fls(unsigned long x);
#section marcel_inline
static __tbx_inline__ unsigned long ma_generic_fls(unsigned long x)
{
	int r = MA_BITS_PER_LONG;

	if (!x)
		return 0;
#if MA_BITS_PER_LONG >= 64
	if (!(x & 0xffffffffffff0000UL)) {
		x <<= 32;
		r -= 32;
	}
#endif
	if (!(x & 0xffff0000UL)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000UL)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000UL)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000UL)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000UL)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

#section marcel_functions
static __tbx_inline__ unsigned int ma_generic_hweight32(unsigned int w);
#section marcel_inline
static __tbx_inline__ unsigned int ma_generic_hweight32(unsigned int w)
{
        unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
        res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
        res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
        res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
        return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
}

#section marcel_functions
static __tbx_inline__ unsigned int ma_generic_hweight16(unsigned int w);
#section marcel_inline
static __tbx_inline__ unsigned int ma_generic_hweight16(unsigned int w)
{
        unsigned int res = (w & 0x5555) + ((w >> 1) & 0x5555);
        res = (res & 0x3333) + ((res >> 2) & 0x3333);
        res = (res & 0x0F0F) + ((res >> 4) & 0x0F0F);
        return (res & 0x00FF) + ((res >> 8) & 0x00FF);
}

#section marcel_functions
static __tbx_inline__ unsigned int ma_generic_hweight8(unsigned int w);
#section marcel_inline
static __tbx_inline__ unsigned int ma_generic_hweight8(unsigned int w)
{
        unsigned int res = (w & 0x55) + ((w >> 1) & 0x55);
        res = (res & 0x33) + ((res >> 2) & 0x33);
        return (res & 0x0F) + ((res >> 4) & 0x0F);
}

#section marcel_functions
static __tbx_inline__ unsigned long ma_generic_hweight64(unsigned long long w);
#section marcel_inline
static __tbx_inline__ unsigned long ma_generic_hweight64(unsigned long long w)
{
#if MA_BITS_PER_LONG < 64
	return ma_generic_hweight32((unsigned int)(w >> 32)) +
				ma_generic_hweight32((unsigned int)w);
#else
	ma_u64 res;
	res = (w & 0x5555555555555555ul) + ((w >> 1) & 0x5555555555555555ul);
	res = (res & 0x3333333333333333ul) + ((res >> 2) & 0x3333333333333333ul);
	res = (res & 0x0F0F0F0F0F0F0F0Ful) + ((res >> 4) & 0x0F0F0F0F0F0F0F0Ful);
	res = (res & 0x00FF00FF00FF00FFul) + ((res >> 8) & 0x00FF00FF00FF00FFul);
	res = (res & 0x0000FFFF0000FFFFul) + ((res >> 16) & 0x0000FFFF0000FFFFul);
	return (res & 0x00000000FFFFFFFFul) + ((res >> 32) & 0x00000000FFFFFFFFul);
#endif
}

#section marcel_functions
static __tbx_inline__ unsigned long ma_hweight_long(unsigned long w);
#section marcel_inline
static __tbx_inline__ unsigned long ma_hweight_long(unsigned long w)
{
	return sizeof(w) == 4 ? ma_generic_hweight32(w) : ma_generic_hweight64(w);
}

