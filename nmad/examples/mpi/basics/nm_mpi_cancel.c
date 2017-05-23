/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

void check_buffer(char *buffer, int *strides, int *blocklengths);
void exchange_index(int ping_side, int rank_dst);

int main(int argc, char **argv) {
  int numtasks, rank;
  int rank_dst, ping_side;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if (numtasks % 2 != 0) {
    printf("Need odd size of processes (%d)\n", numtasks);
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  exchange_index(ping_side, rank_dst);

  MPI_Finalize();
  exit(0);
}

void check_buffer(char *buffer, int *strides, int *blocklengths)
{
  int i = 0, success = 1;
  int nb, count, blength;
  for(nb = 0; nb < 3; nb++)
    {
      int j = 0;
      for(count = 0; count < 3; count++)
	{
	  char value = 'a' + (nb * (strides[2] + blocklengths[2]));
	  value += strides[j];
	  for(blength = 0; blength < blocklengths[j]; blength++)
	    {
	      if(buffer[i] != value)
		{
		  success = 0;
		  break;
		}
	      i++;
	      value++;
	    }
	  j++;
	}
    }
  if (success)
    {
      printf("Index successfully received\n");
    }
  else
    {
      printf("Incorrect index: [ ");
      for(i=0 ; i<18 ; i++)
	printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
      printf("]\n");
    }
}

void exchange_index(int ping_side, int rank_dst)
{
  MPI_Datatype mytype;
  int blocklengths[3] = {1, 2, 3};
  int strides[3] = {0, 2, 3};

  char buffer[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g','h', 'i' ,'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

  MPI_Type_indexed(3, blocklengths, strides, MPI_CHAR, &mytype);
  MPI_Type_commit(&mytype);

  if(ping_side)
    {
      MPI_Send(buffer, 3, mytype, rank_dst, 10, MPI_COMM_WORLD);
    }
  else
    {
      MPI_Request recv_request;
      char buffer2[18];
      MPI_Irecv(buffer2, 3, mytype, rank_dst, 10, MPI_COMM_WORLD, &recv_request);
      MPI_Cancel(&recv_request);
      MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
    }

  MPI_Type_free(&mytype);
  fprintf(stdout, "success\n");
}

