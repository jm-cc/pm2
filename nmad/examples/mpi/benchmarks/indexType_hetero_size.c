#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mpi.h"
#include "toolbox.h"

#define NB_TESTS 1
#define NB_LINES 3

void sendIndexTypeFromSrcToDest(int numberOfElements, int blocks, int rank, int source, int dest) {
  int          blocklengths[blocks];
  int          displacements[blocks];
  MPI_Datatype indextype;
  int          i, test;
  double       t1, t2, sum;

  if (rank != source && rank != dest) return;

  // initialise structs for datatype
  if (numberOfElements == 0) {
    for(i=0 ; i<blocks ; i++) {
      blocklengths[i] = 0;
    }
  }
  else {
    assert(blocks % 2 == 0);
    for(i = 0; i < blocks; i+=2){
      blocklengths[i]   = 64;
      blocklengths[i+1] = (1 * 1024 *1024);
    }
  }

  displacements[0] = 0;
  for(i = 1 ; i < blocks ; i++) {
    displacements[i] = blocklengths[i-1] + displacements[i-1];
  }

  MPI_Type_indexed(blocks, blocklengths, displacements, MPI_CHAR, &indextype);
  MPI_Type_commit(&indextype);

  sum = 0;
  for(test=0 ; test<NB_TESTS ; test++) {
    if (rank == source) {
      char data[numberOfElements];
      char data2[numberOfElements];

      // Initialise data to send
      if (VERBOSE) {
        PRINT_NO_NL("data = ");
      }
      for(i=0 ; i<numberOfElements ; i++) {
        data[i] = 'a'+(i%26);
        if (VERBOSE) {
          PRINT_NO_NL("%c ", data[i]);
        }
      }
      if (VERBOSE) {
        PRINT("\n");
      }

      // send the data to the processor 1
      t1 = MPI_Wtime();

      if (VERBOSE) PRINT("*************MPI_SEND...");
      MPI_Send(data, 1, indextype, dest, TAG, MPI_COMM_WORLD);
      if (VERBOSE) PRINT("*************... COMPLETED");

      // receive data from processor 1
      if (VERBOSE) PRINT("*************MPI_RECV...");
      MPI_Recv(data2, 1, indextype, dest, TAG, MPI_COMM_WORLD, NULL);
      if (VERBOSE) PRINT("*************... COMPLETED");

      t2 = MPI_Wtime();
      sum += (t2 - t1) * 1e6;
    }
    else if (rank == dest) {
      char b[numberOfElements];

      if (VERBOSE) PRINT("numberOfElements = %d", numberOfElements);

      if (VERBOSE) PRINT("*************MPI_RECV...");
      MPI_Recv(b, 1, indextype, source, TAG, MPI_COMM_WORLD, NULL);
      if (VERBOSE) PRINT("*************... COMPLETED");

      if (VERBOSE) PRINT("*************MPI_SEND...");
      MPI_Send(b, 1, indextype, source, TAG, MPI_COMM_WORLD);
      if (VERBOSE) PRINT("*************... COMPLETED");
    }
  }

  if (rank == source) {
    PRINT("%d\t%d\t%s\t%d\t%d\t%3.2f", source, dest, "indexed_type", numberOfElements, blocks, sum/NB_TESTS);
  }

  MPI_Type_free(&indextype);
}

void processAndSendIndexType(int size, int blocks, int rank, int numtasks) {
  int source=0;
  for(source = 0 ; source < numtasks ; source += 2) {
    if (source + 1 != numtasks)
      sendIndexTypeFromSrcToDest(size, blocks, rank, source, source+1);
  }
} // end processAndSendIndexType


int main(int argc, char *argv[]) {
  int numtasks, rank, ret;
  int len = 64 + (1 * 1024 *1024);
  int i, b;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  for(i = 1; i <= NB_LINES; i++) {
    for(b=2 ; b<=i*2 ; b*=2) {
      processAndSendIndexType(len * i, b, rank, numtasks);
      if (VERBOSE) PRINT("------------------------------------");
    }
  }

  MPI_Finalize();
  exit(0);
}
