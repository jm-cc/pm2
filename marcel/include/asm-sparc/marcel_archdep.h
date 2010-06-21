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


#ifndef __ASM_SPARC_MARCEL_ARCHDEP_H__
#define __ASM_SPARC_MARCEL_ARCHDEP_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep.h"
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/* Solaris sparc */
#if defined(SOLARIS_SYS)
#  define STACK_INFO
#  include <sys/stack.h>
#  define TOP_STACK_FREE_AREA     (WINDOWSIZE+128)
#  define SP_FIELD(buf)           ((buf)[1])
#  define FP_FIELD(buf)           ((buf)[3])
#endif

/* Linux sparc */
#if defined(LINUX_SYS)
#  define STACK_INFO
#  include <sys/ucontext.h>
#ifdef PM2_DEV
#warning XXX: est-ce vraiment ça ?
#endif
#  define TOP_STACK_FREE_AREA     (MAL(SPARC_MAXREGWINDOW*4)+128)
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_SP])
#  define FP_FIELD(buf)           ((buf)->__jmpbuf[JB_FP])
#endif

extern void call_ST_FLUSH_WINDOWS(void);

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp()					\
	({						\
		register unsigned long sp asm("%sp");	\
		sp;					\
	})
#define get_fp()					\
	({						\
		register unsigned long fp asm("%fp");	\
		fp;					\
	})
#endif

#  define set_sp(val)						\
	__asm__ __volatile__("mov %0, %%sp\n\t"			\
			     : : "r" (val) : "memory", "sp")

#  define set_fp(val)						\
	__asm__ __volatile__("mov %0, %%fp\n\t"			\
			     : : "r" (val) : "memory", "fp")

#  define set_sp_fp(sp,fp)						\
	__asm__ __volatile__("mov %0, %%sp\n\t"				\
			     "mov %1, %%fp\n\t"				\
			     : : "r" (sp), "r" (fp) : "memory", "sp")


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_SPARC_MARCEL_ARCHDEP_H__ **/
