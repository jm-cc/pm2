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


#define MIN_DEFAULT     0
#define MAX_DEFAULT     (512 * 1024 * 1024)
#define MULT_DEFAULT    2
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   100000

struct mpi_bench_common_s mpi_bench_common =
  {
    .self = -1, .peer = -1, .comm = MPI_COMM_NULL
  };

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

static void fill_buffer(char*buffer, size_t len) __attribute__((unused));
static void clear_buffer(char*buffer, size_t len) __attribute__((unused));
static void control_buffer(const char*msg, const char*buffer, size_t len) __attribute__((unused));

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

static void usage_ping(void)
{
  fprintf(stderr, "-S start_len - starting length [%d]\n", MIN_DEFAULT);
  fprintf(stderr, "-E end_len - ending length [%d]\n", MAX_DEFAULT);
  fprintf(stderr, "-I incr - length increment [%d]\n", INCR_DEFAULT);
  fprintf(stderr, "-M mult - length multiplier [%d]\n", MULT_DEFAULT);
  fprintf(stderr, "\tNext(0)      = 1+increment\n");
  fprintf(stderr, "\tNext(length) = length*multiplier+increment\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", LOOPS_DEFAULT);
}

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

extern const struct mpi_bench_s mpi_bench;

int main(int argc, char	**argv)
{
  size_t start_len      = MIN_DEFAULT;
  size_t end_len        = MAX_DEFAULT;
  double multiplier     = MULT_DEFAULT;
  size_t increment      = INCR_DEFAULT;
  int    iterations     = LOOPS_DEFAULT;
  int    i;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_bench_common.size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_bench_common.self);
  mpi_bench_common.comm = MPI_COMM_WORLD;
  const int is_server = (mpi_bench_common.self == 0);
  mpi_bench_common.peer = 1 - mpi_bench_common.self;

  if (argc > 1 && strcmp(argv[1], "--help") == 0)
    {
      usage_ping();
      MPI_Abort(mpi_bench_common.comm, -1);
    }
  
  for(i = 1; i < argc; i += 2)
    {
      if(strcmp(argv[i], "-S") == 0)
	{
	  start_len = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-E") == 0)
	{
	  end_len = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-I") == 0)
	{
	  increment = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-M") == 0)
	{
	  multiplier = atof(argv[i+1]);
	}
      else if(strcmp(argv[i], "-N") == 0)
	{
	  iterations = atoi(argv[i+1]);
	}
      else
	{
	  fprintf(stderr, "%s: illegal argument %s\n", argv[0], argv[i]);
	  usage_ping();
	  MPI_Abort(mpi_bench_common.comm, -1);
	}
    }
  int param;
  for(param = mpi_bench.param_min ;
      (param < mpi_bench.param_max) || (param == 0 && mpi_bench.setparam == NULL) ;
      param = 1 + param * mpi_bench.param_mult)
    {
      if(mpi_bench.setparam)
	{
	  if(is_server)
	    printf("# set param: %d\n", param);
	  (*mpi_bench.setparam)(param);
	}
      if(is_server)
	{
	  /* server
	   */
	  size_t len;
	  for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	    {
	      int k;
	      char* buf = malloc(len);
	      clear_buffer(buf, len);
	      iterations = _iterations(iterations, len);
	      if(mpi_bench.init != NULL)
		(*mpi_bench.init)(buf, len);
	      MPI_Barrier(mpi_bench_common.comm);
	      for(k = 0; k < iterations; k++)
		{
		  (*mpi_bench.server)(buf, len);
		  MPI_Barrier(mpi_bench_common.comm);
		}
	      free(buf);
	      MPI_Barrier(mpi_bench_common.comm);
	    }
	}
      else 
	{
	  /* client
	   */
	  tbx_tick_t t1, t2;
	  double*lats = malloc(sizeof(double) * iterations);
	  printf("# bench: %s begin\n", mpi_bench.name);
	  printf("# size  \t|  latency \t| 10^6 B/s \t| MB/s   \t| median  \t| avg    \t| max\n");
	  size_t len;
	  for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	    {
	      char* buf = malloc(len);
	      fill_buffer(buf, len);
	      iterations = _iterations(iterations, len);
	      int k;
	      if(mpi_bench.init != NULL)
		(*mpi_bench.init)(buf, len);
	      MPI_Barrier(mpi_bench_common.comm);
	      for(k = 0; k < iterations; k++)
		{
		  TBX_GET_TICK(t1);
		  (*mpi_bench.client)(buf, len);
		  TBX_GET_TICK(t2);
		  const double delay = TBX_TIMING_DELAY(t1, t2);
		  const double t = mpi_bench.rtt ? delay : (delay / 2);
		  lats[k] = t;
		  MPI_Barrier(mpi_bench_common.comm);
		}
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
	  printf("# bench: %s end\n", mpi_bench.name);
	}
    }
  MPI_Finalize();
  exit(0);
}

