
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
#ifdef LINUX_SYS
/* Some distributions don't provide these numbers... */
#  ifndef JB_RSP
#    define JB_RSP 6
#  endif
#  ifndef JB_RBP
#    define JB_RBP 1
#  endif
#  ifndef JB_PC
#    define JB_PC 7
#  endif
#endif
#define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_RSP])
#define FP_FIELD(buf)           ((buf)->__jmpbuf[JB_RBP])
#define PC_FIELD(buf)           ((buf)->__jmpbuf[JB_PC])

#define call_ST_FLUSH_WINDOWS()  ((void)0)
#define SET_MARCEL_SELF_FROM_SP(sp) ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("rsp"); \
  sp; \
})
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#define get_bp() \
({ \
  register unsigned long bp asm("rbp"); \
  bp; \
})

#define set_sp(val) \
  do { \
    __typeof__(val) value=(val); \
    __asm__ __volatile__("movq %0, %%rsp" \
                       : : "m" (value) : "memory", "rsp" ); \
  } while (0)

#define set_sp_bp(sp, bp) \
  do { \
    unsigned long __sp = (unsigned long)(sp); \
    unsigned long __bp = (unsigned long)(bp); \
    SET_MARCEL_SELF_FROM_SP(__sp); \
    __asm__ __volatile__("movq %0, %%rsp;\n\t" \
			 "movq %1, %%rbp;" \
                       : : "r" (__sp), "r" (__bp) : "memory", "rsp" ); \
  } while (0)

extern unsigned short __main_thread_desc;

#ifdef MA__PROVIDE_TLS
#include <stdint.h>
typedef struct {
  void *tcb;
  void *dtv;
  void *self;
  int multiple_threads;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  char padding[128]; //pour la structure thread de nptl...
} lpt_tcb_t;

// Variante II
#define marcel_tcb(new_task) \
  ((void*)(&(new_task)->tls[MA_TLS_AREA_SIZE - sizeof(lpt_tcb_t)]))
#define marcel_ctx_set_tls_reg(new_task) \
  do { \
    unsigned short val; \
    if (new_task == __main_thread) \
      val = __main_thread_desc; \
    else \
      val = ((SLOT_AREA_TOP - (((unsigned long)(new_task)) & ~(THREAD_SLOT_SIZE-1))) / THREAD_SLOT_SIZE - 1) * 8 | 0x4; \
    asm volatile ("movw %w0, %%fs" : : "q" (val)); \
  } while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif

