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

______________________________________________________________________________
$Log: tbx_timing.h,v $
Revision 1.2  2000/03/13 09:48:20  oaumage
- ajout de l'option TBX_SAFE_MALLOC
- support de safe_malloc
- extension des macros de logging

Revision 1.1  2000/01/13 14:51:31  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_timing.h
 * ============
 */

#ifndef TBX_TIMING_H
#define TBX_TIMING_H

#include <sys/types.h>
#include <sys/time.h>

typedef union
{
  unsigned long long tick;
  struct
  {
    unsigned long low, high;
  } sub;
  struct timeval timev;
} tbx_tick_t, *p_tbx_tick_t;

/* Hum hum... Here we suppose that X86ARCH => Pentium! */
#ifdef X86_ARCH
#define TBX_GET_TICK(t) __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))
#define TBX_TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)
#else /* X86_ARCH */
#define TBX_GET_TICK(t) gettimeofday(&(t).timev, NULL)
#define TBX_TICK_RAW_DIFF(t1, t2) \
    ((t2.timev.tv_sec * 1000000L + t2.timev.tv_usec) - \
     (t1.timev.tv_sec * 1000000L + t1.timev.tv_usec))
#endif /* X86_ARCH */

#define TBX_TICK_DIFF(t1, t2) (TBX_TICK_RAW_DIFF(t1, t2) - tbx_residual)
#define TBX_TIMING_DELAY(t1, t2) tbx_tick2usec(TBX_TICK_DIFF(t1, t2));

extern long long     tbx_residual;
extern tbx_tick_t    tbx_new_event, tbx_last_event;

#endif /* TBX_TIMING_H */
