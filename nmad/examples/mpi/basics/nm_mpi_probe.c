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

#define SMALL_SIZE 10
#define BIG_SIZE   640*1024

void test_probe(int rank, int size);

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

  test_probe(rank, SMALL_SIZE);
  test_probe(rank, BIG_SIZE);

  MPI_Finalize();
  exit(0);
}

void test_probe(int rank, int size) {
  int ping_side, rank_dst;

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int *x = calloc(size, sizeof(int));

    x[0] = 29;
    MPI_Send(x, size, MPI_INT, rank_dst, 22, MPI_COMM_WORLD);
    x[0] = 7;
    MPI_Send(x, size, MPI_INT, rank_dst, 2, MPI_COMM_WORLD);

    free(x);
  }
  else {
    int *x, *y;
    int flag, flag1, count;
    MPI_Request request;
    MPI_Status status, status1, status2;

    x = malloc(size*sizeof(int));
    y = malloc(size*sizeof(int));

    MPI_Irecv(x, size, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request);

    int i = 0;
    for(i=0 ; i<10000 ; i++) {
      MPI_Test(&request, &flag, &status);
    }

    MPI_Iprobe(MPI_ANY_SOURCE, 22, MPI_COMM_WORLD, &flag1, &status1);
    //    fprintf(stdout,"\n\nIprobe: flag=%d, source=%d, tag=%d\n", flag1, status1.MPI_SOURCE, status1.MPI_TAG);

    if (flag1 == 1) {
      MPI_Recv(y, size, MPI_INT, status1.MPI_SOURCE, status1.MPI_TAG, MPI_COMM_WORLD, &status2);
    }
    else {
      MPI_Recv(y, size, MPI_INT, rank_dst, 22, MPI_COMM_WORLD, &status2);
    }
    MPI_Get_count(&status2, MPI_INT, &count);
    fprintf(stdout, "Received: source=%d, tag=%d, error=%d, count=%d\n", status2.MPI_SOURCE, status2.MPI_TAG, status2.MPI_ERROR, count);
    fprintf(stdout, "Birth date %d\n", y[0]);

    if (flag == 0) {
      MPI_Wait(&request, &status);
    }
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "Wait: source=%d, tag=%d, error=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR, count);
    fprintf(stdout, "Birth month %d\n", x[0]);

    free(x);
    free(y);
  }
}
