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

void persistent_int(int ping_side, int rank_dst) {
  if (ping_side) {
    int x=42, y;
    MPI_Request send_request;
    MPI_Request recv_request;

    MPI_Send_init(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &send_request);
    MPI_Start(&send_request);
    MPI_Wait(&send_request, MPI_STATUS_IGNORE);

    MPI_Start(&send_request);
    MPI_Wait(&send_request, MPI_STATUS_IGNORE);

    MPI_Recv_init(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, &recv_request);
    MPI_Start(&recv_request);
    MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    if (y == 42) printf("success\n"); else printf("failure\n");

    MPI_Start(&recv_request);
    MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    if (y == 42) printf("success\n"); else printf("failure\n");
  }
  else {
    int x, y;
    MPI_Recv(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
    MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);

    if (x == 42) printf("success\n"); else printf("failure\n");
    if (y == 42) printf("success\n"); else printf("failure\n");
  }
}

void check_buffer(char *buffer, int *strides, int *blocklengths) {
  int i=0, j=0, success=1;
  char value;
  int nb, count, blength;

  for(nb=0 ; nb<3 ; nb++) {
    j=0;
    for(count=0 ; count<3 ; count++) {
      value = 'a' + (nb * (strides[2] + blocklengths[2]));
      value += strides[j];
      for(blength=0 ; blength<blocklengths[j] ; blength++) {
        if (buffer[i] != value) {
          success=0;
          break;
        }
        i++;
        value++;
      }
      j++;
    }
  }
  if (success) {
    printf("Index successfully received\n");
  }
  else {
    printf("Incorrect index: [ ");
    for(i=0 ; i<18 ; i++) printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
    printf("]\n");
  }
}

void persistent_index(int ping_side, int rank_dst, int optimized) {
  MPI_Datatype mytype;
  int blocklengths[3] = {1, 2, 3};
  int strides[3] = {0, 2, 3};

  char buffer[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g','h', 'i' ,'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

  MPI_Type_indexed(3, blocklengths, strides, MPI_CHAR, &mytype);
  MPI_Type_commit(&mytype);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&mytype, 1);
  }
#endif /* MAD_MPI */

  if (ping_side) {
    MPI_Request send_request;
    MPI_Request recv_request;
    char buffer2[18];

    MPI_Send_init(buffer, 3, mytype, rank_dst, 10, MPI_COMM_WORLD, &send_request);
    MPI_Start(&send_request);
    MPI_Wait(&send_request, MPI_STATUS_IGNORE);

    MPI_Start(&send_request);
    MPI_Wait(&send_request, MPI_STATUS_IGNORE);

    MPI_Recv_init(buffer2, 3, mytype, rank_dst, 10, MPI_COMM_WORLD, &recv_request);
    MPI_Start(&recv_request);
    MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    check_buffer(buffer2, strides, blocklengths);

    MPI_Start(&recv_request);
    MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    check_buffer(buffer2, strides, blocklengths);

    MPI_Cancel(&send_request);
    MPI_Cancel(&recv_request);
  }
  else {
    char buffer2[18];
    char buffer3[18];

    MPI_Recv(buffer2, 3, mytype, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    check_buffer(buffer2, strides, blocklengths);

    MPI_Recv(buffer3, 3, mytype, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    check_buffer(buffer3, strides, blocklengths);

    MPI_Send(buffer, 3, mytype, rank_dst, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 3, mytype, rank_dst, 10, MPI_COMM_WORLD);
  }

  MPI_Type_free(&mytype);
}

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

  persistent_int(ping_side, rank_dst);
  persistent_index(ping_side, rank_dst, 0);
  persistent_index(ping_side, rank_dst, 1);

  MPI_Finalize();
  exit(0);
}

