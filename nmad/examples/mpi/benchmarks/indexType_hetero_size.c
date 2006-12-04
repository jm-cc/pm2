#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mpi.h"
#include "toolbox.h"

#include "indexType.h"

#define NB_TESTS 1
#define NB_LINES 2

// Check the elements have been properly received
void checkIndexIsCorrect(float *vector, int rank, int numberOfElements) {
  int i, success=TRUE;

  for(i=0 ; i<numberOfElements ; i++) {
    float correctValue = 1.0 * (i+1);
    if (VERBOSE) {
      PRINT("index[%d] = %3.1f", i, vector[i]);
    }
    if (vector[i] != correctValue) {
      PRINT("rank=%d. Failure for size=%d! index[%d]=%3.1f != %3.1f", rank, numberOfElements, i, vector[i], correctValue);
      success = FALSE;
      break;
    }
  }
  if (success == TRUE && PRINT_SUCCESS) {
    PRINT("rank=%d. Successfully transfered the %d elements!", rank, numberOfElements);
  }
}



void sendIndexTypeFromSrcToDest(int numberOfElements, int blocks, int rank, int source,
				int dest, int numtasks, int use_hindex, int display) {
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
  } else {

    assert(blocks % 2 == 0);

    int nb_blocks = blocks / 2;

    for(i = 0; i < blocks; i+=2){
      blocklengths[i]   = 64;
      blocklengths[i+1] = (1 * 1024 *1024);
    }
  }

  displacements[0] = 0;
  for(i = 1 ; i < blocks ; i++) {
    displacements[i] = blocklengths[i-1] + displacements[i-1];
  }

  if (use_hindex == TRUE) {
    for(i=0 ; i<blocks ; i++) {
      displacements[i] *= sizeof(float);
    }
  } // end if
  //printf("FIN construction du datatype\n");


  // create user datatype
  if (use_hindex == TRUE) {
    MPI_Type_hindexed(blocks, blocklengths, (MPI_Aint *)displacements, MPI_CHAR, &indextype);
  }
  else {
    MPI_Type_indexed(blocks, blocklengths, displacements, MPI_CHAR, &indextype);
  }
  //printf("FIN initialisation du datatype\n");


  MPI_Type_commit(&indextype);
  //printf("MPI_Type_commit OK\n");

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
      //printf("FIN initialisation des données\n");


      // send the data to the processor 1
      t1 = MPI_Wtime();

      printf("*************MPI_SEND...\n");
      MPI_Send(data, 1, indextype, dest, TAG, MPI_COMM_WORLD);
      printf("*************... COMPLETED\n");

      // receive data from processor 1
      printf("*************MPI_RECV...\n");
      MPI_Recv(data2, 1, indextype, dest, TAG, MPI_COMM_WORLD, NULL);
      printf("*************... COMPLETED\n");
      //checkIndexIsCorrect(data2, i, numberOfElements);

      t2 = MPI_Wtime();
      sum += (t2 - t1) * 1e6;
    }
    else if (rank == dest) {
      char b[numberOfElements];

      printf("numberOfElements = %d\n", numberOfElements);

      printf("*************MPI_RECV...\n");
      MPI_Recv(b, 1, indextype, source, TAG, MPI_COMM_WORLD, NULL);
      printf("*************... COMPLETED\n");
      // checkIndexIsCorrect(b, rank, numberOfElements);

      printf("*************MPI_SEND...\n");
      MPI_Send(b, 1, indextype, source, TAG, MPI_COMM_WORLD);
      printf("*************... COMPLETED\n");
    }
  }
  
  if (rank == source && display) {
    PRINT("%d\t%d\t%s\t%d\t%d\t%3.2f", source, dest, "indexed_type", numberOfElements, blocks, sum/NB_TESTS);
  }

  MPI_Type_free(&indextype);
}

void processAndSendIndexType(int size, int blocks, int rank, int numtasks, int use_hindex, int display) {
  int source=0;
  for(source = 0 ; source < numtasks ; source += 2) {
    if (source + 1 != numtasks)
      sendIndexTypeFromSrcToDest(size, blocks, rank, source, source+1, numtasks, use_hindex, display);
  }
} // end processAndSendIndexType


int sendIndexType(int argc, char *argv[], int rank, int numtasks) {
  int curSize, ret;
  int use_hindex = FALSE;
  int short_message = TRUE;
  int minSize = -1;
  int maxSize = -1;
  int stride=-1;
  int blocks = -1;
  int blockNumber = 0;
  int size=-1;
  char *tests=NULL;

  int len = 64 + (1 * 1024 *1024);
  int i;

  // check arguments given to program
  ret = checkArguments(argc, argv, 1, &use_hindex, &short_message, &minSize, &maxSize, &stride, &blocks, &size, tests);
  if (ret) return ret;

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  for(i = 1; i <= NB_LINES; i++){
    processAndSendIndexType(len * i, 2 * i, rank, numtasks, use_hindex, FALSE);
    printf("------------------------------------\n");
  }

  return 0;
}
