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


#ifndef __ASM_ALPHA_MARCEL_ARCHDEP_H__
#define __ASM_ALPHA_MARCEL_ARCHDEP_H__


#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#include "tbx_compiler.h"
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* Alpha Tru64 */
#if defined(OSF_SYS)
#  define TOP_STACK_FREE_AREA     128
#  define JB_REGS         4               /* registers */
#  define JB_SP           (JB_REGS+30)    /* stack pointer */
#  define SP_FIELD(buf)           ((buf)[JB_SP])
#endif

/* Cray T3E */
#if defined(UNICOS_SYS)
#  define TOP_STACK_FREE_AREA	256
#  define SP_FIELD(buf)           ((buf)[2])
#  define FP_FIELD(buf)           ((buf)[3])
#endif

/* Linux */
#if defined(LINUX_SYS)
#  include <setjmp.h>
#  define TOP_STACK_FREE_AREA   256 /* XXX: how much ?? */
#  define SP_FIELD(buf)         ((buf)->__jmpbuf[JB_SP])
#  define FP_FIELD(buf)         ((buf)->__jmpbuf[JB_FP])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("$30"); \
  sp; \
})
#ifndef OSF_SYS
#define get_fp() \
({ \
 register unsigned long fp asm("$15"); \
  fp; \
})
#endif /* OSF_SYS */
#endif

#  define set_sp(val) \
  __asm__ __volatile__("addq %0, $31, $sp" \
                       : : "r" (val) : "memory", "$30")

#ifndef OSF_SYS
#  define set_fp(val) \
  __asm__ __volatile__("addq %0, $31, $15" \
                       : : "r" (val) : "memory", "$15")

#define set_sp_fp(sp,fp) \
  __asm__ __volatile__("addq %0, $31, $sp\n" \
		       "addq %1, $31, $15\n" \
		  : : "r" (sp), "r" (fp) : "memory", "$30")
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_ALPHA_MARCEL_ARCHDEP_H__ **/
