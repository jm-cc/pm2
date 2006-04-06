
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
#define MA_HAVE_COMPAREEXCHANGE 1

#section marcel_variables
#section marcel_macros
#ifdef MA__LWPS
#define MA_EIEIO_ON_SMP "eieio;"
#define MA_ISYNC_ON_SMP "isync;"
#else
#define MA_EIEIO_ON_SMP
#define MA_ISYNC_ON_SMP
#endif

#section marcel_functions
static __tbx_inline__ unsigned long pm2_compareexchange (volatile void *ptr, unsigned long old, unsigned long repl, int size);
#section marcel_inline
#include <stdlib.h>
static __tbx_inline__ unsigned long pm2_compareexchange (volatile void *ptr, unsigned long old, unsigned long repl, int size)
{
  unsigned long prev;

  if (size == 4) {
    volatile int *p = ptr;
    __asm__ __volatile__(
		       MA_EIEIO_ON_SMP
  		       "1:    lwarx %0,0,%2 ;"
  		       "      cmpw 0,%0,%3;"
  		       "      bne- 2f;"
  		       "      stwcx. %4,0,%2;"
  		       "      bne- 1b;"
		       MA_ISYNC_ON_SMP
  		       "2:    "
  	: "=&r"(prev), "=m" (*p)
  	: "r"(p), "r" (old), "r"(repl), "m" (*p)
  	: "cc", "memory");
  } else if (size == 8) {
    volatile long *p = ptr;
    __asm__ __volatile__(
		       MA_EIEIO_ON_SMP
  		       "1:    ldarx %0,0,%2 ;"
  		       "      cmpd 0,%0,%3;"
  		       "      bne- 2f;"
  		       "      stdcx. %4,0,%2;"
  		       "      bne- 1b;"
		       MA_ISYNC_ON_SMP
  		       "2:    "
  	: "=&r"(prev), "=m" (*p)
  	: "r"(p), "r" (old), "r"(repl), "m" (*p)
  	: "cc", "memory");
  } else {
    abort();
  }
  return prev;
}

#section marcel_macros
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
