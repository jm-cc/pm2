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

static int compute = MIN_COMPUTE;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = MIN_COMPUTE,
    .max  = MAX_COMPUTE,
    .mult = MULT_COMPUTE,
    .incr = 1
  };

static const struct mpi_bench_param_bounds_s*mpi_bench_rmaoverlap_put_passive_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_rmaoverlap_put_passive_setparam(int param)
{
  compute = param;
}

static void mpi_bench_rmaoverlap_put_passive_server(void*buf, size_t len)
{
  MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
  mpi_bench_do_compute(compute);
  MPI_Win_free(&win);
  mpi_bench_ack_recv();
}

static void mpi_bench_rmaoverlap_put_passive_client(void*buf, size_t len)
{
  MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_bench_common.peer, 0, win);
  MPI_Put(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, win);
  MPI_Win_unlock(mpi_bench_common.peer, win);
  MPI_Win_free(&win);
  mpi_bench_ack_send();
}

static void mpi_bench_rmaoverlap_put_passive_init(void*buf, size_t len, int count)
{
}

static void mpi_bench_rmaoverlap_put_passive_finalize(void)
{
}

const struct mpi_bench_s mpi_bench_rmaoverlap_put_passive =
  {
    .label     = "mpi_bench_rmaoverlap_put_passive",
    .name      = "MPI passive put, overlap on target sides",
    .rtt       = MPI_BENCH_RTT_SUBLAT,
    .server    = &mpi_bench_rmaoverlap_put_passive_server,
    .client    = &mpi_bench_rmaoverlap_put_passive_client,
    .init      = &mpi_bench_rmaoverlap_put_passive_init,
    .finalize  = &mpi_bench_rmaoverlap_put_passive_finalize,
    .setparam  = &mpi_bench_rmaoverlap_put_passive_setparam,
    .getparams = &mpi_bench_rmaoverlap_put_passive_getparams
  };

