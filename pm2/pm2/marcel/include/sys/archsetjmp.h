/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef ARCHSETJMP_EST_DEF
#define ARCHSETJMP_EST_DEF

#if defined(ALPHA_ARCH)

typedef long jmp_buf[32];

extern void longjmp(jmp_buf buf, int val);
extern int setjmp(jmp_buf buf);

#else

#include <setjmp.h>

#endif


#if defined(RS6K_ARCH)

_PRIVATE_ extern int _jmg(int r);
_PRIVATE_ extern void LONGJMP(jmp_buf buf, int val);
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

static __inline__ void my_longjmp(my_jmp_buf buf, int val)
{
  __asm__ __volatile__("movl %0, %%ecx\n\t"
		       "movl 0(%%ecx), %%ebx\n\t"
		       "movl 4(%%ecx), %%esi\n\t"
		       "movl 8(%%ecx), %%edi\n\t"
		       "movl 12(%%ecx), %%ebp\n\t"
		       "movl 16(%%ecx), %%esp\n\t"
		       "movl 20(%%ecx), %%ecx\n\t"
		       "jmp *%%ecx"
		       : : "g" (buf), "ax" (val) : "memory");
}

#define jmp_buf my_jmp_buf
#undef setjmp
#define setjmp my_setjmp
#undef longjmp
#define longjmp my_longjmp

#endif

#endif
