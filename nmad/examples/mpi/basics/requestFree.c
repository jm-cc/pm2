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

#if !defined(MAD_MPI)
#  define MPI_Request_is_equal(r1, r2) r1 == r2
#endif

#define N 2

int main(int argc, char **argv) {
  int rank, i;
  MPI_Request request;
  MPI_Status status;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    int y, x=42;
    for(i=0 ; i<N ; i++) {
      MPI_Isend(&x, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
      MPI_Request_free(&request);
      MPI_Irecv(&y, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);
      fprintf(stdout, "Value: %d\n", y);
    }
  }
  else if (rank == 1) {
    int x;
    MPI_Irecv(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    for(i=0 ; i<N-1 ; i++) {
      MPI_Isend(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
      MPI_Request_free(&request);
      MPI_Irecv(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, &status);
    }
    MPI_Isend(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Request_free(&request);
  }

  MPI_Finalize();
  exit(0);
}

