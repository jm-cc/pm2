
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
