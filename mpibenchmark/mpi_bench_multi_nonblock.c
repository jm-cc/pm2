/*
 * NewMadeleine
 * Copyright (C) 2015-2016 (see AUTHORS file)
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

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 2,
    .max  = 1024,
    .mult = 2,
    .incr = 0
  };

static int nb_packs = 2;
static int chunk = 0;

static MPI_Request*reqs = NULL;

static const struct mpi_bench_param_bounds_s*mpi_bench_multi_nonblock_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_multi_nonblock_setparam(int param)
{
  nb_packs = param;
  reqs = realloc(reqs, nb_packs * sizeof(MPI_Request));
}

static void mpi_bench_multi_nonblock_init(void*buf, size_t len)
{
  chunk = len / nb_packs;
}

static void mpi_bench_multi_nonblock_server(void*buf, size_t len)
{
  int i;
  for(i = 0; i < nb_packs; i++)
    {
      assert((i+1) * chunk <= len);
      MPI_Irecv(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, &reqs[i]);
    }
  int rc = MPI_Waitall(nb_packs, reqs, MPI_STATUS_IGNORE);
  assert(rc == MPI_SUCCESS);
  for(i = 0; i < nb_packs; i++)
    MPI_Isend(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, &reqs[i]);
  rc = MPI_Waitall(nb_packs, reqs, MPI_STATUS_IGNORE);
  assert(rc == MPI_SUCCESS);
}

static void mpi_bench_multi_nonblock_client(void*buf, size_t len)
{
  int i;
  for(i = 0; i < nb_packs; i++)
    MPI_Isend(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, &reqs[i]);
  int rc = MPI_Waitall(nb_packs, reqs, MPI_STATUS_IGNORE);
  assert(rc == MPI_SUCCESS);
  for(i = 0; i < nb_packs; i++)
    {
      assert((i+1) * chunk <= len);
      MPI_Irecv(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, &reqs[i]);
    }
  rc = MPI_Waitall(nb_packs, reqs, MPI_STATUS_IGNORE);
  assert(rc == MPI_SUCCESS);
}

const struct mpi_bench_s mpi_bench_multi_nonblock =
  {
    .label     = "mpi_bench_multi_nonblock",
    .name      = "MPI multi ping, non-blocking",
    .init      = &mpi_bench_multi_nonblock_init,
    .server    = &mpi_bench_multi_nonblock_server,
    .client    = &mpi_bench_multi_nonblock_client,
    .setparam  = &mpi_bench_multi_nonblock_setparam,
    .getparams = &mpi_bench_multi_nonblock_getparams
  };

