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
$Log: leo_config.c,v $
Revision 1.1  2000/05/15 13:51:55  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_config.c
 * ============
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"


/*
 * List processing
 * ---------------
 */
char *
leo_find_string(p_tbx_list_t  list,
		char         *string)
{
  tbx_list_reference_t ref;

  LOG_IN();
  if (tbx_empty_list(list))
    return NULL;
  
  tbx_list_reference_init(&ref, list);
  do
    {
      char *str = tbx_get_list_reference_object(&ref);

      if (!strcmp(string, str))
	{
	  LOG_OUT();
	  return str;
	}
    }
  while(tbx_forward_list_reference(&ref));

  LOG_OUT();
  return NULL;
}

/*
 * Cluster configuration analyser
 * ------------------------------
 */
p_leo_clu_host_name_t
leo_get_cluster_entry_point(p_leo_clu_cluster_t  cluster,
			    char                *cluster_id)
{
  tbx_list_reference_t ref;

  LOG_IN();
  if (tbx_empty_list(cluster->hosts))
    FAILURE("empty cluster");
  
  tbx_list_reference_init(&ref, cluster->hosts);
  do
    {
      p_leo_clu_host_name_t host_description =
	tbx_get_list_reference_object(&ref);

      if (leo_find_string(host_description->name_list, cluster_id))
	{
	  LOG_OUT();
	  return host_description;
	}
    }
  while(tbx_forward_list_reference(&ref));
  
  LOG_OUT();
  return NULL;
}

/*
 * Session management
 * ------------------
 */
void
leo_cluster_setup(p_leonie_t               leonie,
		  p_leo_app_application_t  application,
		  p_leo_clu_cluster_file_t local_cluster_def)
{
  tbx_list_reference_t ref;
  
  LOG_IN();
  if (tbx_empty_list(application->cluster_list))
    FAILURE("empty cluster");
  
  tbx_list_reference_init(&ref, application->cluster_list);
  do
    {
      p_leo_app_cluster_t app_cluster =
	tbx_get_list_reference_object(&ref);

      if (!strcmp(local_cluster_def->cluster->id, app_cluster->id))
	{
	  /* cluster is local */
	  DISP("Cluster %s is local", app_cluster->id);
	}
      else
	{
	  /* cluster is remote */
	  p_leo_clu_host_name_t host_description;

	  DISP("Cluster %s is remote", app_cluster->id);
	  DISP("Looking for %s entry point", app_cluster->id);

	  host_description =
	    leo_get_cluster_entry_point(local_cluster_def->cluster,
					app_cluster->id);
	  if (!host_description)
	    FAILURE("could not find cluster entry point");

	  DISP("Entry point found");

	  leo_launch_swann_module(leonie,
				  app_cluster->id,
				  host_description);
	}
    }
  while(tbx_forward_list_reference(&ref));
  LOG_OUT();
}
