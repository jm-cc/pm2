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

  if (rank == 0) {
    MPI_Request request1;
    MPI_Request request2;
    int x=42;
    int y=7;

    MPI_Esend(&x, 1, MPI_INT, 1, 1, 0, MPI_COMM_WORLD, &request1);
    MPI_Esend(&y, 1, MPI_INT, 1, 1, 1, MPI_COMM_WORLD, &request2);

    MPI_Wait(&request1, NULL);
    MPI_Wait(&request2, NULL);

  }
  else if (rank == 1) {
    int x, y;

    MPI_Recv(&x, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);
    MPI_Recv(&y, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }

  MPI_Finalize();
  exit(0);
}

