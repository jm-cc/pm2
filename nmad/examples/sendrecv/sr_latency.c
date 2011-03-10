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
#include "clock.h"

#define LOOPS   50000

int main(int argc, char**argv)
{
  init(&argc, argv);
  clock_init();

  if (is_server) 
    {
      int k;
      /* server
       */
      for(k = 0; k < LOOPS; k++)
	{
	  nm_sr_request_t request;
	  nm_sr_irecv(p_core, gate_id, 0, NULL, 0, &request);
	  nm_sr_rwait(p_core, &request);
	  nm_sr_isend(p_core, gate_id, 0, NULL, 0, &request);
	  nm_sr_swait(p_core, &request);
	}
    }
  else 
    {
      /* client
       */
      int k;
      double min = DBL_MAX;
      struct timespec t1, t2;
      for(k = 0; k < LOOPS; k++)
	{
	  nm_sr_request_t request;
	  clock_gettime(CLOCK_MONOTONIC, &t1);
	  nm_sr_isend(p_core, gate_id, 0, NULL, 0, &request);
	  nm_sr_swait(p_core, &request);
	  nm_sr_irecv(p_core, gate_id, 0, NULL, 0, &request);
	  nm_sr_rwait(p_core, &request);
	  clock_gettime(CLOCK_MONOTONIC, &t2);
	  const double delay = clock_diff(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      printf("sendrecv latency: %9.3lf usec.\n", min);
    }
  nmad_exit();
  exit(0);
}
