#include <stdio.h>
#include "mpi.h"

void hello(int ncpu, int nthread, int procid, int threadid) {
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int  namelen;

  MPI_Get_processor_name(processor_name,&namelen);

  printf("[%s] Running on %d hosts, %d threads - hello from process %d, thread %d\n",
         processor_name, ncpu, nthread, procid, threadid);
}

int main(int argc, char **argv) {
  int rank, size;
  int ithread, nthread;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Barrier(MPI_COMM_WORLD);

  #pragma omp parallel private(ithread)
  {
    #pragma omp single
    {
      nthread = omp_get_num_threads();
    }

    ithread = omp_get_thread_num();

    hello(size, nthread, rank, ithread);
  }

  MPI_Finalize();
}
