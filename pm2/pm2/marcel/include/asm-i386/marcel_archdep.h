
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

#define TOP_STACK_FREE_AREA     64
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])

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

static __inline__ long get_gs(void)
{
  register long gs;

    __asm__ __volatile__("movl %%gs, %0" : "=r" (gs));
    return gs;
}

static __inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__("movl %%esp, %0" : "=r" (sp));
  return sp;
}
#define set_sp(val) \
  do { \
    typeof(val) value=(val); \
    SET_MARCEL_SELF_FROM_SP(value); \
    __asm__ __volatile__("movl %0, %%esp" \
                       : : "m" (value) : "memory" ); \
  } while (0)


#endif
