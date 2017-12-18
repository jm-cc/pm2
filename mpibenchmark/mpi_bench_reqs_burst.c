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

static MPI_Request*reqs = NULL;

static void mpi_bench_reqs_burst_init(void*buf, size_t len)
{
  reqs = realloc(reqs, len * sizeof(MPI_Request));
}

static void mpi_bench_reqs_burst_server(void*buf, size_t len)
{
  const int tag = 0;
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
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
      MPI_Isend(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

static void mpi_bench_reqs_burst_client(void*buf, size_t len)
{
  const int tag = 0;
  int rc;
  int i;
  for(i = 0; i < len; i++)
    {
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
      MPI_Irecv(buf + i, 1, MPI_CHAR, mpi_bench_common.peer, tag, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(len, reqs, MPI_STATUS_IGNORE);
  if(rc != MPI_SUCCESS)
    {
      fprintf(stderr, "MPI_Waitall- rc = %d\n", rc);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

const struct mpi_bench_s mpi_bench_reqs_burst =
  {
    .label     = "mpi_bench_reqs_burst",
    .name      = "MPI reqs_burst ping",
    .init      = &mpi_bench_reqs_burst_init,
    .server    = &mpi_bench_reqs_burst_server,
    .client    = &mpi_bench_reqs_burst_client,
    .setparam  = NULL,
    .getparams = NULL
  };

