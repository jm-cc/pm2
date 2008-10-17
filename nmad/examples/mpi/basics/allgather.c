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

int main(int argc, char **argv) {
  int numtasks, rank, i;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int success=1;
    int *rrbuf = malloc(numtasks * 2 * sizeof(int));
    int sendarray[2];
    sendarray[0] = rank*2;
    sendarray[1] = rank+1;

    MPI_Allgather(sendarray, 2, MPI_INT, rrbuf, 2, MPI_INT, MPI_COMM_WORLD);

    for(i=0 ; i<numtasks ; i+=2) {
      if (rrbuf[i] != (i/2)*2  || rrbuf[i+1] != (i/2)+1) {
        success=0;
      }
    }

    if (success) {
      fprintf(stdout, "Success\n");
    }
    else {
      fprintf(stdout, "[%d] Error!!! Received: ", rank);
      for(i=0 ; i<numtasks*2 ; i++) {
        fprintf(stdout, "%d ", rrbuf[i]);
      }
      fprintf(stdout, "\n");
    }

    free(rrbuf);
  }

  fflush(stdout);

  MPI_Finalize();
  exit(0);
}

