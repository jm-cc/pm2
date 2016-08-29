/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

static MPI_Win win;
static MPI_Request request;

static void mpi_bench_get_passive_server(void*buf, size_t len) {}

static void mpi_bench_get_passive_client(void*buf, size_t len)
{
  MPI_Rget(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, win, &request);
  MPI_Wait(&request, MPI_STATUS_IGNORE);
}

static void mpi_bench_get_passive_init(void*buf, size_t len, int count)
{
  if(mpi_bench_common.is_server)
    {
      MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
    }
  else
    {
      MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
    }
}

static void mpi_bench_get_passive_finalize(void)
{
  MPI_Barrier(mpi_bench_common.comm);
  if(!mpi_bench_common.is_server)
    {
      MPI_Win_unlock(mpi_bench_common.peer, win);
    }
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rma_get_passive =
  {
    .label    = "mpi_bench_rma_get_passive",
    .name     = "MPI RMA Get passive",
    .rtt      = MPI_BENCH_RTT_FULL,
    .server   = &mpi_bench_get_passive_server,
    .client   = &mpi_bench_get_passive_client,
    .init     = &mpi_bench_get_passive_init,
    .finalize = &mpi_bench_get_passive_finalize
  };

