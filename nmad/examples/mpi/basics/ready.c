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

  //printf("Rank %d Size %d\n", rank, numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  if (ping_side) {
    int *x;
    int y=7;

    x=malloc(1024*1024*sizeof(int));
    x[0] = 42;
    x[1024*1024-1] = 42;
    MPI_Rsend(x, 1024*1024, MPI_INT, rank_dst, 2, MPI_COMM_WORLD);
    MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, NULL);

    free(x);
  }
  else {
    int *x, y;

    x=malloc(1024*1024*sizeof(int));
    MPI_Recv(x, 1024*1024, MPI_INT, rank_dst, 2, MPI_COMM_WORLD, NULL);
    MPI_Recv(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD, NULL);
    MPI_Send(&y, 1, MPI_INT, rank_dst, 1, MPI_COMM_WORLD);

    printf("The answer to life, the universe, and everything is %d, %d\n", x[0], x[1024*1024-1]);
    printf("There are %d Wonders of the World\n", y);

    free(x);
  }

  MPI_Finalize();
  exit(0);
}

