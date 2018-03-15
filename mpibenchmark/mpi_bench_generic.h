/*
 * NewMadeleine
 * Copyright (C) 2015-2017 (see AUTHORS file)
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
#include <math.h>
#include <assert.h>

#ifndef MPI_BENCH_GENERIC_H
#define MPI_BENCH_GENERIC_H

#ifdef HAVE_HWLOC
#include <hwloc.h>
#endif /* HAVE_HWLOC */

#define TAG 0

#define MIN_DEFAULT         0
#define MAX_DEFAULT        (128 * 1024 * 1024)
#define MULT_DEFAULT        1.4
#define INCR_DEFAULT        0
#define LOOPS_DEFAULT_PARAM 50
#define LOOPS_DEFAULT       1000
#define PARAM_DEFAULT       -1

#define LOOPS_CALIBRATE     10000
#define USEC_CALIBRATE_BARRIER (1000 * 1000 * 5)

#define LOOPS_TIMEOUT_SECONDS 3
#define LOOPS_MAX_DATA        ((uint64_t)(512 * 1024 * 1024))

/* ********************************************************* */

/** parameters for a benchmark */
struct mpi_bench_param_s
{
  size_t start_len;
  size_t end_len;
  double multiplier;
  size_t increment;
  long long iterations;
  int    param;        /**< fixed parameter; -1 to use bounds */
};

/** bounds for parameterized benchmarks */
struct mpi_bench_param_bounds_s
{
  int min, max;
  double mult;
  int incr;
};

enum mpi_bench_rtt_e
  {
    MPI_BENCH_RTT_HALF   = 0, /**< display half-roundtrip (supposed to be one-way latency) */
    MPI_BENCH_RTT_FULL   = 1, /**< display full roundtrip (either it is directly meaningfull, or will be post-processed) */
    MPI_BENCH_RTT_SUBLAT = 2, /**< display roundtrip minus ack latency */
    MPI_BENCH_RTT_SUBBARRIER = 3, /**< display roundtrip time minus barrier latency */
    _MPI_BENCH_RTT_LAST
  };

struct mpi_bench_s
{
  const char*label;
  const char*name;
  const enum mpi_bench_rtt_e rtt; /**< whether we should output round-trip time or half-rtt (one way latency) */
  const int threads; /**< whether we need MPI_THREAD_MULTIPLE */
  const int collective; /**< whether the operaiton is collective (display results only on node 0) */
  void (*server)(void*buf, size_t len);
  void (*client)(void*buf, size_t len);
  void (*init)(void*buf, size_t len); /**< called before a round with a given set of param+size */
  void (*finalize)(void);      /**< called at the end of a round for a given param+size */
  void (*setparam)(int param); /**< set a new param */
  void (*endparam)(void);      /**< called at the end of a round for a given param */
  const char*param_label;      /**< label of parameter */
  const struct mpi_bench_param_bounds_s*(*getparams)(void);
};

void mpi_bench_init(int*argc, char***argv, int threads);
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
 * we cannot rely on it. Use RDTSC on mckernel (and
 * assume constant_tsc).
 */

#ifdef MCKERNEL
#define USE_CLOCK_RDTSC
#else
#define USE_CLOCK_GETTIME
#if defined(__bg__)
#  define CLOCK_TYPE CLOCK_REALTIME
#elif defined(CLOCK_MONOTONIC_RAW)
#  define CLOCK_TYPE CLOCK_MONOTONIC_RAW
#else
#  define CLOCK_TYPE CLOCK_MONOTONIC
#endif
#endif

#if defined(USE_CLOCK_GETTIME)
typedef struct timespec mpi_bench_tick_t;
#elif defined(USE_CLOCK_RDTSC)
typedef uint64_t mpi_bench_tick_t;
#else
#error no clock
#endif

static inline void mpi_bench_get_tick(mpi_bench_tick_t*t)
{
#if defined(USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_TYPE, t);
#elif defined(USE_CLOCK_RDTSC)
#ifdef __i386
  uint64_t r;
  __asm__ volatile ("rdtsc" : "=A" (r));
  *t = r;
#elif defined __amd64
  uint64_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  *t = (d<<32) | a;
#else
#error
#endif
#else
#error
#endif
}
static inline double mpi_bench_timing_delay(const mpi_bench_tick_t* t1, const mpi_bench_tick_t* t2)
{
#if defined(USE_CLOCK_GETTIME)
  const double delay = 1000000.0 * (t2->tv_sec - t1->tv_sec) + (t2->tv_nsec - t1->tv_nsec) / 1000.0;
#elif defined(USE_CLOCK_RDTSC)
  static double scale = -1;
  if(scale < 0)
    {  
      uint64_t s1, s2;
      struct timespec ts = { 0, 10000000 /* 10 ms */ };
      mpi_bench_get_tick(&s1);
      nanosleep(&ts, NULL);
      mpi_bench_get_tick(&s2);
      scale = (ts.tv_sec * 1e6 + ts.tv_nsec / 1e3) / (double)(s2 - s1);
    }
  const uint64_t tick_diff = *t2 - *t1;
  const double delay = (tick_diff * scale);
#else
#error
#endif
  return delay;
}

/* ** Compute ********************************************** */

#define MIN_COMPUTE 0
#define MAX_COMPUTE 20000
#define MULT_COMPUTE 1.4

static volatile double r = 1.0;

/** dummy computation of a given time */
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

/** computation on variable-size vector */
static void mpi_bench_compute_vector(void*buf, size_t len) __attribute__((unused));
static void mpi_bench_compute_vector(void*buf, size_t len)
{
  unsigned char*m = buf;
  size_t i;
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for(i = 0; i < len; i++)
    {
      double v = (double)m[i];
      v = sqrt(v * v + 1.0);
      m[i] = (unsigned char)v;
    }
}

/* ** non-contiguous datatype ****************************** */

/** default blocksize for non-contiguous datatype */
#define MPI_BENCH_NONCONTIG_BLOCKSIZE 32

static void*noncontig_buf = NULL;
static MPI_Datatype noncontig_dtype = MPI_DATATYPE_NULL;

static inline void mpi_bench_noncontig_type_init(int blocksize, size_t len)
{
  const int sparse_size = len * 2 + blocksize;
  noncontig_buf = malloc(sparse_size);
  memset(noncontig_buf, 0, sparse_size);
  MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &noncontig_dtype);
  MPI_Type_commit(&noncontig_dtype);
}

static inline void mpi_bench_noncontig_type_destroy(void)
{
  if(noncontig_dtype != MPI_DATATYPE_NULL)
    {
      MPI_Type_free(&noncontig_dtype);
    }
  free(noncontig_buf);
  noncontig_buf = NULL;
}

/* ** Threads ********************************************** */

#define THREADS_MAX 512
#define THREADS_DEFAULT 16

/** Get the max number of threads to use */
int mpi_bench_get_threads(void);

/* ** ACKs ************************************************* */

void mpi_bench_ack_send(void);
void mpi_bench_ack_recv(void);

#endif /* MPI_BENCH_GENERIC_H */

