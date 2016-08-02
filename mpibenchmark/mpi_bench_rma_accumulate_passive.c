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

static void mpi_bench_accumulate_passive_server(void*buf, size_t len) { }

static void mpi_bench_accumulate_passive_client(void*buf, size_t len)
{
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
  MPI_Accumulate(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, MPI_REPLACE, win);
  MPI_Win_unlock(mpi_bench_common.peer, win);
 }

static void mpi_bench_accumulate_passive_init(void*buf, size_t len, int count)
{
  MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
  MPI_Barrier(mpi_bench_common.comm);
}

static void mpi_bench_accumulate_passive_finalize(void)
{
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rma_accumulate_passive =
  {
    .label    = "mpi_bench_rma_accumulate_passive",
    .name     = "MPI RMA Accumulate passive",
    .rtt      = 1,
    .server   = &mpi_bench_accumulate_passive_server,
    .client   = &mpi_bench_accumulate_passive_client,
    .init     = &mpi_bench_accumulate_passive_init,
    .finalize = &mpi_bench_accumulate_passive_finalize
  };

