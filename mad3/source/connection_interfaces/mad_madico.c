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

/*
 * Mad_madico.c
 * ============
 */

#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

/*
 * local structures
 * ----------------
 */

/*
 * Registration function
 * ---------------------
 */

char *
mad_madico_register(p_mad_driver_interface_t interface)
{
  LOG_IN();
  TRACE("Registering MADICO driver");

  interface->driver_init                = NULL;
  interface->adapter_init               = NULL;
  interface->channel_init               = NULL;
  interface->before_open_channel        = NULL;
  interface->connection_init            = NULL;
  interface->link_init                  = NULL;
  interface->accept                     = NULL;
  interface->connect                    = NULL;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = NULL;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = NULL;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = NULL;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = NULL;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = NULL;
  interface->message_received           = NULL;
  interface->send_buffer                = NULL;
  interface->receive_buffer             = NULL;
  interface->send_buffer_group          = NULL;
  interface->receive_sub_buffer_group   = NULL;
  LOG_OUT();

  return "madico";
}
