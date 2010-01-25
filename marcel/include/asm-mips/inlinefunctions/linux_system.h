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


#ifndef __INLINEFUNCTIONS_ASM_MIPS_LINUX_SYSTEM_H__
#define __INLINEFUNCTIONS_ASM_MIPS_LINUX_SYSTEM_H__


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


#include "asm/linux_system.h"


#ifdef __MARCEL_KERNEL__


/** Public inline functions functions **/
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


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_MIPS_LINUX_SYSTEM_H__ **/
