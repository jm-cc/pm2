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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE (8 * 1024 * 1024)

int main(int argc, char **argv)
{
  int numtasks, rank, i, flag;
  char *buffer;
  MPI_Request *requests;
  
  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  
  if(numtasks != 2)
    {
      fprintf(stderr, "This program requires two machines, aborting...\n");
      printf("failed (wrong configuration)\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
  }
  requests = malloc(5 * sizeof(MPI_Request));
  buffer = calloc(SIZE, sizeof(char));
  
  if(rank == 0)
    {
      MPI_Irecv(buffer, SIZE, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[0]);
      MPI_Iprobe(1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
      MPI_Wait(&requests[0], MPI_STATUS_IGNORE);

      MPI_Irecv(buffer, SIZE, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[1]);
      MPI_Iprobe(1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
      MPI_Wait(&requests[1], MPI_STATUS_IGNORE);

      MPI_Irecv(buffer, SIZE, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[2]);
      MPI_Irecv(buffer, SIZE, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[3]);
      MPI_Irecv(buffer, SIZE, MPI_CHAR, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[4]);

      MPI_Wait(&requests[2], MPI_STATUS_IGNORE);
      /* Wait for req 3 and 4 in a reverse order */
      MPI_Wait(&requests[4], MPI_STATUS_IGNORE);
      MPI_Wait(&requests[3], MPI_STATUS_IGNORE);
    }
  else
    {
      for(i = 0; i < 5; i++)
	{
	  MPI_Isend(buffer, SIZE, MPI_CHAR, 0, i*42, MPI_COMM_WORLD, &requests[i]);
	  MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
	}    
    }
  
  free(buffer);
  free(requests);

  MPI_Finalize();
  printf("success\n");
  return 0;
}

