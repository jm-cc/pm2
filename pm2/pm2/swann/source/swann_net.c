/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: swann_net.c,v $
Revision 1.3  2000/04/21 15:40:20  oaumage
- fusion de la barnche pm2_mad2_multicluster

Revision 1.2.2.2  2000/04/03 13:45:26  oaumage
- ajout de fonctions de transmission pour int, long, double et string
- support du transfert de fichiers distants bloc a bloc

Revision 1.2.2.1  2000/04/03 08:55:11  oaumage
- support partiel du transfert de fichiers (par blocs)

Revision 1.2  2000/03/27 12:54:06  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.1  2000/02/17 09:29:34  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

/*
 * swann_net.c
 * ===========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "swann.h"

/*
 * Network connection initialization
 * ---------------------------------
 */
p_swann_net_server_t
swann_net_server_init(swann_net_server_id_t     id,
		      char                     *master_host_name,
		      p_ntbx_connection_data_t  connection_data)
{
  p_swann_net_server_t net_server = NULL;
  ntbx_status_t        ntbx_status;

  LOG_IN();

  /* Structure */
  net_server = malloc(sizeof(swann_net_server_t));
  CTRL_ALLOC(net_server);
  net_server->id = id;

  /* Server */
  net_server->server = malloc(sizeof(ntbx_server_t));
  CTRL_ALLOC(net_server->server);
  net_server->server->state = ntbx_server_state_uninitialized;
  ntbx_tcp_server_init(net_server->server);
  
  /* Controler client */
  net_server->controler = malloc(sizeof(swann_net_client_t));
  CTRL_ALLOC(net_server->controler);
  net_server->controler->id     = 0;
  net_server->controler->client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(net_server->controler->client);
  net_server->controler->client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(net_server->controler->client);
  ntbx_status = ntbx_tcp_client_connect(net_server->controler->client,
					master_host_name,
					connection_data);

  if (ntbx_status != ntbx_success)
    FAILURE("ntbx_tcp_client_connect failed");

  /* Client list */
  tbx_list_init(&net_server->client_list);

  LOG_OUT();
  return net_server;
}

/*
 * Network data transfer
 * ---------------------
 */
void
swann_net_send_int(p_ntbx_client_t client,
		   int             data)
{
  ntbx_pack_buffer_t  pack_buffer;

  LOG_IN();
  ntbx_pack_int(sizeof(ntbx_pack_buffer_t), &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  
  ntbx_pack_int(data, &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

int
swann_net_receive_int(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t pack_buffer;
  size_t             size;
  int                data;
  
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  size = ntbx_unpack_int(&pack_buffer);
  if (size != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid message size");
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_int(&pack_buffer);
  LOG_OUT();
  return data;
}

void
swann_net_send_long(p_ntbx_client_t client,
		    long            data)
{
  ntbx_pack_buffer_t pack_buffer;
  
  LOG_IN();
  ntbx_pack_int(sizeof(ntbx_pack_buffer_t), &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  
  ntbx_pack_long(data, &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}


long
swann_net_receive_long(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t  pack_buffer;
  size_t size;
  long    data;
  
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  size = ntbx_unpack_int(&pack_buffer);
  if (size != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid message size");
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_long(&pack_buffer);
  LOG_OUT();
  return data;
}

void
swann_net_send_double(p_ntbx_client_t client,
		      double          data)
{
  ntbx_pack_buffer_t  pack_buffer;
  LOG_IN();
  ntbx_pack_int(sizeof(ntbx_pack_buffer_t), &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  
  ntbx_pack_double(data, &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

double
swann_net_receive_double(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t pack_buffer;
  size_t             size;
  double             data;
  
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  size = ntbx_unpack_int(&pack_buffer);
  if (size != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid message size");
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_double(&pack_buffer);
  LOG_OUT();
  return data;
}

void
swann_net_send_string(p_ntbx_client_t  client,
		      char            *data)
{
  ntbx_pack_buffer_t pack_buffer;
  int                len;
  
  LOG_IN();
  len = strlen(data) + 1;
  ntbx_pack_int(len, &pack_buffer);
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, data, len);
  LOG_OUT();
}

char *
swann_net_receive_string(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t  pack_buffer;
  int                 len;
  char               *buffer
  
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  len = ntbx_unpack_int(&pack_buffer);

  buffer = malloc(len);
  CTRL_ALLOC(buffer);
  
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, buffer, len);
  LOG_OUT();
  return buffer;
}
