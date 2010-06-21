
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

#include <sys/time.h>

static int DICHO;

static unsigned cur_proc = 0;

static __inline__ int next_proc(void)
{
  marcel_thread_preemption_disable();

  do {
    cur_proc = (cur_proc+1) % pm2_config_size();
  } while(cur_proc == pm2_self());

  marcel_thread_preemption_enable();
  return cur_proc;
}

static void completion_handler(void *arg)
{
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, (int *)arg, 1);
}

static void thr_dicho(void *arg)
{
  unsigned inf, sup, res;
  pm2_completion_t c;

  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &inf, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &sup, 1);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();

   if(inf == sup) {
      res = inf;
   } else {
      int mid = (inf + sup)/2;
      unsigned res1, res2;
      pm2_completion_t c1, c2;

      pm2_completion_init(&c1, completion_handler, &res1);
      pm2_completion_init(&c2, completion_handler, &res2);

      pm2_rawrpc_begin(next_proc(), DICHO, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &inf, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &mid, 1);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c1);
      pm2_rawrpc_end();

      mid++;

      pm2_rawrpc_begin(next_proc(), DICHO, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &mid, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &sup, 1);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c2);
      pm2_rawrpc_end();

      pm2_completion_wait(&c1); pm2_completion_wait(&c2);

      res = res1 + res2;
   }

   pm2_completion_signal_begin(&c);
   pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &res, 1);
   pm2_completion_signal_end();
}

static void dicho(void)
{
  pm2_thread_create(thr_dicho, NULL);
}

static void f(int argc, char *argv[])
{
  tbx_tick_t t1, t2;
  unsigned long temps;
  unsigned inf, sup, res;
  pm2_completion_t c;
  unsigned one_time_run = 0;

  while(!one_time_run) {

    if(argc > 1) {
      sup = atoi(argv[1]);
      one_time_run = 1;
    } else {
      tfprintf(stderr, "Entrez un entier raisonnable "
	       "(0 pour terminer) : ");
      scanf("%d", &sup);
    }

    if(!sup)
      break;

    inf = 1;

    TBX_GET_TICK(t1);

    pm2_completion_init(&c, completion_handler, &res);

    pm2_rawrpc_begin(next_proc(), DICHO, NULL);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &inf, 1);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &sup, 1);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
    pm2_rawrpc_end();

    pm2_completion_wait(&c);

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);

    tfprintf(stderr, "1+...+%d = %d\n", sup, res);
  }
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&DICHO, dicho);

  pm2_init(argc, argv);

  if(pm2_self() == 0) { /* master process */

    if(pm2_config_size() < 2) {
      fprintf(stderr,
	      "This program requires at least two processes.\n"
	      "Please rerun pm2-conf.\n");
      exit(1);
    }

    f(argc, argv);

    pm2_halt();
  }

  pm2_exit();

  return 0;
}
