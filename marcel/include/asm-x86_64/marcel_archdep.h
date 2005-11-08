
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
  register unsigned long sp asm("rsp"); \
  sp; \
})

#define get_bp() \
({ \
  register unsigned long bp asm("rbp"); \
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


#section marcel_variables
