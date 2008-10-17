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
  int numtasks, rank;
  int ping_side, rank_dst;
  MPI_Datatype mytype;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int buffer[4] = {1, 2, 3, 4};

    fprintf(stdout, "sending data\n");
    MPI_Send(buffer, 2, mytype, rank_dst, 10, MPI_COMM_WORLD);
    MPI_Type_free(&mytype);
  }
  else {
    MPI_Request request;
    MPI_Status status;
    int buffer[4];

    fprintf(stdout, "receiving data\n");
    MPI_Irecv(buffer, 2, mytype, rank_dst, 10, MPI_COMM_WORLD, &request);
    MPI_Type_free(&mytype);
    MPI_Wait(&request, &status);
    printf("Received data [%d, %d, %d, %d]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  }

  MPI_Finalize();
  exit(0);
}
