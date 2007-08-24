/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "mpi.h"
#include "toolbox.h"
#include "indexType.h"
#include <stdio.h>
#include <stdlib.h>

#define NB_TESTS 1000

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
				int dest, int use_hindex, int display) {
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
    for(i=0 ; i<blocks ; i++) {
      blocklengths[i] = numberOfElements/blocks;
    }
    blocklengths[blocks-1] += numberOfElements % blocks;
  }

  displacements[0] = 0;
  for(i=1 ; i<blocks ; i++) {
    displacements[i] = blocklengths[i-1] + displacements[i-1];
  }

  if (use_hindex == TRUE) {
    for(i=0 ; i<blocks ; i++) {
      displacements[i] *= sizeof(float);
    }
  } // end if

  // create user datatype
  if (use_hindex == TRUE) {
    MPI_Type_hindexed(blocks, blocklengths, (MPI_Aint *)displacements, MPI_FLOAT, &indextype);
  }
  else {
    MPI_Type_indexed(blocks, blocklengths, displacements, MPI_FLOAT, &indextype);
  }

  MPI_Type_commit(&indextype);

  sum = 0;
  for(test=0 ; test<NB_TESTS ; test++) {
    if (rank == source) {
      float        data[numberOfElements];
      float        data2[numberOfElements];

      // Initialise data to send
      if (VERBOSE) {
        PRINT_NO_NL("data = ");
      }
      for(i=0 ; i<numberOfElements ; i++) {
        data[i] = 1.0 * (i+1);
        if (VERBOSE) {
          PRINT_NO_NL("%3.1f ", data[i]);
        }
      }
      if (VERBOSE) {
        PRINT("\n");
      }

      // send the data to the processor 1
      t1 = MPI_Wtime();
      MPI_Send(data, 1, indextype, dest, TAG, MPI_COMM_WORLD);

      // receive data from processor 1
      //    MPI_Recv(data2, numberOfElements, MPI_FLOAT, dest, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(data2, 1, indextype, dest, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      //checkIndexIsCorrect(data2, i, numberOfElements);

      t2 = MPI_Wtime(); 
      sum += (t2 - t1) * 1e6;
    }
    else if (rank == dest) {
      float b[numberOfElements];
      //    MPI_Recv(b, numberOfElements, MPI_FLOAT, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(b, 1, indextype, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // checkIndexIsCorrect(b, rank, numberOfElements);

      MPI_Send(b, 1, indextype, source, TAG, MPI_COMM_WORLD);
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
      sendIndexTypeFromSrcToDest(size, blocks, rank, source, source+1, use_hindex, display);
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

  // check arguments given to program
  ret = checkArguments(argc, argv, 1, &use_hindex, &short_message, &minSize, &maxSize, &stride, &blocks, &size, tests);
  if (ret) return ret;

  processAndSendIndexType(0, DEFAULT_BLOCKS, rank, numtasks, use_hindex, FALSE);

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  if (size != -1) {
    if (blocks != -1) {
      for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
        processAndSendIndexType(size, blockNumber, rank, numtasks, use_hindex, TRUE);
      }
    }
    else {
      processAndSendIndexType(size, DEFAULT_BLOCKS, rank, numtasks, use_hindex, TRUE);
    }
  }
  else if (minSize == -1 && maxSize == -1 && stride == -1) {
    if (short_message == TRUE) {
      processAndSendIndexType(SMALL_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_hindex, TRUE);
    }
    else {
      processAndSendIndexType(LONG_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_hindex, TRUE);
    }
  }
  else {
    for(curSize=minSize ; curSize<=maxSize ; curSize+=stride) {
      if (blocks != -1) {
        for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
          processAndSendIndexType(curSize, blockNumber, rank, numtasks, use_hindex, TRUE);
        }
      }
      else {
        processAndSendIndexType(curSize, DEFAULT_BLOCKS, rank, numtasks, use_hindex, TRUE);
      }
    }
  }

  return 0;
}
