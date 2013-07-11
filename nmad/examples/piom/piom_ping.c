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

#include "piom_helper.h"


/* This program performs a latency test with 0..n computing threads per CPU */
#ifdef PIOMAN_MULTITHREAD

#define MIN_DEFAULT	0
#define MAX_DEFAULT	(64 * 1024)
//#define MAX_DEFAULT	(8)
#define MULT_DEFAULT	2
#define INCR_DEFAULT	0
#define WARMUPS_DEFAULT	10
#define LOOPS_DEFAULT	10000
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

static piom_sem_t ready_sem;
static volatile int bench_complete = 0;

static void*greedy_func(void*arg) 
{
  piom_sem_V(&ready_sem);
  while(!bench_complete)
    { }
  return NULL;
}

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
  int            threads        = THREADS_DEFAULT;
  int		 warmups	= WARMUPS_DEFAULT;
  int		 i;
  piom_thread_t *pid;

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
    else if (!strcmp(argv[i], "-T")) {
      threads = atoi(argv[i+1]);
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

#ifdef MARCEL
  /* set highest priority so that the thread 
     is scheduled (almost) immediatly when done */
  struct marcel_sched_param sched_param = { .sched_priority = MA_MAX_SYS_RT_PRIO };
  struct marcel_sched_param old_param;
  marcel_sched_getparam(MARCEL_SELF, &old_param);
  marcel_sched_setparam(MARCEL_SELF, &sched_param);
#endif /* MARCEL */
  

  pid = malloc(sizeof(piom_thread_t) * threads);
  piom_sem_init(&ready_sem,0);
  for(i = 0; i <= threads; i++)
    {
      printf("# ## %d computing threads\n", i);
      if (is_server)
	{
	  int k;
	  /* server */
	  for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	    {
	      for(k = 0; k < iterations + warmups; k++)
		{
		  nm_sr_request_t request;
		  nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
		  control_buffer("réception", buf, len);
#endif
		  nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_swait(p_session, &request);
		}
	    }
	} 
      else
	{
	  /* client */
	  fill_buffer(buf, end_len);
	  if(i == 0)
	    printf("# threads | size  \t|  latency \t| 10^6 B/s \t| MB/s   \t| median  \t| avg    \t| max\n");
	  for(len = start_len; len <= end_len; len = _next(len, multiplier, increment))
	    {
	      int k;
	      tbx_tick_t t1, t2;
	      double*lats = malloc(sizeof(double) * iterations);
	      for(k = 0; k < warmups; k++)
		{
		  nm_sr_request_t request;
		  nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_swait(p_session, &request);
		  nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
		  control_buffer("reception", buf, len);
#endif
		}
	      for(k = 0; k < iterations; k++)
		{
		  nm_sr_request_t request;
		  TBX_GET_TICK(t1);
		  nm_sr_isend(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_swait(p_session, &request);
		  nm_sr_irecv(p_session, p_gate, 0, buf, len, &request);
		  nm_sr_rwait(p_session, &request);
		  TBX_GET_TICK(t2);
#if DATA_CONTROL_ACTIVATED
		  control_buffer("reception", buf, len);
#endif
		  const double delay = TBX_TIMING_DELAY(t1, t2);
		  const double t = delay / 2;
		  lats[k] = t;
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
	      printf("%5d\t%9lld\t%9.3lf\t%9.3f\t%9.3f\t%9.3lf\t%9.3lf\t%9.3lf\n",
		     i, (long long)len, min_lat, bw_million_byte, bw_mbyte, med_lat, avg_lat, max_lat);
	      free(lats);
	    }
	}
      piom_thread_create(&pid[i], NULL, (void*)greedy_func, NULL);
      /* wait for the thread to be actually launched before we continue */
      piom_sem_P(&ready_sem); 
    }
  
  bench_complete = 1;
  for (i=0 ; i<=threads ; i++)
    {
      piom_thread_join(pid[i], NULL);
    }
  fprintf(stderr, "Benchmark done!\n");

  nm_examples_exit();
  exit(0);
}

#else /* PIOMAN_MULTITHREAD */
int main(){ return -1; }

#endif /* PIOMAN_MULTITHREAD */
