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

static void mpi_bench_accumulate_active_server(void*buf, size_t len)
{
  MPI_Win_start(grp_peer, 0, win);
  MPI_Accumulate(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, MPI_SUM, win);
  MPI_Win_complete(win);
  MPI_Win_post(grp_other, 0, win);
  MPI_Win_wait(win);
}

static void mpi_bench_accumulate_active_client(void*buf, size_t len)
{
  MPI_Win_post(grp_other, 0, win);
  MPI_Win_wait(win);
  MPI_Win_start(grp_peer, 0, win);
  MPI_Accumulate(buf, len, MPI_BYTE, mpi_bench_common.peer, 0, len, MPI_BYTE, MPI_SUM, win);
  MPI_Win_complete(win);
}

static void mpi_bench_accumulate_active_init(void*buf, size_t len, int count)
{
  MPI_Comm_group(mpi_bench_common.comm, &world_group);
  MPI_Group_excl(world_group, 1, &mpi_bench_common.self, &grp_other);
  MPI_Group_incl(world_group, 1, &mpi_bench_common.peer, &grp_peer);
  MPI_Win_create(buf, len, 1, MPI_INFO_NULL, mpi_bench_common.comm, &win);
}

static void mpi_bench_accumulate_active_finalize(void)
{
  MPI_Group_free(&grp_peer);
  MPI_Group_free(&grp_other);
  MPI_Group_free(&world_group);
  MPI_Win_free(&win);
}

const struct mpi_bench_s mpi_bench_rma_accumulate_active =
  {
    .label    = "mpi_bench_rma_accumulate_active",
    .name     = "MPI RMA Active Accumulate",
    .server   = &mpi_bench_accumulate_active_server,
    .client   = &mpi_bench_accumulate_active_client,
    .init     = &mpi_bench_accumulate_active_init,
    .finalize = &mpi_bench_accumulate_active_finalize
  };

