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

#define BLOCKSIZE 32

static MPI_Win win;
static MPI_Group world_group, grp_peer, grp_other;
static void*sparse_buf = NULL;
static MPI_Datatype dtype = MPI_DATATYPE_NULL;

static int compute = 500;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = MIN_COMPUTE,
    .max  = MAX_COMPUTE,
    .mult = MULT_COMPUTE,
    .incr = 1
  };

static const struct mpi_bench_param_bounds_s*mpi_bench_rmaoverlap_put_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_rmaoverlap_put_noncontig_setparam(int param)
{
  compute = param;
}

static void mpi_bench_rmaoverlap_put_noncontig_server(void*buf, size_t len)
{
  MPI_Win_post(grp_other, 0, win);
  mpi_bench_do_compute(compute);
  MPI_Win_wait(win);
  mpi_bench_ack_send();
}

static void mpi_bench_rmaoverlap_put_noncontig_client(void*buf, size_t len)
{
  MPI_Win_start(grp_peer, 0, win);
  MPI_Put(sparse_buf, 1, dtype, mpi_bench_common.peer, 0, 1, dtype, win);
  mpi_bench_do_compute(compute);
  MPI_Win_complete(win);
  mpi_bench_ack_recv();
}

static void mpi_bench_rmaoverlap_put_noncontig_init(void*buf, size_t len)
{
  MPI_Comm_group(mpi_bench_common.comm, &world_group);
  MPI_Group_excl(world_group, 1, &mpi_bench_common.self, &grp_other);
  MPI_Group_incl(world_group, 1, &mpi_bench_common.peer, &grp_peer);
  const size_t bufsize = 2 * len + BLOCKSIZE;
  sparse_buf = malloc(bufsize);
  MPI_Type_vector(len / BLOCKSIZE, BLOCKSIZE, 2 * BLOCKSIZE, MPI_CHAR, &dtype);
  MPI_Type_commit(&dtype);
  MPI_Win_create(sparse_buf, bufsize, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
}

static void mpi_bench_rmaoverlap_put_noncontig_finalize(void)
{
  MPI_Group_free(&grp_peer);
  MPI_Group_free(&grp_other);
  MPI_Group_free(&world_group);
  MPI_Win_free(&win);
  if(dtype != MPI_DATATYPE_NULL)
    {
      MPI_Type_free(&dtype);
    }
  free(sparse_buf);
  sparse_buf = NULL;
}

const struct mpi_bench_s mpi_bench_rmaoverlap_put_noncontig =
  {
    .label     = "mpi_bench_rmaoverlap_put_noncontig",
    .name      = "MPI RMA Put Active Overlap Origin",
    .rtt       = MPI_BENCH_RTT_SUBLAT,
    .server    = &mpi_bench_rmaoverlap_put_noncontig_server,
    .client    = &mpi_bench_rmaoverlap_put_noncontig_client,
    .init      = &mpi_bench_rmaoverlap_put_noncontig_init,
    .finalize  = &mpi_bench_rmaoverlap_put_noncontig_finalize,
    .setparam  = &mpi_bench_rmaoverlap_put_noncontig_setparam,
    .getparams = &mpi_bench_rmaoverlap_put_noncontig_getparams
  };

