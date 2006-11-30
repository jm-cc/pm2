#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int numtasks, rank, dest, source, tag;
  int counter, nbpings, message;
  float buffer[2], r_buffer[2];
  MPI_Status stat;

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
    buffer[0] = 12.45;
    buffer[1] = 3.14;

    for(counter=1 ; counter<nbpings ; counter++) {
      MPI_Send(&counter, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
      MPI_Recv(&message, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);

      if (message != counter) {
        fprintf(stderr, "Expected %d - Received %d\n", counter, message);
      }
      else {
        fprintf(stdout, "Message %d\n", message);
      }
    }
    MPI_Send(buffer, 2, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
    MPI_Recv(r_buffer, 2, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &stat);
    if (r_buffer[0] != buffer[0] && r_buffer[1] != buffer[1]) {
      fprintf(stderr, "Expected [%f,%f] - Received [%f,%f]\n", buffer[0], buffer[1], r_buffer[0], r_buffer[1]);
    }
    else {
      fprintf(stdout, "Message [%f,%f]\n", r_buffer[0], r_buffer[1]);
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
    MPI_Recv(r_buffer, 2, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &stat);
    MPI_Send(r_buffer, 2, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);

  }
  MPI_Finalize();
  exit(0);
}
