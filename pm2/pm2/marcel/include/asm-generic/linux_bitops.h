
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
/*
 * Similar to:
 * include/asm-generic/bitops.h
 */


/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assembly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * Also note, these routines assume that you have 32 bit longs.
 * You will have to change this if you are trying to port Linux to the
 * Alpha architecture or to a Cray.  :-)
 * 
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */

#define ATOMIC_BITOPT_RETURN(op,retexpr) \
{ \
	int	mask, old, new, ret; \
	\
	addr += nr / MA_BITS_PER_LONG; \
	mask = 1 << (nr & 0x1f); \
	old = *addr; \
	while (1) { \
		new = old op mask; \
		ret = pm2_compareexchange(addr,old,new,sizeof(*addr)); \
		if (tbx_likely(ret == old)) \
			return retexpr; \
		old = ret; \
	} \
}
#define BITOPT_RETURN(op,retexpr) \
{ \
	int	mask, old; \
	\
	addr += nr >> 5; \
	mask = 1 << (nr & 0x1f); \
	old = *addr; \
	*addr = old op mask; \
	return retexpr; \
}

#section marcel_functions
static __inline__ void ma_set_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ void ma_set_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(|,)

#section marcel_functions
static __inline__ void __ma_set_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __inline__ void __ma_set_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(|,)

static __inline__ void ma_clear_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ void ma_clear_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(& ~,)

#section marcel_functions
static __inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __inline__ void __ma_clear_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(& ~,)

static __inline__ void ma_change_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ void ma_change_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(^,)

#section marcel_functions
static __inline__ void __ma_change_bit(int nr, volatile unsigned long * addr);
#section marcel_inline
static __inline__ void __ma_change_bit(int nr, volatile unsigned long * addr)
	BITOPT_RETURN(^,)

static __inline__ int ma_constant_test_bit(int nr, const volatile unsigned long *addr)
{
        return ((1UL << (nr & 31)) & (addr[nr >> 5])) != 0;
}
static __inline__ int ma_variable_test_bit(int nr, const unsigned long * addr)
{
	int	mask;

	addr += nr / MA_BITS_PER_LONG;
	mask = 1 << (nr & 0x1f);
	return ((mask & *addr) != 0);
}
	
#section marcel_macros
//static __inline__ int ma__test_bit(int nr, const unsigned long * addr);
#define ma_test_bit(nr,addr) \
	(__builtin_constant_p(nr) ? \
	 ma_constant_test_bit((nr),(addr)) : \
	 ma_variable_test_bit((nr),(addr)))


#section marcel_functions
static __inline__ int ma_test_and_set_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int ma_test_and_set_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(|,(old & mask) != 0)

#section marcel_functions
static __inline__ int __ma_test_and_set_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int __ma_test_and_set_bit(int nr, unsigned long * addr)
	BITOPT_RETURN(|,(old & mask) != 0)

#section marcel_functions
static __inline__ int ma_test_and_clear_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int ma_test_and_clear_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(& ~,(old & mask) != 0)

#section marcel_functions
static __inline__ int __ma_test_and_clear_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int __ma_test_and_clear_bit(int nr, unsigned long * addr)
	BITOPT_RETURN(& ~,(old & mask) != 0)

#section marcel_functions
static __inline__ int ma_test_and_change_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int ma_test_and_change_bit(int nr, unsigned long * addr)
	ATOMIC_BITOPT_RETURN(^,(old & mask) != 0)

#section marcel_functions
static __inline__ int __ma_test_and_change_bit(int nr, unsigned long * addr);
#section marcel_inline
static __inline__ int __ma_test_and_change_bit(int nr, unsigned long * addr)
	BITOPT_RETURN(^,(old & mask) != 0)

/*
 * fls: find last bit set.
 */

#section common
#define ma_fls(x) ma_generic_fls(x)

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

#define ma_ffs(x) ma_generic_ffs(x)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

#define ma_hweight32(x) ma_generic_hweight32(x)
#define ma_hweight16(x) ma_generic_hweight16(x)
#define ma_hweight8(x) ma_generic_hweight8(x)

#define ma_smp_mb__before_clear_bit()   ma_barrier()
#define ma_smp_mb__after_clear_bit()    ma_barrier()

#section marcel_functions
static inline int ma_sched_find_first_bit(const unsigned long *b);
#section marcel_inline
static inline int ma_sched_find_first_bit(const unsigned long *b)
{
	if (tbx_unlikely(b[0]))
		return ma_ffs(b[0]);
	if (tbx_unlikely(b[1]))
		return ma_ffs(b[1]) + 32;
	if (tbx_unlikely(b[2]))
		return ma_ffs(b[2]) + 64;
	if (b[3])
		return ma_ffs(b[3]) + 96;
	return ma_ffs(b[4]) + 128;
}
