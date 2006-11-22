#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  float buffer[2];
  int reduce[2];
  int global_sum[2];
  int global_product[2];

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  buffer[0] = 12.45;
  buffer[1] = 3.14;
  MPI_Bcast(buffer, 2, MPI_FLOAT, 0, MPI_COMM_WORLD);
  fprintf(stdout, "[%d] Broadcasted message [%f,%f]\n", rank, buffer[0], buffer[1]);

  reduce[0] = rank*10;
  reduce[1] = 15+rank;
  MPI_Reduce(reduce, global_sum, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    fprintf(stdout, "[%d] Global sum [%d,%d]\n", rank, global_sum[0], global_sum[1]);
  }

  reduce[0] = (rank+1)*2;
  reduce[1] = rank+1;
  MPI_Allreduce(reduce, global_product, 2, MPI_INT, MPI_PROD, MPI_COMM_WORLD);
  fprintf(stdout, "[%d] Global product [%d,%d]\n", rank, global_product[0], global_product[1]);

  MPI_Finalize();
  exit(0);
}

