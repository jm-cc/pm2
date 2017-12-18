/*
 * NewMadeleine
 * Copyright (C) 2015-2017 (see AUTHORS file)
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

#include "mpi_bench_generic.h"
#include <stdlib.h>
#include <unistd.h>

static MPI_Request*reqs = NULL;
static int*shuffle_table = NULL;

static void mpi_bench_reqs_shuffle_init(void*buf, size_t len)
{
  reqs = malloc(len * sizeof(MPI_Request));
  shuffle_table = malloc(len * sizeof(int));
  int i;
  srand(getpid() + mpi_bench_common.peer);
  for(i = 0; i < len; i++)
    {
      shuffle_table[i] = i;
    }
  for(i = 0; i < len; i++)
    {
      const int s = rand() % len;
      if(s != i)
	{
	  int temp = shuffle_table[s];
	  shuffle_table[s] = shuffle_table[i];
	  shuffle_table[i] = temp;
	}
    }
}

static void mpi_bench_reqs_shuffle_finalize(void)
{
  free(reqs);
  reqs = NULL;
  free(shuffle_table);
  shuffle_table = NULL;
}

static void mpi_bench_reqs_shuffle_server(void*buf, size_t len)
{
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
      const int tag = i;
      MPI_Irecv(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  for(i = 0; i < len; i++)
    {
      const int tag = shuffle_table[i];
      MPI_Isend(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

static void mpi_bench_reqs_shuffle_client(void*buf, size_t len)
{
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
      const int tag = shuffle_table[i];
      MPI_Isend(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  for(i = 0; i < len; i++)
    {
      const int tag = i;
      MPI_Irecv(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

const struct mpi_bench_s mpi_bench_reqs_shuffle =
  {
    .label     = "mpi_bench_reqs_shuffle",
    .name      = "MPI reqs_shuffle ping",
    .init      = &mpi_bench_reqs_shuffle_init,
    .finalize  = &mpi_bench_reqs_shuffle_finalize,
    .server    = &mpi_bench_reqs_shuffle_server,
    .client    = &mpi_bench_reqs_shuffle_client,
    .setparam  = NULL,
    .getparams = NULL
  };

