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

static int compute = 500;

static void mpi_bench_overlap_recv_setparam(int param)
{
  compute = param;
}

static void mpi_bench_overlap_recv_server(void*buf, size_t len)
{
  MPI_Recv(buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
}

static void mpi_bench_overlap_recv_client(void*buf, size_t len)
{
  MPI_Send(buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
  MPI_Request rreq;
  MPI_Irecv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &rreq);
  mpi_bench_do_compute(compute);
  MPI_Wait(&rreq, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench =
  {
    .name       = "MPI overlap recv",
    .rtt        = 1,
    .server     = &mpi_bench_overlap_recv_server,
    .client     = &mpi_bench_overlap_recv_client,
    .setparam   = &mpi_bench_overlap_recv_setparam,
    .param_min  = MIN_COMPUTE,
    .param_max  = MAX_COMPUTE,
    .param_mult = MULT_COMPUTE
  };

