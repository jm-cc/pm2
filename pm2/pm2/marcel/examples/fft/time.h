/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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

#ifdef PENTIUM_TIMING
#define GET_TICK(t)           __asm__ volatile("rdtsc" : "=a" ((t).sub.low), "=d" ((t).sub.high))
#else
#define GET_TICK(t)           gettimeofday(&(t).timev, NULL)
#endif

#define TIMING_EVENT(name) \
   { Tick _t; GET_TICK(_t); timing_stat(name, _t); GET_TICK(_last_timing_event); }

extern double microsec_time;

void timing_init(void);

double timing_tick2usec(long long t);

void timing_stat(char *name, Tick t);

extern long long _timing_residual;

extern Tick _last_timing_event;

#define GET_MILISECONDS(t) \
{ Tick _t; gettimeofday(&(_t).timev, NULL); t = (unsigned long)((_t.timev.tv_sec * 1000000L + _t.timev.tv_usec)/1000);}

#define GET_MICROSECONDS(t) \
{ Tick _t; gettimeofday(&(_t).timev, NULL); t = (unsigned long)(_t.timev.tv_sec * 1000000L + _t.timev.tv_usec);}

#endif
