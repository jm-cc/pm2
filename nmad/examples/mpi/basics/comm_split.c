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
  int size, rank;
  int color;
  MPI_Comm comm, comm2;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  fprintf(stdout, "a. [%d] Communicator %d with %d nodes\n", rank, MPI_COMM_WORLD, size);

  color = (rank % 2);

  {
    int newsize, newrank;
    MPI_Comm_split(MPI_COMM_WORLD, color, (size-rank)*10, &comm);
    MPI_Comm_size(comm, &newsize);
    MPI_Comm_rank(comm, &newrank);

    fprintf(stdout, "b. [%d] Communicator %d with %d nodes, local rank %d\n", rank, comm, newsize, newrank);

    if (newsize >= 2) {
      if (newrank == 0) {
        int message = 42;
        MPI_Send(&message, 1, MPI_INT, 1, 1, comm);
      }
      else if (newrank == 1) {
        int message;
        MPI_Recv(&message, 1, MPI_INT, 0, 1, comm, MPI_STATUS_IGNORE);
        fprintf(stdout, "c. [%d] Received message: %d\n", rank, message);
      }
    }
  }

  {
    int newsize, newrank;
    color = (newrank % 2);
    MPI_Comm_split(comm, color, rank*10, &comm2);
    MPI_Comm_size(comm2, &newsize);
    MPI_Comm_rank(comm2, &newrank);

    fprintf(stdout, "d. [%d] Communicator %d with %d nodes, local rank %d\n", rank, comm2, newsize, newrank);
  }

  fflush(stdout);

  MPI_Comm_free(&comm);
  MPI_Comm_free(&comm2);
  MPI_Finalize();
  exit(0);
}

