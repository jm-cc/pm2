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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int numtasks, rank, i;
  MPI_Datatype newtype;

  struct { 
    int val;
    int rank; 
  } in[10], min_out[10], max_out[10];

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  MPI_Type_contiguous(2, MPI_INT, &newtype);
  MPI_Type_commit(&newtype);

  for (i=0; i<10; ++i) {
    if (i%2) {
      in[i].val = i*10-rank;
    }
    else {
      in[i].val = i*10+rank;
    }
    in[i].rank = rank;
  } 

  printf("[%d] ", rank);
  for(i=0 ; i<10 ; i++) printf("%d ", in[i].val);
  printf("\n");

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Reduce(in, max_out, 10, newtype, MPI_MAXLOC, 0, MPI_COMM_WORLD);
  MPI_Reduce(in, min_out, 10, newtype, MPI_MINLOC, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    for(i=0 ; i<10 ; i++) {
      printf("min out[%d] = %d, %d\n", i, min_out[i].val, min_out[i].rank);
      printf("max out[%d] = %d, %d\n", i, max_out[i].val, max_out[i].rank);
    }
  }

  MPI_Finalize();
  exit(0);
}

