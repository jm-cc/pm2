
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

#define ESSAIS 5
#define N      1000

static int SAMPLE, SAMPLE_THR;
static int autre;

static marcel_sem_t sem;

static tbx_tick_t t1, t2;

static int glob_n;

//#define MORE_PACKS  1
#define NON_THREADED
#define TEST_ONE_WAY

#ifdef MORE_PACKS
static unsigned dummy;
#endif

//static pm2_channel_t canal_secondaire;

void SAMPLE_service(void)
{
#ifdef MORE_PACKS
  {
    unsigned p;

    for(p=MORE_PACKS; p>0; p--)
      pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
  }
#endif
  pm2_rawrpc_waitdata();

  marcel_sem_V(&sem);
}

static void threaded_rpc(void *arg)
{
#ifdef MORE_PACKS
  {
    unsigned p;
    
    for(p=MORE_PACKS; p>0; p--)
      pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
  }
#endif
  pm2_rawrpc_waitdata();

  if(pm2_self() == 1) {
     pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
     {
       unsigned p;

       for(p=MORE_PACKS; p>0; p--)
	 pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
     }
#endif
     pm2_rawrpc_end();
  } else {
    if(glob_n) {
      glob_n--;
      pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
      {
	unsigned p;

	for(p=MORE_PACKS; p>0; p--)
	  pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
      }
#endif
      pm2_rawrpc_end();
    } else
      marcel_sem_V(&sem);
  }
}

void SAMPLE_THR_service(void)
{
  pm2_service_thread_create(threaded_rpc, NULL);
}

void f(void)
{
  int i, j;
  unsigned long temps;

#ifdef TEST_ONE_WAY
  fprintf(stderr, "One way non-threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    TBX_GET_TICK(t1);

    for(j=N/2; j; j--) {
      pm2_rawrpc_begin(autre, SAMPLE, NULL);
#ifdef MORE_PACKS
      {
	unsigned p;

	for(p=MORE_PACKS; p>0; p--)
	  pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
      }
#endif
      pm2_rawrpc_end();
    }

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }
#endif // TEST_ONE_WAY

#ifdef NON_THREADED
  fprintf(stderr, "Non-threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    TBX_GET_TICK(t1);

    for(j=N/2; j; j--) {
      pm2_rawrpc_begin(autre, SAMPLE, NULL);
#ifdef MORE_PACKS
      {
	unsigned p;

	for(p=MORE_PACKS; p>0; p--)
	  pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
      }
#endif
      pm2_rawrpc_end();

      marcel_sem_P(&sem);
    }

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }
#endif

  fprintf(stderr, "Threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    TBX_GET_TICK(t1);

    glob_n = N/2-1;
    pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
    {
      unsigned p;

      for(p=MORE_PACKS; p>0; p--)
	pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
    }
#endif
    pm2_rawrpc_end();

    marcel_sem_P(&sem);

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }
}

void g(void)
{
  int i, j;

#ifdef TEST_ONE_WAY
  for(i=0; i<ESSAIS; i++) {
    for(j=N/2; j; j--) {
      marcel_sem_P(&sem);
    }
  }
#endif // TEST_ONE_WAY

#ifdef NON_THREADED
  for(i=0; i<ESSAIS; i++) {
    for(j=N/2; j; j--) {

      marcel_sem_P(&sem);

      pm2_rawrpc_begin(autre, SAMPLE, NULL);
#ifdef MORE_PACKS
      {
	unsigned p;

	for(p=MORE_PACKS; p>0; p--)
	  pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
      }
#endif
      pm2_rawrpc_end();
    }
  }
#endif
}

static void startup_func(int argc, char *argv[], void *args)
{
  autre = (pm2_self() == 0) ? 1 : 0;

  marcel_sem_init(&sem, 0);

  //  pm2_channel_alloc(&canal_secondaire, "second");
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);
  pm2_rawrpc_register(&SAMPLE_THR, SAMPLE_THR_service);

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */

    f();

    pm2_halt();

  } else {

    g();

  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
