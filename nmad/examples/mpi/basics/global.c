#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void my_operation(void *a, void *b, int *r, MPI_Datatype *type) {
  int *in, *inout, i;

  if (*type != MPI_INT) {
    fprintf(stderr, "Erreur. Argument type not recognized\n");
    exit(-1);
  }
  in = (int *) a;
  inout = (int *) b;
  for(i=0 ; i<*r ; i++) {
    printf("Inout[%d] = %d\n", i, inout[i]);
    inout[i] += in[i];
    inout[i] *= 2;
    printf("Inout[%d] = %d\n", i, inout[i]);
  }
}

int main(int argc, char **argv) {
  int numtasks, rank, global_rank, provided;
  float buffer[2];
  int reduce[2];
  int global_sum[2];
  int global_product[2];
  MPI_Op operator;

  // Initialise MPI
  MPI_Init_thread(&argc,&argv, MPI_THREAD_SINGLE, &provided);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    global_rank = 49;
  }
  MPI_Bcast(&global_rank, 1, MPI_INT, 0, MPI_COMM_WORLD);
  fprintf(stdout, "[%d] Broadcasted message [%d]\n", rank, global_rank);

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

  {
    int reduce[3] = {rank+1, rank+2, rank+3};
    int global_reduce[3];
    MPI_Op_create(my_operation, 1, &operator);
    MPI_Allreduce(reduce, global_reduce, 3, MPI_INT, operator, MPI_COMM_WORLD);
    MPI_Op_free(&operator);
    fprintf(stdout, "[%d] Global reduction [%d,%d,%d]\n", rank, global_reduce[0], global_reduce[1], global_reduce[2]);
  }

  MPI_Finalize();
  exit(0);
}

