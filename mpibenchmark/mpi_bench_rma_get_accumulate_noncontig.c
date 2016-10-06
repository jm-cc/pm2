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
#include <string.h>

#define BLOCKSIZE 32

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
static void*result_buf = NULL;
static void*sparse_buf = NULL;
static MPI_Datatype dtype = MPI_DATATYPE_NULL;


static const struct mpi_bench_param_bounds_s*mpi_bench_get_accumulate_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_get_accumulate_noncontig_setparam(int param)
{
  blocksize = param;
}

static void mpi_bench_get_accumulate_noncontig_server(void*buf, size_t len) {}

static void mpi_bench_get_accumulate_noncontig_client(void*buf, size_t len)
{
  MPI_Rget_accumulate(sparse_buf, 1, dtype, result_buf, 1, dtype,
		      mpi_bench_common.peer, 0, 1, dtype, MPI_REPLACE, win, &request);
  MPI_Wait(&request, MPI_STATUS_IGNORE);
}

static void mpi_bench_get_accumulate_noncontig_init(void*buf, size_t len)
{
  const size_t bufsize   = 2 * len + blocksize;
  sparse_buf = malloc(bufsize);
  memcpy(sparse_buf,           buf, len);
  memcpy(sparse_buf + len,     buf, len);
  memset(sparse_buf + 2 * len, 0, blocksize);
  if(mpi_bench_common.is_server)
    {
      MPI_Win_create(sparse_buf, bufsize, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
    }
  else
    {
      result_buf = malloc(bufsize);
      memset(result_buf, 0, len);
      MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &dtype);
      MPI_Type_commit(&dtype);
      MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
    }
    MPI_Barrier(mpi_bench_common.comm);
}

static void mpi_bench_get_accumulate_noncontig_finalize(void)
{
  if(!mpi_bench_common.is_server)
    {
      MPI_Win_unlock(mpi_bench_common.peer, win);
      free(result_buf);
    }
  MPI_Win_free(&win);
  if(dtype != MPI_DATATYPE_NULL)
    {
      MPI_Type_free(&dtype);
    }
  free(sparse_buf);
  sparse_buf = NULL;
}

const struct mpi_bench_s mpi_bench_rma_get_accumulate_noncontig =
  {
    .label     = "mpi_bench_rma_get_accumulate_noncontig",
    .name      = "MPI RMA Non-Contig Get_Accumulate",
    .rtt       = 1,
    .server    = &mpi_bench_get_accumulate_noncontig_server,
    .client    = &mpi_bench_get_accumulate_noncontig_client,
    .init      = &mpi_bench_get_accumulate_noncontig_init,
    .finalize  = &mpi_bench_get_accumulate_noncontig_finalize,
    .setparam  = &mpi_bench_get_accumulate_noncontig_setparam,
    .getparams = &mpi_bench_get_accumulate_noncontig_getparams
  };

