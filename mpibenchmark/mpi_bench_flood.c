/*
 * NewMadeleine
 * Copyright (C) 2006-2018 (see AUTHORS file)
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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mpi_bench_generic.h"

#define SIZE (400 * 1024)
#define ITERS 10

int main(int argc, char **argv)
{
  int rank, commsize;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &commsize);

  char*sbuf = malloc(SIZE);
  char*rbuf = malloc(SIZE);
  memset(sbuf, 0, SIZE);

  if(rank == 0)
    {
      fprintf(stderr, "# commsize | time/iter (usec.) | bw \n");
    }
  
  MPI_Request*rreqs = malloc(commsize * sizeof(MPI_Request));
  MPI_Request*sreqs = malloc(commsize * sizeof(MPI_Request));
  mpi_bench_tick_t t1, t2;
  int c, i, j;
  for(c = 2 ; c < commsize; c++)
    {
      if(rank < c)
	{
	  mpi_bench_get_tick(&t1);
	  for(i = 0; i < ITERS; i++)
	    {
	      for(j = 0; j < c - 1; j++)
		{
		  const int dest = (rank + j) % c;
		  const int from = (rank - j + c) % c;
		  const int tag = j;
		  
		  MPI_Isend(sbuf, SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD, &sreqs[j]);
		  MPI_Irecv(rbuf, SIZE, MPI_CHAR, from, tag, MPI_COMM_WORLD, &rreqs[j]);
		  
		}
	      for(j = 0; j < c - 1; j++)
		{
		  MPI_Wait(&sreqs[j], MPI_STATUS_IGNORE);
		  MPI_Wait(&rreqs[j], MPI_STATUS_IGNORE);
		}
	      mpi_bench_get_tick(&t2);
	    }
	  const double d = mpi_bench_timing_delay(&t1, &t2);
	  if(rank == 0)
	    {
	      const double d_iter = d / (double)ITERS;
	      const double bw = (SIZE * (c - 1) / (d_iter));
	      fprintf(stderr, "%8d       %9.2lf     %9.2lf\n", c, d_iter, bw);
	    }
	}
    }
  
  MPI_Finalize();
  return 0;
}
