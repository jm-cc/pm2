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
static MPI_Request request;

static const struct mpi_bench_param_bounds_s*mpi_bench_get_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_get_noncontig_setparam(int param)
{
  blocksize = param;
}

static void mpi_bench_get_noncontig_server(void*buf, size_t len){}

static void mpi_bench_get_noncontig_client(void*buf, size_t len)
{
  MPI_Rget(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, 0, 1, noncontig_dtype, win, &request);
  MPI_Wait(&request, MPI_STATUS_IGNORE);
}

static void mpi_bench_get_noncontig_init(void*buf, size_t len)
{
  mpi_bench_noncontig_type_init(blocksize, len);
  if(mpi_bench_common.is_server)
    {
      memcpy(noncontig_buf,           buf, len);
      memcpy(noncontig_buf + len,     buf, len);
      memcpy(noncontig_buf + 2 * len, buf, blocksize);
      MPI_Win_create(noncontig_buf, noncontig_bufsize, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
    }
  else
    {
      MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
    }
}

static void mpi_bench_get_noncontig_finalize(void)
{
  MPI_Barrier(mpi_bench_common.comm);
  if(!mpi_bench_common.is_server)
    {
      MPI_Win_unlock(mpi_bench_common.peer, win);
    }
  MPI_Win_free(&win);
  mpi_bench_noncontig_type_destroy();
}

const struct mpi_bench_s mpi_bench_rma_get_noncontig =
  {
    .label     = "mpi_bench_rma_get_noncontig",
    .name      = "MPI RMA Non-Contig Get",
    .rtt       = 1,
    .server    = &mpi_bench_get_noncontig_server,
    .client    = &mpi_bench_get_noncontig_client,
    .init      = &mpi_bench_get_noncontig_init,
    .finalize  = &mpi_bench_get_noncontig_finalize,
    .setparam  = &mpi_bench_get_noncontig_setparam,
    .getparams = &mpi_bench_get_noncontig_getparams
  };

