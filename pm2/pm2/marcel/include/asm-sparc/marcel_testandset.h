
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

#section marcel_functions
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock);
#section marcel_inline
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  char ret = 0;

  __asm__ __volatile__("ldstub [%0], %1"
	: "=r"(spinlock), "=r"(ret)
	: "0"(spinlock), "1" (ret) : "memory");

  return (unsigned)ret;
}

#section marcel_macros
#define pm2_spinlock_release(spinlock) \
  __asm__ __volatile__("stbar\n\tstb %1,%0" : "=m"(*(spinlock)) : "r"(0));

