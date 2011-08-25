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


#ifndef __SYS_MARCEL_ISOMALLOC_H__
#define __SYS_MARCEL_ISOMALLOC_H__


#include "marcel_config.h"
#include "sys/os/marcel_isomalloc.h"


/* SLOT_AREA_TOP doit être un multiple de THREAD_SLOT_SIZE
 * et THREAD_SLOT_SIZE doit être une puissance de deux */

/* Warning: ASM_THREAD_SLOT_SIZE must be a power of two, and be at least twice
 * as much as PTHREAD_STACK_MIN, must be usable in assembly source */

#ifdef MARCEL_THREAD_SLOT_SIZE
#define ASM_THREAD_SLOT_SIZE MARCEL_THREAD_SLOT_SIZE
#else
/* Note: for x86_64 with TLS or valgrind, we have to restrict to 32bit, thus
 * the same stack size as for x86_32 */
#if defined(PPC_ARCH)
#define ASM_THREAD_SLOT_SIZE          (0x40000)	/* 256 KB */
#else
#if (defined(X86_64_ARCH) && !defined(MA__PROVIDE_TLS) && !defined(PM2VALGRIND)) || defined(IA64_ARCH) || defined(PPC64_ARCH)
#define ASM_THREAD_SLOT_SIZE          (0x400000)	/* 4 MB */
#else
#ifdef MA__LIBPTHREAD
#define ASM_THREAD_SLOT_SIZE          (0x200000)	/* 2 MB */
#else
#define ASM_THREAD_SLOT_SIZE          (0x10000)	/* 64 KB */
#endif
#endif
#endif
#endif
#define THREAD_SLOT_SIZE              ((unsigned long) ASM_THREAD_SLOT_SIZE)
/* 8192 is a rough approximation for `struct marcel_task' + some stack, more
 * precise check done at runtime in `marcel_init.c' and
 * `marcel_create_internal()'.  */
#if (ASM_THREAD_SLOT_SIZE < MARCEL_TLS_AREA_SIZE + 8192)
#  error ASM_THREAD_SLOT_SIZE is too small for TLS_AREA_SIZE, please increase it in marcel/include/sys/isomalloc_archdep.h
#endif

#define SLOT_AREA_TOP                 ((unsigned long) ISOADDR_AREA_TOP)

/** Experimental chosen value: may fail on untested system */
#ifndef SLOT_AREA_BOTTOM
#  define SLOT_AREA_BOTTOM 0x1000
#endif

#ifdef PM2VALGRIND
#    ifdef ENABLE_STACK_JUMPING
#	error "valgrind doesn't work with stack jumping enabled"
#    endif
#endif


#endif /** __SYS_MARCEL_ISOMALLOC_ARCHDEP_H__ **/
