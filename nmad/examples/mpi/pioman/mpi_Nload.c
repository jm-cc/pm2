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

#define BUFSIZE (1024*1024)
#define COUNT 20
#define MAX_THREADS 128

volatile double r = 1.0;
volatile int compute_stop = 0;

void*do_compute(void*_dummy)
{
  while(!compute_stop)
    {
      int k;
      for(k = 0; k < 10; k++)
	{
	  r *= 2.213890;
	}
    }
  return NULL;
}

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
  const int requested = MPI_THREAD_FUNNELED;
  int provided = requested;
  int rc = MPI_Init_thread(&argc, &argv, requested, &provided);
  if(rc != 0)
    {
      fprintf(stderr, "MPI_Init rc = %d\n", rc);
      abort();
    }
  if(provided < requested)
    {  
      fprintf(stderr, "MPI_Init_thread- requested = %d; provided = %d\n", MPI_THREAD_MULTIPLE, provided);
      abort();
    }
  int self = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &self);
  const int peer = 1 - self;
  char*buffer = malloc(BUFSIZE);
  
  pthread_t tids[MAX_THREADS];
  int i, k;
  printf("# message size %d bytes, N[0..%d] load thread\n", BUFSIZE, MAX_THREADS);
  printf("# threads | lat. (usec.)\t| min   \t| max \t| avg\n");
  for(i = 0; i <= MAX_THREADS; i++)
    {
      const int iterations = COUNT;
      MPI_Barrier(MPI_COMM_WORLD);
      if(self == 0)
	{
	  double*lats = malloc(sizeof(double) * COUNT);
	  for(k = 0; k < iterations; k++)
	    {
	      MPI_Status status;
	      tbx_tick_t t1, t2;
	      TBX_GET_TICK(t1);
	      MPI_Send(buffer, BUFSIZE, MPI_CHAR, peer, 1, MPI_COMM_WORLD);
	      MPI_Recv(buffer, BUFSIZE, MPI_CHAR, peer, 1, MPI_COMM_WORLD, &status);
	      TBX_GET_TICK(t2);
	      lats[k] = TBX_TIMING_DELAY(t1, t2) / 2;
	    }
	  qsort(lats, iterations, sizeof(double), &comp_double);
	  int n;
	  double sum = 0.0;
	  for(n = 0; n < iterations; n++)
	    sum += lats[n];
	  double avg = sum / (double)iterations;
	  printf("%d  \t %8.2f \t %8.2f \t %8.2f \t %8.2f\n", i, lats[iterations/2], lats[0], lats[iterations-1], avg);
	  fflush(stdout);
	  free(lats);
	}
      else
	{
	  for(k = 0; k < iterations; k++)
	    {
	      MPI_Status status;
	      MPI_Recv(buffer, BUFSIZE, MPI_CHAR, peer, 1, MPI_COMM_WORLD, &status);
	      MPI_Send(buffer, BUFSIZE, MPI_CHAR, peer, 1, MPI_COMM_WORLD);
	    }
	}
      pthread_create(&tids[i], NULL, &do_compute, NULL);
    }

  compute_stop = 1;
  for(i = 1; i <= MAX_THREADS; i++)
    {
      pthread_join(tids[i], NULL);
    }

  MPI_Finalize();
  return 0;
}


