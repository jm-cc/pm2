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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  int ping_side, rank_dst;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side)
    {
      int *x = malloc(1024*1024*sizeof(int));
      int y = 7;
      memset(x, 0, 1024*1024*sizeof(int));
      x[0] = 42;
      x[1024*1024-1] = 42;
      MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
      MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Rsend(x, 1024*1024, MPI_INT, rank_dst, 2, MPI_COMM_WORLD);
      free(x);
    }
  else
    {
      MPI_Request request;
      int *x, y;
      x = malloc(1024*1024*sizeof(int));
      memset(x, 0, 1024*1024*sizeof(int));
      MPI_Irecv(x, 1024*1024, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request);
      MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      printf("The answer to life, the universe, and everything is %d, %d\n", x[0], x[1024*1024-1]);
      printf("There are %d Wonders of the World\n", y);
      free(x);
    }
  MPI_Finalize();
  exit(0);
}

