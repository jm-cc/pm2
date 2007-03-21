#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int i, flag, numtasks, rank, dest;
  MPI_Request requests[4*1024+1];
  int *x=malloc(1024*sizeof(int));

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (rank == 0) {

    dest = 1;
    // ping
    for(i=0 ; i<4*1024 ; i++) {
      MPI_Isend(x, 1024, MPI_INT, dest, 1, MPI_COMM_WORLD, &requests[i]);
      MPI_Test(&requests[i], &flag, NULL);
    }
    MPI_Isend(x, 1024, MPI_INT, dest, 1, MPI_COMM_WORLD, &requests[4*1024]);

    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Wait(&requests[i], NULL);
    }

    // pong
    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Recv(x, 1024, MPI_INT, dest, 1, MPI_COMM_WORLD, NULL);
    }
  }
  else if (rank == 1) {
    dest=0;

    // pong
    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Recv(x, 1024, MPI_INT, dest, 1, MPI_COMM_WORLD, NULL);
    }

    // ping
    for(i=0 ; i<4*1024 ; i++) {
      MPI_Esend(x, 1024, MPI_INT, dest, 1, 0, MPI_COMM_WORLD, &requests[i]);
    }
    MPI_Esend(x, 1024, MPI_INT, dest, 1, 1, MPI_COMM_WORLD, &requests[4*1024]);

    for(i=0 ; i<=4*1024 ; i++) {
      MPI_Wait(&requests[i], NULL);
    }
  }

  MPI_Finalize();
  exit(0);
}

