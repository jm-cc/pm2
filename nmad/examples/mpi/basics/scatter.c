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

//#define SCATTER_DEBUG

int main(int argc, char **argv) {
  int numtasks, rank, i;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int *rrbuf = malloc(2 * sizeof(int));
    int *sendarray = malloc(numtasks * 2 * sizeof(int));

    for(i=0 ; i<numtasks*2 ; i+=2) {
      sendarray[i] = i;
      sendarray[i+1] = i+1;
    }

#ifdef SCATTER_DEBUG
    if (rank == 0) {
      fprintf(stdout, "[%d] Sending: ", rank);
      for(i=0 ; i<numtasks*2 ; i++) {
        fprintf(stdout, "%d ", sendarray[i]);
      }
      fprintf(stdout, "\n");
    }
#endif

    MPI_Scatter(sendarray, 2, MPI_INT, rrbuf, 2, MPI_INT, 0, MPI_COMM_WORLD);
#ifdef SCATTER_DEBUG
    fprintf(stdout, "[%d] Received: ", rank);
    for(i=0 ; i<2 ; i++) {
      fprintf(stdout, "%d ", rrbuf[i]);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
#endif

    if (rrbuf[0] == (rank*2) && rrbuf[1] == (rank*2) + 1) {
      fprintf(stdout, "Success\n");
    }
    else {
      fprintf(stdout, "[%d] Error. rrbuf[0] == %d (!= %d) --- rrbuf[1] == %d (!= %d)\n", rank, rrbuf[0], rank*2,
              rrbuf[1], (rank*2)+1);
    }

    free(rrbuf);
    free(sendarray);
  }

  MPI_Finalize();
  exit(0);
}

