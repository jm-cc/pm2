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

#define BLOCKSIZE 32

static int compute = 500;
static void*sparse_buf = NULL;
static MPI_Datatype dtype = MPI_DATATYPE_NULL;

static void mpi_bench_overlap_sender_noncontig_setparam(int param)
{
  compute = param;
}

static void mpi_bench_overlap_sender_noncontig_init(void*buf, size_t len)
{
  const int blocksize = BLOCKSIZE;
  sparse_buf = realloc(sparse_buf, len * 2 + blocksize);
  MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &dtype);
  MPI_Type_commit(&dtype);
  /* TODO- we never free the dtype from previous iteration */
}

static void mpi_bench_overlap_sender_noncontig_server(void*buf, size_t len)
{
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
}

static void mpi_bench_overlap_sender_noncontig_client(void*buf, size_t len)
{
  MPI_Request sreq;
  MPI_Isend(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &sreq);
  mpi_bench_do_compute(compute);
  MPI_Wait(&sreq, MPI_STATUS_IGNORE);
  MPI_Recv(buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_overlap_sender_noncontig =
  {
    .label      = "mpi_bench_overlap_sender_noncontig",
    .name       = "MPI overlap sender non-contig",
    .rtt        = 1,
    .init       = &mpi_bench_overlap_sender_noncontig_init,
    .server     = &mpi_bench_overlap_sender_noncontig_server,
    .client     = &mpi_bench_overlap_sender_noncontig_client,
    .setparam   = &mpi_bench_overlap_sender_noncontig_setparam,
    .param_min  = MIN_COMPUTE,
    .param_max  = MAX_COMPUTE,
    .param_mult = MULT_COMPUTE
  };

