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
#include <unistd.h>

#if !defined(MAD_MPI)
#  define MPI_Esend(a, b, c, d, e, f, g, h) MPI_Isend(a, b, c, d, e, g, h)
#endif

int main(int argc, char **argv) {
  int i, flag, numtasks, rank;
  int ping_side, rank_dst;
  MPI_Request requests[4*1024+1];
  int *x=calloc(1024*sizeof(int), 1);

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

  if (ping_side) {
    // ping
    for(i=0 ; i<4*1024 ; i++) {
      MPI_Isend(x, 1024, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &requests[i]);
      MPI_Test(&requests[i], &flag, MPI_STATUS_IGNORE);
    }
    MPI_Isend(x, 1024, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &requests[4*1024]);

    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }

    // pong
    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Recv(x, 1024, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  }
  else {
    // pong
    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Recv(x, 1024, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // ping
    for(i=0 ; i<4*1024 ; i++) {
      MPI_Esend(x, 1024, MPI_INT, rank_dst, 1, 0, MPI_COMM_WORLD, &requests[i]);
    }
    MPI_Esend(x, 1024, MPI_INT, rank_dst, 1, 1, MPI_COMM_WORLD, &requests[4*1024]);

    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    }
  }

  free(x);
  MPI_Finalize();
  exit(0);
}

