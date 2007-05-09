#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  int rank_dst, ping_side;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  //printf("Rank %d Size %d\n", rank, numtasks);

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int x=42;
    int y=7;
    MPI_Request request = MPI_REQUEST_NULL;

    if (MPI_Request_is_equal(request, MPI_REQUEST_NULL)) printf("Null request\n");
    MPI_Isend(&x, 1, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request);
    MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
  }
  else {
    int x, y, flag;
    MPI_Request request;

    MPI_Irecv(&x, 1, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, &request);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
    if (!flag) {
      printf("Waiting for the data\n");
      MPI_Wait(&request, MPI_STATUS_IGNORE);
    }

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }

  MPI_Finalize();
  exit(0);
}

