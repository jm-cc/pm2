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
$Log: mad_timing.h,v $
Revision 1.2  1999/12/15 17:31:25  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_timing.h
 * ============
 */

#ifndef MAD_TIMING_H
#define MAD_TIMING_H

#include <sys/types.h>
#include <sys/time.h>

typedef union {
  unsigned long long tick;
  struct {
    unsigned long low, high;
  } sub;
  struct timeval timev;
} mad_tick_t, *p_mad_tick_t;

/* Hum hum... Here we suppose that X86ARCH => Pentium! */
#ifdef X86_ARCH
#define MAD_GET_TICK(t) __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))
#define MAD_TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)
#else /* X86_ARCH */
#define MAD_GET_TICK(t) gettimeofday(&(t).timev, NULL)
#define MAD_TICK_RAW_DIFF(t1, t2) \
    ((t2.timev.tv_sec * 1000000L + t2.timev.tv_usec) - \
     (t1.timev.tv_sec * 1000000L + t1.timev.tv_usec))
#endif /* X86_ARCH */

#define MAD_TICK_DIFF(t1, t2) (MAD_TICK_RAW_DIFF(t1, t2) - mad_residual)
void mad_timing_init(void);
double mad_tick2usec(long long t);
#define MAD_TIMING_DELAY(t1, t2) mad_tick2usec(MAD_TICK_DIFF(t1, t2));

extern long long     mad_residual;
extern mad_tick_t    mad_new_event, mad_last_event;

#endif /* MAD_TIMING_H */
