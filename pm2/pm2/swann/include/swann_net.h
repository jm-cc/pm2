
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
