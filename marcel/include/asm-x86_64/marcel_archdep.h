
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

#define TOP_STACK_FREE_AREA     128
#include <setjmp.h>
#define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_RSP])
#define FP_FIELD(buf)           ((buf)->__jmpbuf[JB_RBP])
#define PC_FIELD(buf)           ((buf)->__jmpbuf[JB_PC])

#define call_ST_FLUSH_WINDOWS()  ((void)0)
#define SET_MARCEL_SELF_FROM_SP(sp) ((void)0)

#define get_sp() \
({ \
  register unsigned long sp; \
  __asm__ __volatile__("movq %%rsp, %0" : "=r" (sp)); \
  sp; \
})

#define get_bp() \
({ \
  register unsigned long bp; \
  __asm__ __volatile__("movq %%rbp, %0" : "=r" (bp)); \
  bp; \
})

#define set_sp(val) \
  do { \
    __typeof__(val) value=(val); \
    __asm__ __volatile__("movq %0, %%rsp" \
                       : : "m" (value) : "memory" ); \
  } while (0)

#define set_sp_bp(sp, bp) \
  do { \
    unsigned long __sp = (unsigned long)(sp); \
    unsigned long __bp = (unsigned long)(bp); \
    SET_MARCEL_SELF_FROM_SP(__sp); \
    __asm__ __volatile__("movq %0, %%rsp;\n\t" \
			 "movq %1, %%rbp;" \
                       : : "r" (__sp), "r" (__bp) : "memory" ); \
  } while (0)


#endif
#section marcel_variables
