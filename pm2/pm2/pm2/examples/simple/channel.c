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


#define __ALIGNED__  __attribute__ ((aligned (sizeof(int))))
#define STRING_SIZE  16

static void ABC_service(void)
{
  char msg[STRING_SIZE];

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_rawrpc_waitdata();
  fprintf(stderr,"%s\n", msg);
  pm2_halt();
}

int pm2_main(int argc, char **argv)
{
  unsigned int	ABC = 0;
  pm2_channel_t	channel_A;
  pm2_channel_t	channel_B;
  pm2_attr_t	attr_channel_A;
  pm2_attr_t	attr_channel_B;


  pm2_rawrpc_register(&ABC, ABC_service);

  common_pre_init(&argc, argv, NULL);
  pm2_channel_alloc(&channel_A, "canal_A");
  pm2_channel_alloc(&channel_B, "canal_B");

  pm2_attr_init(&attr_channel_A);
  pm2_attr_init(&attr_channel_B);

  pm2_attr_setchannel(&attr_channel_A, channel_A);
  pm2_attr_setchannel(&attr_channel_B, channel_B);
  common_post_init(&argc, argv, NULL);

  if (pm2_config_size() < 2) {
      FAILURE("This program requires at least two processes.\n"
	      "Please rerun pm2conf.\n");
  }

#ifdef PROFILE
  profile_activate(FUT_ENABLE, FUT_KEYMASKALL, 0);
#endif

  if (pm2_self() == 0) {
    char msg[STRING_SIZE] __ALIGNED__;

    strcpy(msg, "Hello world!...");
    pm2_rawrpc_begin(1, ABC, &attr_channel_A);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_rawrpc_end();

    pm2_rawrpc_begin(1, ABC, &attr_channel_B);
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
