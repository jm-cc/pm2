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
#include <string.h>

static int blocksize = 0;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 4,
    .max  = 1024,
    .mult = 2,
    .incr = 0
  };

static const struct mpi_bench_param_bounds_s*mpi_bench_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_noncontig_setparam(int param)
{
  blocksize = param;
}

static void mpi_bench_noncontig_init(void*buf, size_t len)
{
  mpi_bench_noncontig_type_init(blocksize, len);
}

static void mpi_bench_noncontig_finalize(void)
{
  mpi_bench_noncontig_type_destroy();
}

static void mpi_bench_noncontig_server(void*buf, size_t len)
{
  MPI_Recv(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
}

static void mpi_bench_noncontig_client(void*buf, size_t len)
{
  MPI_Send(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
  MPI_Recv(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_noncontig =
  {
    .label      = "mpi_bench_noncontig",
    .name       = "MPI non-contig sendrecv (sparse vector with variable blocksize and stride)",
    .rtt        = MPI_BENCH_RTT_HALF,
    .init       = &mpi_bench_noncontig_init,
    .finalize   = &mpi_bench_noncontig_finalize,
    .server     = &mpi_bench_noncontig_server,
    .client     = &mpi_bench_noncontig_client,
    .setparam   = &mpi_bench_noncontig_setparam,
    .getparams  = &mpi_bench_noncontig_getparams
  };

