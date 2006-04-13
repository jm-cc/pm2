
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

#section marcel_macros

#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"

/* SGI */
#if defined(IRIX_SYS)
#  define TOP_STACK_FREE_AREA     128
#  define SP_FIELD(buf)           ((buf)[JB_SP])
#  define BSP_FIELD(buf)          ((buf)[JB_BP])
#  define PC_FIELD(buf)           ((buf)[JB_PC])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("$sp"); \
  sp; \
})
#ifdef IRIX_SYS
#define get_fp() \
({ \
 register unsigned long fp asm("$fp"); \
  fp; \
})
#endif
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#  define set_sp(val) \
  __asm__ __volatile__("move $sp, %0" \
                       : : "r" (val) : "memory", "sp" )

#ifdef IRIX_SYS
#  define set_fp(val) \
  __asm__ __volatile__("move $fp, %0" \
                       : : "r" (val) : "memory", "fp")

#define set_sp_fp(sp,fp) \
  __asm__ __volatile__("move $sp, %0\n" \
		       "move $fp, %1\n" \
		  : : "r" (sp), "r" (fp) : "memory", "sp" /*, "fp" */)
#endif
