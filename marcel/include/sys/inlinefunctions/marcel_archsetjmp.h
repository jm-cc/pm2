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


#ifndef __SYS_INLINEFUNCTIONS_MARCEL_ARCHSETJMP_H__
#define __SYS_INLINEFUNCTIONS_MARCEL_ARCHSETJMP_H__


#include "sys/marcel_archsetjmp.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#if defined(X86_ARCH)
static __tbx_inline__ void ma_longjmp(ma_jmp_buf buf, int val)
{
	__asm__ __volatile__(
#ifdef MA__DEBUG
		/* Before blindly jumping, */
		/* check validity of RSP */
		"movl 16(%0), %%ebx\n\t" "movl 0(%%ebx), %%ebx\n\t"
		/* check validity of PC */
		"movl 20(%0), %%ebx\n\t" "movl 0(%%ebx), %%ebx\n\t"
#endif
		"movl 0(%0), %%ebx\n\t"
		"movl 4(%0), %%esi\n\t"
		"movl 8(%0), %%edi\n\t"
		"movl 12(%0), %%ebp\n\t"
		"movl 16(%0), %%esp\n\t"
		"movl 20(%0), %0\n\t" "jmp *%0"
#ifdef __INTEL_COMPILER
		::"c,d"(buf), "a"(val));
#else
	        ::"c,d"(buf), "a,a"(val));
#endif
                // to make gcc believe us that the above statement doesn't return
                for (;;);
}

#elif defined(X86_64_ARCH)
static __tbx_inline__ void ma_longjmp(ma_jmp_buf buf, int val)
{
	__asm__ __volatile__(
#ifdef MA__DEBUG
		/* Before blindly jumping, */
		/* check validity of RSP */
		"movq 48(%0), %%rbx\n\t" "movq 0(%%rbx), %%rbx\n\t"
		/* check validity of PC */
		"movq 56(%0), %%rbx\n\t" "movq 0(%%rbx), %%rbx\n\t"
#endif
		"movq 0(%0), %%rbx\n\t"
		"movq 8(%0), %%rbp\n\t"
		"movq 16(%0), %%r12\n\t"
		"movq 24(%0), %%r13\n\t"
		"movq 32(%0), %%r14\n\t"
		"movq 40(%0), %%r15\n\t"
		"movq 48(%0), %%rsp\n\t"
		"movq 56(%0), %0\n\t" "jmp *%0"
#ifdef __INTEL_COMPILER
		::"D"(buf), "a"(val));
#else
	        ::"D"(buf), "a,a"(val));
#endif
                // to make gcc believe us that the above statement doesn't return
                for (;;);
}
#endif /** *_ARCH **/


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif				/* __SYS_INLINEFUNCTIONS_MARCEL_ARCHSETJMP_H__ */
