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
#define ITERS 1000

int main(int argc, char **argv)
{
  int rank, commsize;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &commsize);

  char*sbuf = malloc(SIZE);
  char*rbuf = malloc(SIZE);
  memset(sbuf, 0, SIZE);

  MPI_Request*rreqs = malloc(commsize * sizeof(MPI_Request));
  MPI_Request*sreqs = malloc(commsize * sizeof(MPI_Request));
  mpi_bench_tick_t t1, t2;
  mpi_bench_get_tick(&t1);
  int i, j;
  for(i = 0; i < ITERS; i++)
    {
      if(rank ==0)
        fprintf(stderr, "# iter = %d\n", i);
      for(j = 0; j < commsize - 1; j++)
        {
          const int dest = (rank + j) % commsize;
          const int from = (rank - j + commsize) % commsize;
          const int tag = j;

          MPI_Isend(sbuf, SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD, &sreqs[j]);
          MPI_Irecv(rbuf, SIZE, MPI_CHAR, from, tag, MPI_COMM_WORLD, &rreqs[j]);

        }
      for(j = 0; j < commsize; j++)
        {
          MPI_Wait(&sreqs[j], MPI_STATUS_IGNORE);
          MPI_Wait(&rreqs[j], MPI_STATUS_IGNORE);
        }
    }
  mpi_bench_get_tick(&t2);
  const double d = mpi_bench_timing_delay(&t1, &t2);
  if(rank == 0)
    {
      fprintf(stderr, "# time = %g usec. (%g usec. / iter)\n", d, d/(double)ITERS);
    }
  
  MPI_Finalize();
  return 0;
}
