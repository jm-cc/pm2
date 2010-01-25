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


#ifndef __ASM_RS6K_MARCEL_ARCHDEP_H__
#define __ASM_RS6K_MARCEL_ARCHDEP_H__


#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep___marcel_macros.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* IBM SP2 */
#if defined(AIX_SYS)
#  define STACK_INFO
#  define TOP_STACK_FREE_AREA     256
#  define SP_FIELD(buf)           ((buf)[3])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("r1"); \
  sp; \
})
#endif

#define set_sp(val) \
  __asm__ __volatile__("mr 1, %0\n" \
		  : : "r" (val) : "memory", "r1")


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_RS6K_MARCEL_ARCHDEP_H__ **/
