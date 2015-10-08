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
#include <pthread.h>

#define MAX_THREADS 16

static int threads = 0;

static const struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 0,
    .max  = MAX_THREADS,
    .mult = 1,
    .incr = 1
  };

static volatile int compute_stop = 0;
static pthread_t tids[MAX_THREADS];

static void*do_compute(void*_dummy)
{
  while(!compute_stop)
    {
      int k;
      for(k = 0; k < 10; k++)
        {
          r *= 2.213890;
        }
    }
  return NULL;
}

static const struct mpi_bench_param_bounds_s*mpi_bench_overlap_Nload_getparams(void)
{
  return &param_bounds;
}

static void mpi_bench_overlap_Nload_setparam(int param)
{
  /* start this round threads */
  int i;
  compute_stop = 0;
  threads = param;
  for(i = 0; i < threads; i++)
    {
      pthread_create(&tids[i], NULL, &do_compute, NULL);
    }
}

static void mpi_bench_overlap_Nload_finalize(void)
{
  /* stop previous threads */
  int i;
  compute_stop = 1;
  for(i = 0; i < threads; i++)
    {
      pthread_join(tids[i], NULL);
    }
  threads = 0;
}

static void mpi_bench_overlap_Nload_server(void*buf, size_t len)
{
  MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
}

static void mpi_bench_overlap_Nload_client(void*buf, size_t len)
{
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_overlap_Nload =
  {
    .label      = "mpi_bench_overlap_Nload",
    .name       = "MPI overlap with N threads",
    .rtt        = 0,
    .server     = &mpi_bench_overlap_Nload_server,
    .client     = &mpi_bench_overlap_Nload_client,
    .setparam   = &mpi_bench_overlap_Nload_setparam,
    .getparams  = &mpi_bench_overlap_Nload_getparams,
    .finalize   = &mpi_bench_overlap_Nload_finalize
  };

