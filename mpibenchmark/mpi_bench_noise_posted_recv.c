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

static int value = -1;
static MPI_Request req;

static void mpi_bench_noise_posted_recv_init(void*buf, size_t len)
{
  MPI_Irecv(&value, 1, MPI_INT, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &req);
}
static void mpi_bench_noise_posted_recv_finalize(void)
{
  MPI_Send(&value, 1, MPI_INT, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
  MPI_Wait(&req, MPI_STATUS_IGNORE);
}

static void mpi_bench_noise_posted_recv_server(void*buf, size_t len)
{
  mpi_bench_compute_vector(buf, len);
}

static void mpi_bench_noise_posted_recv_client(void*buf, size_t len)
{
  mpi_bench_compute_vector(buf, len);
}

const struct mpi_bench_s mpi_bench_noise_posted_recv =
  {
    .label      = "mpi_bench_noise_posted_recv",
    .name       = "MPI system noise with a posted MPI_Irecv",
    .rtt        = 0,
    .server     = &mpi_bench_noise_posted_recv_server,
    .client     = &mpi_bench_noise_posted_recv_client,
    .setparam   = NULL,
    .getparams  = NULL,
    .init       = &mpi_bench_noise_posted_recv_init,
    .finalize   = &mpi_bench_noise_posted_recv_finalize
  };
