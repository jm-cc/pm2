#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE (8 * 1024 * 1024)

int main(int argc, char **argv) {
  int numtasks, rank, i, source, flag;
  char *buffer;
  MPI_Request *requests;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  //  printf("Rank %d Size %d\n", rank, numtasks);

  requests = malloc(5 * sizeof(MPI_Request));
  buffer = calloc(SIZE * sizeof(char), 1);

  if (rank == 0) {
    for(i=1 ; i<numtasks ; i++) {
      source=i;
      MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &requests[0]);
      MPI_Iprobe(source, 2, MPI_COMM_WORLD, &flag, NULL);
      MPI_Wait(&requests[0], NULL);

      source = MPI_ANY_SOURCE;

      MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &requests[1]);
      MPI_Iprobe(source, 2, MPI_COMM_WORLD, &flag, NULL);
      MPI_Wait(&requests[1], NULL);

      MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &requests[2]);
      MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &requests[3]);
      MPI_Irecv(buffer, SIZE, MPI_CHAR, source, 2, MPI_COMM_WORLD, &requests[4]);

      MPI_Wait(&requests[2], NULL);
      MPI_Wait(&requests[4], NULL);
      MPI_Wait(&requests[3], NULL);
    }
    printf("success\n");
  }
  else {
    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &requests[0]);
    MPI_Wait(&requests[0], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &requests[1]);
    MPI_Wait(&requests[1], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &requests[2]);
    MPI_Wait(&requests[2], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &requests[3]);
    MPI_Wait(&requests[3], NULL);

    MPI_Isend(buffer, SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &requests[4]);
    MPI_Wait(&requests[4], NULL);
    printf("success\n");
  }

  free(buffer);
  free(requests);

  MPI_Finalize();
  printf("success\n");
  exit(0);
}

