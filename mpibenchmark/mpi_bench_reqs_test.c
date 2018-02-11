/*
 * NewMadeleine
 * Copyright (C) 2015-2018 (see AUTHORS file)
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

static void mpi_bench_reqs_test_init(void*buf, size_t len)
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

static void mpi_bench_reqs_test_finalize(void)
{
  free(reqs);
  reqs = NULL;
  free(shuffle_table);
  shuffle_table = NULL;
}

static void mpi_bench_reqs_wait_all(MPI_Request*reqs, size_t count)
{
  int ok = 0;
  while(ok < count)
    {
      ok = 0;
      int i;
      for(i = 0; i < count; i++)
	{
	  int flag = 0;
	  MPI_Test(&reqs[i], &flag, MPI_STATUS_IGNORE);
	  if(flag)
	    ok++;
	}
    }
}

static void mpi_bench_reqs_test_server(void*buf, size_t len)
{
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
      const int tag = i;
      MPI_Irecv(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  mpi_bench_reqs_wait_all(reqs, len);
  for(i = 0; i < len; i++)
    {
      const int tag = shuffle_table[i];
      MPI_Isend(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  mpi_bench_reqs_wait_all(reqs, len);
}

static void mpi_bench_reqs_test_client(void*buf, size_t len)
{
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
      const int tag = shuffle_table[i];
      MPI_Isend(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  mpi_bench_reqs_wait_all(reqs, len);
  for(i = 0; i < len; i++)
    {
      const int tag = i;
      MPI_Irecv(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  mpi_bench_reqs_wait_all(reqs, len);
}

const struct mpi_bench_s mpi_bench_reqs_test =
  {
    .label     = "mpi_bench_reqs_test",
    .name      = "MPI reqs_test ping",
    .init      = &mpi_bench_reqs_test_init,
    .finalize  = &mpi_bench_reqs_test_finalize,
    .server    = &mpi_bench_reqs_test_server,
    .client    = &mpi_bench_reqs_test_client,
    .setparam  = NULL,
    .getparams = NULL
  };

