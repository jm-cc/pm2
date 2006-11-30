#include "mpi.h"
#include "structType.h"

int main(int argc, char *argv[]) {
  int numtasks, rank, ret;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  ret = sendStructType(argc, argv, rank, numtasks);

  MPI_Finalize();
  exit(ret);
}
