#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank, i;
  MPI_Comm comm;

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
  }

  MPI_Finalize();
  exit(0);
}

