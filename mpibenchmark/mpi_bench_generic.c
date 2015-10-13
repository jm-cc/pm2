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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <values.h>

#include <mpi.h>
#include "mpi_bench_generic.h"


struct mpi_bench_common_s mpi_bench_common =
  {
    .self = -1,
    .peer = -1,
    .comm = MPI_COMM_NULL
  };

/* ** Iterations ******************************************* */

static inline size_t _iterations(int iterations, size_t len)
{
  const uint64_t max_data = 512 * 1024 * 1024;
  if(len <= 0)
    len = 1;
  uint64_t data_size = ((uint64_t)iterations * (uint64_t)len);
  if(data_size  > max_data)
    {
      iterations = (max_data / (uint64_t)len);
      if(iterations < 2)
	iterations = 2;
    }
  return iterations;
}

static inline size_t _next(size_t len, double multiplier, size_t increment)
{
  size_t next = len * multiplier + increment;
  if(next <= len)
    next++;
  return next;
}

/* ** Buffers ********************************************** */

static void fill_buffer(char*buffer, size_t len) __attribute__((unused));
static void clear_buffer(char*buffer, size_t len) __attribute__((unused));

static void fill_buffer(char*buffer, size_t len)
{
  size_t i = 0;
  for(i = 0; i < len; i++)
    {
      buffer[i] = 'a' + (i % 26);
    }
}

static void clear_buffer(char*buffer, size_t len)
{
  memset(buffer, 0, len);
}

/* ** Threads ********************************************** */

int mpi_bench_get_threads(void)
{
  int n = THREADS_DEFAULT;
#ifdef HAVE_HWLOC
  printf("# counting cores with hwloc...\n");
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
  char*level = "processing units";
  if(depth == HWLOC_TYPE_DEPTH_UNKNOWN)
    {
      depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
      level = "cores";
    }
  if(depth == HWLOC_TYPE_DEPTH_UNKNOWN)
    {
      printf("# WARNING- cannot find number of cores.\n");
    }
  else
    {
      n = hwloc_get_nbobjs_by_depth(topology, depth);
      printf("# found %d %s; using %d threads.\n", n, level, n);
    }
  hwloc_topology_destroy(topology);
#else /* HAVE_HWLOC */
  printf("# hwloc not available, using default threads count = %d\n", n);
#endif /* HAVE_HWLOC */
  if(n > THREADS_MAX)
    {
      printf(" WARNING- %d threads is more than max. Limiting to %d threads.\n", n, THREADS_MAX);
      n = THREADS_MAX;
    }
  return n;
}

/* ** Benchmark ******************************************** */

static int comp_double(const void*_a, const void*_b)
{
  const double*a = _a;
  const double*b = _b;
  if(*a < *b)
    return -1;
  else if(*a > *b)
    return 1;
  else
    return 0;
}

void mpi_bench_init(int argc, char**argv, int threads)
{
  const int requested = threads ? MPI_THREAD_MULTIPLE : MPI_THREAD_FUNNELED;
  int provided = requested;
  int rc = MPI_Init_thread(&argc, &argv, requested, &provided);
  if(rc != 0)
    {
      fprintf(stderr, "# MadMPI benchmark: MPI_Init_thread()- rc = %d\n", rc);
      abort();
    }
  if(provided < requested)
    {
      fprintf(stderr, "# MadMPI benchmark: MPI_Init_thread()- requested = %d; provided = %d\n", requested, provided);
      abort();
    }

  MPI_Comm_size(MPI_COMM_WORLD, &mpi_bench_common.size);
  if(mpi_bench_common.size != 2)
    {
      fprintf(stderr, "# MadMPI benchmark: needs 2 nodes, cannot run on %d nodes.\n", mpi_bench_common.size);
      abort();
    }
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_bench_common.self);
  mpi_bench_common.comm = MPI_COMM_WORLD;
  mpi_bench_common.is_server = (mpi_bench_common.self == 0);
  mpi_bench_common.peer = 1 - mpi_bench_common.self;
  if(!mpi_bench_common.is_server)
    {
      char hostname[256];
      gethostname(hostname, 256);
      printf("# MadMPI benchmark - copyright (C) 2015 INRIA\n");
      printf("# This program comes with ABSOLUTELY NO WARRANTY.\n"
	     "# This is free software, and you are welcome to redistribute it\n"
	     "# under certain conditions; see file 'COPYING' for details.\n#\n");
      printf("# running on host: %s; build date: %s\n", hostname, __DATE__);
      printf("# \n");
    }
}

void mpi_bench_run(const struct mpi_bench_s*mpi_bench, const struct mpi_bench_param_s*params)
{
  if(!mpi_bench_common.is_server)
    {
      printf("# bench: %s begin\n", mpi_bench->label);
      printf("# measured time is: %s\n", mpi_bench->rtt ? "round-trip" : "one-way");
    }
  const struct mpi_bench_param_bounds_s*param_bounds = NULL;
  if(mpi_bench->setparam != NULL && mpi_bench->getparams != NULL)
    {
      param_bounds = (*mpi_bench->getparams)();
    }
  int p = (param_bounds != NULL) ? param_bounds->min : 0;
  do
    {
      int iterations = params->iterations;
      if(mpi_bench->setparam)
	{
	  if(!mpi_bench_common.is_server)
	    printf("# bench: %s/%d begin\n", mpi_bench->label, p);
	  (*mpi_bench->setparam)(p);
	}
      if(mpi_bench_common.is_server)
	{
	  /* server
	   */
	  size_t len;
	  for(len = params->start_len; len <= params->end_len; len = _next(len, params->multiplier, params->increment))
	    {
	      int k;
	      char* buf = malloc(len);
	      clear_buffer(buf, len);
	      iterations = _iterations(iterations, len);
	      if(mpi_bench->init != NULL)
		(*mpi_bench->init)(buf, len, iterations);
	      MPI_Barrier(mpi_bench_common.comm);
	      for(k = 0; k < iterations; k++)
		{
		  (*mpi_bench->server)(buf, len);
		  MPI_Barrier(mpi_bench_common.comm);
		}
	      if(mpi_bench->finalize != NULL)
		(*mpi_bench->finalize)();
	      free(buf);
	      MPI_Barrier(mpi_bench_common.comm);
	    }
	}
      else 
	{
	  /* client
	   */
	  mpi_bench_tick_t t1, t2;
	  double*lats = malloc(sizeof(double) * params->iterations);
	  printf("# size  \t|  latency \t| 10^6 B/s \t| MB/s   \t| median  \t| avg    \t| max\n");
	  size_t len;
	  for(len = params->start_len; len <= params->end_len; len = _next(len, params->multiplier, params->increment))
	    {
	      char* buf = malloc(len);
	      fill_buffer(buf, len);
	      iterations = _iterations(iterations, len);
	      int k;
	      if(mpi_bench->init != NULL)
		(*mpi_bench->init)(buf, len, iterations);
	      MPI_Barrier(mpi_bench_common.comm);
	      for(k = 0; k < iterations; k++)
		{
		  mpi_bench_get_tick(&t1);
		  (*mpi_bench->client)(buf, len);
		  mpi_bench_get_tick(&t2);
		  const double delay = mpi_bench_timing_delay(&t1, &t2);
		  const double t = mpi_bench->rtt ? delay : (delay / 2);
		  lats[k] = t;
		  MPI_Barrier(mpi_bench_common.comm);
		}
	      if(mpi_bench->finalize != NULL)
		(*mpi_bench->finalize)();
	      qsort(lats, iterations, sizeof(double), &comp_double);
	      const double min_lat = lats[0];
	      const double max_lat = lats[iterations - 1];
	      const double med_lat = lats[(iterations - 1) / 2];
	      double avg_lat = 0.0;
	      for(k = 0; k < iterations; k++)
		{
		  avg_lat += lats[k];
		}
	      avg_lat /= iterations;
	      const double bw_million_byte = len / min_lat;
	      const double bw_mbyte        = bw_million_byte / 1.048576;
	      
	      printf("%9lld\t%9.3lf\t%9.3f\t%9.3f\t%9.3lf\t%9.3lf\t%9.3lf\n",
		     (long long)len, min_lat, bw_million_byte, bw_mbyte, med_lat, avg_lat, max_lat);
	      fflush(stdout);
	      free(buf);
	      MPI_Barrier(mpi_bench_common.comm);
	    }
	  if(mpi_bench->setparam)
	    printf("# bench: %s/%d end\n", mpi_bench->label, p);
	}
      if(param_bounds)
	{
	  p = param_bounds->incr + p * param_bounds->mult;
	}
      if(mpi_bench->finalizeparam != NULL)
	{
	  (*mpi_bench->finalizeparam)();
	}
    }
  while((param_bounds != NULL) && (p <= param_bounds->max));
  if(!mpi_bench_common.is_server)
      printf("# bench: %s end\n", mpi_bench->label);
}

void mpi_bench_finalize(void)
{
  MPI_Finalize();
}


