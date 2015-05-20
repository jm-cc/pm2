/*
 * NewMadeleine
 * Copyright (C) 2006-2013 (see AUTHORS file)
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

#ifdef PIOMAN_MULTITHREAD

//#define DEBUG 1

/* This program performs several ping pong in parallel.
 * This evaluates the efficienty to access nmad from 1, 2, 3, ...n threads simultanously
*/

#define LEN_DEFAULT     8 /* (128*1024) */
#define WARMUPS_DEFAULT	10 /* 100 */
#define LOOPS_DEFAULT	2000 /* 2000 */
#define THREADS_DEFAULT 32
#define DATA_CONTROL_ACTIVATED 0

static uint32_t	 len;
static int	 loops;
static int       max_threads;
static int       warmups;

static struct
{
  int*go;
  piom_thread_mutex_t mutex;
  piom_thread_cond_t cond;
  int threads;
} synchro;

void usage_ping() {
  fprintf(stderr, "-L len - packet length [%d]\n", LEN_DEFAULT);
  fprintf(stderr, "-N iterations - iterations [%d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "-T thread - number of communicating threads [%d]\n", THREADS_DEFAULT);
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
}


static void*comm_thread(void* arg)
{
  const int my_pos = *(int*)arg;
  nm_tag_t tag = (nm_tag_t)my_pos;
  int k;
  char*buf = malloc(len);
  clear_buffer(buf, len);
  for(;;)
    {
      /* wait for the start signal */
      piom_thread_mutex_lock(&synchro.mutex);
      while(synchro.go[my_pos] != 1 && synchro.go[my_pos] != -1)
	piom_thread_cond_wait(&synchro.cond, &synchro.mutex);
      piom_thread_mutex_unlock(&synchro.mutex);
      if(synchro.go[my_pos] == -1)
	break;
      if(is_server)
	{
	  for(k = 0; k < loops + warmups; k++)
	    {
	      nm_sr_request_t request;
#if DEBUG
	      fprintf(stderr, "[%d] Recv %d\n", my_pos, k);
#endif
	      nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_rwait(p_session, &request);
#if DEBUG
	      fprintf(stderr, "[%d] Send %d\n", my_pos, k);
#endif
#if DATA_CONTROL_ACTIVATED
	      control_buffer("réception", buf, len);
#endif
	      nm_sr_isend(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_swait(p_session, &request);
	    }
	}
      else
	{
	  tbx_tick_t t1, t2;
	  clear_buffer(buf, len);
	  fill_buffer(buf, len);
	  for(k = 0; k < warmups; k++) 
	    {
	      nm_sr_request_t request;
#if DATA_CONTROL_ACTIVATED
	      control_buffer("envoi", buf, len);
#endif
#if DEBUG
	      fprintf(stderr, "[%d] Send %d\n", my_pos, k);
#endif
	      nm_sr_isend(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_swait(p_session, &request);
#if DEBUG
	      fprintf(stderr, "[%d] Recv %d\n", my_pos, k);
#endif
	      nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
	      control_buffer("reception", buf, len);
#endif
	    }
	  
	  TBX_GET_TICK(t1);
	  
	  for(k = 0; k < loops; k++)
	    {
	      nm_sr_request_t request;
#if DATA_CONTROL_ACTIVATED
	      control_buffer("envoi", buf, len);
#endif 
#if DEBUG
	      fprintf(stderr, "[%d] Send %d\n", my_pos, k+warmups);
#endif
	      nm_sr_isend(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_swait(p_session, &request);
	      
#if DEBUG
	      fprintf(stderr, "[%d] Recv %d\n", my_pos, k+warmups);
#endif
	      nm_sr_irecv(p_session, p_gate, tag, buf, len, &request);
	      nm_sr_rwait(p_session, &request);
#if DATA_CONTROL_ACTIVATED
	      control_buffer("reception", buf, len);
#endif
	    }
	  
	  TBX_GET_TICK(t2);
	  
	  double sum = TBX_TIMING_DELAY(t1, t2);
	  double lat	      = sum / (2 * loops);
	  double bw_million_byte = len * (loops / (sum / 2));
	  double bw_mbyte        = bw_million_byte / 1.048576;
	  printf("%d\t%d\t%d\t%lf\t%8.3f\t%8.3f\n", synchro.threads, my_pos, len, lat, bw_million_byte, bw_mbyte);
	}
      piom_thread_mutex_lock(&synchro.mutex);
      synchro.go[my_pos] = 2;
      piom_thread_cond_broadcast(&synchro.cond);
      piom_thread_mutex_unlock(&synchro.mutex);
    }
  return NULL;
}


int main(int argc, char**argv) 
{
  int		 i, j;
  piom_thread_t*pid = NULL;

  len     = LEN_DEFAULT;
  loops   = LOOPS_DEFAULT;
  max_threads = THREADS_DEFAULT;
  warmups = WARMUPS_DEFAULT;

  nm_examples_init(&argc, argv);

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nm_examples_exit();
    exit(0);
  }

  for(i = 1; i < argc; i += 2) {
    if (!strcmp(argv[i], "-N")) {
      loops = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-L")) {
      len = atoi(argv[i+1]);
    }
    else if (!strcmp(argv[i], "-T")) {
      max_threads = atoi(argv[i+1]);
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

  pid = malloc(sizeof(piom_thread_t) * max_threads);
  synchro.go = malloc(sizeof(int) * max_threads);
  piom_thread_cond_init(&synchro.cond, NULL);
  piom_thread_mutex_init(&synchro.mutex, NULL);
  for(i = 0; i < max_threads; i++)
    {
      synchro.go[i] = 0;
    }

  for(i = 0 ; i < max_threads; i++) 
    {
      printf("# %d x %d threads\n", i+1, i+1);
      piom_thread_create(&pid[i], NULL, (void*)comm_thread, &i);
      piom_thread_mutex_lock(&synchro.mutex);
      synchro.threads = i + 1;
      for(j = 0; j <= i; j++)
	{
	  assert(synchro.go[j] == 0);
	  synchro.go[j] = 1;
	}
      piom_thread_cond_broadcast(&synchro.cond);
      for(j = 0; j <= i; j++)
	{
	  while(synchro.go[j] == 1)
	    piom_thread_cond_wait(&synchro.cond, &synchro.mutex);
	  synchro.go[j] = 0;
	}
      piom_thread_mutex_unlock(&synchro.mutex);
    }
  fprintf(stderr, "# finalizing...\n");
  piom_thread_mutex_lock(&synchro.mutex);
  for(j = 0; j < max_threads; j++)
    {
      synchro.go[j] = -1;
      piom_thread_cond_broadcast(&synchro.cond);
    }
  piom_thread_mutex_unlock(&synchro.mutex);
  for(i = 0; i < max_threads; i++)
    piom_thread_join(pid[i], NULL);

  printf("# done.\n");
  nm_examples_exit();
  exit(0);
}

#else /* PIOMAN_MULTITHREAD */

int main(){ return -1; }

#endif /* PIOMAN_MULTITHREAD */
