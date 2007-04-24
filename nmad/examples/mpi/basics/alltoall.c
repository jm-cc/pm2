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

  printf("Rank %d Size %d\n", rank, numtasks);

  {
    int *rrbuf = malloc(numtasks * 2 * sizeof(int));
    int *sendarray = malloc(numtasks * 2 * sizeof(int));
    for(i=0 ; i<numtasks*2 ; i++) {
      sendarray[i] = rank*10+i;
    }
    fprintf(stdout, "[%d] Sending: ", rank);
    for(i=0 ; i<numtasks*2 ; i++) {
      fprintf(stdout, "%d ", sendarray[i]);
    }
    fprintf(stdout, "\n");

    MPI_Alltoall(sendarray, 2, MPI_INT, rrbuf, 2, MPI_INT, MPI_COMM_WORLD);

    fprintf(stdout, "[%d] Received: ", rank);
    for(i=0 ; i<numtasks*2 ; i++) {
      fprintf(stdout, "%d ", rrbuf[i]);
    }
    fprintf(stdout, "\n");
  }

  fflush(stdout);

  MPI_Finalize();
  exit(0);
}

