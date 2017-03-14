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

#include "../common/nm_examples_helper.h"

#define MIN_DEFAULT	0
#define MAX_DEFAULT	(8 * 1024 * 1024)
#define MULT_DEFAULT	2
#define INCR_DEFAULT	0
#define WARMUPS_DEFAULT	100
#define LOOPS_DEFAULT	2000
#define MAX_DATA        (100ULL * 1024ULL * 1024ULL) /* 100 MB */

#define DATA_CONTROL_ACTIVATED 0

static __inline uint32_t _iterations(int iterations, uint32_t len)
{
  uint64_t data_size = ((uint64_t)iterations * (uint64_t)len);
  if(data_size  > MAX_DATA)
    {
      iterations = (MAX_DATA / (uint64_t)len);
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
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
}

int
main(int	  argc,
     char	**argv) {
  char		*buf		= NULL;
  uint32_t	 len;
  uint32_t	 start_len	= MIN_DEFAULT;
  uint32_t	 end_len	= MAX_DEFAULT;
  uint32_t	 multiplier	= MULT_DEFAULT;
  uint32_t	 increment	= INCR_DEFAULT;
  int		 iterations	= LOOPS_DEFAULT;
  int		 warmups	= WARMUPS_DEFAULT;
  int		 i;

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
      multiplier = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-N")) {
      iterations = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-W")) {
      warmups = atoi(argv[i+1]);
    }
    else {
      fprintf(stderr, "Illegal argument %s\n", argv[i]);
      usage_ping();
      nm_examples_exit();
      exit(0);
    }
  }

  buf = malloc(end_len);
  clear_buffer(buf, end_len);

  if (is_server) {
    int k;
    /* server */
    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      iterations = _iterations(iterations, len);
      for(k = 0; k < iterations + warmups; k++) {
        nm_sr_request_t request;

        nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
        nm_sr_rwait(p_session, &request);

#if DATA_CONTROL_ACTIVATED
        control_buffer("r�ception", buf, len);
#endif
        nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
        nm_sr_swait(p_session, &request);
      }
    }
  } else {
    tbx_tick_t t1, t2;
    double sum, lat, bw_million_byte, bw_mbyte;
    int k;

    /* client */
    fill_buffer(buf, end_len);

    printf(" size     |  latency     |   10^6 B/s   |   MB/s    |\n");

    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      iterations = _iterations(iterations, len);
      for(k = 0; k < warmups; k++) {
        nm_sr_request_t request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
        nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
        nm_sr_swait(p_session, &request);

        nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
        nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      TBX_GET_TICK(t1);

      for(k = 0; k < iterations; k++) {
        nm_sr_request_t request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
        nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
        nm_sr_swait(p_session, &request);

        nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
        nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      TBX_GET_TICK(t2);

      sum = TBX_TIMING_DELAY(t1, t2);

      lat	      = sum / (2 * iterations);
      bw_million_byte = len * (iterations / (sum / 2));
      bw_mbyte        = bw_million_byte / 1.048576;

      printf("%d\t%lf\t%8.3f\t%8.3f\n", len, lat, bw_million_byte, bw_mbyte);
    }
  }

  free(buf);
  nm_examples_exit();
  exit(0);
}
