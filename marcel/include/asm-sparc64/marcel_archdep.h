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


#ifndef __ASM_SPARC64_MARCEL_ARCHDEP_H__
#define __ASM_SPARC64_MARCEL_ARCHDEP_H__


#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#include "tbx_compiler.h"
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/

/* Solaris sparc */
#if defined(SOLARIS_SYS)
#  define STACK_INFO
#  include <sys/stack.h>
#  define TOP_STACK_FREE_AREA     (WINDOWSIZE+128)
#  define SP_FIELD(buf)           ((buf)[1])
#endif

/* Linux sparc */
#if defined(LINUX_SYS)
#  define STACK_INFO
#  include <sys/ucontext.h>
#ifdef PM2_DEV
#warning XXX: est-ce vraiment ça ?
#endif
#  define TOP_STACK_FREE_AREA     (MAL(SPARC_MAXREGWINDOW*8)+128)
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_SP])
#endif

extern void call_ST_FLUSH_WINDOWS(void);

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("%sp"); \
  sp; \
})
#endif

#  define set_sp(val) \
    __asm__ __volatile__("mov %0, %%sp\n\t" \
                         : : "r" (val) : "memory", "sp")


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_SPARC64_MARCEL_ARCHDEP_H__ **/
