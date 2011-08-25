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


#ifndef __INLINEFUNCTIONS_ASM_I386_MARCEL_CTX_JMP_H__
#define __INLINEFUNCTIONS_ASM_I386_MARCEL_CTX_JMP_H__


#include "asm/marcel_ctx_jmp.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


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


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif				/* __INLINEFUNCTIONS_ASM_I386_MARCEL_CTX_JMP_H__ */
