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
#include <mpi.h>
#include <tbx.h>
#include <pthread.h>

#include "piom_helper.h"

#define COUNT 100
#define MAX_THREADS 32
#define INTERLEAVE 1

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

static void*ping_thread(void*_tag)
{
  const int iterations = MAX_THREADS * COUNT;
  int tag = *(int*)_tag;
  int k;
  int small = 0;
  while(small != 0xFF)
    {
      nm_sr_request_t request;
      nm_sr_irecv(p_session, p_gate, tag, &small, sizeof(small), &request);
      nm_sr_rwait(p_session, &request);
      nm_sr_isend(p_session, p_gate, tag, &small, sizeof(small), &request);
      nm_sr_swait(p_session, &request);
    }
  return NULL;
}

int main(int argc, char**argv)
{
  nm_examples_init(&argc, argv);

  if(is_server)
    {
      printf("# threads | med. (usec.)\t| min   \t| max\n");
      int small = 1, rsmall = 0;
      double*lats = malloc(sizeof(double) * COUNT);
      int i, j, k;
      for(i = 1; i < MAX_THREADS; i++)
	{
	  tbx_tick_t t1, t2;
	  for(j = 0; j < COUNT; j++)
	    {
	      nm_sr_request_t sreqs[MAX_THREADS];
	      nm_sr_request_t rreqs[MAX_THREADS];
	      TBX_GET_TICK(t1);
	      for(k = 1; k <= i; k++)
		{
		  const int tag = (INTERLEAVE)? k : i;
		  nm_sr_isend(p_session, p_gate, tag, &small, sizeof(small), &sreqs[k]);
		  nm_sr_irecv(p_session, p_gate, tag, &rsmall, sizeof(rsmall), &rreqs[k]);
		}
	      for(k = 1; k <= i; k++)
		{
		  nm_sr_swait(p_session, &sreqs[k]);
		  nm_sr_rwait(p_session, &rreqs[k]);
		}
	      TBX_GET_TICK(t2);
	      lats[j] = TBX_TIMING_DELAY(t1, t2) / 2 / i;
	    }
	  qsort(lats, COUNT, sizeof(double), &comp_double);
	  printf("%d  \t %8.2f \t %8.2f \t %8.2f\n", i, lats[COUNT/2], lats[0], lats[COUNT-2]);
	  nm_examples_barrier(-1);
	}
      free(lats);
      small = 0xFF;
      for(k = 1; k <= i; k++)
	{
	  nm_sr_request_t sreq;
	  nm_sr_isend(p_session, p_gate, k, &small, sizeof(small), &sreq);
	  nm_sr_swait(p_session, &sreq);
	}
    }
  else
    {
      pthread_t tids[MAX_THREADS];
      int tags[MAX_THREADS];
      int i;
      for(i = 1; i < MAX_THREADS; i++)
	{
	  tags[i] = i;
	  int rc = pthread_create(&tids[i], NULL, &ping_thread, &tags[i]);
	  if(rc != 0)
	    abort();
	  nm_examples_barrier(-1);
	}
      for(i = 1; i < MAX_THREADS; i++)
	{
	  pthread_join(tids[i], NULL);
	}
    }

  nm_examples_exit();
  return 0;
}


