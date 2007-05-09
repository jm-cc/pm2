#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  int ping_side, rank_dst;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    MPI_Request request1;
    MPI_Request request2;
    int x=42;
    int y=7;

    MPI_Esend(&x, 1, MPI_INT, rank_dst, 1, 0, MPI_COMM_WORLD, &request1);
    MPI_Esend(&y, 1, MPI_INT, rank_dst, 1, 1, MPI_COMM_WORLD, &request2);

    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);

  }
  else {
    int x, y;

    MPI_Recv(&x, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    printf("The answer to life, the universe, and everything is %d\n", x);
    printf("There are %d Wonders of the World\n", y);
  }

  MPI_Finalize();
  exit(0);
}

