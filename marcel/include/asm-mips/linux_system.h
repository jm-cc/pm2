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

/* dans le noyau linux, ils disent que cela dépend du processeur: utiliser sync ou wb */
#define __ma_sync() \
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		".set	mips2\n\t"		\
		"sync\n\t"			\
		".set	pop"			\
		: /* no output */		\
		: /* no input */		\
		: "memory")

#define __ma_fast_iob()				\
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		"lw	$0,%0\n\t"		\
		"nop\n\t"			\
		".set	pop"			\
		: /* no output */		\
		: "m" (*(int *)CKSEG1)		\
		: "memory")

#define ma_fast_wmb()	__ma_sync()
#define ma_fast_rmb()	__ma_sync()
#define ma_fast_mb()	__ma_sync()
#define ma_fast_iob()				\
	do {					\
		__ma_sync();			\
		__ma_fast_iob();			\
	} while (0)

#ifdef IRIX_SYS
extern void cacheflush(void);
#define ma_cacheflush cacheflush
#else /* IRIX_SYS */
#error "to write !"
#endif /* IRIX_SYS */

#define ma_mb()		ma_fast_wmb()
#define ma_rmb()	ma_fast_rmb()
#define ma_wmb()	ma_cacheflush()
#define ma_iob()	ma_cacheflush()
#define ma_read_barrier_depends()	do { } while(0)

#ifdef MA__LWPS
# define ma_smp_mb()	ma_mb()
# define ma_smp_rmb()	ma_rmb()
# define ma_smp_wmb()	ma_wmb()
# define ma_smp_read_barrier_depends()	ma_read_barrier_depends()
#else
# define ma_smp_mb()	ma_barrier()
# define ma_smp_rmb()	ma_barrier()
# define ma_smp_wmb()	ma_barrier()
# define ma_smp_read_barrier_depends()	do { } while(0)
#endif

#define ma_set_mb(var, value)	do { (var) = (value); ma_mb(); } while (0)
#define ma_set_wmb(var, value)	do { (var) = (value); ma_wmb(); } while (0)

static __tbx_inline__ unsigned long __ma_xchg_u32(volatile int * m, unsigned int val)
{
	__ma_u32 retval;

	//if (cpu_has_llsc && R10000_LLSC_WAR) {
		unsigned long dummy;

		__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	ll	%0, %3			# xchg_u32	\n"
		"	.set	mips0					\n"
		"	move	%2, %z4					\n"
		"	.set	mips3					\n"
		"	sc	%2, %1					\n"
		"	beqzl	%2, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"	.set	mips0					\n"
		: "=&r" (retval), "=m" (*m), "=&r" (dummy)
		: "R" (*m), "Jr" (val)
		: "memory");
#if 0
	} else if (cpu_has_llsc) {
		unsigned long dummy;

		__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	ll	%0, %3			# xchg_u32	\n"
		"	.set	mips0					\n"
		"	move	%2, %z4					\n"
		"	.set	mips3					\n"
		"	sc	%2, %1					\n"
		"	beqz	%2, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"	.set	mips0					\n"
		: "=&r" (retval), "=m" (*m), "=&r" (dummy)
		: "R" (*m), "Jr" (val)
		: "memory");
	} else {
		unsigned long flags;

		local_irq_save(flags);
		retval = *m;
		*m = val;
		local_irq_restore(flags);	/* implies memory barrier  */
	}
#endif

	return retval;
}

#if MA_BITS_PER_LONG == 64
static __tbx_inline__ unsigned long __ma_xchg_u64(volatile unsigned long * m, unsigned long val)
{
	__ma_u64 retval;

	//if (cpu_has_llsc && R10000_LLSC_WAR) {
		unsigned long dummy;

		__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	lld	%0, %3			# xchg_u64	\n"
		"	move	%2, %z4					\n"
		"	scd	%2, %1					\n"
		"	beqzl	%2, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"	.set	mips0					\n"
		: "=&r" (retval), "=m" (*m), "=&r" (dummy)
		: "R" (*m), "Jr" (val)
		: "memory");
#if 0
	} else if (cpu_has_llsc) {
		unsigned long dummy;

		__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	lld	%0, %3			# xchg_u64	\n"
		"	move	%2, %z4					\n"
		"	scd	%2, %1					\n"
		"	beqz	%2, 1b					\n"
#ifdef CONFIG_SMP
		"	sync						\n"
#endif
		"	.set	mips0					\n"
		: "=&r" (retval), "=m" (*m), "=&r" (dummy)
		: "R" (*m), "Jr" (val)
		: "memory");
	} else {
		unsigned long flags;

		local_irq_save(flags);
		retval = *m;
		*m = val;
		local_irq_restore(flags);	/* implies memory barrier  */
	}
#endif

	return retval;
}
#endif

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid xchg().  */
extern void __ma_xchg_called_with_bad_pointer(void);
extern unsigned long __ma_xchg_u8(volatile unsigned long * m, unsigned long val);
extern unsigned long __ma_xchg_u16(volatile unsigned long * m, unsigned long val);

static __tbx_inline__ unsigned long __ma_xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
	case 1:
		return __ma_xchg_u8(ptr, x);
	case 2:
		return __ma_xchg_u16(ptr, x);
	case 4:
		return __ma_xchg_u32(ptr, x);
#if MA_BITS_PER_LONG == 64
	case 8:
		return __ma_xchg_u64(ptr, x);
#endif
	}
	__ma_xchg_called_with_bad_pointer();
	return x;
}

#define ma_xchg(ptr,x) ((__typeof__(*(ptr)))__ma_xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

static __tbx_inline__ unsigned long TBX_NOINST __ma_cmpxchg_u32(volatile int * m, unsigned long old,
	unsigned long replace)
{
	__ma_u32 retval;

	/* XXX assume this */
	//if (cpu_has_llsc && R10000_LLSC_WAR) {
		__asm__ __volatile__(
		"	.set	push					\n"
		"	.set	noat					\n"
		"	.set	mips3					\n"
		"1:	ll	%0, %2			# __cmpxchg_u32	\n"
		"	bne	%0, %z3, 2f				\n"
		"	.set	mips0					\n"
		"	move	$1, %z4					\n"
		"	.set	mips3					\n"
		"	sc	$1, %1					\n"
		"	beqzl	$1, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"2:							\n"
		"	.set	pop					\n"
		: "=&r" (retval),
#if (__GNUC__ >= 3)
			"=R" (*m)
#else
			"=m" (*m)
#endif
		: "R" (*m), "Jr" (old), "Jr" (replace)
		: "memory");
#if 0
	} else if (cpu_has_llsc) {
		__asm__ __volatile__(
		"	.set	push					\n"
		"	.set	noat					\n"
		"	.set	mips3					\n"
		"1:	ll	%0, %2			# __cmpxchg_u32	\n"
		"	bne	%0, %z3, 2f				\n"
		"	.set	mips0					\n"
		"	move	$1, %z4					\n"
		"	.set	mips3					\n"
		"	sc	$1, %1					\n"
		"	beqz	$1, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"2:							\n"
		"	.set	pop					\n"
		: "=&r" (retval),
#if (__GNUC__ >= 3)
			"=R" (*m)
#else
			"=m" (*m)
#endif
		: "R" (*m), "Jr" (old), "Jr" (replace)
		: "memory");
	} else {
		unsigned long flags;

		local_irq_save(flags);
		retval = *m;
		if (retval == old)
			*m = replace;
		local_irq_restore(flags);	/* implies memory barrier  */
	}
#endif

	return retval;
}

#if MA_BITS_PER_LONG == 64
static __tbx_inline__ unsigned long __ma_cmpxchg_u64(volatile int * m, unsigned long old,
	unsigned long replace)
{
	__ma_u64 retval;

	/* XXX assume this */
//	if (cpu_has_llsc) {
		__asm__ __volatile__(
		"	.set	push					\n"
		"	.set	noat					\n"
		"	.set	mips3					\n"
		"1:	lld	%0, %2			# __cmpxchg_u64	\n"
		"	bne	%0, %z3, 2f				\n"
		"	move	$1, %z4					\n"
		"	scd	$1, %1					\n"
		"	beqzl	$1, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"2:							\n"
		"	.set	pop					\n"
		: "=&r" (retval),
#if (__GNUC__ >= 3)
			"=R" (*m)
#else
			"=m" (*m)
#endif
		: "R" (*m), "Jr" (old), "Jr" (replace)
		: "memory");
#if 0
	} else if (cpu_has_llsc) {
		__asm__ __volatile__(
		"	.set	push					\n"
		"	.set	noat					\n"
		"	.set	mips3					\n"
		"1:	lld	%0, %2			# __cmpxchg_u64	\n"
		"	bne	%0, %z3, 2f				\n"
		"	move	$1, %z4					\n"
		"	scd	$1, %1					\n"
		"	beqz	$1, 1b					\n"
#ifdef MA__LWPS
		"	sync						\n"
#endif
		"2:							\n"
		"	.set	pop					\n"
		: "=&r" (retval),
#if (__GNUC__ >= 3)
			"=R" (*m)
#else
			"=m" (*m)
#endif
		: "R" (*m), "Jr" (old), "Jr" (replace)
		: "memory");
	} else {
		unsigned long flags;

		local_irq_save(flags);
		retval = *m;
		if (retval == old)
			*m = replace;
		local_irq_restore(flags);	/* implies memory barrier  */
	}
#endif

	return retval;
}
#endif

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid xchg().  */
extern void __ma_cmpxchg_called_with_bad_pointer(void);
extern unsigned long __ma_cmpxchg_u8(volatile int *m, unsigned long old, unsigned long replace);
extern unsigned long __ma_cmpxchg_u16(volatile int *m, unsigned long old, unsigned long replace);

static __tbx_inline__ unsigned long TBX_NOINST __ma_cmpxchg(volatile void * ptr, unsigned long old,
	unsigned long replace, int size)
{
	switch (size) {
	case 1:
		return __ma_cmpxchg_u8(ptr, old, replace);
	case 2:
		return __ma_cmpxchg_u16(ptr, old, replace);
	case 4:
		return __ma_cmpxchg_u32(ptr, old, replace);
#if MA_BITS_PER_LONG == 64
	case 8:
		return __ma_cmpxchg_u64(ptr, old, replace);
#endif
	}
	__ma_cmpxchg_called_with_bad_pointer();
	return old;
}

#define ma_cmpxchg(ptr,old,replace) ((__typeof__(*(ptr)))__ma_cmpxchg((ptr), (unsigned long)(old), (unsigned long)(replace),sizeof(*(ptr))))

#define ma_cpu_relax() ma_barrier()
