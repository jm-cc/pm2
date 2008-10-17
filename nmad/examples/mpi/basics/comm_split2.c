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
  int size, rank;
  int newsize, newrank;
  int color, key;
  MPI_Comm comm;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size != 8) {
    fprintf(stdout, "This application needs 8 nodes\n");
    MPI_Finalize();
    exit(1);
  }

  //fprintf(stdout, "[%d] Communicator %d with %d nodes\n", rank, MPI_COMM_WORLD, size);

  if (rank == 0 || rank == 3 || rank == 5 || rank == 6)
    color = 0;
  else if (rank == 7)
    color = 5;
  else if (rank == 1)
    color = MPI_UNDEFINED;
  else
    color = 3;

  if (rank == 1 || rank == 4 || rank == 5 || rank == 6)
    key = 1;
  else if (rank == 0)
    key = 3;
  else if (rank == 2)
    key = 2;
  else if (rank == 3)
    key = 5;
  else
    key = 2;

  MPI_Comm_split(MPI_COMM_WORLD, color, key, &comm);
  if (comm != MPI_COMM_NULL) {
    MPI_Comm_size(comm, &newsize);
    MPI_Comm_rank(comm, &newrank);

    fprintf(stdout, "[%d] Communicator %d with %d nodes, local rank %d\n", rank, comm, newsize, newrank);

    MPI_Comm_free(&comm);
  }
  else {
    fprintf(stdout, "[%d] No new communicator\n", rank);
  }

  fflush(stdout);
  MPI_Finalize();
  exit(0);
}

