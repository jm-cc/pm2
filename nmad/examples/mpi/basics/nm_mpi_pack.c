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
    int x=42;
    float fx[2];
    char buffer[110];
    int position=0;

    fx[0] = 3.1415;
    fx[1] = 12.34;
    MPI_Pack(&x, 1, MPI_INT, buffer, 110, &position, MPI_COMM_WORLD);
    MPI_Pack(fx, 2, MPI_FLOAT, buffer, 110, &position, MPI_COMM_WORLD);
    MPI_Send(buffer, position, MPI_PACKED, rank_dst, 1, MPI_COMM_WORLD);
  }
  else {
    int x;
    float fx[2];
    char buffer[110];
    int position=0;

    MPI_Recv(buffer, 110, MPI_PACKED, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Unpack(buffer, 110, &position, &x, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(buffer, 110, &position, &fx, 2, MPI_FLOAT, MPI_COMM_WORLD);

    printf("[%d] The answer to life, the universe, and everything is %d\n", rank, x);
    printf("[%d] The estimated value of PI is %3.6f\n", rank, fx[0]);
    printf("[%d] Second value %3.6f\n", rank, fx[1]);
  }

  MPI_Finalize();
  exit(0);
}

