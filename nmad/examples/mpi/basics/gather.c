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

//#define GATHER_DEBUG

int main(int argc, char **argv) {
  int numtasks, rank, i;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int success=1;
    int sendarray[2];
    int *rbuf;
    if (rank == 0) {
      rbuf = malloc(numtasks * 2 * sizeof(int));
    }
    sendarray[0] = rank;
    sendarray[1] = rank+6;

#ifdef GATHER_DEBUG
    fprintf(stdout, "[%d] Sending: ", rank);
    for(i=0 ; i<2 ; i++) {
      fprintf(stdout, "%d ", sendarray[i]);
    }
    fprintf(stdout, "\n");
#endif

    MPI_Gather(sendarray, 2, MPI_INT, rbuf, 2, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
#ifdef GATHER_DEBUG
      fprintf(stdout, "[%d] Received: ", rank);
      for(i=0 ; i<numtasks*2 ; i++) {
        fprintf(stdout, "%d ", rbuf[i]);
      }
      fprintf(stdout, "\n");
#endif
      for(i=0 ; i<numtasks*2 ; i+=2) {
        if (rbuf[i] != i/2 || rbuf[i+1] != i/2+6) {
          fprintf(stdout, "[%d] Error. rbuf[%d] != %d -- rbuf[%d] != %d\n", rank, i, i/2, i+1, i/2+6);
          success=0;
        }
      }
      if (success) {
        fprintf(stdout, "success\n");
      }
      free(rbuf);
    }
  }

  fflush(stdout);

  MPI_Finalize();
  exit(0);
}

