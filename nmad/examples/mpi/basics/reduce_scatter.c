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
  int numtasks, rank, i;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int *rrbuf = malloc(2 * sizeof(int));
    int *sendarray = malloc(numtasks * 2 * sizeof(int));
    int *recvcounts = malloc(numtasks * sizeof(int));

    for(i=0 ; i<numtasks ; i++) {
      recvcounts[i] = 2;
    }

    for(i=0 ; i<numtasks*2 ; i+=2) {
      sendarray[i] = i;
      sendarray[i+1] = i+1;
    }

    fprintf(stdout, "[%d] Sending:  ", rank);
    for(i=0 ; i<numtasks*2 ; i++) {
      fprintf(stdout, "%3d ", sendarray[i]);
    }
    fprintf(stdout, "\n");

    MPI_Reduce_scatter(sendarray, rrbuf, recvcounts, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    fprintf(stdout, "[%d] Received: ", rank);
    for(i=0 ; i<2 ; i++) {
      fprintf(stdout, "%3d ", rrbuf[i]);
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    free(rrbuf);
    free(sendarray);
    free(recvcounts);
  }

  MPI_Finalize();
  exit(0);
}

