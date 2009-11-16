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

#ifdef PIOMAN

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

#ifdef MARCEL
static pmarcel_sem_t ready_sem;
static int go;
static int done;
#endif

static __inline__
uint32_t _next(uint32_t len, uint32_t multiplier, uint32_t increment)
{
  if (!len)
    return 1+increment;
  else
    return len*multiplier+increment;
}

void usage_ping() {
  fprintf(stderr, "-L len - packet length [%d]\n", LEN_DEFAULT);
  fprintf(stderr, "-N iterations - iterations [%d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "-T thread - number of communicating threads [%d]\n", THREADS_DEFAULT);
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
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


void 
server(any_t arg) {
  int    my_pos = *(int*)arg;
  char	*buf	= NULL;
  nm_tag_t tag   = (nm_tag_t)my_pos;
  int    i, k;

  buf = malloc(len);
  clear_buffer(buf, len);
  for(i = my_pos; i <= threads; i++) {   
    while(go < i )
      marcel_yield();
    for(k = 0; k < loops + warmups; k++) {
      nm_sr_request_t request, request2;

      nm_sr_isend(p_core, gate_id, tag, buf, len, &request2);
      nm_sr_irecv(p_core, gate_id, tag, buf, len, &request);
      nm_sr_rwait(p_core, &request);
      nm_sr_swait(p_core, &request2);

#if DATA_CONTROL_ACTIVATED
      control_buffer("réception", buf, len);
#endif
    }
    marcel_sem_V(&ready_sem); 	
  } 
}

void
client(any_t arg) {
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
      marcel_yield();
    for(k = 0; k < warmups; k++) {
	    nm_sr_request_t request, request2;
#if DATA_CONTROL_ACTIVATED
	    control_buffer("envoi", buf, len);
#endif
	    nm_sr_isend(p_core, gate_id, tag, buf, len, &request2);
	    nm_sr_irecv(p_core, gate_id, tag, buf, len, &request);
	    nm_sr_rwait(p_core, &request);
	    nm_sr_swait(p_core, &request2);

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
      nm_sr_isend(p_core, gate_id, tag, buf, len, &request2);
      nm_sr_irecv(p_core, gate_id, tag, buf, len, &request);
      nm_sr_rwait(p_core, &request);
      nm_sr_swait(p_core, &request2);
#if DATA_CONTROL_ACTIVATED
      control_buffer("reception", buf, len);
#endif
    }
    done++;
    while(done <i)
           marcel_yield();

    TBX_GET_TICK(t2);
  
    sum = TBX_TIMING_DELAY(t1, t2);
  
    lat	      = sum / (2 * loops);
    bw_million_byte = len * (loops / (sum / 2));
    bw_mbyte        = bw_million_byte / 1.048576;
  
    printf("[%d]\t%d\t%lf\t%8.3f\t%8.3f\n", my_pos, len, lat, bw_million_byte, bw_mbyte);
    marcel_sem_V(&ready_sem); 	
  }
}

int
main(int	  argc,
     char	**argv) {
  int		 i, j;
#ifdef MARCEL
  marcel_t      *pid;
  marcel_attr_t attr;
#endif

  len =        LEN_DEFAULT;
  loops = LOOPS_DEFAULT;
  threads =    THREADS_DEFAULT;
  warmups =    WARMUPS_DEFAULT;

  init(&argc, argv);

  if (argc > 1 && !strcmp(argv[1], "--help")) {
    usage_ping();
    nmad_exit();
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
      nmad_exit();
      exit(0);
    }
  }

  if(!is_server)
    printf("thread |  size     |   latency     |    10^6 B/s   |    MB/s    |\n");

#ifdef MARCEL
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, tbx_true);
 
  pid = malloc(sizeof(marcel_t) * threads);
  marcel_sem_init(&ready_sem,0);
  go = 0;
  for (i=0 ; i<=threads ; i++) {
    printf("[%d communicating threads]\n", i+1);
#endif
    if (is_server) {
      marcel_create(&pid[i], &attr, (void*)server, &i);
    } else {
      marcel_create(&pid[i], &attr, (void*)client, &i);	    
    }
#ifdef MARCEL
    for( j = 0; j <= i; j++){
      marcel_sem_P(&ready_sem); 	
      go=j;
    }
    go++;
    done = 0;
  }
#endif
  printf("##### Benchmark Done!####\n");
  nmad_exit();
  exit(0);
}

#else
int main(){ return -1;}
#endif /* PIOMAN */
