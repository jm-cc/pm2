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
 * include/asm-ia64/atomic.h
 */

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 *
 * NOTE: don't mess with the types below!  The "unsigned long" and
 * "int" types were carefully placed so as to ensure proper operation
 * of the macros.
 *
 * Copyright (C) 1998, 1999, 2002-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 */

//#include <linux/types.h>


//#include <asm/intrinsics.h>


#section common

/*
 * On IA-64, counter must always be volatile to ensure that that the
 * memory accesses are ordered.
 */



#section marcel_types
#depend "linux_types.h[]"

typedef struct { volatile __ma_s32 counter; } ma_atomic_t;
typedef struct { volatile __ma_s64 counter; } ma_atomic64_t;

#section marcel_macros
#define MA_ATOMIC_INIT(i)	((ma_atomic_t) { (i) })
#define MA_ATOMIC64_INIT(i)	((ma_atomic64_t) { (i) })

#define ma_atomic_read(v)	((v)->counter)
#define ma_atomic64_read(v)	((v)->counter)

#define ma_atomic_set(v,i)	(((v)->counter) = (i))
#define ma_atomic64_set(v,i)	(((v)->counter) = (i))

#section marcel_functions
static __inline__ int
ma_ia64_atomic_add (int i, ma_atomic_t *v);

#section marcel_inline
static __inline__ int
ma_ia64_atomic_add (int i, ma_atomic_t *v)
{
	__ma_s32 old, new;
	MA_CMPXCHG_BUGCHECK_DECL

	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		new = old + i;
	} while (ma_ia64_cmpxchg("acq", v, old, new, sizeof(ma_atomic_t)) != old);
	return new;
}

#section marcel_functions
static __inline__ int
ma_ia64_atomic64_add (__ma_s64 i, ma_atomic64_t *v);
#section marcel_inline
static __inline__ int
ma_ia64_atomic64_add (__ma_s64 i, ma_atomic64_t *v)
{
	__ma_s64 old, new;
	MA_CMPXCHG_BUGCHECK_DECL

	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		new = old + i;
	} while (ma_ia64_cmpxchg("acq", v, old, new, sizeof(ma_atomic_t)) != old);
	return new;
}

#section marcel_functions
static __inline__ int
ma_ia64_atomic_sub (int i, ma_atomic_t *v);
#section marcel_inline
static __inline__ int
ma_ia64_atomic_sub (int i, ma_atomic_t *v)
{
	__ma_s32 old, new;
	MA_CMPXCHG_BUGCHECK_DECL

	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		new = old - i;
	} while (ma_ia64_cmpxchg("acq", v, old, new, sizeof(ma_atomic_t)) != old);
	return new;
}

#section marcel_functions
static __inline__ int
ma_ia64_atomic64_sub (__ma_s64 i, ma_atomic64_t *v);
#section marcel_inline
static __inline__ int
ma_ia64_atomic64_sub (__ma_s64 i, ma_atomic64_t *v)
{
	__ma_s64 old, new;
	MA_CMPXCHG_BUGCHECK_DECL

	do {
		MA_CMPXCHG_BUGCHECK(v);
		old = ma_atomic_read(v);
		new = old - i;
	} while (ma_ia64_cmpxchg("acq", v, old, new, sizeof(ma_atomic_t)) != old);
	return new;
}

#section marcel_macros
#define ma_atomic_add_return(i,v)						\
({									\
	int __ia64_aar_i = (i);						\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_aar_i ==  1) || (__ia64_aar_i ==   4)		\
	     || (__ia64_aar_i ==  8) || (__ia64_aar_i ==  16)		\
	     || (__ia64_aar_i == -1) || (__ia64_aar_i ==  -4)		\
	     || (__ia64_aar_i == -8) || (__ia64_aar_i == -16)))		\
		? ma_ia64_fetch_and_add(__ia64_aar_i, &(v)->counter)	\
		: ma_ia64_atomic_add(__ia64_aar_i, v);			\
})

#define ma_atomic64_add_return(i,v)					\
({									\
	long __ia64_aar_i = (i);					\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_aar_i ==  1) || (__ia64_aar_i ==   4)		\
	     || (__ia64_aar_i ==  8) || (__ia64_aar_i ==  16)		\
	     || (__ia64_aar_i == -1) || (__ia64_aar_i ==  -4)		\
	     || (__ia64_aar_i == -8) || (__ia64_aar_i == -16)))		\
		? ma_ia64_fetch_and_add(__ia64_aar_i, &(v)->counter)	\
		: ma_ia64_atomic64_add(__ia64_aar_i, v);			\
})

#section marcel_functions
static __inline__ int
ma_atomic_add_negative (int i, ma_atomic_t *v);
#section marcel_inline
/*
 * Atomically add I to V and return TRUE if the resulting value is
 * negative.
 */
static __inline__ int
ma_atomic_add_negative (int i, ma_atomic_t *v)
{
	return ma_atomic_add_return(i, v) < 0;
}

#section marcel_functions
static __inline__ int
ma_atomic64_add_negative (__ma_s64 i, ma_atomic64_t *v);
#section marcel_inline
static __inline__ int
ma_atomic64_add_negative (__ma_s64 i, ma_atomic64_t *v)
{
	return ma_atomic64_add_return(i, v) < 0;
}

#section marcel_macros
#define ma_atomic_sub_return(i,v)						\
({									\
	int __ia64_asr_i = (i);						\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_asr_i ==   1) || (__ia64_asr_i ==   4)		\
	     || (__ia64_asr_i ==   8) || (__ia64_asr_i ==  16)		\
	     || (__ia64_asr_i ==  -1) || (__ia64_asr_i ==  -4)		\
	     || (__ia64_asr_i ==  -8) || (__ia64_asr_i == -16)))	\
		? ma_ia64_fetch_and_add(-__ia64_asr_i, &(v)->counter)	\
		: ma_ia64_atomic_sub(__ia64_asr_i, v);			\
})

#define ma_atomic64_sub_return(i,v)					\
({									\
	long __ia64_asr_i = (i);					\
	(__builtin_constant_p(i)					\
	 && (   (__ia64_asr_i ==   1) || (__ia64_asr_i ==   4)		\
	     || (__ia64_asr_i ==   8) || (__ia64_asr_i ==  16)		\
	     || (__ia64_asr_i ==  -1) || (__ia64_asr_i ==  -4)		\
	     || (__ia64_asr_i ==  -8) || (__ia64_asr_i == -16)))	\
		? ma_ia64_fetch_and_add(-__ia64_asr_i, &(v)->counter)	\
		: ma_ia64_atomic64_sub(__ia64_asr_i, v);			\
})

#define ma_atomic_dec_return(v)		ma_atomic_sub_return(1, (v))
#define ma_atomic_inc_return(v)		ma_atomic_add_return(1, (v))
#define ma_atomic64_dec_return(v)	ma_atomic64_sub_return(1, (v))
#define ma_atomic64_inc_return(v)	ma_atomic64_add_return(1, (v))

#define ma_atomic_sub_and_test(i,v)	(ma_atomic_sub_return((i), (v)) == 0)
#define ma_atomic_dec_and_test(v)	(ma_atomic_sub_return(1, (v)) == 0)
#define ma_atomic_inc_and_test(v)	(ma_atomic_add_return(1, (v)) != 0)
#define ma_atomic64_sub_and_test(i,v)	(ma_atomic64_sub_return((i), (v)) == 0)
#define ma_atomic64_dec_and_test(v)	(ma_atomic64_sub_return(1, (v)) == 0)
#define ma_atomic64_inc_and_test(v)	(ma_atomic64_add_return(1, (v)) != 0)

#define ma_atomic_add(i,v)		ma_atomic_add_return((i), (v))
#define ma_atomic_sub(i,v)		ma_atomic_sub_return((i), (v))
#define ma_atomic_inc(v)		ma_atomic_add(1, (v))
#define ma_atomic_dec(v)		ma_atomic_sub(1, (v))

#define ma_atomic64_add(i,v)		ma_atomic64_add_return((i), (v))
#define ma_atomic64_sub(i,v)		ma_atomic64_sub_return((i), (v))
#define ma_atomic64_inc(v)		ma_atomic64_add(1, (v))
#define ma_atomic64_dec(v)		ma_atomic64_sub(1, (v))

/* Atomic operations are already serializing */
#define ma_smp_mb__before_atomic_dec()	ma_barrier()
#define ma_smp_mb__after_atomic_dec()	ma_barrier()
#define ma_smp_mb__before_atomic_inc()	ma_barrier()
#define ma_smp_mb__after_atomic_inc()	ma_barrier()

