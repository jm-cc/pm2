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


#include <mpi.h>
#include <tbx.h>

#ifndef MPI_BENCH_GENERIC_H
#define MPI_BENCH_GENERIC_H

#define TAG 0

/* ********************************************************* */

/** parameters for a benchmark */
struct mpi_bench_param_s
{
  size_t start_len;
  size_t end_len;
  double multiplier;
  size_t increment;
  int    iterations;
};

struct mpi_bench_s
{
  const char*label;
  const char*name;
  const int rtt; /** whether we should output round-trip time or half-rtt (one way latency) */
  void (*server)(void*buf, size_t len);
  void (*client)(void*buf, size_t len);
  void (*init)(void*buf, size_t len);
  void (*setparam)(int param);
  int param_min, param_max;
  double param_mult;
};

void mpi_bench_init(int argc, char**argv);
void mpi_bench_run(const struct mpi_bench_s*mpi_bench, const struct mpi_bench_param_s*params);
void mpi_bench_finalize(void);


/* ********************************************************* */

struct mpi_bench_common_s
{
  int self, peer, size;
  int is_server;
  MPI_Comm comm;
};

extern struct mpi_bench_common_s mpi_bench_common;


/* ********************************************************* */

#define MIN_COMPUTE 0
#define MAX_COMPUTE 20000
#define MULT_COMPUTE 1.5

static volatile double r = 1.0;

static void mpi_bench_do_compute(int usec)
{
  tbx_tick_t t1, t2;
  double delay = 0.0;
  TBX_GET_TICK(t1);
  while(delay < usec)
    {
      int k;
      for(k = 0; k < 10; k++)
	{
	  r = (r * 1.1) + 2.213890 - k;
	}
      TBX_GET_TICK(t2);
      delay = TBX_TIMING_DELAY(t1, t2);
    }
}


#endif /* MPI_BENCH_GENERIC_H */

