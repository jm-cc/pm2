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

static void mpi_bench_overlap_send_overhead_setparam(int param)
{
  compute = param;
}

static void mpi_bench_overlap_send_overhead_server(void*buf, size_t len)
{
  MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

static void mpi_bench_overlap_send_overhead_client(void*buf, size_t len)
{
  MPI_Request sreq;
  MPI_Isend(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &sreq);
  mpi_bench_do_compute(compute);
  MPI_Wait(&sreq, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_overlap_send_overhead =
  {
    .label      = "mpi_bench_overlap_send_overhead",
    .name       = "MPI send overhead",
    .rtt        = 1,
    .server     = &mpi_bench_overlap_send_overhead_server,
    .client     = &mpi_bench_overlap_send_overhead_client,
    .setparam   = &mpi_bench_overlap_send_overhead_setparam,
    .param_min  = MIN_COMPUTE,
    .param_max  = MAX_COMPUTE,
    .param_mult = MULT_COMPUTE
  };

