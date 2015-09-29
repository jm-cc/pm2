/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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

#define SMALL 16

static int small = 0, large = 0;

static void mpi_bench_multi_hetero_init(void*buf, size_t len)
{
  small = (len > SMALL) ? SMALL : len;
  large = len - small;
}

static void mpi_bench_multi_hetero_server(void*buf, size_t len)
{
  MPI_Recv(buf, small, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(buf + small, large, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(buf, small, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  MPI_Send(buf + small, large, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
}

static void mpi_bench_multi_hetero_client(void*buf, size_t len)
{
  MPI_Send(buf, small, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  MPI_Send(buf + small, large, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  MPI_Recv(buf, small, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(buf + small, large, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_multi_hetero =
  {
    .name = "MPI heterogeneous multi ping",
    .server = &mpi_bench_multi_hetero_server,
    .client = &mpi_bench_multi_hetero_client
  };

