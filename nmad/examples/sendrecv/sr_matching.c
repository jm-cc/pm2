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

#include "sr_examples_helper.h"

#define COUNT 100
#define MAX_REQS 32768

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

  if(is_server)
    {
      printf("# reqs  | lat. (usec.)\t| min   \t| max\n");
      double*lats = malloc(sizeof(double) * COUNT);
      int i, k, r;
      for(i = 1; i < MAX_REQS; i = i*1.1 + 1)
	{
	  nm_sr_request_t sreqs[i], rreqs[i];
	  tbx_tick_t t1, t2;
	  nm_examples_barrier(-1);
	  for(k = 0; k < COUNT; k++)
	    {
	      TBX_GET_TICK(t1);
	      for(r = 0; r < i; r++)
		{
		  int tag = r;
		  nm_sr_isend(p_session, p_gate, tag, &small, 1, &sreqs[r]);
		  nm_sr_irecv(p_session, p_gate, tag, &small, 1, &rreqs[r]);
		}
	      for(r = 0; r < i; r++)
		{
		  nm_sr_swait(p_session, &sreqs[r]);
		  nm_sr_rwait(p_session, &rreqs[r]);
		}
	      TBX_GET_TICK(t2);
	      lats[k] = TBX_TIMING_DELAY(t1, t2) / i;
	    }
	  qsort(lats, COUNT, sizeof(double), &comp_double);
	  printf("%d  \t %8.2f \t %8.2f \t %8.2f\n", i, lats[COUNT/2], lats[0], lats[COUNT-2]);
	}
      free(lats);
    }
  else
    {
      int i, k, r;
      for(i = 1; i < MAX_REQS; i = i*1.1 + 1)
	{
	  nm_sr_request_t sreqs[i], rreqs[i];
	  nm_examples_barrier(-1);
	  for(k = 0; k < COUNT; k++)
	    {
	      for(r = 0; r < i; r++)
		{
		  int tag = r;
		  nm_sr_irecv(p_session, p_gate, tag, &small, 1, &rreqs[r]);
		}
	      for(r = 0; r < i; r++)
		{
		  int tag = r;
		  nm_sr_rwait(p_session, &rreqs[r]);
		  nm_sr_isend(p_session, p_gate, tag, &small, 1, &sreqs[r]);
		}
	      for(r = 0; r < i; r++)
		{
		  nm_sr_swait(p_session, &sreqs[r]);
		}
	    }
	}
    }

  nm_examples_exit();
  return 0;
}


