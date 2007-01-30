#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank;
  int tag = 1;
  MPI_Status stat;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  //printf("Rank %d Size %d\n", rank, numtasks);

  if (rank == 0) {
    int child;
    MPI_Request *out_requests;
    MPI_Request *in_requests;
    float buffer[2], r_buffer[2];

    buffer[0] = 12.45;
    buffer[1] = 3.14;

    out_requests = malloc(numtasks * sizeof(MPI_Request));
    in_requests = malloc(numtasks * sizeof(MPI_Request));

    for(child=1 ; child<numtasks ; child++) {
      MPI_Isend(buffer, 2, MPI_FLOAT, child, tag, MPI_COMM_WORLD, &out_requests[child-1]);
      fprintf(stderr, "ISending to child %d completed\n", child);
    }

    for(child=1 ; child<numtasks ; child++) {
      fprintf(stderr, "Waiting for sending to child %d completed\n", child);
      MPI_Wait(&out_requests[child-1], NULL);
    }

    for(child=1 ; child<numtasks ; child++) {
      MPI_Recv(r_buffer, 2, MPI_FLOAT, child, tag, MPI_COMM_WORLD, &stat);
      if (r_buffer[0] != buffer[0] && r_buffer[1] != buffer[1]) {
	fprintf(stderr, "Expected [%f,%f] - Received [%f,%f]\n", buffer[0], buffer[1], r_buffer[0], r_buffer[1]);
      }
      else {
	fprintf(stdout, "Message from child %d [%f,%f]\n", child, r_buffer[0], r_buffer[1]);
      }
    }

    free(out_requests);
    free(in_requests);
  }
  else {
    int father = 0;
    float r_buffer[2];
    MPI_Recv(r_buffer, 2, MPI_FLOAT, father, tag, MPI_COMM_WORLD, &stat);
    MPI_Send(r_buffer, 2, MPI_FLOAT, father, tag, MPI_COMM_WORLD);
  }
  MPI_Finalize();
  exit(0);
}
