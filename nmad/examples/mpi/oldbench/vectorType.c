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

#include <mpi.h>
#include "toolbox.h"
#include "vectorType.h"
#include <math.h>

float getValue(int x, int y, int size) {
  return (y+1)+(size*x);
}

void getIndices(int displacement, int size, int initialValue, int *x, int *y) {
  displacement -= initialValue;
  *x = displacement / size;
  *y = displacement - (size * *x);
}

void checkVectorIsCorrect(float *b, int rank, int count, int blocklength, int size, int stride) {
  int originalDisplacement = 1;
  int currentPosition = 0;
  int success = TRUE;
  int x, y, i, j;
  float initialValue;

  if (VERBOSE) {
    PRINT_NO_NL("b[] = ");
    for(i=0 ; i<count*blocklength ; i++) {
      PRINT_NO_NL("%3.1f ", b[i]);
    }
    PRINT("\n");
  }

  for(i=0 ; i<count ; i++) {
    if (success == TRUE) {
      for(j=0 ; j<blocklength ; j++) {
        getIndices(originalDisplacement, size, getValue(0, 0, size), &x, &y);
        initialValue = getValue(x, y, size);
        if (VERBOSE) {
          PRINT("b[%d] = %3.1f", currentPosition, b[currentPosition]);
        }
        if (initialValue != b[currentPosition]) {
          PRINT("rank=%d. Failure for vectorType. (b[%d] = %3.3f) != (a[%d][%d] = %3.3f [%d]])", rank, currentPosition, b[currentPosition], x, y, initialValue, originalDisplacement);
          success = FALSE;
          break;
        } // end if
        currentPosition ++;
        originalDisplacement ++;
      }
    }
    originalDisplacement += stride-blocklength;
  }

  if (success == TRUE && PRINT_SUCCESS) {
    PRINT("rank=%d. Successfully transfered the %d elements!", rank, count*blocklength);
  }
}

int getRealSize(int size, int blocks) {
  int realSize = size;
  while (realSize % blocks != 0) realSize ++;
  return realSize;
}

void sendVectorTypeFromSrcToDest(int size, int blocks, int rank, int source, int dest,
				 int use_hvector, int display) {
  int realSize = getRealSize(sqrt(size), blocks);
  float a[realSize][realSize];
  MPI_Datatype columntype;
  int i, j;
  float *b;
  MPI_Status stat;
  double t1, t2, sum;

  int count = blocks;
  int blocklength = realSize*realSize/blocks;
  int stride = blocklength;

  if (VERBOSE) {
    PRINT("count=%d blocklength=%d, stride=%d\n", count, blocklength, stride);
  }

  if (rank != source && rank != dest) return;

  // Initialise data to send
  for(i=0 ; i<realSize ; i++) {
    for(j=0 ; j<realSize ; j++) {
      a[i][j] = getValue(i, j, realSize);
      if (VERBOSE) {
        PRINT_NO_NL("%3.1f ", a[i][j]);
      }
    }
    if (VERBOSE) {
      PRINT("\n");
    }
  }

  // create user datatype
  if (use_hvector == TRUE) {
    MPI_Type_hvector(count, blocklength, stride*sizeof(float), MPI_FLOAT, &columntype);
  }
  else {
    MPI_Type_vector(count, blocklength, stride, MPI_FLOAT, &columntype);
  }
  MPI_Type_commit(&columntype);

  if (rank == source) {
    // send data to the process dest
    t1 = MPI_Wtime();
    MPI_Send(&a[0][0], 1, columntype, dest, TAG, MPI_COMM_WORLD);

    // receive the data
    b = (float *) malloc(count*blocklength*sizeof(float));
    MPI_Recv(b, count*blocklength, MPI_FLOAT, dest, TAG, MPI_COMM_WORLD, &stat);
    checkVectorIsCorrect(b, rank, count, blocklength, size, stride);
    free(b);

    t2 = MPI_Wtime();
    sum = (t2 - t1) * 1e6;
    if (display)
      PRINT("%d\t%d\t%s\t%d\t%d\t%3.2f", source, dest, "vector_type", count*blocklength, blocks, sum);
  }
  else if (rank == dest) {
    // receive the data
    b = (float *) malloc(count*blocklength*sizeof(float));
    MPI_Recv(b, count*blocklength, MPI_FLOAT, source, TAG, MPI_COMM_WORLD, &stat);

    checkVectorIsCorrect(b, rank, count, blocklength, realSize, stride);
    free(b);

    MPI_Send(&a[0][0], 1, columntype, source, TAG, MPI_COMM_WORLD);
  }

  MPI_Type_free(&columntype);
}

void processAndsendVectorType(int size, int blocks, int rank, int numtasks, int use_hvector, int display) {
  int source=0;
  for(source = 0 ; source < numtasks ; source += 2) {
    if (source + 1 != numtasks)
      sendVectorTypeFromSrcToDest(size, blocks, rank, source, source+1, use_hvector, display);
  }
} // end processAndsendVectorType

int sendVectorType(int argc, char *argv[], int rank, int numtasks) {
  int curSize, ret;
  int use_hvector = FALSE;
  int short_message = TRUE;
  int minSize = -1;
  int maxSize = -1;
  int stride=-1;
  int blocks = -1;
  int blockNumber = 0;
  int size=-1;
  char *tests = NULL;

  // check arguments given to program
  ret = checkArguments(argc, argv, 1, &use_hvector, &short_message, &minSize, &maxSize, &stride, &blocks, &size, tests);
  if (ret) return ret;

  processAndsendVectorType(1, DEFAULT_BLOCKS, rank, numtasks, use_hvector, FALSE);

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  if (size != -1) {
    if (blocks != -1) {
      for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
        processAndsendVectorType(size, blockNumber, rank, numtasks, use_hvector, TRUE);
      }
    }
    else {
      processAndsendVectorType(size, DEFAULT_BLOCKS, rank, numtasks, use_hvector, TRUE);
    }
  }
  else if (minSize == -1 && maxSize == -1 && stride == -1) {
    if (short_message == TRUE) {
      processAndsendVectorType(SMALL_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_hvector, TRUE);
    }
    else {
      processAndsendVectorType(LONG_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_hvector, TRUE);
    }
  }
  else {
    for(curSize=minSize ; curSize<=maxSize ; curSize+=stride) {
      if (blocks != -1) {
        for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
          processAndsendVectorType(curSize, blockNumber, rank, numtasks, use_hvector, TRUE);
        }
      }
      else {
        processAndsendVectorType(curSize, DEFAULT_BLOCKS, rank, numtasks, use_hvector, TRUE);
      }
    }
  }

  return 0;
}
