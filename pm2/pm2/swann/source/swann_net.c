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
