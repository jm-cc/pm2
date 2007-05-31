#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int rank;
  size_t size = 256;
  char *cwd;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  cwd = malloc(size+1);
  getcwd(cwd, size);

  printf("[%d] Current directory %s\n", rank, cwd);

  free(cwd);
  MPI_Finalize();
  exit(0);
}

