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
 suppong documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leo_swann.c,v $
Revision 1.6  2000/06/16 13:46:35  oaumage
- Progression du code

Revision 1.5  2000/06/09 12:40:35  oaumage
- Progression du code

Revision 1.4  2000/06/09 08:45:58  oaumage
- Progression du code

Revision 1.3  2000/05/18 11:34:18  oaumage
- remplacement des types `tableau' par des types `structure de tableau'
  par securite

Revision 1.2  2000/05/17 12:41:00  oaumage
- reorganisation du code de demarrage de Leonie

Revision 1.1  2000/05/15 13:51:56  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_swann.c
 * ===========
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Constants and Macros
 * --------------------
 */
#define MAX_ARG_STR_LEN   1024
#define MAX_ARG_LEN        256

/*
 * Swann interfacing
 * -----------------
 */
p_leo_swann_module_t
leo_launch_swann_module(p_leonie_t                  leonie,
			p_leo_application_cluster_t cluster)
{
  p_leo_clu_cluster_file_t  remote_cluster_def = NULL;
  p_leo_swann_module_t      module             = NULL;
  char                     *cmd                = NULL;
  ntbx_status_t             status;  
  /* int                    n; */
  
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  module = malloc(sizeof(leo_swann_module_t));
  CTRL_ALLOC(module);

  module->id          = ++leonie->cluster_counter;
  module->relay       = NULL;
  module->app_cluster = cluster;
  module->clu_def     = NULL;
  
  module->net_client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(module->net_client);
  module->net_client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(module->net_client);

  sprintf(cmd,
	  "rsh %s swann -id %d -cnx %s -master %s &",
	  module->app_cluster->id,
	  module->id,
	  leonie->net_server->connection_data.data,
	  module->net_client->local_host);
  DISP("Launching swann module: %s", cmd);
  system(cmd);
  status = ntbx_tcp_server_accept(leonie->net_server, module->net_client);
  if (status != ntbx_success)
    FAILURE("could not establish a connection with the swann module");

  remote_cluster_def = leo_parse_cluster_file(module, "~/.pm2/leo_cluster");
  DISP("Remote cluster id is %s", remote_cluster_def->cluster->id);
 
  module->clu_def = leo_build_cluster_definition(remote_cluster_def);
  module->clu_def->mad_module   = NULL;
  module->clu_def->swann_module = module;
  LOG_OUT();
  return module;
}

p_leo_mad_module_t
leo_launch_mad_module(p_leonie_t                  leonie,
		      p_leo_application_cluster_t cluster)
{
  p_leo_mad_module_t        module             = NULL;
  char                     *cmd                = NULL;
  ntbx_status_t             status;  
  
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  module = malloc(sizeof(leo_swann_module_t));
  CTRL_ALLOC(module);

  module->id          = ++leonie->cluster_counter;
  module->relay       = NULL;
  module->app_cluster = cluster;
  module->clu_def     = NULL;
  
  module->net_client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(module->net_client);
  module->net_client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(module->net_client);

  sprintf(cmd,
	  "cd %s;./pm2load %s -leonie -id %d -cnx %s -master %s &",
	  cluster->path,
	  cluster->executable,
	  module->id,
	  leonie->net_server->connection_data.data,
	  module->net_client->local_host);
  DISP("Launching mad module: %s", cmd);
  system(cmd);
  status = ntbx_tcp_server_accept(leonie->net_server, module->net_client);
  if (status != ntbx_success)
    FAILURE("could not establish a connection with the mad module");

  LOG_OUT();
  return module;
}

