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


#ifndef __ASM_IA64_LINUX_SPINLOCK_H__
#define __ASM_IA64_LINUX_SPINLOCK_H__


/*
 * Copyright (C) 1998-2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 *
 * This file is used for SMP configurations only.
 */


/** Public macros **/
#ifdef MA__LWPS
#define MA_HAVE_RWLOCK 1
#define MA_SPIN_LOCK_UNLOCKED			{ 0 }
#endif /* MA__LWPS */


/** Public data types **/
#ifdef MA__LWPS
typedef struct {
	volatile unsigned int lock;
} ma_spinlock_t;
#endif /* MA__LWPS */


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MA__LWPS
#define ma_spin_lock_init(x)			((x)->lock = 0)

#define ma_spin_is_locked(x)	((x)->lock != 0)
#define _ma_raw_spin_unlock(x)	do { ma_barrier(); ((ma_spinlock_t *) x)->lock = 0; } while (0)
#define _ma_raw_spin_trylock(x)	(ma_cmpxchg_acq(&(x)->lock, 0, 1) == 0)
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while ((x)->lock)


#define MA_RW_LOCK_UNLOCKED { 0, 0, 0 }

#define ma_rwlock_init(x)		do { *(x) = (ma_rwlock_t) MA_RW_LOCK_UNLOCKED; } while(0)
#define ma_rwlock_is_locked(x)	(*(volatile int *) (x) != 0)

#define _ma_raw_read_lock(rw)								\
do {											\
	ma_rwlock_t *__ma_read_lock_ptr = (rw);						\
	while(((volatile int *)__ma_read_lock_ptr)[1] > 0);				\
	while (tbx_unlikely(ma_ia64_fetchadd(1, (volatile int *) __ma_read_lock_ptr, acq) < 0)) {		\
		ma_ia64_fetchadd(-1, (volatile int *) __ma_read_lock_ptr, rel);			\
		while (*(volatile int *)__ma_read_lock_ptr < 0)				\
			ma_cpu_relax();							\
	}										\
} while (0)


#define _ma_raw_read_unlock(rw)					\
do {								\
	ma_rwlock_t *__ma_read_lock_ptr = (rw);			\
	ma_ia64_fetchadd(-1, (volatile int *) __ma_read_lock_ptr, rel);	\
} while (0)

#ifdef __INTEL_COMPILER

#define _ma_raw_write_lock(l)								\
({											\
	__u64 ia64_val, ia64_set_val = ia64_dep_mi(-1, 0, 31, 1);			\
	volatile __u32 *ia64_write_lock_ptr = (volatile __u32 *) (l);					\
	ma_ia64_fetchadd(1 , ia64_write_lock_ptr+1);\
	do {										\
		while (*ia64_write_lock_ptr)						\
			ia64_barrier();							\
		ia64_val = ia64_cmpxchg4_acq(ia64_write_lock_ptr, ia64_set_val, 0);	\
	} while (ia64_val);								\
        ma_ia64_fetchadd(-1 , ia64_write_lock_ptr+1);\
})

#define _ma_raw_write_trylock(rw)						\
({									\
	__u64 ia64_val;							\
	__u64 ia64_set_val = ia64_dep_mi(-1, 0, 31,1);			\
	ia64_val = ia64_cmpxchg4_acq((volatile __u32 *)(rw), ia64_set_val, 0);	\
	(ia64_val == 0);						\
})
#else
#define _ma_raw_write_lock(rw)							\
do {										\
	ma_ia64_fetchadd(1 , ((volatile int*)rw)+1,acq);\
 	__asm__ __volatile__ (							\
		"mov ar.ccv = r0\n"						\
		"dep r29 = -1, r0, 31, 1;;\n"					\
		"1:\n"								\
		"ld4 r2 = [%0];;\n"						\
		"cmp4.eq p0,p7 = r0,r2\n"					\
		"(p7) br.cond.spnt.few 1b \n"					\
		"cmpxchg4.acq r2 = [%0], r29, ar.ccv;;\n"			\
		"cmp4.eq p0,p7 = r0, r2\n"					\
		"(p7) br.cond.spnt.few 1b;;\n"					\
		:: "r"(rw) : "ar.ccv", "p7", "r2", "r29", "memory");		\
	 ma_ia64_fetchadd(-1 , ((volatile int*)rw)+1,acq);\
} while(0)

#define _ma_raw_write_trylock(rw)							\
({										\
	register long result;							\
										\
	__asm__ __volatile__ (							\
		"mov ar.ccv = r0\n"						\
		"dep r29 = -1, r0, 31, 1;;\n"					\
		"cmpxchg4.acq %0 = [%1], r29, ar.ccv\n"				\
		: "=r"(result) : "r"(rw) : "ar.ccv", "r29", "memory");		\
	(result == 0);								\
})
#endif

#define _ma_raw_write_unlock(x)								\
({											\
	ma_smp_mb__before_clear_bit();	/* need barrier before releasing lock... */	\
	ma_clear_bit(31, (x));								\
})
#endif /* MA__LWPS */


/** Internal data types **/
#ifdef MA__LWPS
typedef struct {
	volatile unsigned int read_counter	: 31;
	volatile unsigned int write_lock	:  1;
	volatile unsigned int need_write;
} ma_rwlock_t;
#endif /* MA__LWPS */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_LINUX_SPINLOCK_H__ **/
