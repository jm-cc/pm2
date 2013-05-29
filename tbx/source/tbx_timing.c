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
#include <time.h>
#include <unistd.h>
#include "tbx.h"

/** converts a tick diff to microseconds */
static double tbx_tick2usec_raw(tbx_tick_t t)
{
  double raw_delay;
  
#if defined(TBX_USE_MACH_ABSOLUTE_TIME)
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  raw_delay = ((double)t * info.numer / info.denom / 1000.0);
#elif defined(TBX_USE_CLOCK_GETTIME)
  raw_delay = 1000000.0 * t.tv_sec + t.tv_nsec / 1000.0;
#elif defined(TBX_USE_RDTSC)
  static double scale = -1.0;
  if(scale < 0)
    {  
      tbx_tick_t t1, t2;
      struct timespec ts = { 0, 10000000 };
      TBX_GET_TICK(t1);
      nanosleep(&ts, NULL);
      TBX_GET_TICK(t2);
      scale = (ts.tv_sec * 1e6 + ts.tv_nsec / 1e3) / (double)(TBX_TICK_RAW_DIFF(t1, t2));
    }
  raw_delay = (t * scale);
#else
  raw_delay = 1000000.0 *t.tv_sec + t.tv_usec;
#endif
  
  return raw_delay;
}

/** converts a tick diff to microseconds, with offset compensation */
double tbx_tick2usec(tbx_tick_t t)
{
	static double clock_offset = -1.0;
	double delay;

	if (clock_offset < 0) {
		/* lazy clock offset init */
		int i;

		clock_offset = 1000.0;
		for (i = 0; i < 1000; i++) {
			tbx_tick_t t1, t2;
			double raw_delay;

			TBX_GET_TICK(t1);
			TBX_GET_TICK(t2);
			raw_delay = tbx_tick2usec_raw(TBX_TICK_RAW_DIFF(t1, t2));
			if (raw_delay < clock_offset)
				clock_offset = raw_delay;
		}
	}
	
	delay = tbx_tick2usec_raw(t) - clock_offset;
	return (delay > 0.0) ? delay : 0.0; /* tolerate slight offset inacurracy */
}

/** computes a delay between ticks, in microseconds */
double tbx_ticks2delay(const tbx_tick_t*t1, const tbx_tick_t *t2)
{
	tbx_tick_t tick_diff = TBX_TICK_RAW_DIFF(*t1, *t2);
	return tbx_tick2usec(tick_diff);
}
