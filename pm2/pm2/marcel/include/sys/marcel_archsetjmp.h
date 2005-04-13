
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

#ifndef ARCHSETJMP_EST_DEF
#define ARCHSETJMP_EST_DEF

#include <setjmp.h>
#include <tbx_compiler.h>

#if defined(ALPHA_ARCH) && defined(LINUX_SYS)

#ifdef setjmp
#undef setjmp
#endif

#define setjmp(env) __sigsetjmp ((env), 0)

#endif

#if defined(RS6K_ARCH)

_PRIVATE_ extern int _jmg(int r);
_PRIVATE_ extern TBX_NORETURN void LONGJMP(jmp_buf buf, int val);
#ifdef setjmp
#undef setjmp
#endif

#ifdef longjmp
#undef longjmp
#endif

#define setjmp(buf)		_jmg(setjmp(buf))
#define longjmp(buf, v)		LONGJMP(buf, v)

#endif

#if defined(X86_ARCH)

#define MARCEL_JB_BX   0
#define MARCEL_JB_SI   1
#define MARCEL_JB_DI   2
#define MARCEL_JB_BP   3
#define MARCEL_JB_SP   4
#define MARCEL_JB_PC   5

typedef int my_jmp_buf[6];

extern int my_setjmp(my_jmp_buf buf);

static __inline__ void TBX_NORETURN TBX_UNUSED my_longjmp(my_jmp_buf buf, int val);

static __inline__ void my_longjmp(my_jmp_buf buf, int val)
{
  __asm__ __volatile__ (
		       "movl 0(%0), %%ebx\n\t"
		       "movl 4(%0), %%esi\n\t"
		       "movl 8(%0), %%edi\n\t"
		       "movl 12(%0), %%ebp\n\t"
		       "movl 16(%0), %%esp\n\t"
		       "movl 20(%0), %0\n\t"
		       "jmp *%0"
		       : : "c,d" (buf), "a" (val));
  // to make gcc believe us that the above statement doesn't return
  for(;;);
}

#define jmp_buf my_jmp_buf
#undef setjmp
#define setjmp my_setjmp
#undef longjmp
#define longjmp my_longjmp
#endif

#endif
