/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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

#include <stdio.h>
#include <stdlib.h>
#include <tbx.h>
#include <pthread.h>

#include "../common/nm_examples_helper.h"

#define ITERATIONS 5
#define MAX_REQS 200000

int peer = -1;

static int comp_double(const void*_a, const void*_b)
{
  const double*a = _a;
  const double*b = _b;
  if(*a < *b)
    return -1;
  else if(*a > *b)
    return 1;
  else
    return 0;
}

int main(int argc, char**argv)
{
  nm_examples_init_topo(&argc, argv, NM_EXAMPLES_TOPO_PAIRS);

  char small = 1;
  nm_sr_request_t*sreqs = malloc(MAX_REQS * sizeof(nm_sr_request_t));
  nm_sr_request_t*rreqs = malloc(MAX_REQS * sizeof(nm_sr_request_t));

  if(is_server)
    {
      printf("# reqs  |\t min lat |\t med |\t \t d1  |\t d9  \n");
      int burst, k, r;
      for(burst = 1; burst < MAX_REQS; burst = burst * 1.1 + 1)
	{
	  tbx_tick_t t1, t2;
	  const int iterations = (ITERATIONS * MAX_REQS) / burst;
	  double*lats = malloc(sizeof(double) * iterations);
	  nm_examples_barrier(-1);
	  for(k = 0; k < iterations; k++)
	    {
	      TBX_GET_TICK(t1);
	      for(r = 0; r < burst; r++)
		{
		  nm_tag_t tag = burst - 1 - r;
		  nm_sr_isend(p_session, p_gate, tag, &small, sizeof(small), &sreqs[r]);
		  nm_sr_irecv(p_session, p_gate, tag, &small, sizeof(small), &rreqs[r]);
		}
	      for(r = 0; r < burst; r++)
		{
		  nm_sr_swait(p_session, &sreqs[r]);
		  nm_sr_rwait(p_session, &rreqs[r]);
		}
	      TBX_GET_TICK(t2);
	      lats[k] = TBX_TIMING_DELAY(t1, t2) / burst;
	    }
	  qsort(lats, iterations, sizeof(double), &comp_double);
	  const double min_lat = lats[0];
          /*  const double max_lat = lats[iterations - 1]; */
	  const double med_lat = lats[(iterations - 1) / 2];
	  const double d1_lat  = lats[(iterations - 1) / 10];
	  const double d9_lat  = lats[ 9 *(iterations - 1) / 10];
	  printf("%d \t %8.03f \t %8.03f \t %8.03f \t %8.03f \n", burst, min_lat, med_lat, d1_lat, d9_lat);
	  free(lats);
	}
    }
  else
    {
      int burst, k, r;
      for(burst = 1; burst < MAX_REQS; burst = burst * 1.1 + 1)
	{
	  const int iterations = (ITERATIONS * MAX_REQS) / burst;
	  nm_examples_barrier(-1);
	  for(k = 0; k < iterations; k++)
	    {
	      for(r = 0; r < burst; r++)
		{
		  int tag = r;
		  nm_sr_irecv(p_session, p_gate, tag, &small, 1, &rreqs[r]);
		}
	      for(r = 0; r < burst; r++)
		{
		  nm_sr_rwait(p_session, &rreqs[r]);
		}
	      for(r = 0; r < burst; r++)
		{
		  int tag = r;
		  nm_sr_isend(p_session, p_gate, tag, &small, 1, &sreqs[r]);
		}
	      for(r = 0; r < burst; r++)
		{
		  nm_sr_swait(p_session, &sreqs[r]);
		}
	    }
	}
    }

  free(sreqs);
  free(rreqs);

  nm_examples_exit();
  return 0;
}


