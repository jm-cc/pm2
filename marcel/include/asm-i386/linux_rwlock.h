
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
 * include/asm-i386/rwlock.h
 *
 *	Helpers used by both rw spinlocks and rw semaphores.
 *
 *	Based in part on code from semaphore.h and
 *	spinlock.h Copyright 1996 Linus Torvalds.
 *
 *	Copyright 1999 Red Hat, Inc.
 *
 *	Written by Benjamin LaHaise.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#section macros
#ifdef MA__LWPS
#define MA_HAVE_RWLOCK 1
#endif

#section marcel_types
/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 */
#ifdef MA__LWPS
typedef struct {
	volatile unsigned int lock;
} ma_rwlock_t;
#endif

#section marcel_macros
#ifdef MA__LWPS
#define MA_RW_LOCK_BIAS		 0x01000000
#define MA_RW_LOCK_BIAS_STR	"0x01000000"
#define MA_RW_LOCK_UNLOCKED { MA_RW_LOCK_BIAS }

#define ma_rwlock_init(x)	do { *(x) = (ma_rwlock_t) MA_RW_LOCK_UNLOCKED; } while(0)

#define ma_rwlock_is_locked(x) ((x)->lock != MA_RW_LOCK_BIAS)

#ifdef MA__HAS_SUBSECTION
#define __ma_build_read_lock_ptr(rw, helper)   \
	asm volatile(MA_LOCK_PREFIX "subl $1,(%0)\n\t" \
		     "js 2f\n" \
		     "1:\n" \
		     MA_LOCK_SECTION_START("") \
		     "2:\tcall " helper "\n\t" \
		     "jmp 1b\n" \
		     MA_LOCK_SECTION_END \
		     ::"a" (rw) : "memory")

#define __ma_build_read_lock_const(rw, helper)   \
	asm volatile(MA_LOCK_PREFIX "subl $1,%0\n\t" \
		     "js 2f\n" \
		     "1:\n" \
		     MA_LOCK_SECTION_START("") \
		     "2:\tpushl %%eax\n\t" \
		     "leal %0,%%eax\n\t" \
		     "call " helper "\n\t" \
		     "popl %%eax\n\t" \
		     "jmp 1b\n" \
		     MA_LOCK_SECTION_END \
		     :"=m" (*(volatile int *)rw) : : "memory")

#define __ma_build_read_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__ma_build_read_lock_const(rw, helper); \
						else \
							__ma_build_read_lock_ptr(rw, helper); \
					} while (0)

#define __ma_build_write_lock_ptr(rw, helper) \
	asm volatile(MA_LOCK_PREFIX "subl $" MA_RW_LOCK_BIAS_STR ",(%0)\n\t" \
		     "jnz 2f\n" \
		     "1:\n" \
		     MA_LOCK_SECTION_START("") \
		     "2:\tcall " helper "\n\t" \
		     "jmp 1b\n" \
		     MA_LOCK_SECTION_END \
		     ::"a" (rw) : "memory")

#define __ma_build_write_lock_const(rw, helper) \
	asm volatile(MA_LOCK_PREFIX "subl $" MA_RW_LOCK_BIAS_STR ",%0\n\t" \
		     "jnz 2f\n" \
		     "1:\n" \
		     MA_LOCK_SECTION_START("") \
		     "2:\tpushl %%eax\n\t" \
		     "leal %0,%%eax\n\t" \
		     "call " helper "\n\t" \
		     "popl %%eax\n\t" \
		     "jmp 1b\n" \
		     MA_LOCK_SECTION_END \
		     :"=m" (*(volatile int *)rw) : : "memory")
#else /* MA__HAS_SUBSECTION */
#define __ma_build_read_lock_ptr(rw, helper)   \
	asm volatile(MA_LOCK_PREFIX "subl $1,(%0)\n\t" \
		     "jns 1f\n\t" \
		     "call " helper "\n\t" \
		     "1:\n" \
		     ::"a" (rw) : "memory")

#define __ma_build_read_lock_const(rw, helper)   \
	asm volatile(MA_LOCK_PREFIX "subl $1,%0\n\t" \
		     "jns 1f\n" \
		     "\tpushl %%eax\n\t" \
		     "leal %0,%%eax\n\t" \
		     "call " helper "\n\t" \
		     "popl %%eax\n\t" \
		     "1:\n" \
		     :"=m" (*(volatile int *)rw) : : "memory")

#define __ma_build_read_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__ma_build_read_lock_const(rw, helper); \
						else \
							__ma_build_read_lock_ptr(rw, helper); \
					} while (0)

#define __ma_build_write_lock_ptr(rw, helper) \
	asm volatile(MA_LOCK_PREFIX "subl $" MA_RW_LOCK_BIAS_STR ",(%0)\n\t" \
		     "jz 1f\n" \
		     "\tcall " helper "\n\t" \
		     "1:\n" \
		     ::"a" (rw) : "memory")

#define __ma_build_write_lock_const(rw, helper) \
	asm volatile(MA_LOCK_PREFIX "subl $" MA_RW_LOCK_BIAS_STR ",%0\n\t" \
		     "jz 1f\n" \
		     "\tpushl %%eax\n\t" \
		     "leal %0,%%eax\n\t" \
		     "call " helper "\n\t" \
		     "popl %%eax\n\t" \
		     "1:\n" \
		     :"=m" (*(volatile int *)rw) : : "memory")
#endif /* MA__HAS_SUBSECTION */

#define __ma_build_write_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__ma_build_write_lock_const(rw, helper); \
						else \
							__ma_build_write_lock_ptr(rw, helper); \
					} while (0)
#endif

#section marcel_inline
/*
 * On x86, we implement read-write locks as a 32-bit counter
 * with the high bit (sign) being the "contended" bit.
 *
 * The __tbx_inline__ assembly is non-obvious. Think about it.
 *
 * Changed to use the same technique as rw semaphores.  See
 * semaphore.h for details.  -ben
 */
/* the spinlock helpers are in arch/i386/kernel/semaphore.c */

#ifdef MA__LWPS
static __tbx_inline__ void _ma_raw_read_lock(ma_rwlock_t *rw)
{
	__ma_build_read_lock(rw, "__ma_read_lock_failed");
}

static __tbx_inline__ void _ma_raw_write_lock(ma_rwlock_t *rw)
{
	__ma_build_write_lock(rw, "__ma_write_lock_failed");
}
#endif

#section marcel_macros
#ifdef MA__LWPS
#define _ma_raw_read_unlock(rw)		asm volatile("lock ; incl %0" :"=m" ((rw)->lock) : : "memory")
#define _ma_raw_write_unlock(rw)	asm volatile("lock ; addl $" MA_RW_LOCK_BIAS_STR ",%0":"=m" ((rw)->lock) : : "memory")
#endif

#section marcel_inline
#ifdef MA__LWPS
static __tbx_inline__ int _ma_raw_write_trylock(ma_rwlock_t *lock)
{
	ma_atomic_t *count = (ma_atomic_t *)lock;
	if (ma_atomic_sub_and_test(MA_RW_LOCK_BIAS, count))
		return 1;
	ma_atomic_add(MA_RW_LOCK_BIAS, count);
	return 0;
}
#endif
