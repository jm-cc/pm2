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

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  int rank_dst, ping_side;

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
    int y, x=42;
    float fy, fx=3.1415;
    MPI_Status status;

    MPI_Send(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("[%d] The answer to life, the universe, and everything is %d\n", rank, y);

    MPI_Sendrecv(&fx, 1, MPI_FLOAT, rank_dst, 1,
                 &fy, 1, MPI_FLOAT, rank_dst, 1,
                 MPI_COMM_WORLD, &status);
    printf("[%d] The estimated value of PI is %f\n", rank, fy);
  }
  else {
    int x;
    float f;

    MPI_Recv(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);

    MPI_Recv(&f, 1, MPI_FLOAT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(&f, 1, MPI_FLOAT, rank_dst, 1, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  exit(0);
}

