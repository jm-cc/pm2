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
 * include/asm-ia64/system.h
 *
 * System defines. Note that this is included both from .c and .S
 * files, so it does only defines, not any C code.  This is based
 * on information published in the Processor Abstraction Layer
 * and the System Abstraction Layer manual.
 *
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */

#section marcel_macros

/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   ma_wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   ma_rmb():	Like wmb(), but for reads.
 *   ma_mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "ma_mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */

#define ma_mb()		__asm__ __volatile__("mb": : :"memory")
#define ma_rmb()	ma_mb()
#define ma_wmb()	__asm__ __volatile__("wmb": : :"memory")
#define ma_read_barrier_depends()	ma_mb()

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	barrier()
#endif

/*
 * XXX check on these---I suspect what Linus really wants here is
 * acquire vs release semantics but we can't discuss this stuff with
 * Linus just yet.  Grrr...
 */
#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

#ifndef OSF_SYS
/*
 * Atomic exchange.
 * Since it can be used to implement critical sections
 * it must clobber "memory" (also for interrupts in UP).
 */

static __tbx_inline__ unsigned long
__ma_xchg_u8(volatile char *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(
	"	andnot	%4,7,%3\n"
	"	insbl	%1,%4,%1\n"
	"1:	ldq_l	%2,0(%3)\n"
	"	extbl	%2,%4,%0\n"
	"	mskbl	%2,%4,%2\n"
	"	or	%1,%2,%2\n"
	"	stq_c	%2,0(%3)\n"
	"	beq	%2,2f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static __tbx_inline__ unsigned long
__ma_xchg_u16(volatile short *m, unsigned long val)
{
	unsigned long ret, tmp, addr64;

	__asm__ __volatile__(
	"	andnot	%4,7,%3\n"
	"	inswl	%1,%4,%1\n"
	"1:	ldq_l	%2,0(%3)\n"
	"	extwl	%2,%4,%0\n"
	"	mskwl	%2,%4,%2\n"
	"	or	%1,%2,%2\n"
	"	stq_c	%2,0(%3)\n"
	"	beq	%2,2f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	".subsection 2\n"
	"2:	br	1b\n"
	".previous"
	: "=&r" (ret), "=&r" (val), "=&r" (tmp), "=&r" (addr64)
	: "r" ((long)m), "1" (val) : "memory");

	return ret;
}

static __tbx_inline__ unsigned long
__ma_xchg_u32(volatile int *m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
	"1:	ldl_l %0,%4\n"
	"	bis $31,%3,%1\n"
	"	stl_c %1,%2\n"
	"	beq %1,2f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

static __tbx_inline__ unsigned long
__ma_xchg_u64(volatile long *m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
	"1:	ldq_l %0,%4\n"
	"	bis $31,%3,%1\n"
	"	stq_c %1,%2\n"
	"	beq %1,2f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	: "=&r" (val), "=&r" (dummy), "=m" (*m)
	: "rI" (val), "m" (*m) : "memory");

	return val;
}

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid xchg().  */
extern void __ma_xchg_called_with_bad_pointer(void);

#define __ma_xchg(ptr, x, size) \
({ \
	unsigned long __ma_xchg__res; \
	volatile void *__ma_xchg__ptr = (ptr); \
	switch (size) { \
		case 1: __ma_xchg__res = __ma_xchg_u8(__ma_xchg__ptr, x); break; \
		case 2: __ma_xchg__res = __ma_xchg_u16(__ma_xchg__ptr, x); break; \
		case 4: __ma_xchg__res = __ma_xchg_u32(__ma_xchg__ptr, x); break; \
		case 8: __ma_xchg__res = __ma_xchg_u64(__ma_xchg__ptr, x); break; \
		default: __ma_xchg_called_with_bad_pointer(); __ma_xchg__res = x; \
	} \
	__ma_xchg__res; \
})

#define ma_xchg(ptr,x)							     \
  ({									     \
     __typeof__(*(ptr)) _x_ = (x);					     \
     (__typeof__(*(ptr))) __ma_xchg((ptr), (unsigned long)_x_, sizeof(*(ptr))); \
  })
static __tbx_inline__ unsigned long
__ma_cmpxchg_u8(volatile char *m, long old, long repl)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5,7,%4\n"
	"	insbl	%1,%5,%1\n"
	"1:	ldq_l	%2,0(%4)\n"
	"	extbl	%2,%5,%0\n"
	"	cmpeq	%0,%6,%3\n"
	"	beq	%3,2f\n"
	"	mskbl	%2,%5,%2\n"
	"	or	%1,%2,%2\n"
	"	stq_c	%2,0(%4)\n"
	"	beq	%2,3f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (repl), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (repl) : "memory");

	return prev;
}

static __tbx_inline__ unsigned long
__ma_cmpxchg_u16(volatile short *m, long old, long repl)
{
	unsigned long prev, tmp, cmp, addr64;

	__asm__ __volatile__(
	"	andnot	%5,7,%4\n"
	"	inswl	%1,%5,%1\n"
	"1:	ldq_l	%2,0(%4)\n"
	"	extwl	%2,%5,%0\n"
	"	cmpeq	%0,%6,%3\n"
	"	beq	%3,2f\n"
	"	mskwl	%2,%5,%2\n"
	"	or	%1,%2,%2\n"
	"	stq_c	%2,0(%4)\n"
	"	beq	%2,3f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	"2:\n"
	".subsection 2\n"
	"3:	br	1b\n"
	".previous"
	: "=&r" (prev), "=&r" (repl), "=&r" (tmp), "=&r" (cmp), "=&r" (addr64)
	: "r" ((long)m), "Ir" (old), "1" (repl) : "memory");

	return prev;
}

static __tbx_inline__ unsigned long
__ma_cmpxchg_u32(volatile int *m, int old, int repl)
{
	unsigned long prev, cmp;

	__asm__ __volatile__(
	"1:	ldl_l %0,%5\n"
	"	cmpeq %0,%3,%1\n"
	"	beq %1,2f\n"
	"	mov %4,%1\n"
	"	stl_c %1,%2\n"
	"	beq %1,3f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	"2:\n"
	".subsection 2\n"
	"3:	br 1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m)
	: "r"((long) old), "r"(repl), "m"(*m) : "memory");

	return prev;
}

static __tbx_inline__ unsigned long
__ma_cmpxchg_u64(volatile long *m, unsigned long old, unsigned long repl)
{
	unsigned long prev, cmp;

	__asm__ __volatile__(
	"1:	ldq_l %0,%5\n"
	"	cmpeq %0,%3,%1\n"
	"	beq %1,2f\n"
	"	mov %4,%1\n"
	"	stq_c %1,%2\n"
	"	beq %1,3f\n"
#ifdef MA__LWPS
	"	mb\n"
#endif
	"2:\n"
	".subsection 2\n"
	"3:	br 1b\n"
	".previous"
	: "=&r"(prev), "=&r"(cmp), "=m"(*m)
	: "r"((long) old), "r"(repl), "m"(*m) : "memory");

	return prev;
}

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid cmpxchg().  */
extern void __ma_cmpxchg_called_with_bad_pointer(void);

static __tbx_inline__ unsigned long
__ma_cmpxchg(volatile void *ptr, unsigned long old, unsigned long repl, int size)
{
	switch (size) {
		case 1:
			return __ma_cmpxchg_u8(ptr, old, repl);
		case 2:
			return __ma_cmpxchg_u16(ptr, old, repl);
		case 4:
			return __ma_cmpxchg_u32(ptr, old, repl);
		case 8:
			return __ma_cmpxchg_u64(ptr, old, repl);
	}
	__ma_cmpxchg_called_with_bad_pointer();
	return old;
}

#define ma_cmpxchg(ptr,o,n)						 \
  ({									 \
     __typeof__(*(ptr)) _o_ = (o);					 \
     __typeof__(*(ptr)) _n_ = (n);					 \
     (__typeof__(*(ptr))) __ma_cmpxchg((ptr), (unsigned long)_o_,	 \
				    (unsigned long)_n_, sizeof(*(ptr))); \
  })

#endif

#define ma_cpu_relax() ma_barrier()
