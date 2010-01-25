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


#ifndef __INLINEFUNCTIONS_ASM_ALPHA_MARCEL_TESTANDSET_H__
#define __INLINEFUNCTIONS_ASM_ALPHA_MARCEL_TESTANDSET_H__


#include "asm-alpha/marcel_testandset.h"


#ifdef __MARCEL_KERNEL__


/** Public inline functions functions **/
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  long unsigned ret, temp;

  __asm__ __volatile__(
	"\n1:\t"
	"ldl_l %0,%3\n\t"
	"bne %0,2f\n\t"
	"or $31,1,%1\n\t"
	"stl_c %1,%2\n\t"
	"beq %1,1b\n"
	"2:\tmb\n"
	: "=&r"(ret), "=&r"(temp), "=m"(*spinlock)
	: "m"(*spinlock)
        : "memory");

  return ret;
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_ASM_ALPHA_MARCEL_TESTANDSET_H__ **/
