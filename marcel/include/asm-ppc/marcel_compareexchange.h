
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
#ifdef AIX_SYS
		    "Lma%=1"
#else
  		       "1"
#endif
		       ":\n    lwarx %0,0,%2 ;\n"
  		       "      cmpw 0,%0,%3;\n"
  		       "      bne "
#ifdef AIX_SYS
		       "Lma%=2"
#else
		       "2f"
#endif
		       ";\n"
  		       "      stwcx. %4,0,%2;\n"
  		       "      bne- "
#ifdef AIX_SYS
		       "Lma%=1"
#else
		       "1b"
#endif
		       ";\n"
#ifdef MA__LWPS
		       "      sync;\n"
#endif
#ifdef AIX_SYS
		    "Lma%=2"
#undef CMPXCHGBEGIN
#undef CMPXCHGEND
#else
  		       "2"
#endif
		       ":\n"
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
