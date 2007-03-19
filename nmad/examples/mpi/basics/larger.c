#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SMALL_SIZE 4
#define LARGE_SIZE 1024*1024

void test_larger(int rank, int size);

int main(int argc, char **argv) {
  int rank;

  // Initialise MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  test_larger(rank, SMALL_SIZE);
  test_larger(rank, LARGE_SIZE);

  MPI_Finalize();
  exit(0);
}

void test_larger(int rank, int size) {
  if (rank == 1) {
    int *x;

    x=malloc(size*sizeof(int));
    x[0] = 42;
    x[size-2] = 7;
    MPI_Send(x, size, MPI_INT, 0, 2, MPI_COMM_WORLD);
    MPI_Send(x, size, MPI_INT, 0, 2, MPI_COMM_WORLD);
  }
  else if (rank == 0) {
    int *x;
    int *xx;
    MPI_Status status;
    MPI_Request request;
    int count;

    x=malloc(size*sizeof(int));
    MPI_Recv(x, size, MPI_INT, 1, 2, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "source=%d, tag=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, count);

    xx=malloc(2*size*sizeof(int));
    MPI_Irecv(xx, 2*size, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "source=%d, tag=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, count);

    fprintf(stdout, "The answer to life, the universe, and everything is %d, %d\n", x[0], xx[0]);
    fprintf(stdout, "There is %d wonders of the world\n\n", xx[size-2]);
  }
}
