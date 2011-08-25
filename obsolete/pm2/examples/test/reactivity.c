
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#include "pm2.h"

static int SAMPLE, START_BUSY, STOP_BUSY;

static volatile tbx_bool_t stay_busy = tbx_true;

void busy_thread(void *arg)
{
  pm2_completion_t c;

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();

  pm2_completion_signal(&c);

  while(stay_busy) ;
}

static void START_BUSY_service(void)
{
  pm2_thread_create(busy_thread, NULL); // Pas pm2_service_thread_create !
}

static void STOP_BUSY_service(void)
{
  pm2_rawrpc_waitdata();

  stay_busy = tbx_false;
}

void sample_thread(void *arg)
{
  pm2_completion_t c;

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();

  //pm2_printf("Hi! I'm a reactive service thread, believe me or not!\n");

  pm2_completion_signal(&c);
}

static void SAMPLE_service(void)
{
  pm2_service_thread_create(sample_thread, NULL);
}

static void usage(char *prog)
{
  fprintf(stderr, "Usage: %s [ --busy <nb_threads> ]\n", prog);
  pm2_halt();
  pm2_exit();
  exit(1);
}

static void spawn_busy_threads(int argc, char **argv)
{
  unsigned i, nb = 0;
  pm2_completion_t c;

  if(argc == 3) {
    if(!strcmp(argv[1], "--busy")) {
      nb = atoi(argv[2]);
    } else {
      usage(argv[0]);
    }
  } else if(argc > 1) {
    usage(argv[0]);
  }

  pm2_completion_init(&c, NULL, NULL);

  for(i=0; i<nb; i++) {
    pm2_rawrpc_begin(1, START_BUSY, NULL);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
    pm2_rawrpc_end();
  }

  for(i=0; i<nb; i++) {
    pm2_completion_wait(&c);
  }

  fprintf(stderr, "%d \"busy\" threads spawned on node 1...\n", nb);
}

static void stop_busy_threads(void)
{
  pm2_rawrpc_begin(1, STOP_BUSY, NULL);
  pm2_rawrpc_end();
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);
  pm2_rawrpc_register(&START_BUSY, START_BUSY_service);
  pm2_rawrpc_register(&STOP_BUSY, STOP_BUSY_service);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) {

    pm2_completion_t c;
    tbx_tick_t t1, t2;
    unsigned long temps;
    int i;

    spawn_busy_threads(argc, argv);

    for(i=0; i<10; i++) {
      pm2_completion_init(&c, NULL, NULL);

      TBX_GET_TICK(t1);
      pm2_rawrpc_begin(1, SAMPLE, NULL);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end();
      fprintf(stderr, "ping %d in progress...\n", i);

      pm2_completion_wait(&c);
      TBX_GET_TICK(t2);
      temps = TBX_TIMING_DELAY(t1, t2);
      fprintf(stderr, "ping %d: round trip time = %ld.%03ldms\n",
	     i, temps/1000, temps%1000);
    }

    stop_busy_threads();

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
