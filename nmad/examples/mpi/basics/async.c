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

  printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    int x=42;
    int y=7;
    MPI_Request request = MPI_REQUEST_NULL;

    if (MPI_Request_is_equal(request, MPI_REQUEST_NULL)) printf("Null request\n");
    MPI_Isend(&x, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, &request);
    MPI_Send(&y, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
  }
  else if (rank == 1) {
    int x, y, flag;
    MPI_Request request;

    MPI_Irecv(&x, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &request);
    MPI_Recv(&y, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, NULL);
    MPI_Test(&request, &flag, NULL);
    if (!flag) {
      printf("Waiting for the data\n");
      MPI_Wait(&request, NULL);
    }

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }

  MPI_Finalize();
  exit(0);
}

