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
Revision 1.13  2000/12/11 08:31:15  oaumage
- support Leonie

Revision 1.12  2000/06/09 12:40:37  oaumage
- Progression du code

Revision 1.11  2000/06/09 08:45:59  oaumage
- Progression du code

Revision 1.10  2000/06/05 11:37:11  oaumage
- Progression du code

Revision 1.9  2000/05/31 14:17:19  oaumage
- Leonie

Revision 1.8  2000/05/23 15:33:14  oaumage
- extension de la partie analyse de la configuration

Revision 1.7  2000/05/18 11:34:18  oaumage
- remplacement des types `tableau' par des types `structure de tableau'
  par securite

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
#include "leoparse.h"
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
*/
/*
 * List building
 * -------------
 */
/*
void
leo_build_cluster_host_model_list(p_leo_cluster_definition_t cluster,
				  p_leo_clu_cluster_file_t   cluster_file)
{
  tbx_list_reference_t ref;

  LOG_IN();
  tbx_list_reference_init(&ref, cluster_file->host_model_list);

  do
    {
      p_leo_clu_host_model_t n_host_model = NULL;
      p_leo_clu_host_model_t host_model   =
	tbx_get_list_reference_object(&ref);

      n_host_model = malloc(sizeof(leo_clu_host_model_t));
      CTRL_ALLOC(n_host_model);

      *n_host_model = *host_model;

      tbx_slist_append_tail(cluster->host_model_list, n_host_model);
    }
  while(tbx_forward_list_reference(&ref));
  LOG_OUT();
}
*/
/*
void
leo_build_cluster_host_list(p_leo_cluster_definition_t cluster,
			    p_leo_clu_cluster_file_t   cluster_file)
{
  tbx_list_reference_t ref;

  LOG_IN();
  tbx_list_reference_init(&ref, cluster_file->cluster->hosts);

  do
    {
      p_leo_clu_host_name_t n_host_name = NULL;
      p_leo_clu_host_name_t host_name   =
	tbx_get_list_reference_object(&ref);

      n_host_name = malloc(sizeof(leo_clu_host_name_t));
      CTRL_ALLOC(n_host_name);

      *n_host_name = *host_name;

      tbx_slist_append_tail(cluster->host_list, n_host_name);
    }
  while(tbx_forward_list_reference(&ref));
  LOG_OUT();
}
*/
/*
p_leo_cluster_definition_t
leo_build_cluster_definition(p_leo_clu_cluster_file_t cluster_file)
{

  p_leo_cluster_definition_t cluster = NULL;

  LOG_IN();
  cluster = malloc(sizeof(leo_cluster_definition_t));
  CTRL_ALLOC(cluster);

  cluster->id = malloc(strlen(cluster_file->cluster->id) + 1);
  CTRL_ALLOC(cluster->id);
  strcpy(cluster->id, cluster_file->cluster->id);

  cluster->host_model_list = malloc(sizeof(tbx_slist_t));
  CTRL_ALLOC(cluster->host_model_list);
  tbx_slist_init(cluster->host_model_list);
      
  cluster->host_list = malloc(sizeof(tbx_slist_t));
  CTRL_ALLOC(cluster->host_list);
  tbx_slist_init(cluster->host_list);
      
  leo_build_cluster_host_model_list(cluster, cluster_file);
  leo_build_cluster_host_list(cluster, cluster_file);

  cluster->mad_module   = NULL;
  cluster->swann_module = NULL;

  LOG_OUT();
  return cluster;
}
*/

/*---*/
/*
void
leo_build_host_list(p_leo_app_cluster_t app_cluster,
		    p_tbx_slist_t       host_list)
{
  tbx_list_reference_t h_ref;

  LOG_IN();
  tbx_list_reference_init(&h_ref, app_cluster->hosts);

  do
    {
      char *host      = tbx_get_list_reference_object(&h_ref);
      char *host_name = NULL;

      host_name = malloc(strlen(host) + 1);
      CTRL_ALLOC(host_name);
      strcpy(host_name, host);

      tbx_slist_append_tail(host_list, host_name);
    }
  while(tbx_forward_list_reference(&h_ref));
  LOG_OUT();
}
*/
/*
void
leo_build_channel_list(p_leo_app_cluster_t app_cluster,
		       p_tbx_slist_t       channel_list)
{
  tbx_list_reference_t c_ref;

  LOG_IN();
  tbx_list_reference_init(&c_ref, app_cluster->channels);

  do
    {
      p_leo_app_channel_t channel   =
	tbx_get_list_reference_object(&c_ref);
      p_leo_app_channel_t n_channel = NULL;

      n_channel = malloc(sizeof(leo_app_channel_t));
      CTRL_ALLOC(n_channel);
      *n_channel = *channel;

      tbx_slist_append_tail(channel_list, n_channel);
    }
  while(tbx_forward_list_reference(&c_ref));
  LOG_OUT();
}
*/
/*
void
leo_build_application_cluster_list(p_leo_app_application_t  application,
				   p_tbx_slist_t application_cluster_list)
{
  tbx_list_reference_t ref;

  LOG_IN();
  tbx_list_reference_init(&ref, application->cluster_list);

  do
    {
      p_leo_app_cluster_t         app_cluster =
	tbx_get_list_reference_object(&ref);
      p_leo_application_cluster_t cluster     = NULL;

      cluster = malloc(sizeof(leo_application_cluster_t));
      CTRL_ALLOC(cluster);

      cluster->id = malloc(strlen(app_cluster->id) + 1);
      CTRL_ALLOC(cluster->id);
      strcpy(cluster->id, app_cluster->id);
      LOG_STR("app_clu id", app_cluster->id);
      LOG_STR("cluster id", cluster->id);

      cluster->executable = malloc(strlen(app_cluster->executable) + 1);
      CTRL_ALLOC(cluster->executable);
      strcpy(cluster->executable, app_cluster->executable);
      LOG_STR("cluster exec", cluster->executable);

      cluster->path = malloc(strlen(app_cluster->path) + 1);
      CTRL_ALLOC(cluster->path);
      strcpy(cluster->path, app_cluster->path);
      LOG_STR("cluster path", cluster->path);

      cluster->host_list = malloc(sizeof(tbx_slist_t));
      CTRL_ALLOC(cluster->host_list);
      tbx_slist_init(cluster->host_list);

      cluster->channel_list = malloc(sizeof(tbx_slist_t));
      CTRL_ALLOC(cluster->channel_list);
      tbx_slist_init(cluster->channel_list);
	
      leo_build_host_list(app_cluster, cluster->host_list);
      leo_build_channel_list(app_cluster, cluster->channel_list);
      
      LOG_STR("cluster id", cluster->id);
      tbx_slist_append_tail(application_cluster_list, cluster);
    }
  while (tbx_forward_list_reference(&ref));
  LOG_OUT();
}
*/

/*
 * Search functions
 * ----------------
 */
/*
tbx_bool_t
leo_search_for_host_name(void *ref_obj, void *obj)
{
  tbx_list_reference_t   ref;
  char                  *ref_name = ref_obj;
  p_leo_clu_host_name_t  host     = obj;
  
  LOG_IN();
  tbx_list_reference_init(&ref, host->name_list);
  LOG_STR("ref_name", ref_name);
  
  do
    {
      char *name = tbx_get_list_reference_object(&ref);

      if (!strcmp(ref_name, name))
	{
	  LOG_OUT();
	  return tbx_true;
	}
    }
  while(tbx_forward_list_reference(&ref));

  LOG_OUT();
  return tbx_false;
}
*/
/*
tbx_bool_t
leo_search_for_cluster_entry_point(void *ref_obj, void *obj)
{
  p_leo_application_cluster_t app_clu = ref_obj;
  p_leo_cluster_definition_t  clu_def = obj;
  tbx_slist_reference_t       ref;

  LOG_IN();
  LOG_STR("clu_def", clu_def->id);
  LOG_STR("app_clu", app_clu->id);
  tbx_slist_ref_init_head(clu_def->host_list, &ref);
  LOG_OUT();
  return tbx_slist_search(leo_search_for_host_name, app_clu->id, &ref);
}
*/
/*
tbx_bool_t
leo_search_for_cluster(void *ref_obj, void *obj)
{
  p_leo_cluster_definition_t  clu_def = ref_obj;
  p_leo_application_cluster_t app_clu = obj;

  LOG_IN();
  LOG_STR("clu_def id", clu_def->id);
  LOG_STR("app_clu id", app_clu->id);
  LOG_OUT();
  return strcmp(clu_def->id, app_clu->id) == 0;
}
*/
/*
 * Initialization
 * --------------
 */
/*
int
old_main (int argc, char *argv[])
{
  p_leo_app_application_t    application              = NULL;
  p_leo_clu_cluster_file_t   local_cluster_def        = NULL;
  char                      *filename                 = NULL;
  p_tbx_slist_t              application_cluster_list = NULL;
  p_tbx_slist_t              cluster_definition_list  = NULL;
  p_leo_cluster_definition_t local                    = NULL;
    

  LOG_IN();
  tbx_init(&argc, argv, PM2DEBUG_DO_OPT);
  ntbx_init();
  main_leonie = leo_init();
  leo_parser_init();

  application_cluster_list = malloc(sizeof(tbx_slist_t));
  CTRL_ALLOC(application_cluster_list);
  tbx_slist_init(application_cluster_list);

  if (argc < 2)
    FAILURE("no application description file specified");

  application = leo_parse_application_file(argv[1]);
  DISP("Starting application %s", application->id);
  
  leo_build_application_cluster_list(application, application_cluster_list);

  cluster_definition_list = malloc(sizeof(tbx_slist_t));
  CTRL_ALLOC(cluster_definition_list);
  tbx_slist_init(cluster_definition_list);


  main_leonie->net_server = leo_net_server_init();

  filename = malloc(MAX_FILENAME_SIZE);
  strcpy(filename, getenv("HOME"));
  strcat(filename, "/.pm2/leo_cluster");
  local_cluster_def = leo_parse_cluster_file(NULL, filename);
  
  {
    // Cluster list construction
    // Local Cluster
    local = leo_build_cluster_definition(local_cluster_def);
    tbx_slist_append_tail(cluster_definition_list, local);

    if (application_cluster_list->length)
      // Remote Clusters 
      {
	tbx_slist_t           temp_list;
	tbx_slist_reference_t app_ref;
	tbx_slist_reference_t clu_ref;

	tbx_slist_init(&temp_list);
	tbx_slist_dup(&temp_list, application_cluster_list);
      
	tbx_slist_ref_init_head(cluster_definition_list, &clu_ref);

	tbx_slist_ref_init_head(&temp_list, &app_ref);
	if (tbx_slist_search(leo_search_for_cluster, local, &app_ref))
	  {
	    tbx_slist_remove(&app_ref);	
	  }
	else
	  {
	    DISP("Local cluster is not among application requested clusters");
	  }
	
	while (temp_list.length)
	  {
	    tbx_bool_t iteration_status = tbx_false;
	    int        length           = temp_list.length;
	    
	    while (length--)
	      {
		p_leo_application_cluster_t app_clu = NULL;

		app_clu = tbx_slist_extract_head(&temp_list);
		LOG_STR("App_clu", app_clu->id);

		if (tbx_slist_search(leo_search_for_cluster_entry_point,
				     app_clu,
				     &clu_ref))
		  {
		    p_leo_cluster_definition_t clu_def =
		      tbx_slist_get(&clu_ref);

		    DISP("Found %s cluster entry point on %% cluster",
			 app_clu->id,
			 clu_def->id);
		    
		    iteration_status = tbx_true;
		    // Remote cluster connection

		    if (clu_def == local)
		      {
			// Ajouter le module a la liste des modules
			p_leo_swann_module_t module = NULL;
			
			module =
			  leo_launch_swann_module(main_leonie, app_clu);
			tbx_slist_append_tail(cluster_definition_list,
					      module->clu_def);
			
			tbx_append_list(&main_leonie->swann_modules, module);
		      }
		    else
		      {
			DISP("Cluster not connected");
		      }
		  }
		else
		  {
		    tbx_slist_append_tail(&temp_list, app_clu);
		  }
	      }
	    if (!iteration_status)
	      FAILURE("Some clusters are unreachable");
	  }
      } 
  }

  DISP("All clusters found");

  {
    tbx_slist_t           temp_list;
    tbx_slist_reference_t app_ref;
    
    tbx_slist_init(&temp_list);
    tbx_slist_dup(&temp_list, application_cluster_list);    
    tbx_slist_ref_init_head(&temp_list, &app_ref);
    
    while (temp_list.length)
      {
	p_leo_application_cluster_t app_clu = NULL;

	app_clu = tbx_slist_extract_head(&temp_list);
	LOG_STR("Launching Madeleine II on ", app_clu->id);
	
	if (!strcmp(app_clu->id, local->id))
	  {
	    p_leo_mad_module_t module = NULL;
	    
	    DISP("Cluster is local");
	    module = leo_launch_mad_module(main_leonie, app_clu);
	    tbx_append_list(&main_leonie->mad_modules, module);
	    LOG_STR("MadII session started on ", app_clu->id);
	  }
	else
	  {
	    DISP("Cluster is remote");
	  }
	
      }
	
  }
  


  //leo_start(application, local_cluster_def);
  
  //leo_cluster_setup(main_leonie, application, local_cluster_def);

  LOG_OUT();
  return 0;
}
*/

int
main (int argc, char *argv[])
{
}
