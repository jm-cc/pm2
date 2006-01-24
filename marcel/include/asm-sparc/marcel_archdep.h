
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
#  define TOP_STACK_FREE_AREA     (MAL(SPARC_MAXREGWINDOW*4)+128)
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_SP])
#endif

extern void call_ST_FLUSH_WINDOWS(void);

#ifdef __GNUC__
#define get_sp() \
({ \
  register unsigned long sp asm("%sp"); \
  sp; \
})
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#  define set_sp(val) \
    __asm__ __volatile__("mov %0, %%sp\n\t" \
                         : : "r" (val) : "memory", "sp")

#section marcel_variables
