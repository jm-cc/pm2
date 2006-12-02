#include "mpi.h"
#include "indexType.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int numtasks, rank, ret;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  ret = sendIndexType(argc, argv, rank, numtasks);

  MPI_Finalize();
  exit(ret);
}
