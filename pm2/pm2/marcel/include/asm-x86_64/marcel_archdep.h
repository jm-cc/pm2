
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
#include <setjmp.h>

#define TOP_STACK_FREE_AREA     128
#define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_RSP])

#define call_ST_FLUSH_WINDOWS()  ((void)0)

static __tbx_inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__("movq %%rsp, %0" : "=r" (sp));
  return sp;
}

#define set_sp(val) \
  do { \
    typeof(val) value=(val); \
    __asm__ __volatile__("movq %0, %%rsp" \
                       : : "m" (value) : "memory" ); \
  } while (0)


#endif
