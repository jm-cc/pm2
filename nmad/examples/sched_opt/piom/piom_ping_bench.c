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

#include "../helper.h"


/* This program performs a latency test with 0..n threads per CPU */
#ifndef PIOMAN
int main(){}
#else

#define MIN_DEFAULT	0
//#define MAX_DEFAULT	(64 * 1024)
#define MAX_DEFAULT	(8)
#define MULT_DEFAULT	2
#define INCR_DEFAULT	0
#define WARMUPS_DEFAULT	10
#define LOOPS_DEFAULT	1000
#define THREADS_DEFAULT 8
#define DATA_CONTROL_ACTIVATED 0

static __inline__
uint32_t _next(uint32_t len, uint32_t multiplier, uint32_t increment)
{
  if (!len)
    return 1+increment;
  else
    return len*multiplier+increment;
}

void usage_ping() {
  fprintf(stderr, "-S start_len - starting length [%d]\n", MIN_DEFAULT);
  fprintf(stderr, "-E end_len - ending length [%d]\n", MAX_DEFAULT);
  fprintf(stderr, "-I incr - length increment [%d]\n", INCR_DEFAULT);
  fprintf(stderr, "-M mult - length multiplier [%d]\n", MULT_DEFAULT);
  fprintf(stderr, "\tNext(0)      = 1+increment\n");
  fprintf(stderr, "\tNext(length) = length*multiplier+increment\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "-T thread - number of computing thread [%d]\n", THREADS_DEFAULT);
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
}

#ifdef MARCEL
static pmarcel_sem_t ready_sem;
static volatile int fini;

static void greedy_func() {
        marcel_sem_V(&ready_sem);
	while(fini!=1);
}
#endif
static void fill_buffer(char *buffer, int len) {
  unsigned int i = 0;

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
  int            threads;
  int		 i;
#ifdef MARCEL
  marcel_t      *pid;
  marcel_attr_t attr;
#endif

  init(&argc, argv);

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nmad_exit();
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
      nmad_exit();
      exit(0);
    }
  }

  buf = malloc(end_len);
  clear_buffer(buf, end_len);

#ifdef MARCEL
  marcel_attr_init(&attr);
  fini = 0;
  pid = malloc(sizeof(marcel_t) * marcel_nbvps());
  threads = marcel_nbvps();
  fprintf(stderr, "[%d Computing threads]\n", threads);

  marcel_sem_init(&ready_sem,0);
  for(i=0;i<threads;i++) {
    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(marcel_nbvps() - 1 - (i % marcel_nbvps())));
    //fprintf(stderr,"creating thread with vpset %d\n", attr.vpset);
    marcel_create(&pid[i], &attr, (void*)greedy_func, NULL);
    /* wait for the thread to be actually launched before we continue */
    marcel_sem_P(&ready_sem);
    //fprintf(stderr,"Thread %d created\n", i);
  }

#endif

  if (is_server) {
    int k;
    /* server */
    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {
      for(k = 0; k < iterations + warmups; k++) {
        nm_so_request request;

        nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_rwait(sr_if, request);

#if DATA_CONTROL_ACTIVATED
        control_buffer("réception", buf, len);
#endif
        nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_swait(sr_if, request);
      }
    }
  }
  else {
    tbx_tick_t t1, t2;
    double sum, lat, bw_million_byte, bw_mbyte;
    int k;

    /* client */
    fill_buffer(buf, end_len);

    fprintf(stderr," size     |  latency     |   10^6 B/s   |   MB/s    |\n");
    for(len = start_len; len <= end_len; len = _next(len, multiplier, increment)) {

      for(k = 0; k < warmups; k++) {
        nm_so_request request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
        nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_swait(sr_if, request);

        nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_rwait(sr_if, request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      TBX_GET_TICK(t1);

      for(k = 0; k < iterations; k++) {
        nm_so_request request;

#if DATA_CONTROL_ACTIVATED
        control_buffer("envoi", buf, len);
#endif
        nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_swait(sr_if, request);

        nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
        nm_so_sr_rwait(sr_if, request);
#if DATA_CONTROL_ACTIVATED
        control_buffer("reception", buf, len);
#endif
      }

      TBX_GET_TICK(t2);

      sum = TBX_TIMING_DELAY(t1, t2);

      lat	      = sum / (2 * iterations);
      bw_million_byte = len * (iterations / (sum / 2));
      bw_mbyte        = bw_million_byte / 1.048576;

      fprintf(stderr,"%d\t%lf\t%8.3f\t%8.3f\n", len, lat, bw_million_byte, bw_mbyte);
    }
  }

#ifdef MARCEL
  fini=1;
  for(i=0;i<threads;i++) {
    marcel_join(pid[i], NULL);
  }
#endif
  nmad_exit();
  exit(0);
}
#endif /* PIOMAN */

