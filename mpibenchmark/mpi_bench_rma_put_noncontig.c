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

static int blocksize = 0;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 4,
    .max  = 1024,
    .mult = 2,
    .incr = 0
  };

static MPI_Win win;
static MPI_Group world_group, grp_peer, grp_other;

static const struct mpi_bench_param_bounds_s*mpi_bench_put_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_put_noncontig_setparam(int param)
{
  blocksize = param;
}

static void mpi_bench_put_noncontig_server(void*buf, size_t len)
{
  MPI_Win_post(grp_other, 0, win);
  MPI_Win_wait(win);
}

static void mpi_bench_put_noncontig_client(void*buf, size_t len)
{
  MPI_Win_start(grp_peer, 0, win);
  MPI_Put(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, 0, 1, noncontig_dtype, win);
  MPI_Win_complete(win);
}

static void mpi_bench_put_noncontig_init(void*buf, size_t len)
{
  MPI_Comm_group(mpi_bench_common.comm, &world_group);
  MPI_Group_excl(world_group, 1, &mpi_bench_common.self, &grp_other);
  MPI_Group_incl(world_group, 1, &mpi_bench_common.peer, &grp_peer);
  mpi_bench_noncontig_type_init(MPI_BENCH_NONCONTIG_BLOCKSIZE, len);
  MPI_Win_create(noncontig_buf, noncontig_bufsize, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
}

static void mpi_bench_put_noncontig_finalize(void)
{
  MPI_Group_free(&grp_peer);
  MPI_Group_free(&grp_other);
  MPI_Group_free(&world_group);
  MPI_Win_free(&win);
  mpi_bench_noncontig_type_destroy();
}

const struct mpi_bench_s mpi_bench_rma_put_noncontig =
  {
    .label     = "mpi_bench_rma_put_noncontig",
    .name      = "MPI RMA Non-Contig Put",
    .rtt       = 1,
    .server    = &mpi_bench_put_noncontig_server,
    .client    = &mpi_bench_put_noncontig_client,
    .init      = &mpi_bench_put_noncontig_init,
    .finalize  = &mpi_bench_put_noncontig_finalize,
    .setparam  = &mpi_bench_put_noncontig_setparam,
    .getparams = &mpi_bench_put_noncontig_getparams
  };

