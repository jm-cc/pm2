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


#ifndef __INLINEFUNCTIONS_ASM_IA64_MARCEL_ARCHDEP_H__
#define __INLINEFUNCTIONS_ASM_IA64_MARCEL_ARCHDEP_H__


#include "tbx_compiler.h"
#include "asm/marcel_archdep.h"
#include "sys/marcel_flags.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal functions **/
#ifdef __GNUC__
#ifndef __INTEL_COMPILER
static __tbx_inline__ unsigned long get_bsp(void)
{
	register unsigned long bsp;

      __asm__(";; \n\t" "flushrs ;; \n\t" "mov %0 = ar.bsp ;; \n\t" ";; \n\t":"=r"(bsp));

	return bsp;
}
#endif
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_MARCEL_ARCHDEP_H__ **/
