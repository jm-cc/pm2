/*! \file tbx_timing.c
 *  \brief TBX timing support routines
 *
 *  This file contains the support functions for the
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
 * Tbx_timing.c
 * ============
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "tbx.h"

static double      scale        = 0.0;
unsigned long long tbx_residual = 0;
tbx_tick_t         tbx_new_event ;
tbx_tick_t         tbx_last_event;

void
tbx_timing_init()
{
  static tbx_tick_t t1, t2;
  int i;

  LOG_IN();
  tbx_residual = (unsigned long long)1 << 32;
  for(i=0; i<5; i++) {
    TBX_GET_TICK(t1);
    TBX_GET_TICK(t2);
    tbx_residual = tbx_min(tbx_residual, TBX_TICK_RAW_DIFF(t1, t2));
  }

#ifdef TBX_TIMING_GETTIMEOFDAY
  scale = 1.0;
#else
  {
    struct timeval tv1,tv2;

    TBX_GET_TICK(t1);
    gettimeofday(&tv1,0);
    usleep(50000);
    TBX_GET_TICK(t2);
    gettimeofday(&tv2,0);
    scale = ((tv2.tv_sec*1e6 + tv2.tv_usec) -
	     (tv1.tv_sec*1e6 + tv1.tv_usec)) /
             (double)(TBX_TICK_DIFF(t1, t2));
  }
#endif

  TBX_GET_TICK(tbx_last_event);
  LOG_OUT();
}

void
tbx_timing_exit()
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

double
tbx_tick2usec(long long t)
{
  return (double)(t)*scale;
}
