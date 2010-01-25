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


#ifndef __ASM_MIPS_MARCEL_TESTANDSET_H__
#define __ASM_MIPS_MARCEL_TESTANDSET_H__


#include "tbx_compiler.h"
#include "asm/marcel_testandset.h"


#ifdef __MARCEL_KERNEL__


/** Public inline functions functions **/
static __tbx_inline__ long unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  long unsigned ret, temp;

  __asm__ __volatile__(
        ".set\tmips2\n"
        "1:\tll\t%0,0(%2)\n\t"
        "bnez\t%0,2f\n\t"
        ".set\tnoreorder\n\t"
        "li\t%1,1\n\t"
        "sc\t%1,0(%2)\n\t"
        ".set\treorder\n\t"
        "beqz\t%1,1b\n\t"
        "2:\t.set\tmips0\n\t"
        : "=&r"(ret), "=&r" (temp)
        : "r"(spinlock)
        : "memory");

  return ret;
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_MIPS_MARCEL_TESTANDSET_H__ **/
