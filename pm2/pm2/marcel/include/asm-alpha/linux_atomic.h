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
 * include/asm-alpha/atomic.h
 */

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc...
 *
 * But use these as seldom as possible since they are much slower
 * than regular operations.
 */

/*
 * Counter is volatile to make sure gcc doesn't try to be clever
 * and move things around on us. We need to use _exactly_ the address
 * the user gave us, not some alias that contains the same information.
 */
#section marcel_types
typedef struct { volatile int counter; } ma_atomic_t;
typedef struct { volatile long counter; } ma_atomic64_t;

#section marcel_macros
#define MA_ATOMIC_INIT(i)	( (ma_atomic_t) { (i) } )
#define MA_ATOMIC64_INIT(i)	( (ma_atomic64_t) { (i) } )

#define ma_atomic_read(v)	((v)->counter + 0)
#define ma_atomic64_read(v)	((v)->counter + 0)

#define ma_atomic_set(v,i)	((v)->counter = (i))
#define ma_atomic64_set(v,i)	((v)->counter = (i))

/*
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 */

#section marcel_functions
static __inline__ void ma_atomic_add(int i, ma_atomic_t * v);
#section marcel_inline
static __inline__ void ma_atomic_add(int i, ma_atomic_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	addl %0,%2,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}

#section marcel_functions
static __inline__ void ma_atomic64_add(long i, ma_atomic64_t * v);
#section marcel_inline
static __inline__ void ma_atomic64_add(long i, ma_atomic64_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldq_l %0,%1\n"
	"	addq %0,%2,%0\n"
	"	stq_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}

#section marcel_functions
static __inline__ void ma_atomic_sub(int i, ma_atomic_t * v);
#section marcel_inline
static __inline__ void ma_atomic_sub(int i, ma_atomic_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	subl %0,%2,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}

#section marcel_functions
static __inline__ void ma_atomic64_sub(long i, ma_atomic64_t * v);
#section marcel_inline
static __inline__ void ma_atomic64_sub(long i, ma_atomic64_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldq_l %0,%1\n"
	"	subq %0,%2,%0\n"
	"	stq_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}


/*
 * Same as above, but return the result value
 */
#section marcel_functions
static __inline__ long ma_atomic_add_return(int i, ma_atomic_t * v);
#section marcel_inline
static __inline__ long ma_atomic_add_return(int i, ma_atomic_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	addl %0,%3,%2\n"
	"	addl %0,%3,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

#section marcel_functions
static __inline__ long ma_atomic64_add_return(long i, ma_atomic64_t * v);
#section marcel_inline
static __inline__ long ma_atomic64_add_return(long i, ma_atomic64_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldq_l %0,%1\n"
	"	addq %0,%3,%2\n"
	"	addq %0,%3,%0\n"
	"	stq_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

#section marcel_functions
static __inline__ long ma_atomic_sub_return(int i, ma_atomic_t * v);
#section marcel_inline
static __inline__ long ma_atomic_sub_return(int i, ma_atomic_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	subl %0,%3,%2\n"
	"	subl %0,%3,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

#section marcel_functions
static __inline__ long ma_atomic64_sub_return(long i, ma_atomic64_t * v);
#section marcel_inline
static __inline__ long ma_atomic64_sub_return(long i, ma_atomic64_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldq_l %0,%1\n"
	"	subq %0,%3,%2\n"
	"	subq %0,%3,%0\n"
	"	stq_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

#section marcel_macros
#define ma_atomic_dec_return(v) ma_atomic_sub_return(1,(v))
#define ma_atomic64_dec_return(v) ma_atomic64_sub_return(1,(v))

#define ma_atomic_inc_return(v) ma_atomic_add_return(1,(v))
#define ma_atomic64_inc_return(v) ma_atomic64_add_return(1,(v))

#define ma_atomic_sub_and_test(i,v) (ma_atomic_sub_return((i), (v)) == 0)
#define ma_atomic64_sub_and_test(i,v) (ma_atomic64_sub_return((i), (v)) == 0)

#define ma_atomic_dec_and_test(v) (ma_atomic_sub_return(1, (v)) == 0)
#define ma_atomic64_dec_and_test(v) (ma_atomic64_sub_return(1, (v)) == 0)

#define ma_atomic_inc(v) ma_atomic_add(1,(v))
#define ma_atomic64_inc(v) ma_atomic64_add(1,(v))

#define ma_atomic_dec(v) ma_atomic_sub(1,(v))
#define ma_atomic64_dec(v) ma_atomic64_sub(1,(v))

#define ma_smp_mb__before_atomic_dec()	ma_smp_mb()
#define ma_smp_mb__after_atomic_dec()	ma_smp_mb()
#define ma_smp_mb__before_atomic_inc()	ma_smp_mb()
#define ma_smp_mb__after_atomic_inc()	ma_smp_mb()
