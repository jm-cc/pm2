#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE (8 * 1024 * 1024)

int main(int argc, char **argv) {
  int numtasks, rank;
  char *buffer;
  MPI_Request request[5];

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  //  printf("Rank %d Size %d\n", rank, numtasks);

  buffer = malloc(SIZE);
  memset(buffer, 0, SIZE);

  if (rank == 0) {
    int source = 1;

    MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &request[0]);
    MPI_Wait(&request[0], NULL);

    source = MPI_ANY_SOURCE;

    MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &request[1]);
    MPI_Wait(&request[1], NULL);

    MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &request[2]);
    MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &request[3]);
    MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &request[4]);

    MPI_Wait(&request[2], NULL);
    MPI_Wait(&request[4], NULL);
    MPI_Wait(&request[3], NULL);
    printf("success\n");
  }
  else if (rank == 1) {
    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &request[0]);
    MPI_Wait(&request[0], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &request[1]);
    MPI_Wait(&request[1], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &request[2]);
    MPI_Wait(&request[2], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &request[3]);
    MPI_Wait(&request[3], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &request[4]);
    MPI_Wait(&request[4], NULL);
    printf("success\n");
  }

  free(buffer);
  MPI_Finalize();
  printf("success\n");
  exit(0);
}

