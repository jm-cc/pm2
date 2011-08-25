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


#ifndef __ASM_IA64_MARCEL_ARCH_SWITCHTO_H__
#define __ASM_IA64_MARCEL_ARCH_SWITCHTO_H__


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/

/* because tls does not change during the kthread life, we need to store
 * the tls location once (at start, marcel_allo.c for main_lwp and 
 * sys/marcel_lwp for the others */
#define MA_ARCH_INTERRUPT_ENTER_LWP_FIX(current, p_ucontext_t) (void)0

/* When delivering a signal, the kernel will have saved r13 (the TLS pointer)
 * on the current thread's stack.
 * When we do not provide TLS ourselves and switch thread in the signal
 * handler, we thus have to put back the value for this kernel thread in the
 * next thread's ucontext_t area of its stack. */
#define MA_ARCH_INTERRUPT_EXIT_LWP_FIX(current, p_ucontext_t)		\
	do {								\
		register unsigned long reg_13 asm("r13");		\
		((ucontext_t *)p_ucontext_t)->uc_mcontext.sc_gr[13] = reg_13; \
	} while (0)

#ifndef MA__PROVIDE_TLS
/* context switch: we need to set the good tls location
 * __sigsetjmp save r13 (related to the previous lwp)
 * after longjmp instead of restore previously saved r13 content, we need
 * to push tls pointer of the new running lwp */
#define MA_ARCH_SWITCHTO_LWP_FIX(current)				\
	do {								\
		asm volatile ("mov r13 = %0 \n\t"			\
			      : : "r" (MA_LWP_SELF->ma_tls_tp) : "r13" ); \
	} while (0) 
#else
/* empty macro because marcel_ctx_set_tls_reg do the job for us */
#define MA_ARCH_SWITCHTO_LWP_FIX(current) (void)0
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_MARCEL_ARCH_SWITCHTO_H__ **/
