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


#ifndef __ASM_I386_INLINEFUNCTIONS_MARCEL_ARCHDEP_H__
#define __ASM_I386_INLINEFUNCTIONS_MARCEL_ARCHDEP_H__


#include "asm/marcel_archdep.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL 


static __tbx_inline__ unsigned short get_gs(void)
{
	unsigned short gs;

	asm volatile ("movw %%gs, %w0":"=q"(gs));
	return gs;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_I386_INLINEFUNCTIONS_MARCEL_ARCHDEP_H__ **/
