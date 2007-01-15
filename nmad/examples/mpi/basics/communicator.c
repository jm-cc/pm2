#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void send_with_different_tags(int rank) {
  if (rank == 0) {
    int x=42;
    int y=7;
    MPI_Request request1, request2;

    MPI_Isend(&x, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &request1);
    MPI_Isend(&y, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, &request2);
    MPI_Wait(&request1, NULL);
    MPI_Wait(&request2, NULL);
  }
  else if (rank == 1) {
    int x, y;
    MPI_Request request1, request2;

    MPI_Irecv(&y, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &request2);
    MPI_Irecv(&x, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &request1);
    MPI_Wait(&request1, NULL);
    MPI_Wait(&request2, NULL);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }
}

void send_with_different_communicators(int rank, MPI_Comm comm) {
  if (rank == 0) {
    int x=42;
    int y=7;
    MPI_Request request1, request2;

    MPI_Isend(&x, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &request1);
    MPI_Isend(&y, 1, MPI_INT, 1, 1, comm, &request2);
    MPI_Wait(&request1, NULL);
    MPI_Wait(&request2, NULL);
  }
  else if (rank == 1) {
    int x, y;
    MPI_Request request1, request2;

    MPI_Irecv(&y, 1, MPI_INT, 0, 1, comm, &request2);
    MPI_Irecv(&x, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &request1);
    MPI_Wait(&request1, NULL);
    MPI_Wait(&request2, NULL);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }
}

int main(int argc, char **argv) {
  int numtasks, rank;
  int comm1;
  int comm2;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  MPI_Comm_dup(MPI_COMM_WORLD, &comm1);
  printf("Communicator is %d\n", comm1);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm2);
  printf("Communicator is %d\n", comm2);
  MPI_Comm_free(&comm1);
  MPI_Comm_dup(comm2, &comm1);
  printf("Communicator is %d\n", comm1);

  send_with_different_tags(rank);
  send_with_different_communicators(rank, comm1);

  MPI_Comm_free(&comm1);
  MPI_Comm_free(&comm2);
  MPI_Finalize();
  exit(0);
}

