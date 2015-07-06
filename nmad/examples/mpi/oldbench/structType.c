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
#include "structType.h"

void checkStructIsCorrect(Particle *subparticle, int size, int rank) {
  int i, error=-1;

  for(i = 0 ; i<size ; i++) {
    if (subparticle[i].x != (i+1) * 2.0) {
      error = i;
      PRINT("rank=%d. Failure for particle %d --> %3.2f",
	    rank, error, subparticle[error].x);
      break;
    }
    else if (subparticle[i].y != (i+1) * -2.0) {
      error = i;
      PRINT("rank=%d. Failure for particle %d --> %3.2f",
	    rank, error, subparticle[error].y);
      break;
    }
    else if (subparticle[i].c != (i+1) * 4) {
      error = i;
      PRINT("rank=%d. Failure for particle %d --> %d",
	    rank, error, subparticle[error].c);
      break;
    }
    else if (subparticle[i].z != (i+1) * 4.0) {
      error = i;
      PRINT("rank=%d. Failure for particle %d --> %3.2f",
	    rank, error, subparticle[error].z);
      break;
    }
  }

  if (error == -1 && PRINT_SUCCESS)
    PRINT("rank=%d. Successfully transfered the %d particles", rank, size);
}

void sendStructTypeFromSrcToDest(int numberOfElements, int rank, int source, int dest,
				 int display) {
  MPI_Datatype particletype;
  MPI_Aint offsets[3];
  MPI_Datatype oldtypes[3];
  int blockcounts[3];
  Particle *particles;
  Particle *p;
  int i;
  MPI_Status stat;
  double t1, t2, sum;

  if (rank != source && rank != dest) return;

  // create the datatype to send and receive the data
  oldtypes[0] = MPI_FLOAT;
  oldtypes[1] = MPI_INT;
  oldtypes[2] = MPI_FLOAT;
  offsets[0] = 0;
  offsets[1] = 2 * sizeof(float);
  offsets[2] = offsets[1] + sizeof(int);
  blockcounts[0] = 2;
  blockcounts[1] = 1;
  blockcounts[2] = 1;

  /* Now define structured types and commit them */
  MPI_Type_struct(3, blockcounts, offsets, oldtypes, &particletype);
  MPI_Type_commit(&particletype);

  // Initialize the particle array and then send it to each task
  if (rank == source) {
    // Initialise data to send
    particles = (Particle *) malloc(numberOfElements * sizeof(Particle));
    for (i=0; i < numberOfElements; i++) {
      particles[i].x = (i+1) * 2.0;
      particles[i].y = (i+1) * -2.0;
      particles[i].c = (i+1) * 4;
      particles[i].z = (i+1) * 4.0;
    }

    t1 = MPI_Wtime();
    MPI_Send(particles, numberOfElements, particletype, dest, TAG, MPI_COMM_WORLD);

    p = (Particle *) malloc(numberOfElements * sizeof(Particle));
    MPI_Recv(p, numberOfElements, particletype, dest, TAG, MPI_COMM_WORLD, &stat);
    //      checkStructIsCorrect(p, numberOfElements, rank);
    free(p);

    t2 = MPI_Wtime(); 
    sum = (t2 - t1) * 1e6;
    if (display)
      PRINT("%d\t%d\t%s\t%d\t%s\t%3.2f", source, dest, "struct_type", numberOfElements, "-", sum);
    free(particles);
  } // end if
  else if (rank == dest) {
    p = (Particle *) malloc(numberOfElements * sizeof(Particle));
    MPI_Recv(p, numberOfElements, particletype, source, TAG, MPI_COMM_WORLD, &stat);
    checkStructIsCorrect(p, numberOfElements, rank);

    MPI_Send(p, numberOfElements, particletype, source, TAG, MPI_COMM_WORLD);

    free(p);
  } // end else

  MPI_Type_free(&particletype);
}

void processAndSendStructType(int size, int rank, int numtasks, int display) {
  int source=0;
  for(source = 0 ; source < numtasks ; source += 2) {
    if (source + 1 != numtasks)
      sendStructTypeFromSrcToDest(size, rank, source, source+1, display);
  }
} // end processAndSendStructType


int sendStructType(int argc, char *argv[], int rank, int numtasks) {
  int size = -1;
  int ret;
  int use_hindex = FALSE;
  int short_message = TRUE;
  int minSize = -1;
  int maxSize = -1;
  int stride=-1;
  int blocks = -1;
  char *tests = NULL;

  // check arguments given to program
  ret = checkArguments(argc, argv, 1, &use_hindex, &short_message, &minSize, &maxSize, &stride, &blocks, &size, tests);
  if (ret) return ret;

  processAndSendStructType(0, rank, numtasks, FALSE);

  if (rank == 0) {
    PRINT("src  | dest  | type	     | size    | blocks | time");
  }

  if (size != -1) {
    processAndSendStructType(size, rank, numtasks, TRUE);
  }
  else if (minSize == -1 && maxSize == -1 && stride == -1) {
    if (short_message == TRUE) {
      processAndSendStructType(SMALL_SIZE, rank, numtasks, TRUE);
    }
    else {
      processAndSendStructType(LONG_SIZE, rank, numtasks, TRUE);
    }
  }
  else {
    for(size=minSize ; size<=maxSize ; size+=stride) {
      processAndSendStructType(size, rank, numtasks, TRUE);
    }
  }

  return 0;
}
