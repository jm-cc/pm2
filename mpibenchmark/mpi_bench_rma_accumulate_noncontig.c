/*
 * NewMadeleine
 * Copyright (C) 2016-2018 (see AUTHORS file)
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
#include <string.h>

static int blocksize = 0;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 4,
    .max  = 1024,
    .mult = 2,
    .incr = 0
  };

static MPI_Win win;

static const struct mpi_bench_param_bounds_s*mpi_bench_accumulate_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_accumulate_noncontig_setparam(int param)
{
  blocksize = param;
}

static void mpi_bench_accumulate_noncontig_server(void*buf, size_t len)
{
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
  MPI_Accumulate(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, 0, 1, noncontig_dtype, MPI_REPLACE, win);
  MPI_Win_unlock(mpi_bench_common.peer, win);
  MPI_Barrier(mpi_bench_common.comm);
  MPI_Barrier(mpi_bench_common.comm);
}

static void mpi_bench_accumulate_noncontig_client(void*buf, size_t len)
{
  MPI_Barrier(mpi_bench_common.comm);
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
  MPI_Accumulate(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, 0, 1, noncontig_dtype, MPI_REPLACE, win);
  MPI_Win_unlock(mpi_bench_common.peer, win);
  MPI_Barrier(mpi_bench_common.comm);
 }

static void mpi_bench_accumulate_noncontig_init(void*buf, size_t len)
{
  mpi_bench_noncontig_type_init(blocksize, len);
  memcpy(noncontig_buf,           buf, len);
  memcpy(noncontig_buf + len,     buf, len);
  memcpy(noncontig_buf + 2 * len, buf, blocksize);
  MPI_Win_create(noncontig_buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
  MPI_Barrier(mpi_bench_common.comm);
}

static void mpi_bench_accumulate_noncontig_finalize(void)
{
  MPI_Win_free(&win);
  mpi_bench_noncontig_type_destroy();
}

const struct mpi_bench_s mpi_bench_rma_accumulate_noncontig =
  {
    .label     = "mpi_bench_rma_accumulate_noncontig",
    .name      = "MPI RMA Non-Contig Accumulate",
    .server    = &mpi_bench_accumulate_noncontig_server,
    .client    = &mpi_bench_accumulate_noncontig_client,
    .init      = &mpi_bench_accumulate_noncontig_init,
    .finalize  = &mpi_bench_accumulate_noncontig_finalize,
    .setparam  = &mpi_bench_accumulate_noncontig_setparam,
    .getparams = &mpi_bench_accumulate_noncontig_getparams
  };

