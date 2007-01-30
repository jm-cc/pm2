#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int numtasks, rank, tag=1;
  int counter,  message;
  MPI_Status stat;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  //printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    message=-1;
    counter=1;
    MPI_Send(&counter, 0, MPI_INT, 1, tag, MPI_COMM_WORLD);
    MPI_Recv(&message, 0, MPI_INT, 1, tag, MPI_COMM_WORLD, &stat);
    fprintf(stderr, "Received %d\n", message);
  }
  else {
    MPI_Recv(&message, 0, MPI_INT, 0, tag, MPI_COMM_WORLD, &stat);
    MPI_Send(&message, 0, MPI_INT, 0, tag, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  exit(0);
}
