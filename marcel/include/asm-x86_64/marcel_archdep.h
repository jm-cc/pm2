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


#ifndef __ASM_X86_64_MARCEL_ARCHDEP_H__
#define __ASM_X86_64_MARCEL_ARCHDEP_H__


#ifdef MA__PROVIDE_TLS
#include <sys/syscall.h>
#include <asm/prctl.h>
#endif
#include <unistd.h>
#include "sys/marcel_flags.h"
#include "tbx_compiler.h"
#include "tbx_intdef.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/* x86_64 has a 128 byte red zone */
#define TOP_STACK_FREE_AREA     128

/* marcel setjmp/longjmp */
#define MA_JMPBUF 1
typedef intptr_t ma_jmp_buf[8];
#define MARCEL_JB_RBX   0
#define MARCEL_JB_RBP   1
#define MARCEL_JB_R12   2
#define MARCEL_JB_R13   3
#define MARCEL_JB_R14   4
#define MARCEL_JB_R15   5
#define MARCEL_JB_RSP   6
#define MARCEL_JB_PC    7
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_RSP])
#define FP_FIELD(buf)           ((buf)[MARCEL_JB_RBP])
#define PC_FIELD(buf)           ((buf)[MARCEL_JB_PC])

#define ma_ST_FLUSH_WINDOWS()  ((void)0)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp()					\
	({						\
		register unsigned long __sp asm("rsp");	\
		__sp;					\
	})
#else
#include "asm-generic/marcel_archdep.h"
#endif

#define get_bp()					\
	({						\
		register unsigned long __bp asm("rbp"); \
		__bp;					\
	})
#define set_sp(val)							\
	do {								\
		__typeof__(val) value=(val);				\
		__asm__ __volatile__("movq %0, %%rsp"			\
				     : : "m" (value) : "memory", "rsp" ); \
	} while (0)
#define set_sp_bp(sp, bp)						\
	do {								\
		unsigned long __sp = (unsigned long)(sp);		\
		unsigned long __bp = (unsigned long)(bp);		\
		__asm__ __volatile__("movq %0, %%rsp;\n\t"		\
				     "movq %1, %%rbp;"			\
				     : : "r" (__sp), "r" (__bp) : "memory", "rsp" ); \
	} while (0)

#ifdef MA__PROVIDE_TLS
/* nptl/sysdeps/x86_64/tls.h */
typedef struct {
	/* LPT binary compatibility */
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
	char padding[1024];	//for the NPTL thread structure
	/* LPT binary compatibility end */
} lpt_tcb_t;

extern unsigned long __main_thread_tls_base;

/* Variante II */
#define marcel_tcb(new_task)						\
	((void*)(&(new_task)->tls[MA_TLS_AREA_SIZE - sizeof(lpt_tcb_t)]))
#define marcel_ctx_set_tls_reg(new_task)				\
	do {								\
		if (new_task == __main_thread) {			\
			syscall(SYS_arch_prctl, ARCH_SET_FS, __main_thread_tls_base); \
		} else {						\
			asm volatile ("movw %w0, %%fs" : : "q" (new_task->tls_desc)); \
		}							\
	} while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_X86_64_MARCEL_ARCHDEP_H__ **/
