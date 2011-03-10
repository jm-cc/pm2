/*
 * NewMadeleine
 * Copyright (C) 2011
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
#include <string.h>
#include <unistd.h>
#include <values.h>

#include <mpi.h>
#include "../../sendrecv/clock.h"

#define LOOPS   50000

int main(int argc, char**argv)
{
  MPI_Init(&argc, &argv);
  clock_init();

  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(size % 2 != 0)
    {
      fprintf(stderr, "need an even number of nodes (size = %d).\n", size);
      abort();
    }
  const int peer = size - rank - 1;
  const int is_server = (rank > peer);
  MPI_Barrier(MPI_COMM_WORLD);
  if (is_server) 
    {
      int k;
      /* server
       */
      for(k = 0; k < LOOPS; k++)
	{
	  MPI_Recv(NULL, 0, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	  MPI_Send(NULL, 0, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
	}
    }
  else 
    {
      /* client
       */
      int k;
      double min = DBL_MAX;
      for(k = 0; k < LOOPS; k++)
	{
	  struct timespec t1, t2;
	  clock_gettime(CLOCK_MONOTONIC, &t1);
	  MPI_Send(NULL, 0, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
	  MPI_Recv(NULL, 0, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	  clock_gettime(CLOCK_MONOTONIC, &t2);
	  const double delay = clock_diff(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      printf("MPI latency:      %9.3lf usec.\n", min);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  exit(0);
}

