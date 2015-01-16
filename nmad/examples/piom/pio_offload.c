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

#include "../sendrecv/helper.h"
#include <tbx.h>
#ifdef PIOMAN_MARCEL
#include <marcel.h>
#endif

#define MIN_DEFAULT	0
#define MAX_DEFAULT	(8 * 1024 * 1024)
#define MULT_DEFAULT	2
#define INCR_DEFAULT	0
#define WARMUPS_DEFAULT	50
#define LOOPS_DEFAULT	200
#define COMPUTE_DEFAULT 200
#define DATA_CONTROL_ACTIVATED 0

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
  fprintf(stderr, "-C compute_time - computation time [%d]\n", COMPUTE_DEFAULT);
}

static void compute(unsigned usec){
	//return;
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	do {
		TBX_GET_TICK(t2);
	}while(TBX_TIMING_DELAY(t1,t2)<usec);
	//fprintf(stderr, "prout\n");
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

#if DATA_CONTROL_ACTIVATED
static void control_buffer(char *msg, char *buffer, int len) {
  tbx_bool_t   ok = tbx_true;
  unsigned char expected_char;
  unsigned int          i  = 0;

  for(i = 0; i < len; i++){
    expected_char = 'a'+(i%26);

    if(buffer[i] != expected_char){
      printf("Bad data at byte %d: expected %c, received %c\n",
             i, expected_char, buffer[i]);
      ok = tbx_false;
    }
  }

  printf("Controle de %s - ", msg);

  if (!ok) {
    printf("%d bytes reception failed\n", len);

    TBX_FAILURE("data corruption");
  } else {
    printf("ok\n");
  }
}
#endif

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
  int		 compute_time	= COMPUTE_DEFAULT;
  int		 i;

  nm_examples_init(&argc, argv);

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
    else if (!strcmp(argv[i], "-C")) {
      compute_time = atoi(argv[i+1]);
    }
    else {
      fprintf(stderr, "Illegal argument %s\n", argv[i]);
      usage_ping();
      nm_examples_exit();
      exit(0);
    }
  }
#ifdef PIOMAN_MARCEL
  marcel_vpset_t vpset= MARCEL_VPSET_VP(1);
  marcel_apply_vpset(&vpset);
#endif
  buf = malloc(end_len);
  clear_buffer(buf, end_len);

  if (is_server) {
    int k;
    /* server */
    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      for(k = 0; k < iterations + warmups; k++) {
        nm_sr_request_t request;

        nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
        nm_sr_rwait(p_session, &request);

#if DATA_CONTROL_ACTIVATED
        control_buffer("r�ception", buf, len);
#endif
        nm_sr_isend(p_session, p_gate, 0, buf, 0, &request);
        nm_sr_swait(p_session, &request);
      }
    }
  } else {
    int k;

    /* client */
    fill_buffer(buf, end_len);

    printf("# size |  latency     |    10^6 B/s   |    MB/s      |     isend   |     swait   |\n");

    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {

      for(k = 0; k < warmups; k++) {
        nm_sr_request_t request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
        nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
        nm_sr_swait(p_session, &request);

        nm_sr_irecv(p_session, p_gate, 0, buf, 0, &request);
        nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      double sum = 0, isum = 0.0, wsum = 0;

      for(k = 0; k < iterations; k++) {
	tbx_tick_t t1, t2, t3, t4;
        nm_sr_request_t request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
	TBX_GET_TICK(t1);      
        nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
	TBX_GET_TICK(t2);      
	compute(compute_time);
	TBX_GET_TICK(t3);      
        nm_sr_swait(p_session, &request);
	TBX_GET_TICK(t4);

	sum += TBX_TIMING_DELAY(t1, t4);
	isum += TBX_TIMING_DELAY(t1, t2);
	wsum += TBX_TIMING_DELAY(t3, t4);

        nm_sr_irecv(p_session, p_gate, 0, buf, 0, &request);
        nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      double lat	     = sum / (iterations);
      double bw_million_byte = len * (iterations / (sum ));
      double bw_mbyte        = bw_million_byte / 1.048576;
      double ilat	     = isum / (iterations);
      double wlat	     = wsum / (iterations);

      printf("%d\t%lf\t%8.3f\t%8.3f\t%8.3f\t%8.3f\n", len, lat, bw_million_byte, bw_mbyte, ilat, wlat);
    }
  }

  free(buf);
  nm_examples_exit();
  exit(0);
}
