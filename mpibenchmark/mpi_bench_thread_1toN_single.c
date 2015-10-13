/*
 * NewMadeleine
 * Copyright (C) 2014-2015 (see AUTHORS file)
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

#define COUNT 100

static int threads = 0;

static struct mpi_bench_param_bounds_s param_bounds =
  {
    .min  = 1,
    .max  = THREADS_DEFAULT,
    .mult = 1,
    .incr = 1
  };

static pthread_t tids[THREADS_MAX];


static void*ping_thread(void*_i)
{
  const int i = (uintptr_t)_i; /* thread id */
  char dummy;
  MPI_Recv(&dummy, 0, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  return NULL;
}

static const struct mpi_bench_param_bounds_s*mpi_bench_thread_1toN_single_getparams(void)
{
  param_bounds.max = mpi_bench_get_threads();
  return &param_bounds;
}

static void mpi_bench_thread_1toN_single_setparam(int param)
{
  threads = param;
}

static void mpi_bench_thread_1toN_single_endparam(void)
{
  threads = 0;
}

static void mpi_bench_thread_1toN_single_init(void*buf, size_t len, int count)
{
  if(mpi_bench_common.is_server)
    {
      /* start this round threads */
      int i;
      for(i = 1; i < threads; i++)
	{
	  pthread_create(&tids[i], NULL, &ping_thread, (void*)(uintptr_t)i);
	}
    }
}

static void mpi_bench_thread_1toN_single_finalize(void)
{
  if(mpi_bench_common.is_server)
    {
      /* stop previous threads */
      int i;
      for(i = 1; i < threads; i++)
	{
	  pthread_join(tids[i], NULL);
	}
    }
  else
    {
      int i;
      for(i = 1; i < threads; i++)
	{
	  char dummy;
	  MPI_Send(&dummy, 0, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD);
	}
    }
}

static void mpi_bench_thread_1toN_single_server(void*buf, size_t len)
{
  MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
}

static void mpi_bench_thread_1toN_single_client(void*buf, size_t len)
{
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD);
  MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}


const struct mpi_bench_s mpi_bench_thread_1toN_single =
  {
    .label      = "mpi_bench_thread_1toN_single",
    .name       = "MPI threaded 1 to N threads, N recv posted, a single thread actually receives",
    .rtt        = 0,
    .threads    = 1,
    .server     = &mpi_bench_thread_1toN_single_server,
    .client     = &mpi_bench_thread_1toN_single_client,
    .getparams  = &mpi_bench_thread_1toN_single_getparams,
    .setparam   = &mpi_bench_thread_1toN_single_setparam,
    .endparam   = &mpi_bench_thread_1toN_single_endparam,
    .init       = &mpi_bench_thread_1toN_single_init,
    .finalize   = &mpi_bench_thread_1toN_single_finalize
  };

