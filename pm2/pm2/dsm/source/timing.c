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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <timing.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#ifdef PENTIUM_TIMING

#define TICK_RAW_DIFF(t1, t2) ((t2).tick - (t1).tick)

#else

#define TICK_RAW_DIFF(t1, t2) ((t2.timev.tv_sec * 1000000L + t2.timev.tv_usec) - \
			       (t1.timev.tv_sec * 1000000L + t1.timev.tv_usec))

#endif

#define TICK_DIFF(t1, t2)     (TICK_RAW_DIFF(t1, t2) - _timing_residual)

static double scale = 0.0;
double microsec_time;
long long _timing_residual;

static Tick t1, t2;

Tick _last_timing_event;

void timing_init()
{
  int i;

  _timing_residual = (unsigned long long)1 << 32;
  for(i=0; i<5; i++) {
    GET_TICK(t1);
    GET_TICK(t2);
    _timing_residual = min(_timing_residual, TICK_RAW_DIFF(t1, t2));
  }

#ifdef PENTIUM_TIMING
  {
    struct timeval tv1,tv2;

    GET_TICK(t1);
    gettimeofday(&tv1,0);
    usleep(100000);
    GET_TICK(t2);
    gettimeofday(&tv2,0);
    scale = ((tv2.tv_sec*1e6 + tv2.tv_usec) - (tv1.tv_sec*1e6 + tv1.tv_usec))
      / (double)(TICK_DIFF(t1, t2));
  }
#else
  scale = 1.0;
#endif

  GET_TICK(_last_timing_event);
}

double timing_tick2usec(long long t)
{
  return (double)(t)*scale;
}

void timing_stat(char *name, Tick t)
{
  fprintf(stderr, "EVT<%s> at +%lld ticks (+%f usecs)\n",
	  name, TICK_DIFF(_last_timing_event, t),
	  microsec_time = timing_tick2usec(TICK_DIFF(_last_timing_event, t)));
}

