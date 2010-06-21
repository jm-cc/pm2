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


#ifndef __INLINEFUNCTIONS_ASM_SPARC_MARCEL_TESTANDSET_H__
#define __INLINEFUNCTIONS_ASM_SPARC_MARCEL_TESTANDSET_H__


#include "tbx_compiler.h"
#include "asm/marcel_testandset.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
	char ret = 0;

	__asm__ __volatile__("ldstub [%0], %1":"=r"(spinlock), "=r"(ret)
			     :"0"(spinlock), "1"(ret):"memory");

	return (unsigned) ret;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_SPARC_MARCEL_TESTANDSET_H__ **/
