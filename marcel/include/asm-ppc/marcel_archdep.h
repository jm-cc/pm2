
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

#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"

#  define TOP_STACK_FREE_AREA     256

/* Linux PPC */
#if defined(LINUX_SYS)
#  include <setjmp.h>
#  ifndef JB_GPR1
#    define JB_GPR1 0
#  endif
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_GPR1])
#endif

/* Darwin PPC (Mac OS X) */
#if defined(DARWIN_SYS)
#  define STACK_INFO
#  define SP_FIELD(buf)           ((buf)[0])
#endif

#if defined(AIX_SYS)
#  define SP_FIELD(buf)           ((buf)[3])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("r1"); \
  sp; \
})

#define get_fp() \
({ \
  register unsigned long sp asm("r31"); \
  sp; \
})
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#ifdef DARWIN_SYS
#define MA_ASM_R "r"
#else
#define MA_ASM_R
#endif

#define set_sp(val) \
  __asm__ __volatile__("mr "MA_ASM_R"1, %0\n" \
		  : : "r" (val) )

#define set_fp(val) \
  __asm__ __volatile__("mr "MA_ASM_R"31, %0\n" \
		  : : "r" (val) )

#define set_sp_fp(sp,fp) \
  __asm__ __volatile__("mr "MA_ASM_R"1, %0\n" \
		       "mr "MA_ASM_R"31, %1\n" \
		  : : "r" (sp), "r" (fp) )
