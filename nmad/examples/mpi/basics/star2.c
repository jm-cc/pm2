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

#define LOOPS 10

int main(int argc, char **argv) {
  int numtasks, rank;
  int tag = 1;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (rank == 0) {
    int child, loop, success=1;
    MPI_Request *out_requests;
    MPI_Request *in_requests;

    for(loop=0 ; loop<LOOPS ; loop++) {
      out_requests = malloc(numtasks * sizeof(MPI_Request));
      in_requests = malloc(numtasks * sizeof(MPI_Request));

      for(child=1 ; child<numtasks ; child++) {
        MPI_Isend(&child, 1, MPI_INT, child, tag, MPI_COMM_WORLD, &out_requests[child-1]);
      }

      for(child=1 ; child<numtasks ; child++) {
        MPI_Wait(&out_requests[child-1], MPI_STATUS_IGNORE);
      }

      for(child=1 ; child<numtasks ; child++) {
        int recv;
        MPI_Recv(&recv, 1, MPI_INT, child, tag, MPI_COMM_WORLD, NULL);
        if (recv != child) {
          success = 0;
          fprintf(stdout, "Expected [%d] - Received [%d]\n", child, recv);
        }
      }

      free(out_requests);
      free(in_requests);
    }
    if (success) {
      fprintf(stdout, "Success\n");
    }
  }
  else {
    int father = 0;
    int r_buffer, loop;
    for(loop=0 ; loop<LOOPS ; loop++) {
      MPI_Recv(&r_buffer, 1, MPI_INT, father, tag, MPI_COMM_WORLD, NULL);
      MPI_Send(&r_buffer, 1, MPI_INT, father, tag, MPI_COMM_WORLD);
    }
    fprintf(stdout, "Success\n");
  }
  MPI_Finalize();
  exit(0);
}
