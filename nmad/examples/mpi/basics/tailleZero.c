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

int main(int argc, char **argv) {
  int numtasks, rank, tag=1;
  int ping_side, rank_dst;
  int counter,  message;
  MPI_Status stat;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  //printf("Rank %d Size %d\n", rank, numtasks);

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    message=-1;
    counter=1;
    MPI_Send(&counter, 0, MPI_INT, rank_dst, tag, MPI_COMM_WORLD);
    MPI_Recv(&message, 0, MPI_INT, rank_dst, tag, MPI_COMM_WORLD, &stat);
    fprintf(stdout, "Received %d\n", message);
  }
  else {
    MPI_Recv(&message, 0, MPI_INT, rank_dst, tag, MPI_COMM_WORLD, &stat);
    MPI_Send(&message, 0, MPI_INT, rank_dst, tag, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  exit(0);
}
