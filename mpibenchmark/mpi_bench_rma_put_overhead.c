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

static void mpi_bench_put_overhead_server(void*buf, size_t len){}

static void mpi_bench_put_overhead_client(void*buf, size_t len)
{
  MPI_Rput(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, win, &request);
  MPI_Wait(&request, MPI_STATUS_IGNORE);
}

static void mpi_bench_put_overhead_init(void*buf, size_t len)
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

static void mpi_bench_put_overhead_finalize(void)
{
  if(!mpi_bench_common.is_server)
    {
      MPI_Win_unlock(mpi_bench_common.peer, win);
    }
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rma_put_overhead =
  {
    .label    = "mpi_bench_rma_put_overhead",
    .name     = "MPI RMA Put Overhead",
    .rtt      = 1,
    .server   = &mpi_bench_put_overhead_server,
    .client   = &mpi_bench_put_overhead_client,
    .init     = &mpi_bench_put_overhead_init,
    .finalize = &mpi_bench_put_overhead_finalize
  };

