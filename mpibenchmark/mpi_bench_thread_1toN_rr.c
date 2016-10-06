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
static struct
{
  void*buf;
  size_t len;
  int dest;
} global = { .buf = NULL, .len = 0, .dest = 0 };

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
  for(;;)
    {
      int dummy;
      void*buf = (global.len > 0) ? global.buf : &dummy;
      int len = (global.len > 0) ? global.len : 1;
      MPI_Status status;
      MPI_Recv(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD, &status);
      int count;
      MPI_Get_count(&status, MPI_CHAR, &count);
      if(count != global.len)
	{
	  return NULL;
	}
      MPI_Send(global.buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD);
    }
  return NULL;
}

static const struct mpi_bench_param_bounds_s*mpi_bench_thread_1toN_rr_getparams(void)
{
  param_bounds.max = mpi_bench_get_threads();
  return &param_bounds;
}

static void mpi_bench_thread_1toN_rr_setparam(int param)
{
  threads = param;
}

static void mpi_bench_thread_1toN_rr_endparam(void)
{
  threads = 0;
}

static void mpi_bench_thread_1toN_rr_init(void*buf, size_t len, int count)
{
  global.buf = buf;
  global.len = len;
  global.dest = 0;
  if(mpi_bench_common.is_server)
    {
      /* start this round threads */
      int i;
      for(i = 0; i < threads; i++)
	{
	  pthread_create(&tids[i], NULL, &ping_thread, (void*)(uintptr_t)i);
	}
    }
}

static void mpi_bench_thread_1toN_rr_finalize(void)
{
  if(mpi_bench_common.is_server)
    {
      /* stop all threads */
      int i;
      for(i = 0; i < threads; i++)
	{
	  pthread_join(tids[i], NULL);
	}
    }
  else
    {
      /* stop remote threads- convention used: 
       * send 0 bytes if bytes are expected, send 1 byte if 0 expected
       * note: receiver shoud post recv > 0 even if 0 byte expected */
      int i;
      for(i = 0; i < threads; i++)
	{
	  int dummy = 0;
	  int count = (global.len > 0) ? 0 : 1;
	  MPI_Send(&dummy, count, MPI_CHAR, mpi_bench_common.peer, TAG + i, MPI_COMM_WORLD);
	}
    }
}

static void mpi_bench_thread_1toN_rr_server(void*buf, size_t len)
{
  /* do nothing- work is done in thread */
}

static void mpi_bench_thread_1toN_rr_client(void*buf, size_t len)
{
  MPI_Send(buf, len, MPI_CHAR, mpi_bench_common.peer, TAG + global.dest, MPI_COMM_WORLD);
  MPI_Recv(buf, 0, MPI_CHAR, mpi_bench_common.peer, TAG + global.dest, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  global.dest = (global.dest + 1) % threads;
}


const struct mpi_bench_s mpi_bench_thread_1toN_rr =
  {
    .label      = "mpi_bench_thread_1toN_rr",
    .name       = "MPI threaded 1 to N threads, round-robin",
    .rtt        = 1,
    .threads    = 1,
    .server     = &mpi_bench_thread_1toN_rr_server,
    .client     = &mpi_bench_thread_1toN_rr_client,
    .getparams  = &mpi_bench_thread_1toN_rr_getparams,
    .setparam   = &mpi_bench_thread_1toN_rr_setparam,
    .endparam   = &mpi_bench_thread_1toN_rr_endparam,
    .init       = &mpi_bench_thread_1toN_rr_init,
    .finalize   = &mpi_bench_thread_1toN_rr_finalize
  };

