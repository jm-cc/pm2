
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

#ifdef MA__PROVIDE_TLS
#include <sys/syscall.h>
#include <unistd.h>
#include <asm/prctl.h>
#endif
#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#include "tbx_compiler.h"
#depend "sys/marcel_archsetjmp.h"

/* x86_64 has a 128 byte red zone */
#define TOP_STACK_FREE_AREA     128
#ifdef MA_JMPBUF
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_RSP])
#define FP_FIELD(buf)           ((buf)[MARCEL_JB_RBP])
#define PC_FIELD(buf)           ((buf)[MARCEL_JB_PC])
#else
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
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)
#define SET_MARCEL_SELF_FROM_SP(sp) ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long __sp asm("rsp"); \
  __sp; \
})
#else
#depend "asm-generic/marcel_archdep.h[marcel_macros]"
#endif

#define get_bp() \
({ \
  register unsigned long __bp asm("rbp"); \
  __bp; \
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

#ifdef MA__PROVIDE_TLS
#include <stdint.h>
/* nptl/sysdeps/x86_64/tls.h */
typedef struct {
  void *tcb;
  void *dtv;
  void *self;
  int multiple_threads;
  int gscope_flag;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  unsigned long int vgetcpu_cache[2];
  int private_futex;
  char padding[128]; //pour la structure thread de nptl...
} lpt_tcb_t;

extern unsigned long __main_thread_tls_base;

/* Variante II */
#define marcel_tcb(new_task) \
  ((void*)(&(new_task)->tls[MA_TLS_AREA_SIZE - sizeof(lpt_tcb_t)]))
#define marcel_ctx_set_tls_reg(new_task) \
  do { \
    unsigned short val; \
    if (new_task == __main_thread) { \
      syscall(SYS_arch_prctl, ARCH_SET_FS, __main_thread_tls_base); \
    } else { \
      asm volatile ("movw %w0, %%fs" : : "q" (new_task->tls_desc)); \
    } \
  } while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif

