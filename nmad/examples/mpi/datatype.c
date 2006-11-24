#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

void print_buffer(int *buffer) {
  if (buffer[0] != 1 && buffer[1] != 2 && buffer[2] != 3 && buffer[3] != 4) {
    printf("Incorrect data\n");
  }
  printf("Received data [%d, %d, %d, %d]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
}

int main(int argc, char **argv) {
  int numtasks, rank;
  MPI_Status stat;
  MPI_Datatype mytype;
  MPI_Datatype mytype3;
  MPI_Datatype mytype2;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_contiguous(2, mytype, &mytype2);
  MPI_Type_commit(&mytype2);

  if (rank == 0) {
    int buffer[4] = {1, 2, 3, 4};
    MPI_Send(buffer, 4, MPI_INT, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 2, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 1, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    int buffer[4], buffer2[4], buffer3[4];
    MPI_Recv(buffer, 4, MPI_INT, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer);

    MPI_Recv(buffer2, 2, mytype, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer2);

    MPI_Recv(buffer3, 1, mytype2, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer3);
  }

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_contiguous(2, MPI_INT, &mytype3);
  MPI_Type_commit(&mytype3);
  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_free(&mytype3);
  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);

  MPI_Finalize();
  exit(0);
}
