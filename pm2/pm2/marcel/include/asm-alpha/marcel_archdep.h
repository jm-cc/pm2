
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

#ifndef ARCHDEP_EST_DEF
#define ARCHDEP_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#include "tbx_compiler.h"

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

static __tbx_inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__("addq $sp, $31, %0" : "=r" (sp));
  return sp;
}
#  define set_sp(val) \
  __asm__ __volatile__("addq %0, $31, $sp" \
                       : : "r" (val) : "memory" )

#endif

