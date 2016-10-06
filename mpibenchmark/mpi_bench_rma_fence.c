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

static void mpi_bench_fence_server(void*buf, size_t len)
{
  MPI_Win_fence(0, win);
}

static void mpi_bench_fence_client(void*buf, size_t len)
{
  MPI_Win_fence(0, win);
}

static void mpi_bench_fence_init(void*buf, size_t len)
{
  MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
}

static void mpi_bench_fence_finalize(void)
{
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rma_fence =
  {
    .label    = "mpi_bench_rma_fence",
    .name     = "MPI RMA Fence",
    .rtt      = 1,
    .server   = &mpi_bench_fence_server,
    .client   = &mpi_bench_fence_client,
    .init     = &mpi_bench_fence_init,
    .finalize = &mpi_bench_fence_finalize
  };

