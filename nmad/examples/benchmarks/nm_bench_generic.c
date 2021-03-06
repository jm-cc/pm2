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

#include "nm_bench_generic.h"
#include "../common/nm_examples_helper.h"

#define MIN_DEFAULT     0
#define MAX_DEFAULT     (512 * 1024 * 1024)
#define MULT_DEFAULT    2
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   100000

struct nm_bench_common_s nm_bench_common =
  {
    .p_session = NULL,
    .p_gate = NULL,
    .p_comm = NULL
  };

static inline nm_len_t _iterations(int iterations, nm_len_t len)
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

extern const struct nm_bench_s nm_bench;

int main(int argc, char	**argv)
{
  nm_len_t start_len      = MIN_DEFAULT;
  nm_len_t end_len        = MAX_DEFAULT;
  double   multiplier     = MULT_DEFAULT;
  nm_len_t increment      = INCR_DEFAULT;
  int      iterations     = LOOPS_DEFAULT;
  int      i;
  
  nm_examples_init_topo(&argc, argv, NM_EXAMPLES_TOPO_PAIRS);
  nm_bench_common.p_session = p_session;
  nm_bench_common.p_gate = p_gate;
  nm_bench_common.p_comm = p_comm;

  if (argc > 1 && strcmp(argv[1], "--help") == 0)
    {
      usage_ping();
      nm_examples_exit();
      exit(0);
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
	  nm_examples_exit();
	  exit(0);
	}
    }
  
  if(is_server)
    {
      /* server
       */
      nm_len_t len;
      for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	{
	  int k;
	  char* buf = malloc(len);
	  clear_buffer(buf, len);
	  iterations = _iterations(iterations, len);
	  if(nm_bench.init != NULL)
	    (*nm_bench.init)(buf, len);
	  nm_examples_barrier(sync_tag);
	  for(k = 0; k < iterations; k++)
	    {
	      (*nm_bench.server)(buf, len);
	      nm_examples_barrier(sync_tag);
	    }
	  free(buf);
	  nm_examples_barrier(sync_tag);
	}
    }
  else 
    {
      /* client
       */
      tbx_tick_t t1, t2;
      double*lats = malloc(sizeof(double) * iterations);
      printf("# bench: %s begin\n", nm_bench.name);
      printf("# size  \t|  latency \t| 10^6 B/s \t| MB/s   \t| median  \t| avg    \t| max\n");
      nm_len_t	 len;
      for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	{
	  char* buf = malloc(len);
	  fill_buffer(buf, len);
	  iterations = _iterations(iterations, len);
	  int k;
	  if(nm_bench.init != NULL)
	    (*nm_bench.init)(buf, len);
	  nm_examples_barrier(sync_tag);
	  for(k = 0; k < iterations; k++)
	    {
	      TBX_GET_TICK(t1);
	      (*nm_bench.client)(buf, len);
	      TBX_GET_TICK(t2);
	      const double delay = TBX_TIMING_DELAY(t1, t2);
	      const double t = delay / 2;
	      lats[k] = t;
#ifdef DEBUG
	      control_buffer(buf, len);
#endif
	      nm_examples_barrier(sync_tag);
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

	  char*tag = "";
#ifdef MARCEL
	  tag = "marcel";
#endif
	  printf("%9lld\t%9.3lf\t%9.3f\t%9.3f\t%9.3lf\t%9.3lf\t%9.3lf\t%s\n",
		 (long long)len, min_lat, bw_million_byte, bw_mbyte, med_lat, avg_lat, max_lat, tag);
	  fflush(stdout);
	  free(buf);
	  nm_examples_barrier(sync_tag);
	}
      printf("# bench: %s end\n", nm_bench.name);
    }
  
  nm_examples_exit();
  exit(0);
}
