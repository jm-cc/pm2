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


#ifndef __INLINEFUNCTIONS_ASM_PPC64_MARCEL_COMPAREEXCHANGE_H__
#define __INLINEFUNCTIONS_ASM_PPC64_MARCEL_COMPAREEXCHANGE_H__


#include <stdlib.h>
#include "tbx_compiler.h"
#include "asm/marcel_compareexchange.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
static __tbx_inline__ unsigned long pm2_compareexchange(volatile void *ptr,
							unsigned long old,
							unsigned long repl, int size)
{
	unsigned long prev;

	if (size == 4) {
		volatile int32_t *p = ptr;
		__asm__ __volatile__(MA_EIEIO_ON_SMP
				     TBX_LOCAL_LBL(1) ":\n"
				     "      lwarx %0,0,%2;\n"
				     "      cmpw 0,%0,%3;\n"
				     "      bne- " TBX_LOCAL_LBLF(2) ";\n"
				     "      stwcx. %4,0,%2;\n"
				     "      bne- " TBX_LOCAL_LBLB(1) ";\n"
				     MA_ISYNC_ON_SMP
				     TBX_LOCAL_LBL(2) ":\n":"=&r"(prev), "=m"(*p)
				     :"r"(p), "r"(old), "r"(repl), "m"(*p)
				     :"cc", "memory");
	} else if (size == 8) {
		volatile int64_t *p = ptr;
		__asm__ __volatile__(MA_EIEIO_ON_SMP
				     TBX_LOCAL_LBL(1) ":\n"
				     "      ldarx %0,0,%2;\n"
				     "      cmpd 0,%0,%3;\n"
				     "      bne- " TBX_LOCAL_LBLF(2) ";\n"
				     "      stdcx. %4,0,%2;\n"
				     "      bne- " TBX_LOCAL_LBLB(1) ";\n"
				     MA_ISYNC_ON_SMP
				     TBX_LOCAL_LBL(2) ":\n":"=&r"(prev), "=m"(*p)
				     :"r"(p), "r"(old), "r"(repl), "m"(*p)
				     :"cc", "memory");
	} else {
		abort();
	}
	return prev;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_PPC64_MARCEL_COMPAREEXCHANGE_H__ **/
