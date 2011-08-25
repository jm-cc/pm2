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


#include "tbx_compiler.h"
#include "tbx_types.h"
#include "sys/marcel_flags.h"
#if !defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "asm-generic/marcel_archdep.h"
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define TOP_STACK_FREE_AREA     64

/* marcel setjmp */
typedef intptr_t ma_jmp_buf[6];
#define MARCEL_JB_BX   0
#define MARCEL_JB_SI   1
#define MARCEL_JB_DI   2
#define MARCEL_JB_BP   3
#define MARCEL_JB_SP   4
#define MARCEL_JB_PC   5
#define SP_FIELD(buf)           ((buf)[MARCEL_JB_SP])
#define FP_FIELD(buf)           ((buf)[MARCEL_JB_BP])
#define PC_FIELD(buf)           ((buf)[MARCEL_JB_PC])

#define ma_ST_FLUSH_WINDOWS()  ((void)0)
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define get_sp()			\
	({						\
		register unsigned long sp asm("esp");	\
		sp;					\
	})
#endif
#define get_bp()					\
	({						\
		register unsigned long bp asm("ebp");	\
		bp;					\
	})
#define set_sp(val)				\
	do {					\
		__typeof__(val) value=(val);	\
		__asm__ __volatile__("movl %0, %%esp"	    \
				     : : "m" (value) : "memory", "esp" ); \
	} while (0)
#define set_sp_bp(sp, bp)			\
	do {					\
		unsigned long __sp = (unsigned long)(sp);	\
		unsigned long __bp = (unsigned long)(bp);	\
		__asm__ __volatile__("movl %0, %%esp;\n\t"	\
				     "movl %1, %%ebp;"		       \
				     : : "r" (__sp), "r" (__bp) : "memory", "esp" ); \
	} while (0)

extern unsigned short __main_thread_desc;

#ifdef __GLIBC__
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
	/* Reservation of some values for the TM ABI.  */
	void *__private_tm[5];
	char padding[MA_NPTL_PTHREAD_SIZE];	//for the NPTL thread structure
	/* LPT binary compatibility end */
} lpt_tcb_t;

#define MA_GET_TCB(field) ({\
	__typeof__(((lpt_tcb_t*)NULL)->field) __val; \
	switch (sizeof(__val)) { \
		case 1: \
			asm("movb %%gs:%P1,%b0" \
				: "=q" (__val) \
				: "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		case 2: \
			asm("movw %%gs:%P1,%0" \
				: "=r" (__val) \
				: "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		case 4: \
			asm("movl %%gs:%P1,%0" \
				: "=r" (__val) \
				: "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		default: MA_BUG(); \
	} \
	__val; \
})

#define MA_SET_TCB(field, val) {\
	__typeof__(((lpt_tcb_t*)NULL)->field) __val; \
	switch (sizeof(__val)) { \
		case 1: \
			asm volatile("movb %b0,%%gs:%P1" \
				:: "iq" (val), \
				   "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		case 2: \
			asm volatile("movw %0,%%gs:%P1" \
				:: "ir" (val), \
				   "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		case 4: \
			asm volatile("movl %0,%%gs:%P1" \
				:: "ir" (val), \
				   "i" (tbx_offset_of (lpt_tcb_t, field))); \
			break; \
		default: MA_BUG(); \
	} \
}

#endif

#ifdef MA__PROVIDE_TLS
#define MA_TLS_MULTIPLE_THREADS_IN_TCB

// Variante II
#define marcel_tcb(new_task) \
  ((void*)(&(new_task)->tls[MARCEL_TLS_AREA_SIZE - sizeof(lpt_tcb_t)]))
#define marcel_ctx_set_tls_reg(new_task)				\
  do { asm volatile ("movw %w0, %%gs" : : "q"(new_task->tls_desc));} while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif


/** Internal inline functions **/
static __tbx_inline__ unsigned short get_gs(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_MARCEL_ARCHDEP_H__ **/
