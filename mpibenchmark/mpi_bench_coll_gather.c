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

static int rank = -1;
static int size = -1;
static void*root_buf = NULL;

static void mpi_bench_coll_gather_init(void*buf, size_t len)
{
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if(rank == 0)
    {
      root_buf = malloc(size * len);
    }
}

static void mpi_bench_coll_gather_finalize(void)
{
  if(root_buf != NULL)
    {
      free(root_buf);
      root_buf = NULL;
    }
}

static void mpi_bench_coll_gather_func(void*buf, size_t len)
{
  MPI_Gather(buf, len, MPI_BYTE, root_buf, len, MPI_BYTE, 0, MPI_COMM_WORLD);
  /* no barrier needed */
}

const struct mpi_bench_s mpi_bench_coll_gather =
  {
    .label      = "mpi_bench_coll_gather",
    .name       = "MPI gather",
    .rtt        = MPI_BENCH_RTT_FULL,
    .collective = 1,
    .init       = &mpi_bench_coll_gather_init,
    .finalize   = &mpi_bench_coll_gather_finalize,
    .server     = &mpi_bench_coll_gather_func,
    .client     = &mpi_bench_coll_gather_func
  };

