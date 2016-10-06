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

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = MIN_COMPUTE,
    .max  = MAX_COMPUTE,
    .mult = MULT_COMPUTE,
    .incr = 1
};

static const struct mpi_bench_param_bounds_s*mpi_bench_overlap_sender_noncontig_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_overlap_sender_noncontig_setparam(int param)
{
  compute = param;
}

static void mpi_bench_overlap_sender_noncontig_init(void*buf, size_t len)
{
  const int blocksize = BLOCKSIZE;
  sparse_buf = malloc(len * 2 + blocksize);
  MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &dtype);
  MPI_Type_commit(&dtype);
}

static void mpi_bench_overlap_sender_noncontig_finalize(void)
{
  if(dtype != MPI_DATATYPE_NULL)
    {
      MPI_Type_free(&dtype);
    }
  free(sparse_buf);
  sparse_buf = NULL;
}


static void mpi_bench_overlap_sender_noncontig_server(void*buf, size_t len)
{
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  mpi_bench_ack_send();
}

static void mpi_bench_overlap_sender_noncontig_client(void*buf, size_t len)
{
  MPI_Request sreq;
  MPI_Isend(sparse_buf, 1, dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &sreq);
  mpi_bench_do_compute(compute);
  MPI_Wait(&sreq, MPI_STATUS_IGNORE);
  mpi_bench_ack_recv();
}

const struct mpi_bench_s mpi_bench_overlap_sender_noncontig =
  {
    .label       = "mpi_bench_overlap_sender_noncontig",
    .name        = "MPI overlap, sender side, non-contig datatype",
    .rtt         = 1,
    .param_label = "computation time (usec.)",
    .init        = &mpi_bench_overlap_sender_noncontig_init,
    .finalize    = &mpi_bench_overlap_sender_noncontig_finalize,
    .server      = &mpi_bench_overlap_sender_noncontig_server,
    .client      = &mpi_bench_overlap_sender_noncontig_client,
    .setparam    = &mpi_bench_overlap_sender_noncontig_setparam,
    .getparams   = &mpi_bench_overlap_sender_noncontig_getparams
  };

