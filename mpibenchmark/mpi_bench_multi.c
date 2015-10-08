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

#define NB_PACKS 4

static int chunk = 0;

static void mpi_bench_multi_init(void*buf, size_t len)
{
  chunk = len / NB_PACKS;
}

static void mpi_bench_multi_server(void*buf, size_t len)
{
  int i;
  for(i = 0; i < NB_PACKS; i++)
    MPI_Recv(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  for(i = 0; i < NB_PACKS; i++)
    MPI_Send(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
}

static void mpi_bench_multi_client(void*buf, size_t len)
{
  int i;
  for(i = 0; i < NB_PACKS; i++)
    MPI_Send(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  for(i = 0; i < NB_PACKS; i++)
    MPI_Recv(buf + i * chunk, chunk, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_multi =
  {
    .label   = "mpi_bench_multi",
    .name    = "MPI multi ping",
    .server  = &mpi_bench_multi_server,
    .client  = &mpi_bench_multi_client
  };

