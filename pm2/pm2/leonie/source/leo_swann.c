
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

p_leo_swann_module_t
leo_launch_swann_module(p_leonie_t                  leonie,
			p_leo_application_cluster_t cluster)
{
  p_leo_clu_cluster_file_t  remote_cluster_def = NULL;
  p_leo_swann_module_t      module             = NULL;
  char                     *cmd                = NULL;
  ntbx_status_t             status;  
  // int                    n;
  
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
*/
