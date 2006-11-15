#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int numtasks, rank, dest, source, tag;
  int counter, nbpings, message;
  MPI_Status   stat;

  nbpings = 10;
  tag = 1;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    dest = 1;
    source = numtasks-1;

    for(counter=1 ; counter<nbpings ; counter++) {
      MPI_Send(&counter, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
      MPI_Recv(&message, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);

      if (message != counter) {
        fprintf(stderr, "Expected %d - Received %d\n", counter, message);
      }
      else {
        printf("Message %d\n", message);
      }
    }
  }
  else {
    source = rank - 1;
      if (rank == numtasks-1) {
        dest = 0;
      }
      else {
        dest = rank + 1;
      }

    for(counter=1 ; counter<nbpings ; counter++) {
      MPI_Recv(&message, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
      MPI_Send(&message, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
    }
  }
  MPI_Finalize();
  exit(0);
}
