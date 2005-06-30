
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

#ifndef NETSERVER_EST_DEF
#define NETSERVER_EST_DEF

#include "marcel.h"
#include "madeleine.h"

typedef enum e_pm2_net_request_tag
{
   NETSERVER_END          = 0,
   NETSERVER_REQUEST_HALT = 1,

   NETSERVER_RAW_RPC,    /* Must be the last one */
} pm2_net_request_tag, *p_pm2_net_request_tag;

void
pm2_net_init_channels(int   *argc,
		      char **argv);

void
pm2_net_servers_start(int   *argc,
		      char **argv);

void
pm2_net_request_end(void);

void
pm2_net_wait_end(void);

p_mad_channel_t
pm2_net_get_channel(pm2_channel_t c);

/*
 * Interface
 * ---------
 */
void
pm2_channel_alloc(pm2_channel_t *channel,
		  char          *name);

#endif
