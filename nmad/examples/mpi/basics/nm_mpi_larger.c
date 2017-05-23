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

#define SMALL_SIZE 4
#define LARGE_SIZE 1024*1024

void test_larger(int rank, int size);

int main(int argc, char **argv) {
  int rank, numtasks;

  // Initialise MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  test_larger(rank, SMALL_SIZE);
  test_larger(rank, LARGE_SIZE);

  MPI_Finalize();
  exit(0);
}

void test_larger(int rank, int size) {
  int ping_side, rank_dst;

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (!ping_side) {
    int *x;

    x=malloc(size*sizeof(int));
    x[0] = 42;
    x[size-2] = 7;
    MPI_Send(x, size, MPI_INT, rank_dst, 2, MPI_COMM_WORLD);
    MPI_Send(x, size, MPI_INT, rank_dst, 2, MPI_COMM_WORLD);

    free(x);
  }
  else {
    int *x;
    int *xx;
    MPI_Status status;
    MPI_Request request;
    int count;

    x=malloc(size*sizeof(int));
    MPI_Recv(x, size, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "source=%d, tag=%d, error=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR, count);

    xx=malloc(2*size*sizeof(int));
    MPI_Irecv(xx, 2*size, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "source=%d, tag=%d, error=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR, count);

    fprintf(stdout, "The answer to life, the universe, and everything is %d, %d\n", x[0], xx[0]);
    fprintf(stdout, "There are %d wonders of the world\n\n", xx[size-2]);

    free(x);
    free(xx);
  }
}
