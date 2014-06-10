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

#define COUNT 20
#define MAX_THREADS 128

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

static void*ping_thread(void*_tag)
{
  const int iterations = MAX_THREADS * COUNT;
  int tag = *(int*)_tag;
  int k;
  int small = -1;
  for(k = 0; k < iterations; k++)
    {
      MPI_Status status;
      MPI_Recv(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
      MPI_Send(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD);
    }
  return NULL;
}

int main(int argc, char**argv)
{
  const int requested = MPI_THREAD_MULTIPLE;
  int provided = requested;
  int rc = MPI_Init_thread(&argc, &argv, requested, &provided);
  if(rc != 0)
    {
      fprintf(stderr, "MPI_Init rc = %d\n", rc);
      abort();
    }
  if(provided != requested)
    {
      fprintf(stderr, "MPI_Init_thread- requested = %d; provided = %d\n", MPI_THREAD_MULTIPLE, provided);
      abort();
    }
  int self = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &self);
  peer = 1 - self;
  char small = 1;

  if(self == 0)
    {
      printf("# threads | lat. (usec.)\t| min   \t| max\n");
      double*lats = malloc(sizeof(double) * COUNT * MAX_THREADS);
      int i, k;
      for(i = 1; i <= MAX_THREADS; i++)
	{
	  MPI_Status status;
	  tbx_tick_t t1, t2;
	  const int iterations = i * COUNT;
	  //	  MPI_Barrier(MPI_COMM_WORLD);
	  for(k = 0; k < iterations; k++)
	    {
	      int tag = i;
	      TBX_GET_TICK(t1);
	      MPI_Send(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD);
	      MPI_Recv(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
	      TBX_GET_TICK(t2);
	      lats[k] = TBX_TIMING_DELAY(t1, t2);
	    }
	  qsort(lats, iterations, sizeof(double), &comp_double);
	  printf("%d  \t %8.2f \t %8.2f \t %8.2f\n", i, lats[iterations/2], lats[0], lats[iterations-1]);
	}
      free(lats);
    }
  else
    {
      pthread_t tids[MAX_THREADS];
      int tags[MAX_THREADS];
      int i;
      for(i = 1; i <= MAX_THREADS; i++)
	{
	  tags[i] = i;
	  pthread_create(&tids[i], NULL, &ping_thread, &tags[i]);
	  //	  MPI_Barrier(MPI_COMM_WORLD);
	}
      for(i = 1; i <= MAX_THREADS; i++)
	{
	  pthread_join(tids[i], NULL);
	}
    }

  MPI_Finalize();
  return 0;
}


