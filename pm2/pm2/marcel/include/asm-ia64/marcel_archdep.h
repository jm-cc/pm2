
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
#define JB_SP                   12
#define SP_FIELD(buf)           ((buf).uc_mcontext.sc_gr[JB_SP])
#define BSP_FIELD(buf)          ((buf).uc_mcontext.sc_ar_bsp)

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#define SET_MARCEL_SELF_FROM_SP(val) (void)(0)
static __inline__ long get_sp(void)
{
  register long sp;

  __asm__ __volatile__(
		  ";; \n\t" \
		  "mov %0 = sp ;; \n\t"
		  ";; \n\t" \
		  : "=r" (sp));

  return sp;
}
static __inline__ long get_bsp(void)
{
  register long bsp;

  __asm__ __volatile__(
		  ";; \n\t" \
		  "flushrs \n\t" \
		  "mov %0 = ar.bsp ;; \n\t"
		  ";; \n\t" \
		  : "=r" (bsp));

  return bsp;
}
#define set_sp(val) \
  do { \
    SET_MARCEL_SELF_FROM_SP(val); \
    __asm__ __volatile__( \
		    ";; \n\t" \
		    "mov sp = %0 \n\t" \
		    ";; \n\t" \
                       : : "r" (val) : "memory", "sp" ); \
  } while (0)
#define rTMP "%0"
#define rRSC "%1"
#define set_sp_bsp(val, bsp) \
  do { \
    SET_MARCEL_SELF_FROM_SP(val); \
    __asm__ __volatile__( \
		    ";; \n\t" \
		    "flushrs \n\t" \
		    ";; \n\t" \
		    "mov.m "rRSC" = ar.rsc \n\t" \
		    ";; \n\t" \
		    "dep "rTMP" = 0, "rRSC", 16, 14 \n\t" \
		    ";; \n\t" \
		    "and "rTMP" = ~0x3, "rTMP" \n\t" \
		    ";; \n\t" \
		    "mov.m ar.rsc = "rTMP" \n\t" \
		    ";; \n\t" \
		    "loadrs \n\t" \
		    ";; \n\t" \
		    "mov.m ar.bspstore = %3 \n\t" \
		    ";; \n\t" \
		    "mov.m ar.rsc = "rRSC" \n\t" \
		    ";; \n\t" \
		    "mov sp = %2 \n\t" \
		    ";; \n\t" \
                       : : "r" (0), "r"(0), "r" (val), "r" (bsp) : "memory", "sp" ); \
  } while (0)



#endif
