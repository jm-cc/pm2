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

static void mpi_bench_coll_bcast_func(void*buf, size_t len)
{
  MPI_Bcast(buf, len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

const struct mpi_bench_s mpi_bench_coll_bcast =
  {
    .label   = "mpi_bench_coll_bcast",
    .name    = "MPI bcast",
    .rtt     = MPI_BENCH_RTT_FULL,
    .collective = 1,
    .server  = &mpi_bench_coll_bcast_func,
    .client  = &mpi_bench_coll_bcast_func
  };

