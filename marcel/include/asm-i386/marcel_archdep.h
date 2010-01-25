/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __ASM_I386_MARCEL_ARCHDEP_H__
#define __ASM_I386_MARCEL_ARCHDEP_H__


#ifdef __MARCEL_KERNEL__
#include <stdint.h>
#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"
#include "sys/marcel_archsetjmp.h"
#ifndef MA_JMPBUF
#include <setjmp.h>
#endif
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep.h"
#endif
#endif /** __MARCEL_KERNEL__ **/


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define TOP_STACK_FREE_AREA     64
#ifdef MA_JMPBUF
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#define FP_FIELD(buf)           ((buf)[MARCEL_JB_BP])
#define PC_FIELD(buf)           ((buf)[MARCEL_JB_PC])
#else
#define SP_FIELD(buf)         ((buf)->__jmpbuf[JB_SP])
#define FP_FIELD(buf)         ((buf)->__jmpbuf[JB_BP])
#define PC_FIELD(buf)         ((buf)->__jmpbuf[JB_PC])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)


#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp() \
({ \
  register unsigned long sp asm("esp"); \
  sp; \
})
#endif


#define get_bp() \
({ \
  register unsigned long bp asm("ebp"); \
  bp; \
})

#define set_sp(val) \
  do { \
    __typeof__(val) value=(val); \
    __asm__ __volatile__("movl %0, %%esp" \
                       : : "m" (value) : "memory", "esp" ); \
  } while (0)

#define set_sp_bp(sp, bp) \
  do { \
    unsigned long __sp = (unsigned long)(sp); \
    unsigned long __bp = (unsigned long)(bp); \
    __asm__ __volatile__("movl %0, %%esp;\n\t" \
			 "movl %1, %%ebp;" \
                       : : "r" (__sp), "r" (__bp) : "memory", "esp" ); \
  } while (0)

extern unsigned short __main_thread_desc;

#ifdef MA__PROVIDE_TLS
typedef struct {
  /* LPT binary compatibility */
  void *tcb;
  void *dtv;
  void *self;
  int multiple_threads;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  int gscope_flag;
  int private_futex;
  char padding[1024]; //for the NPTL thread structure
  /* LPT binary compatibility end */
} lpt_tcb_t;

#define MA_TLS_MULTIPLE_THREADS_IN_TCB

// Variante II
#define marcel_tcb(new_task) \
  ((void*)(&(new_task)->tls[MA_TLS_AREA_SIZE - sizeof(lpt_tcb_t)]))
#define marcel_ctx_set_tls_reg(new_task) \
  do { asm volatile ("movw %w0, %%gs" : : "q" (new_task->tls_desc)); } while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif


/** Internal inline functions **/
static __tbx_inline__ long get_gs(void) ;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_MARCEL_ARCHDEP_H__ **/
