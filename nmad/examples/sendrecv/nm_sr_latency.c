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

#include "../common/nm_examples_helper.h"

#define LOOPS   200000

int main(int argc, char**argv)
{
  nm_examples_init(&argc, argv);

  if (is_server) 
    {
      int k;
      /* server
       */
      for(k = 0; k < LOOPS; k++)
	{
	  nm_sr_request_t request;
	  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
	  nm_sr_rwait(p_session, &request);
	  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
	  nm_sr_swait(p_session, &request);
	}
    }
  else 
    {
      /* client
       */
      int k;
      double min = DBL_MAX;
      tbx_tick_t t1, t2;
      for(k = 0; k < LOOPS; k++)
	{
	  nm_sr_request_t request;
	  TBX_GET_TICK(t1);
	  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
	  nm_sr_swait(p_session, &request);
	  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
	  nm_sr_rwait(p_session, &request);
	  TBX_GET_TICK(t2);
	  const double delay = TBX_TIMING_DELAY(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      printf("sendrecv latency: %9.3lf usec.\n", min);
    }
  nm_examples_exit();
  exit(0);
}
