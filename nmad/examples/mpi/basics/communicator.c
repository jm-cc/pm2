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

void send_with_different_tags(int rank) {
  int ping_side, rank_dst;

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int x=42;
    int y=7;
    MPI_Request request1, request2;

    MPI_Isend(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &request1);
    MPI_Isend(&y, 1, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request2);
    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);
  }
  else {
    int x, y;
    MPI_Request request1, request2;

    MPI_Irecv(&y, 1, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request2);
    MPI_Irecv(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &request1);
    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }
}

void send_with_different_communicators(int rank, MPI_Comm comm) {
  int ping_side, rank_dst;

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int x=42;
    int y=7;
    MPI_Request request1, request2;

    MPI_Isend(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &request1);
    MPI_Isend(&y, 1, MPI_INT, rank_dst, 1, comm, &request2);
    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);
  }
  else {
    int x, y;
    MPI_Request request1, request2;

    MPI_Irecv(&y, 1, MPI_INT, rank_dst, 1, comm, &request2);
    MPI_Irecv(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &request1);
    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }
}

int main(int argc, char **argv) {
  int numtasks, rank;
  int comm1;
  int comm2;

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

  MPI_Comm_dup(MPI_COMM_WORLD, &comm1);
  printf("Communicator 1 is %d\n", comm1);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm2);
  printf("Communicator 2 is %d\n", comm2);
  MPI_Comm_free(&comm1);
  MPI_Comm_dup(comm2, &comm1);
  printf("Communicator 3 is %d\n", comm1);

  send_with_different_tags(rank);
  send_with_different_communicators(rank, comm1);

  MPI_Comm_free(&comm1);
  MPI_Comm_free(&comm2);
  MPI_Finalize();
  exit(0);
}

