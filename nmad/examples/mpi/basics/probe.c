#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SMALL_SIZE 10
#define BIG_SIZE   640*1024

void test_probe(int rank, int size);

int main(int argc, char **argv) {
  int rank;

  // Initialise MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  test_probe(rank, SMALL_SIZE);
  test_probe(rank, BIG_SIZE);

  MPI_Finalize();
  exit(0);
}

void test_probe(int rank, int size) {
  if (rank == 1) {
    int *x = malloc(size*sizeof(int));

    x[0] = 29;
    MPI_Send(x, size, MPI_INT, 0, 22, MPI_COMM_WORLD);
    x[0] = 7;
    MPI_Send(x, size, MPI_INT, 0, 2, MPI_COMM_WORLD);
 }
  else if (rank == 0) {
    int *x, flag, flag1, count;
    MPI_Request request, request1;
    MPI_Status status, status1, status2;

    x = malloc(size*sizeof(int));

    MPI_Irecv(x, size, MPI_INT, 1, 2, MPI_COMM_WORLD, &request);

    int i = 0;
    for(i=0 ; i<10000 ; i++) {
      MPI_Test(&request, &flag, &status);
    }

    MPI_Iprobe(1, 22, MPI_COMM_WORLD, &flag1, &status1);
    fprintf(stdout,"flag=%d, source=%d, flag=%d\n", flag1, status1.MPI_SOURCE, status1.MPI_TAG);

    if (flag1 == 1) {
      MPI_Recv(x, size, MPI_INT, status1.MPI_SOURCE, status1.MPI_TAG, MPI_COMM_WORLD, &status2);

      MPI_Get_count(&status2, MPI_INT, &count);
      fprintf(stdout, "source=%d, tag=%d, error=%d, count=%d\n", status2.MPI_SOURCE, status2.MPI_TAG, status2.MPI_ERROR, count);
      fprintf(stdout, "Birth date %d\n", x[0]);
    }
    else {
      fprintf(stdout, "Test failed\n");
    }

    if (flag == 0) {
      MPI_Wait(&request, &status);
    }
    MPI_Get_count(&status, MPI_INT, &count);
    fprintf(stdout, "source=%d, tag=%d, error=%d, count=%d\n", status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR, count);
    fprintf(stdout, "Birth month %d\n", x[0]);
  }
}
