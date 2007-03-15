#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  //printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    int *x;
    int y=7;

    x=malloc(1024*1024*sizeof(int));
    x[0] = 42;
    x[1024*1024-1] = 42;
    MPI_Rsend(x, 1024*1024, MPI_INT, 1, 2, MPI_COMM_WORLD);
    MPI_Send(&y, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
    MPI_Recv(&y, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, NULL);
  }
  else if (rank == 1) {
    int *x, y;

    x=malloc(1024*1024*sizeof(int));
    MPI_Recv(x, 1024*1024, MPI_INT, 0, 2, MPI_COMM_WORLD, NULL);
    MPI_Recv(&y, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);
    MPI_Send(&y, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);

    printf("The answer to life, the universe, and everything is %d, %d\n", x[0], x[1024*1024-1]);
    printf("There are %d Wonders of the World\n", y);
  }

  MPI_Finalize();
  exit(0);
}

