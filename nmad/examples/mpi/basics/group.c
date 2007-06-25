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
  int newsize, newrank;
  int color;
  int ranks1[] = {0,3,0,2};
  int ranks2[4];

  MPI_Comm comm;
  MPI_Group group1, group2;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size != 4) {
    fprintf(stdout, "Need 4 processes (%d)\n", size);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  fprintf(stdout, "[%d] Communicator %d with %d nodes, local rank %d\n", rank, MPI_COMM_WORLD, size, rank);

  color = (rank % 2);

  MPI_Comm_split(MPI_COMM_WORLD, color, rank*10, &comm);
  MPI_Comm_size(comm, &newsize);
  MPI_Comm_rank(comm, &newrank);
  fprintf(stdout, "[%d] Communicator %d with %d nodes, local rank %d, color %d\n", rank, comm, newsize, newrank, color);
  fflush(stdout);

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Comm_group(MPI_COMM_WORLD, &group1);
  MPI_Comm_group(comm, &group2);

  MPI_Group_translate_ranks(group1, 4, ranks1, group2, ranks2);

  fprintf(stdout, "[%d] Old ranks[%d, %d, %d, %d]\n", rank, ranks1[0], ranks1[1], ranks1[2], ranks1[3]);
  fprintf(stdout, "[%d] New ranks[%d, %d, %d, %d]\n", rank, ranks2[0], ranks2[1], ranks2[2], ranks2[3]);
  fflush(stdout);

  MPI_Comm_free(&comm);
  MPI_Finalize();
  exit(0);
}

