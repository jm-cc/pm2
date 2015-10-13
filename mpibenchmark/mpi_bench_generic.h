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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef MPI_BENCH_GENERIC_H
#define MPI_BENCH_GENERIC_H

#ifdef HAVE_HWLOC
#include <hwloc.h>
#endif /* HAVE_HWLOC */

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

/** bounds for parameterized benchmarks */
struct mpi_bench_param_bounds_s
{
  int min, max;
  double mult;
  int incr;
};

struct mpi_bench_s
{
  const char*label;
  const char*name;
  const int rtt; /**< whether we should output round-trip time or half-rtt (one way latency) */
  const int threads; /**< whether we need MPI_THREAD_MULTIPLE */
  void (*server)(void*buf, size_t len);
  void (*client)(void*buf, size_t len);
  void (*init)(void*buf, size_t len, int count); /**< called before a round with a given set of param+size */
  void (*finalize)(void); /**< called at the end of a round for a given param+size */
  void (*setparam)(int param); /**< set a new param */
  void (*finalizeparam)(void); /**< called at the end of a round for a given param */
  const struct mpi_bench_param_bounds_s*(*getparams)(void);
};

void mpi_bench_init(int argc, char**argv, int threads);
void mpi_bench_run(const struct mpi_bench_s*mpi_bench, const struct mpi_bench_param_s*params);
void mpi_bench_finalize(void);


/* ********************************************************* */

/** common variables shared between init, main, and individual benchmarks */
struct mpi_bench_common_s
{
  int self, peer, size;
  int is_server;
  MPI_Comm comm;
};

extern struct mpi_bench_common_s mpi_bench_common;


/* ** Timing *********************************************** */

/* Use POSIX clock_gettime to measure time. Some MPI 
 * implementations still use gettimeofday for MPI_Wtime(), 
 * we cannot rely on it.
 */

typedef struct timespec mpi_bench_tick_t;

static inline void mpi_bench_get_tick(mpi_bench_tick_t*t)
{
#ifdef CLOCK_MONOTONIC_RAW
#  define CLOCK_TYPE CLOCK_MONOTONIC_RAW
#else
#  define CLOCK_TYPE CLOCK_MONOTONIC
#endif
  clock_gettime(CLOCK_TYPE, t);
}
static inline double mpi_bench_timing_delay(const mpi_bench_tick_t*const t1, const mpi_bench_tick_t*const t2)
{
  const double delay = 1000000.0 * (t2->tv_sec - t1->tv_sec) + (t2->tv_nsec - t1->tv_nsec) / 1000.0;
  return delay;
}

/* ** Compute ********************************************** */

#define MIN_COMPUTE 0
#define MAX_COMPUTE 20000
#define MULT_COMPUTE 1.5

static volatile double r = 1.0;

static void mpi_bench_do_compute(int usec) __attribute__((unused));
static void mpi_bench_do_compute(int usec)
{
  mpi_bench_tick_t t1, t2;
  double delay = 0.0;
  mpi_bench_get_tick(&t1);
  while(delay < usec)
    {
      int k;
      for(k = 0; k < 10; k++)
	{
	  r = (r * 1.1) + 2.213890 - k;
	}
      mpi_bench_get_tick(&t2);
      delay = mpi_bench_timing_delay(&t1, &t2);
    }
}

/* ** Threads ********************************************** */

#define THREADS_MAX 512
#define THREADS_DEFAULT 16

/** Get the max number of threads to use */
int mpi_bench_get_threads(void);


#endif /* MPI_BENCH_GENERIC_H */

