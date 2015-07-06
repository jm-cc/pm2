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
#include <stdio.h>
#include <unistd.h>

#include "indexType.h"
#include "structType.h"
#include "vectorType.h"
#include "toolbox.h"

#define WARM_UP    1

void sendDataFromSourceToDest(int size, int blocks, int rank, int source, int dest, int use_htype, char *tests, int display);
void sendDataFromSource(int size, int blocks, int rank, int source, int numtasks, int use_htype, char *tests, int display);
void sendData(int size, int blocks, int rank, int numtasks, int use_htype, char *tests, char *pairs, int display);
char* createAllPairs(int n);

int main(int argc, char *argv[]) {
  int numtasks, rank;

  int ret, curSize;
  int use_htype = FALSE;
  int short_message = TRUE;
  int minSize = -1;
  int maxSize = -1;
  int stride=-1;
  int blocks = -1;
  int blockNumber = 0;
  int size=-1;
  char tests[50] = "all";
  char *pairs = NULL;
  int startPos = 1;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  // is -help required
  if (argc > 1 && !strcmp(argv[1], "-help")) {
    if (rank == 0) {
      PRINT("Available options:");
      PRINT("\t-showRanksOnly     Show processors ranks and exit");
      PRINT("\t-showRanks         Show processor ranks");
      PRINT("\t-setPairs <pairs>  Perform pingpong between specified pairs");
      PRINT("\t-setAllPairs       Perform pingpong between all pairs");
      PRINT("\t-hindex            Use hindex instead of index");
      PRINT("\t-hvector           Use hvector instead of vector");
      PRINT("\t-short             Perform pingpong with short messages only");
      PRINT("\t-long              Perform pingpong with long messages only");
      PRINT("\t-blocks <size>     Set the size of the block for the datatypes");
      PRINT("\t-min <size>        Set the minimum size of the messages");
      PRINT("\t-max <size>        Set the maximium size of the messages");
      PRINT("\t-stride <size>     Set the stride for the message size");
      PRINT("\t-size <size>       Perform pingpong for the specified message size only");
      PRINT("\t-tests <tests>     Perform the specified tests only");
    }
    MPI_Finalize();
    exit(0);
  }

  // check arguments given to program
  if (argc > 1 && !strcmp(argv[1], "-showRanksOnly")) {
    char name[100];
    size_t len = 99;
    gethostname(name, len);
    PRINT("(%s) rank %d", name, rank);
    MPI_Finalize();
    exit(0);
  } // end if

  if (argc > 1 && !strcmp(argv[1], "-showRanks")) {
    char name[100];
    size_t len = 99;
    gethostname(name, len);
    PRINT("(%s) rank %d", name, rank);
    startPos = 2;
  } // end if

  if (argc > 1 && !strcmp(argv[1], "-setPairs") && argc > 2) {
    pairs = strdup(argv[2]);
    startPos = 3;
  }

  if (argc > 1 && !strcmp(argv[1], "-setAllPairs")) {
    pairs = createAllPairs(numtasks);
    startPos = 2;
  }

  ret = checkArguments(argc, argv, startPos, &use_htype, &short_message, &minSize, &maxSize, &stride, &blocks, &size, tests);
  if (ret) return ret;

  // Send data of size 0 to initialise timers
  if (WARM_UP) {
    sendData(0, 3, rank, numtasks, use_htype, "vector", pairs, FALSE);
  }

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  // Process with the ping-pongs
  if (size != -1) {
    if (blocks != -1) {
      for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
        sendData(size, blockNumber, rank, numtasks, use_htype, tests, pairs, TRUE);
      }
    }
    else {
      sendData(size, DEFAULT_BLOCKS, rank, numtasks, use_htype, tests, pairs, TRUE);
    }
  }
  else if (minSize != -1 && maxSize != -1 && stride != -1) {
    for(curSize=minSize ; curSize<=maxSize ; curSize+=stride) {
      if (blocks != -1) {
        for(blockNumber = 1 ; blockNumber <= blocks ; blockNumber++) {
          sendData(curSize, blockNumber, rank, numtasks, use_htype, tests, pairs, TRUE);
        }
      }
      else {
        sendData(curSize, DEFAULT_BLOCKS, rank, numtasks, use_htype, tests, pairs, TRUE);
      }
    }
  }
  else {
    if (short_message == TRUE) {
      sendData(SMALL_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_htype, tests, pairs, TRUE);
    }
    else {
      sendData(LONG_SIZE, DEFAULT_BLOCKS, rank, numtasks, use_htype, tests, pairs, TRUE);
    }
  }

  MPI_Finalize();
  exit(0);
}

void sendDataFromSourceToDest(int size, int blocks, int rank, int source, int dest, int use_htype, char *tests, int display) {
  if (rank != source && rank != dest) return;

  if ((strstr(tests, "vector") != NULL) || (strstr(tests, "all") != NULL)) {
    sendVectorTypeFromSrcToDest(size, blocks, rank, source, dest, use_htype, display);
  }
  if ((strstr(tests, "index") != NULL) || (strstr(tests, "all") != NULL)) {
    sendIndexTypeFromSrcToDest(size, blocks, rank, source, dest, use_htype, display);
  }
  if ((strstr(tests, "struct") != NULL) || (strstr(tests, "all") != NULL)) {
    sendStructTypeFromSrcToDest(size, rank, source, dest, display);
  }
}

void sendDataFromSource(int size, int blocks, int rank, int source, int numtasks, int use_htype, char *tests, int display) {
  int dest;
  for(dest = source+1 ; dest < numtasks ; dest ++) {
    sendDataFromSourceToDest(size, blocks, rank, source, dest, use_htype, tests, display);
  }
}

void sendData(int size, int blocks, int rank, int numtasks, int use_htype, char *tests, char *pairs, int display) {
  int source=0;
  if (pairs == NULL) {
    for(source = 0 ; source < numtasks ; source ++) {
      sendDataFromSource(size, blocks, rank, source, numtasks, use_htype, tests, display);
    }
  }
  else {
    char *rdup = NULL;
    char *token = NULL;
    int dest;

    // pairs is expected to be a string of numbers delimited by the character "-"
    // such as x1-x2-x3-x4-x5-x6. Pingpong will be achieved between the pairs of
    // processors x1-x2, x3-x4, ...
    rdup = strdup(pairs);
    token = strtok(rdup, "-");
    while (token != NULL) {
      source = atoi(token);
      token = strtok(NULL, "-");
      if (token != NULL) {
	dest = atoi(token);
	sendDataFromSourceToDest(size, blocks, rank, source, dest, use_htype, tests, display);
	token = strtok(NULL, "-");
      }
    }
    free(rdup);
  }
}

char* createAllPairs(int n) {
  char *pairs = (char *) malloc(1000);
  int i, j;
  for(i=0 ; i<n ; i++) {
    for(j=0 ; j<n ; j++) {
      if (i != j) {
	sprintf(pairs, "%s-%d-%d", pairs, i, j);
      }
    }
  }
  return pairs;
}
