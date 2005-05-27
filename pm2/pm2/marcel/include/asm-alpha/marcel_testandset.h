
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#section common
#include "tbx_compiler.h"
#section macros
#define MA_HAVE_TESTANDSET 1

#section marcel_functions
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock);
#section marcel_inline
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

#section marcel_macros
#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("mb" : : : "memory"); \
  *spinlock = 0


