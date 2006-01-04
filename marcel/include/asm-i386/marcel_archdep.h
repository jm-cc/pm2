
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

#define TOP_STACK_FREE_AREA     64
#if 1
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#define FP_FIELD(buf)           ((buf)[MARCEL_JB_BP])
#define PC_FIELD(buf)           ((buf)[MARCEL_JB_PC])
#else
#include <setjmp.h>
#define SP_FIELD(buf)         ((buf)->__jmpbuf[JB_SP])
#define FP_FIELD(buf)         ((buf)->__jmpbuf[JB_BP])
#define PC_FIELD(buf)         ((buf)->__jmpbuf[JB_PC])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#ifdef MARCEL_SELF_IN_REG
//register marcel_t __marcel_self_in_reg asm ("%%gs");
#  define SET_MARCEL_SELF_FROM_SP(sp) \
       __asm__ __volatile__("movl %0, %%gs" \
                         : : "m" ( \
       ((((sp) & ~(SLOT_SIZE-1)) + SLOT_SIZE) - MAL(sizeof(task_desc)))\
        ) : "memory" )
#else
#    define SET_MARCEL_SELF_FROM_SP(val) (void)(0)
#endif

#include "tbx_compiler.h"
static __tbx_inline__ long get_gs(void)
{
  register long gs;

    __asm__("movl %%gs, %0" : "=r" (gs));
    return gs;
}

#define get_sp() \
({ \
  register unsigned long sp asm("esp"); \
  sp; \
})

#define get_bp() \
({ \
  register unsigned long bp asm("ebp"); \
  bp; \
})

#define set_sp(val) \
  do { \
    __typeof__(val) value=(val); \
    SET_MARCEL_SELF_FROM_SP(value); \
    __asm__ __volatile__("movl %0, %%esp" \
                       : : "m" (value) : "memory", "esp" ); \
  } while (0)

#define set_sp_bp(sp, bp) \
  do { \
    unsigned long __sp = (unsigned long)(sp); \
    unsigned long __bp = (unsigned long)(bp); \
    SET_MARCEL_SELF_FROM_SP(__sp); \
    __asm__ __volatile__("movl %0, %%esp;\n\t" \
			 "movl %1, %%ebp;" \
                       : : "r" (__sp), "r" (__bp) : "memory", "esp" ); \
  } while (0)


#section marcel_variables
