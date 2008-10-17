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

//#define ALLTOALL_DEBUG

int main(int argc, char **argv) {
  int numtasks, rank, i;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int success=1;
    int *rrbuf = malloc(numtasks * 2 * sizeof(int));
    int *sendarray = malloc(numtasks * 2 * sizeof(int));
    for(i=0 ; i<numtasks*2 ; i++) {
      sendarray[i] = rank*10+i;
    }
#ifdef ALLTOALL_DEBUG
    fprintf(stdout, "[%d] Sending: ", rank);
    for(i=0 ; i<numtasks*2 ; i++) {
      fprintf(stdout, "%d ", sendarray[i]);
    }
    fprintf(stdout, "\n");
#endif

    MPI_Alltoall(sendarray, 2, MPI_INT, rrbuf, 2, MPI_INT, MPI_COMM_WORLD);

#ifdef ALLTOALL_DEBUG
    fprintf(stdout, "[%d] Received: ", rank);
    for(i=0 ; i<numtasks*2 ; i++) {
      fprintf(stdout, "%d ", rrbuf[i]);
    }
    fprintf(stdout, "\n");
#endif

    for(i=0 ; i<numtasks ; i+=2) {
      if (rrbuf[i] != (rank*2)+(i/2)*10  || rrbuf[i+1] != (rank*2+1)+(i/2)*10) {
        fprintf(stdout, "[%d] Error. rrbuf[%d] != %d --- rrbuf[%d] != %d\n",
                rank, i, (rank*2)+(i/2)*10, i+1, (rank*2+1)+(i/2)*10);
        success=0;
      }
    }

    if (success) {
      fprintf(stdout, "Success\n");
    }

    free(rrbuf);
    free(sendarray);
  }

  fflush(stdout);

  MPI_Finalize();
  exit(0);
}

