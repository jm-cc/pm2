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

#include "nm_piom_helper.h"

#ifdef PIOMAN_MULTITHREAD

//#define LEN_DEFAULT      16*1024*1024
#define LEN_DEFAULT      (128*1024)
#define WARMUPS_DEFAULT	10
#define LOOPS_DEFAULT	100
#define THREADS_DEFAULT 4
#define DATA_CONTROL_ACTIVATED 0

static uint32_t	 len;
static int	 loops;
static int       threads;
static int       warmups;

static piom_sem_t ready_sem;
static int go;
static int done;


void usage_ping(void)
{
  fprintf(stderr, "-L len - packet length [%d]\n", LEN_DEFAULT);
  fprintf(stderr, "-N iterations - iterations [%d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "-T thread - number of communicating threads [%d]\n", THREADS_DEFAULT);
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
}

void server(void* arg) 
{
  int    my_pos = *(int*)arg;
  char	*buf	= NULL;
  nm_tag_t tag   = (nm_tag_t)my_pos;
  int    i, k;

  buf = malloc(len);
  clear_buffer(buf, len);
  for(i = my_pos; i <= threads; i++) {   
    while(go < i )
      piom_thread_yield();
    for(k = 0; k < loops + warmups; k++) {
      nm_sr_request_t request, request2;

      nm_sr_isend(p_session, p_gate, tag, buf, len, &request2);
      nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
      nm_sr_rwait(p_session, &request);
      nm_sr_swait(p_session, &request2);

#if DATA_CONTROL_ACTIVATED
      control_buffer("réception", buf, len);
#endif
    }
    piom_sem_V(&ready_sem); 	
  } 
}

void client(void*arg)
{
  int        my_pos = *(int*)arg;
  nm_tag_t   tag    = (nm_tag_t)my_pos;
  char	    *buf    = NULL;
  tbx_tick_t t1, t2;
  double     sum, lat, bw_million_byte, bw_mbyte;
  int        i, k;

  buf = malloc(len);
  clear_buffer(buf, len);

  fill_buffer(buf, len);
  for(i = my_pos; i <= threads; i++) {
    while(go < i )
      piom_thread_yield();
    for(k = 0; k < warmups; k++) {
	    nm_sr_request_t request, request2;
#if DATA_CONTROL_ACTIVATED
	    control_buffer("envoi", buf, len);
#endif
	    nm_sr_isend(p_session, p_gate, tag, buf, len, &request2);
	    nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
	    nm_sr_rwait(p_session, &request);
	    nm_sr_swait(p_session, &request2);

#if DATA_CONTROL_ACTIVATED
	    control_buffer("reception", buf, len);
#endif
    }
  
    TBX_GET_TICK(t1);
    
    for(k = 0; k < loops; k++) {
      nm_sr_request_t request, request2;
#if DATA_CONTROL_ACTIVATED
      control_buffer("envoi", buf, len);
#endif
      nm_sr_isend(p_session, p_gate, tag, buf, len, &request2);
      nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
      nm_sr_rwait(p_session, &request);
      nm_sr_swait(p_session, &request2);
#if DATA_CONTROL_ACTIVATED
      control_buffer("reception", buf, len);
#endif
    }
    done++;
    while(done <i)
      piom_thread_yield();

    TBX_GET_TICK(t2);
  
    sum = TBX_TIMING_DELAY(t1, t2);
  
    lat	      = sum / (2 * loops);
    bw_million_byte = len * (loops / (sum / 2));
    bw_mbyte        = bw_million_byte / 1.048576;
  
    printf("[%d]\t%d\t%lf\t%8.3f\t%8.3f\n", my_pos, len, lat, bw_million_byte, bw_mbyte);
    piom_sem_V(&ready_sem); 	
  }
}

int main(int	  argc, char**argv)
{
  int		 i, j;
  piom_thread_t  *pid;

  len =        LEN_DEFAULT;
  loops = LOOPS_DEFAULT;
  threads =    THREADS_DEFAULT;
  warmups =    WARMUPS_DEFAULT;

  nm_examples_init(&argc, argv);

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nm_examples_exit();
    exit(0);
  }

  for(i=1 ; i<argc ; i+=2) {
    if (!strcmp(argv[i], "-N")) {
      loops = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-L")) {
      len = atoi(argv[i+1]);
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

  if(!is_server)
    printf("thread |  size     |   latency     |    10^6 B/s   |    MB/s    |\n");

  pid = malloc(sizeof(piom_thread_t) * threads);
  piom_sem_init(&ready_sem,0);
  go = 0;
  for (i=0 ; i<=threads ; i++) {
    printf("[%d communicating threads]\n", i+1);
    if (is_server) {
      piom_thread_create(&pid[i], NULL, (void*)server, &i);
    } else {
      piom_thread_create(&pid[i], NULL, (void*)client, &i);	    
    }
    for( j = 0; j <= i; j++){
      piom_sem_P(&ready_sem); 	
      go=j;
    }
    go++;
    done = 0;
  }
  printf("##### Benchmark Done!####\n");
  nm_examples_exit();
  exit(0);
}

#else /* PIOMAN_MULTITHREAD */
int main(){ return -1;}
#endif /* PIOMAN_MULTITHREAD */
