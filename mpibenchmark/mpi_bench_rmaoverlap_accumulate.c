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
static MPI_Group world_group, grp_peer, grp_other;

static int compute = 500;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = MIN_COMPUTE,
    .max  = MAX_COMPUTE,
    .mult = MULT_COMPUTE,
    .incr = 1
  };

static const struct mpi_bench_param_bounds_s*mpi_bench_rmaoverlap_accumulate_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_rmaoverlap_accumulate_setparam(int param)
{
  compute = param;
}

static void mpi_bench_rmaoverlap_accumulate_server(void*buf, size_t len)
{
  MPI_Win_post(grp_other, 0, win);
  mpi_bench_do_compute(compute);
  MPI_Win_wait(win);
  mpi_bench_ack_send();
}

static void mpi_bench_rmaoverlap_accumulate_client(void*buf, size_t len)
{
  MPI_Win_start(grp_peer, 0, win);
  MPI_Accumulate(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, MPI_SUM, win);
  mpi_bench_do_compute(compute);
  MPI_Win_complete(win);
  mpi_bench_ack_recv();
}

static void mpi_bench_rmaoverlap_accumulate_init(void*buf, size_t len)
{
  MPI_Comm_group(mpi_bench_common.comm, &world_group);
  MPI_Group_excl(world_group, 1, &mpi_bench_common.self, &grp_other);
  MPI_Group_incl(world_group, 1, &mpi_bench_common.peer, &grp_peer);
  MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
}

static void mpi_bench_rmaoverlap_accumulate_finalize(void)
{
  MPI_Group_free(&grp_peer);
  MPI_Group_free(&grp_other);
  MPI_Group_free(&world_group);
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rmaoverlap_accumulate =
  {
    .label     = "mpi_bench_rmaoverlap_accumulate",
    .name      = "MPI active accumulate, overlap both sides",
    .rtt       = MPI_BENCH_RTT_SUBLAT,
    .server    = &mpi_bench_rmaoverlap_accumulate_server,
    .client    = &mpi_bench_rmaoverlap_accumulate_client,
    .init      = &mpi_bench_rmaoverlap_accumulate_init,
    .finalize  = &mpi_bench_rmaoverlap_accumulate_finalize,
    .setparam  = &mpi_bench_rmaoverlap_accumulate_setparam,
    .getparams = &mpi_bench_rmaoverlap_accumulate_getparams
  };

