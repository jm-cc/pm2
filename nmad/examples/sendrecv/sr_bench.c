/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include "helper.h"

#define MIN_DEFAULT     0
#define MAX_DEFAULT     (512 * 1024 * 1024)
#define MULT_DEFAULT    2
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   100000


static inline uint32_t _next(uint32_t len, double multiplier, uint32_t increment)
{
  if(len == 0)
    len = 1;
  return len * multiplier + increment;
}

static inline uint32_t _iterations(int iterations, uint32_t len)
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


static void usage_ping(void) {
  fprintf(stderr, "-S start_len - starting length [%d]\n", MIN_DEFAULT);
  fprintf(stderr, "-E end_len - ending length [%d]\n", MAX_DEFAULT);
  fprintf(stderr, "-I incr - length increment [%d]\n", INCR_DEFAULT);
  fprintf(stderr, "-M mult - length multiplier [%d]\n", MULT_DEFAULT);
  fprintf(stderr, "\tNext(0)      = 1+increment\n");
  fprintf(stderr, "\tNext(length) = length*multiplier+increment\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", LOOPS_DEFAULT);
}

static void fill_buffer(char *buffer, int len) {
  int i = 0;

  for (i = 0; i < len; i++) {
    buffer[i] = 'a'+(i%26);
  }
}

static void clear_buffer(char *buffer, int len) {
  memset(buffer, 0, len);
}

int main(int argc, char	**argv)
{
  uint32_t	 start_len      = MIN_DEFAULT;
  uint32_t	 end_len        = MAX_DEFAULT;
  double         multiplier     = MULT_DEFAULT;
  uint32_t         increment      = INCR_DEFAULT;
  int              iterations     = LOOPS_DEFAULT;
  int              i;
  
  nm_examples_init_topo(&argc, argv, NM_EXAMPLES_TOPO_PAIRS);
  
  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nm_examples_exit();
    exit(0);
  }
  
  for(i=1 ; i<argc ; i+=2) {
    if (!strcmp(argv[i], "-S")) {
      start_len = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-E")) {
      end_len = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-I")) {
      increment = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-M")) {
      multiplier = atof(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-N")) {
      iterations = atoi(argv[i+1]);
    }
    else {
      fprintf(stderr, "Illegal argument %s\n", argv[i]);
      usage_ping();
      nm_examples_exit();
      exit(0);
    }
  }
  
  if(is_server)
    {
      /* server
       */
      uint32_t	 len;
      for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	{
	  int k;
	  char* buf = malloc(len);
	  clear_buffer(buf, len);
	  iterations = _iterations(iterations, len);
	  for(k = 0; k < iterations; k++)
	    {
	      nm_sr_request_t request;
	      
	      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
	      nm_sr_rwait(p_session, &request);
	      
	      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
	      nm_sr_swait(p_session, &request);
	    }
	  free(buf);
	}
    }
  else 
    {
      /* client
       */
      tbx_tick_t t1, t2;
      printf("# sr_bench begin\n");
      printf("# size |  latency     |   10^6 B/s   |   MB/s    |\n");
      uint32_t	 len;
      for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	{
	  char* buf = malloc(len);
	  iterations = _iterations(iterations, len);
	  double lat = DBL_MAX;
	  int k;
	  for(k = 0; k < iterations; k++)
	    {
	      nm_sr_request_t request;
	      TBX_GET_TICK(t1);
	      nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
	      nm_sr_swait(p_session, &request);
	      nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
	      nm_sr_rwait(p_session, &request);
	      TBX_GET_TICK(t2);
	      const double delay = TBX_TIMING_DELAY(t1, t2);
	      const double t = delay / 2;
	      if(t < lat)
		lat = t;
	    }	  
	  double bw_million_byte = len / lat;
	  double bw_mbyte        = bw_million_byte / 1.048576;
	  
#ifdef MARCEL
	  printf("%9d\t%9.3lf\t%9.3f\t%9.3f\tmarcel\n",
		 len, lat, bw_million_byte, bw_mbyte);
#else
	  printf("%9d\t%9.3lf\t%9.3f\t%9.3f\n",
		 len, lat, bw_million_byte, bw_mbyte);
#endif
	  free(buf);
	}
      printf("# sr_bench end\n");
    }
  
  nm_examples_exit();
  exit(0);
}
