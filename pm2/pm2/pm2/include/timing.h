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
$Log: timing.h,v $
Revision 1.2  2000/01/31 15:49:50  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef TIMING_EST_DEF
#define TIMING_EST_DEF

#include <sys/types.h>
#include <sys/time.h>

typedef union {
  unsigned long long tick;
  struct {
    unsigned long low, high;
  } sub;
  struct timeval timev;
} Tick;

/* Hum hum... Here we suppose that X86ARCH => Pentium! */
#if defined(X86_ARCH) && !defined(FREEBSD_SYS)

#define GET_TICK(t)           __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))

#define TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)

#else

#define GET_TICK(t)           gettimeofday(&(t).timev, NULL)

#define TICK_RAW_DIFF(t1, t2) ((t2.timev.tv_sec * 1000000L + t2.timev.tv_usec) - \
			       (t1.timev.tv_sec * 1000000L + t1.timev.tv_usec))

#endif

#define TICK_DIFF(t1, t2)     (TICK_RAW_DIFF(t1, t2) - _timing_residual)

#define TIMING_EVENT(name) \
  { GET_TICK(_new_timing_event); timing_stat(name); GET_TICK(_last_timing_event); }

#define SILENT_TIMING_EVENT() \
  GET_TICK(_last_timing_event)

void timing_init(void);

double timing_tick2usec(long long t);

void timing_stat(char *name);

extern long long _timing_residual;

extern Tick _new_timing_event, _last_timing_event;

#endif
