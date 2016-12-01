/*! \file tbx_timing.h
 *  \brief TBX timing data structures and macros
 *
 *  This file contains the support data structures and macros for the
 *  TBX timing facilities.
 * 
 */

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

/*
 * Tbx_timing.h
 * ============
 */


#ifndef TBX_TIMING_H
#define TBX_TIMING_H

#ifndef TBX
#  error TBX is undefined. Please build with flags given by pkg-config --cflags tbx
#endif /* TBX */


#if defined(MCKERNEL)
#define TBX_USE_RDTSC
#undef TBX_USE_CLOCK_GETTIME
#endif

#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

/** \defgroup timing_interface timing interface
 *
 * This is the timing interface
 *
 * @{
 */


#ifdef TBX_USE_CLOCK_GETTIME


typedef struct timespec tbx_tick_t, *p_tbx_tick_t;

#ifdef CLOCK_MONOTONIC_RAW
#  define CLOCK_TYPE CLOCK_MONOTONIC_RAW
#else
#  define CLOCK_TYPE CLOCK_MONOTONIC
#endif

#define TBX_GET_TICK(t)				\
	clock_gettime(CLOCK_TYPE, &(t))
#define TBX_TICK_RAW_DIFF(start, end)		\
        tbx_tick_raw_diff(&(start), &(end))

static inline tbx_tick_t tbx_tick_raw_diff(const tbx_tick_t*const t1, const tbx_tick_t*const t2)
{
  tbx_tick_t diff;
  if(t2->tv_nsec > t1->tv_nsec)
    {
      diff.tv_sec  = t2->tv_sec - t1->tv_sec;
      diff.tv_nsec = t2->tv_nsec - t1->tv_nsec;
    }
  else
    {
      diff.tv_sec  = t2->tv_sec - t1->tv_sec - 1;
      diff.tv_nsec = t2->tv_nsec - t1->tv_nsec + 1000000000L;
    }
  return diff;
}

#else
#ifdef TBX_USE_MACH_ABSOLUTE_TIME
#  include <mach/mach_time.h>


typedef long long tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t)				\
	t = mach_absolute_time()
#define TBX_TICK_RAW_DIFF(start, end)		\
	((end) - (start))


#else
#ifdef TBX_USE_RDTSC

typedef unsigned long long tbx_tick_t, *p_tbx_tick_t;
#ifdef X86_ARCH
#define TBX_GET_TICK(t) \
  __asm__ volatile("rdtsc" : "=A" (t))
#else
#define TBX_GET_TICK(t) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (t) = ((unsigned long)__a) | (((unsigned long)__d)<<32); }	\
  while(0)
#endif
#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2) - (t1))


#else

typedef struct timeval tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t)				\
	gettimeofday(&(t), NULL)
#define TBX_TICK_RAW_DIFF(start, end)		\
        tbx_tick_raw_diff(&(start), &(end))

static inline tbx_tick_t tbx_tick_raw_diff(const tbx_tick_t*const t1, const tbx_tick_t*const t2)
{
  tbx_tick_t diff;
  if(t2->tv_usec > t1->tv_usec)
    {
      diff.tv_sec  = t2->tv_sec - t1->tv_sec;
      diff.tv_usec = t2->tv_usec - t1->tv_usec;
    }
  else
    {
      diff.tv_sec  = t2->tv_sec - t1->tv_sec - 1;
      diff.tv_usec = t2->tv_usec - t1->tv_usec + 1000000L;
    }
  return diff;
}

#endif /* TBX_USE_RDTSC */
#endif /** TBX_USE_MACH_ABSOLUTE_TIME **/
#endif /** TBX_USE_CLOCK_GETTIME **/

extern double tbx_ticks2delay(const tbx_tick_t *t1, const tbx_tick_t *t2);

extern double tbx_tick2usec(tbx_tick_t t);

/** compute delay between to ticks, result given in microsecond as a double
 */
#define TBX_TIMING_DELAY(start, end) tbx_ticks2delay(&(start), &(end))

/* @} */


#endif				/* TBX_TIMING_H */
