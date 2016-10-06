/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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

static void*sparse_buf = NULL;
static MPI_Datatype dtype = MPI_DATATYPE_NULL;

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
  const int sparse_size = len * 2 + blocksize;
  sparse_buf = malloc(sparse_size);
  memset(sparse_buf, 0, sparse_size);
  MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &dtype);
  MPI_Type_commit(&dtype);
}

static void mpi_bench_noncontig_finalize(void)
{
  if(dtype != MPI_DATATYPE_NULL)
    {
      MPI_Type_free(&dtype);
    }
  free(sparse_buf);
  sparse_buf = NULL;
}

static void mpi_bench_noncontig_server(void*buf, size_t len)
{
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
}

static void mpi_bench_noncontig_client(void*buf, size_t len)
{
  MPI_Send(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_noncontig =
  {
    .label      = "mpi_bench_noncontig",
    .name       = "MPI non-contig sendrecv",
    .rtt        = 0,
    .init       = &mpi_bench_noncontig_init,
    .finalize   = &mpi_bench_noncontig_finalize,
    .server     = &mpi_bench_noncontig_server,
    .client     = &mpi_bench_noncontig_client,
    .setparam   = &mpi_bench_noncontig_setparam,
    .getparams  = &mpi_bench_noncontig_getparams
  };

