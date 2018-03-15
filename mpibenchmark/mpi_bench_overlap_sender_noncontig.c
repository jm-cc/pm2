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

static int compute = 500;

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
  mpi_bench_noncontig_type_init(MPI_BENCH_NONCONTIG_BLOCKSIZE, len);
}

static void mpi_bench_overlap_sender_noncontig_finalize(void)
{
  mpi_bench_noncontig_type_destroy();
}


static void mpi_bench_overlap_sender_noncontig_server(void*buf, size_t len)
{
  MPI_Recv(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  mpi_bench_ack_send();
}

static void mpi_bench_overlap_sender_noncontig_client(void*buf, size_t len)
{
  MPI_Request sreq;
  MPI_Isend(noncontig_buf, 1, noncontig_dtype, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, &sreq);
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

