
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
#define MA_HAVE_RWLOCK 1

#section marcel_macros
#define MA_RW_LOCK_BIAS		 0x01000000
#define MA_RW_LOCK_BIAS_STR	"0x01000000"

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

#define __ma_build_write_lock(rw, helper)	do { \
						if (__builtin_constant_p(rw)) \
							__ma_build_write_lock_const(rw, helper); \
						else \
							__ma_build_write_lock_ptr(rw, helper); \
					} while (0)

