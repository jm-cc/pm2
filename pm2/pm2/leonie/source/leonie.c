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
$Log: leonie.c,v $
Revision 1.6  2000/05/17 12:41:00  oaumage
- reorganisation du code de demarrage de Leonie

Revision 1.5  2000/05/15 13:51:56  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leonie.c
 * ========
 */ 
/* #define DEBUG */
/* #define TRACING */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Constants and Macros
 * --------------------
 */
#define MAX_FILENAME_SIZE 1024
#define MAX_ARG_STR_LEN   1024
#define MAX_ARG_LEN        256

/*
 * Static Variables
 * ----------------
 */
static p_leonie_t main_leonie = NULL;

/*
 * Functions
 * ==========================================================================
 */

/*
 * Madeleine interfacing
 * ---------------------
 */
static p_leo_mad_module_t
leo_launch_mad_module(p_leo_swann_module_t  swann_module,
		      p_leo_app_cluster_t   app_cluster)
{
  p_leo_mad_module_t  mad_module = NULL;
  char               *cmd        = NULL;
  ntbx_status_t       status;  
  
  LOG_IN();
  mad_module = TBX_MALLOC(sizeof(leo_mad_module_t));
  CTRL_ALLOC(mad_module);

  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  mad_module->id = ++main_leonie->cluster_counter;

  mad_module->cluster_id = malloc(strlen(app_cluster->id) + 1);
  CTRL_ALLOC(mad_module->cluster_id);
  strcpy(mad_module->cluster_id, app_cluster->id);

  mad_module->net_client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(mad_module->net_client);
  mad_module->net_client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(mad_module->net_client);

  sprintf(cmd,
	  "%s/%s -leonie -id %d -cnx %s -master %s &",
	  app_cluster->path,
	  app_cluster->executable,
	  mad_module->id,
	  main_leonie->net_server->connection_data,
	  mad_module->net_client->local_host);
  system(cmd);
  status =
    ntbx_tcp_server_accept(main_leonie->net_server, mad_module->net_client);
  if (status != ntbx_success)
    FAILURE("could not establish a connection with the madeleine module");
  LOG_OUT();
  return mad_module;
}

/* ---  */
p_leonie_t leo_init()
{
  p_leonie_t leonie = NULL;
  
  LOG_IN();
  leonie = TBX_MALLOC(sizeof(leonie_t));
  CTRL_ALLOC(leonie);

  leonie->cluster_counter = 0;
  
  tbx_list_init(&leonie->swann_modules);

  LOG_OUT();
  return leonie;
}

void
leo_start(p_leo_app_application_t  application,
	  p_leo_clu_cluster_file_t local_cluster_def)
{
  tbx_list_reference_t ref;
  p_leo_app_cluster_t app_cluster = NULL;
  
  LOG_IN();
  if (tbx_empty_list(application->cluster_list))
    FAILURE("no cluster in application description file");
  
  if (application->cluster_list->length > 1)
    FAILURE("multicluster sessions are not yet supported");

  tbx_list_reference_init(&ref, application->cluster_list);

  app_cluster = tbx_get_list_reference_object(&ref);

  if (tbx_forward_list_reference(&ref))
    FAILURE("multicluster sessions are not yet supported");

  if (strcmp(local_cluster_def->cluster->id, app_cluster->id))
    {
      p_leo_clu_host_name_t host_description;

      /* Cluster is remote */
      host_description =
	leo_get_cluster_entry_point(local_cluster_def->cluster,
				    app_cluster->id);
      if (!host_description)
	FAILURE("could not find cluster entry point");

      leo_launch_swann_module(main_leonie, app_cluster->id, host_description);
    }
  else
    {
      /* Cluster is local */
      /*
       *  TODO:
       *  - recuperer les noeuds concernes
       *  - recuperer les canaux requis
       *  = voir comment integrer les infos de canaux dans madeleine
       *  - lancer la session madeleine
       *  - transmettre les donnees de topologie 
       *    . soit par reseau TCP
       *    . soit par ligne de commande, mais le reseau est probablement
       *      mieux en vue de la suite
       *  - executer la session.
       */

      p_tbx_list_t       host_list        = app_cluster->hosts;
      p_tbx_list_t       channel_list     = app_cluster->channels;
      p_leo_mad_module_t mad_module       = NULL;

      DISP("Launching mad module");
      mad_module = leo_launch_mad_module(NULL, app_cluster);
    }

  /* Rajouter un type pour designer une session madeleine */

  LOG_OUT();
}

/*
 * Initialization
 * --------------
 */
int
main (int argc, char *argv[])
{
  p_leo_app_application_t  application       = NULL;
  p_leo_clu_cluster_file_t local_cluster_def = NULL;
  char                    *filename          = NULL;

  LOG_IN();
  tbx_init();
  ntbx_init();
  main_leonie = leo_init();
  leo_parser_init();
 
  if (argc < 2)
    FAILURE("no application description file specified");

  application = leo_parse_application_file(argv[1]);
  DISP("Starting application %s", application->id);

  main_leonie->net_server = leo_net_server_init();

  /*
   * Local cluster information retrieval
   * -----------------------------------
   */
  filename = malloc(MAX_FILENAME_SIZE);
  strcpy(filename, getenv("HOME"));
  strcat(filename, "/.pm2/leo_cluster");
  local_cluster_def = leo_parse_cluster_file(NULL, filename);

  /*
   * Cluster initialization
   * ---------------------
   */
  leo_start(application, local_cluster_def);
  
  //leo_cluster_setup(main_leonie, application, local_cluster_def);

  LOG_OUT();
  return 0;
}
