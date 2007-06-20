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
    int *sendarray, sendcount;
    int *rbuf, *recvcounts, *displs, totalcount;
    if (rank == 0) {
      totalcount = 1 + (numtasks * (numtasks - 1) / 2);
      rbuf = malloc(totalcount * sizeof(int));
      recvcounts = malloc((numtasks+1) * sizeof(int));
      displs = malloc((numtasks+1) * sizeof(int));
      recvcounts[0] = 1;
      displs[0] = 0;
      for(i=1 ; i<=numtasks ; i++) {
        recvcounts[i] = i;
        displs[i] = displs[i-1] + recvcounts[i-1];
      }
      sendarray = malloc(sizeof(int));
      sendarray[0] = rank;
      sendcount = 1;
    }
    else {
      sendcount = rank;
      sendarray = malloc(rank * sizeof(int));
      for(i=0 ; i<rank ; i++) sendarray[i] = rank;
    }
    fprintf(stdout, "[%d] Sending: ", rank);
    for(i=0 ; i<sendcount ; i++) {
      fprintf(stdout, "%d ", sendarray[i]);
    }
    fprintf(stdout, "\n");

    MPI_Gatherv(sendarray, sendcount, MPI_INT, rbuf, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
      fprintf(stdout, "[%d] Received: ", rank);
      for(i=0 ; i<totalcount; i++) {
        fprintf(stdout, "%d ", rbuf[i]);
      }
      fprintf(stdout, "\n");
      free(rbuf);
      free(recvcounts);
      free(displs);
    }
    free(sendarray);
  }

  {
    int *sendarray, sendcount;
    int *rbuf, *recvcounts=NULL, *displs, totalcount;

    totalcount = 1 + (numtasks * (numtasks - 1) / 2);
    rbuf = malloc(totalcount * sizeof(int));

    recvcounts = malloc((numtasks+1) * sizeof(int));
    displs = malloc((numtasks+1) * sizeof(int));
    recvcounts[0] = 1;
    displs[0] = 0;
    for(i=1 ; i<=numtasks ; i++) {
      recvcounts[i] = i;
      displs[i] = displs[i-1] + recvcounts[i-1];
    }

    if (rank == 0) {
      sendarray = malloc(sizeof(int));
      sendarray[0] = rank;
      sendcount = 1;
    }
    else {
      sendcount = rank;
      sendarray = malloc(rank * sizeof(int));
      for(i=0 ; i<rank ; i++) sendarray[i] = rank*10;
    }
    fprintf(stdout, "[%d] Sending: ", rank);
    for(i=0 ; i<sendcount ; i++) {
      fprintf(stdout, "%d ", sendarray[i]);
    }
    fprintf(stdout, "\n");

    MPI_Allgatherv(sendarray, sendcount, MPI_INT, rbuf, recvcounts, displs, MPI_INT, MPI_COMM_WORLD);
    fprintf(stdout, "[%d] Received: ", rank);
    for(i=0 ; i<totalcount; i++) {
      fprintf(stdout, "%d ", rbuf[i]);
    }
    fprintf(stdout, "\n");

    free(recvcounts);
    free(displs);
    free(sendarray);
    free(rbuf);
  }

  fflush(stdout);

  MPI_Finalize();
  exit(0);
}

