
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

#section marcel_macros
#define sync() __asm__ __volatile__ ("sync")

#section marcel_functions
static __inline__ unsigned __compare_and_swap (long unsigned *p, long unsigned oldval, long unsigned newval);
#section marcel_inline
static __inline__ unsigned __compare_and_swap (long unsigned *p, long unsigned oldval, long unsigned newval)
{
  unsigned ret;

  sync();
  __asm__ __volatile__(
		       "0:    lwarx %0,0,%1 ;"
		       "      xor. %0,%3,%0;"
		       "      bne 1f;"
		       "      stwcx. %2,0,%1;"
		       "      bne- 0b;"
		       "1:    "
	: "=&r"(ret)
	: "r"(p), "r"(newval), "r"(oldval)
	: "cr0", "memory");
  sync();
  return ret == 0;
}

#section marcel_macros
#define pm2_spinlock_testandset(spinlock) __compare_and_swap(spinlock, 0, 1)

#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)
