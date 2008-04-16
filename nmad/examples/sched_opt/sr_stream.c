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

#include "helper.h"

#define MIN_DEFAULT	0
#define MAX_DEFAULT	(8 * 1024 * 1024)
#define MULT_DEFAULT	2
#define INCR_DEFAULT	0
#define WARMUPS_DEFAULT	100
#define LOOPS_DEFAULT	2000

static __inline__
uint32_t _next(uint32_t len, uint32_t multiplier, uint32_t increment)
{
  if (!len)
    return 1+increment;
  else
    return len*multiplier+increment;
}

static void usage_ping(void) {
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

  struct nm_so_interface *sr_if;
  nm_gate_id_t gate_id;

  nm_so_init(&argc, argv);
  nm_so_get_sr_if(&sr_if);

  if (is_server()) {
    nm_so_get_gate_in_id(1, &gate_id);
  }
  else {
    nm_so_get_gate_out_id(0, &gate_id);
  }

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nm_so_exit();
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
      nm_so_exit();
      exit(0);
    }
  }

  buf = malloc(end_len);

  if (is_server()) {
    tbx_tick_t t1, t2;
    double sum, lat, bw_million_byte, bw_mbyte;
    int k;

    /* server */

    printf(" size     |  latency     |   10^6 B/s   |   MB/s    |\n");

    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      nm_so_request request;

      for(k = 0; k < warmups; k++) {
        nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_swait(sr_if, request);
      }

      TBX_GET_TICK(t1);

      for(k = 0; k < iterations; k++) {
        nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_swait(sr_if, request);
      }

      nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
      nm_so_sr_rwait(sr_if, request);

      TBX_GET_TICK(t2);

      sum = TBX_TIMING_DELAY(t1, t2);

      lat	      = sum / iterations;
      bw_million_byte = len * (iterations / sum);
      bw_mbyte        = bw_million_byte / 1.048576;

      printf("%d\t%lf\t%8.3f\t%8.3f\n", len, lat, bw_million_byte, bw_mbyte);
    }

  } else {
    int k;

    /* client */

    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      nm_so_request request;

      for(k = 0; k < warmups + iterations; k++) {
        nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_rwait(sr_if, request);
      }
 
      nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
      nm_so_sr_swait(sr_if, request);
   }
  }

  free(buf);
  nm_so_exit();
  exit(0);
}
