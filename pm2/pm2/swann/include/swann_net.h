
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * swann_net.h
 * -----------
 */

#ifndef __SWANN_NET_H
#define __SWANN_NET_H

typedef struct s_swann_net_client
{
  swann_net_client_id_t id;
  p_ntbx_client_t       client;
} swann_net_client_t;

typedef struct s_swann_net_server
{
  swann_net_server_id_t id;
  p_ntbx_server_t       server;
  p_swann_net_client_t  controler;
  tbx_list_t            client_list;
} swann_net_server_t;

#endif /* __SWANN_NET_H */
