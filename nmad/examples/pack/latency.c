/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <values.h>

#include "helper.h"

#define LOOPS 20000

static double clock_offset = 0.0;

int main(int argc, char**argv)
{
  nm_pack_cnx_t cnx;
  
  init(&argc, argv);

  /* clock calibration */
  double offset = 100.0;
  int i;
  for(i = 0; i < 1000; i++)
    {
      struct timespec t1, t2;
      clock_gettime(CLOCK_MONOTONIC, &t1);
      clock_gettime(CLOCK_MONOTONIC, &t2);
      const double delay = 1000000.0*(t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
      if(delay < offset)
	offset = delay;
    }
  struct timespec res;
  clock_getres(CLOCK_MONOTONIC, &res);
  clock_offset = offset;

  if (is_server)
    {
      /* server
       */
      for(i = 0; i < LOOPS; i++)
	{
	  nm_begin_unpacking(p_core, gate_id, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);

	  nm_begin_packing(p_core, gate_id, 0, &cnx);
	  nm_pack(&cnx, NULL, 0);
	  nm_end_packing(&cnx);
	}
    }
  else
    {
      /* client
       */
      double min = DBL_MAX;
      for(i = 0; i < LOOPS; i++) 
	{
	  struct timespec t1, t2;
	  clock_gettime(CLOCK_MONOTONIC, &t1);

	  nm_begin_packing(p_core, gate_id, 0, &cnx);
	  nm_pack(&cnx, NULL, 0);
	  nm_end_packing(&cnx);
	  
	  nm_begin_unpacking(p_core, gate_id, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);
	  clock_gettime(CLOCK_MONOTONIC, &t2);
	  const double delay = 1000000.0*(t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1000.0 - clock_offset;
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      
      printf("latency: %9.3lf usec.\n", min);
      
    }
  
  nmad_exit();
  exit(0);
}
