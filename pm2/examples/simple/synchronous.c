
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

#define STRING_SIZE  16

static char msg[STRING_SIZE] __ALIGNED__;

void sample_thread(void *arg)
{
  pm2_completion_t c;

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();

  pm2_printf("%s\n", msg);

  pm2_completion_signal(&c);
}

static void SAMPLE_service(void)
{
  pm2_service_thread_create(sample_thread, NULL);
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) {
    pm2_completion_t c;

    strcpy(msg, "Hello world!");

    pm2_completion_init(&c, NULL, NULL);

    pm2_rawrpc_begin(1, SAMPLE, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
    pm2_rawrpc_end();

    pm2_completion_wait(&c);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
