/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>
#include <pthread.h>
#include "mpi_bench_generic.h"

static int threads = 0;

static struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 1,
    .max  = THREADS_DEFAULT,
    .mult = 1,
    .incr = 1
  };

static const struct mpi_bench_param_bounds_s*mpi_bench_thread_NtoN_getparams(void)
{
  param_bounds.max = mpi_bench_get_threads();
  return &param_bounds;
}

static void mpi_bench_thread_NtoN_setparam(int param)
{
  threads = param;
}

static void mpi_bench_thread_NtoN_endparam(void)
{
  threads = 0;
}


static void mpi_bench_thread_NtoN_server(void*buf, size_t len)
{
  int i;
#pragma omp parallel for schedule(static, 1)
  for(i = 0; i < threads; i++)
    {
      MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD);
    }
}

static void mpi_bench_thread_NtoN_client(void*buf, size_t len)
{
  int i;
#pragma omp parallel for schedule(static, 1)
  for(i = 0; i < threads; i++)
    {
      MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD);
      MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}


const struct mpi_bench_s mpi_bench_thread_NtoN =
  {
    .label      = "mpi_bench_thread_NtoN",
    .name       = "MPI threaded N to N threads",
    .rtt        = 0,
    .threads    = 1,
    .server     = &mpi_bench_thread_NtoN_server,
    .client     = &mpi_bench_thread_NtoN_client,
    .getparams  = &mpi_bench_thread_NtoN_getparams,
    .setparam   = &mpi_bench_thread_NtoN_setparam,
    .endparam   = &mpi_bench_thread_NtoN_endparam,
    .init       = NULL,
    .finalize   = NULL
  };

