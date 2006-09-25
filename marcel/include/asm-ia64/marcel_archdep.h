
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
#define JB_SP                   12
#define SP_FIELD(buf)           ((buf).uc_mcontext.sc_gr[JB_SP])
#define BSP_FIELD(buf)          ((buf).uc_mcontext.sc_ar_bsp)

#define call_ST_FLUSH_WINDOWS()  ((void)0)

#define SET_MARCEL_SELF_FROM_SP(val) (void)(0)
#ifdef __GNUC__
#ifndef __INTEL_COMPILER
#define get_sp_fresh() \
({ \
  register unsigned long sp; \
  __asm__ (";; mov %0 = sp ;; \n\t": "=r"(sp)); \
  sp; \
})

#define get_sp() \
({ \
  register unsigned long sp asm("sp"); \
  sp; \
})

static __tbx_inline__ unsigned long get_bsp(void)
{
  register unsigned long bsp;

  __asm__(
		  ";; \n\t" \
		  "flushrs ;; \n\t" \
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
#else /* __INTEL_COMPILER */
#define _MA_IA64_REG_SP		1036	/* R12 */
__ma_u64 __getReg(const int whichReg);
#define get_sp() ((unsigned long)__getReg(_MA_IA64_REG_SP))
#define get_sp_fresh() get_sp()
#endif /* __INTEL_COMPILER */
#else /* neither gcc nor icc */
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#ifdef MA__PROVIDE_TLS
typedef struct {
  void *dtv;
  void *__private;
  char padding[128]; //pour la structure thread de nptl...
} lpt_tcb_t;

// Variante I
#define marcel_tcb(new_task) \
  ((void*)(&(new_task)->tls))

#define marcel_ctx_set_tls_reg(new_task) \
  do { \
    __asm__ __volatile__( \
      ";; \n\t" \
      "mov r13 = %0 \n\t" \
      ";; \n\t" \
        : : "r" (marcel_tcb(new_task)) : "r13" ); \
  } while (0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif
