
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

static int SAMPLE;

#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define NB	3

#define STRING_SIZE  12

static char msg1[STRING_SIZE] __ALIGNED__ = "Hi Guys!   ";
static char msg2[STRING_SIZE] __ALIGNED__ = "Hi Girls!  ";
static char msg3[STRING_SIZE] __ALIGNED__ = "Hi world!  ";

static char *mess[NB];

static void SAMPLE_thread(void *arg)
{
  int i;
  char str[64];
  pm2_completion_t c;

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, str, STRING_SIZE);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);

  pm2_rawrpc_waitdata();

  for(i=0; i<10; i++) {
    pm2_printf("%s (I am %p on LWP %d)\n",
	       str, marcel_self(), marcel_current_vp());
    marcel_delay(100);
  }

  pm2_completion_signal(&c);
}

static void SAMPLE_service(void)
{
  pm2_service_thread_create(SAMPLE_thread, NULL);
}

int pm2_main(int argc, char **argv)
{
  int i;

  mess[0] = msg1; mess[1] = msg2; mess[2] = msg3;

  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */
    pm2_completion_t c;

    pm2_completion_init(&c, NULL, NULL);

    for(i=0; i<NB; i++) {
      pm2_rawrpc_begin(1, SAMPLE, NULL);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, mess[i], STRING_SIZE);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end();
    }

    for(i=0; i<NB; i++)
      pm2_completion_wait(&c);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
