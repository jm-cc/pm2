#include "pm2.h"
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


#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define STRING_SIZE  16

static char msg[STRING_SIZE] __ALIGNED__;

static int SAMPLE;

static void SAMPLE_service(void)
{
  char msg[STRING_SIZE];

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_rawrpc_waitdata();

  fprintf(stderr,"%s\n", msg);

  pm2_halt();
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

#ifdef PROFILE
  profile_activate(FUT_ENABLE, FUT_KEYMASKALL, 0);
#endif

  if(pm2_self() == 0) {
    strcpy(msg, "Hello world!   ");
    pm2_rawrpc_begin(1, SAMPLE, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_rawrpc_end();
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

   fprintf(stderr, "Main ended\n");
  return 0;
}
