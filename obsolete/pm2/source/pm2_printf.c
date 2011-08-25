
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
#include "sys/pm2_printf.h"

#include <stdarg.h>

static int PM2_PRINTF;

static marcel_sem_t print_mutex;
static char _pm2_print_buf[1024] __MAD_ALIGNED__;

void pm2_printf(char *format, ...)
{
  va_list args;
  char *ptr;

  if(!__pm2_self) {
    ma_preempt_disable();
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    ma_preempt_enable();
  } else {
    marcel_sem_P(&print_mutex);
    sprintf(_pm2_print_buf, "[n%x] ", __pm2_self);
    ptr = _pm2_print_buf + strlen(_pm2_print_buf);
    va_start(args, format);
    vsprintf(ptr, format, args);
    va_end(args);
    {
      int len = strlen(_pm2_print_buf)+1;

      pm2_rawrpc_begin(0, PM2_PRINTF, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, _pm2_print_buf, len);
      pm2_rawrpc_end();
    }
    marcel_sem_V(&print_mutex);
  }
}

static void netserver_printf(void)
{ 
  static char buf[2048] __MAD_ALIGNED__;
  int len;

  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, buf, len);

  pm2_rawrpc_waitdata();

  tfprintf(stderr, "%s", buf);
}

void pm2_printf_init(void)
{
  marcel_sem_init(&print_mutex, 1);

  pm2_rawrpc_register(&PM2_PRINTF, netserver_printf);
}


