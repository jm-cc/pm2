/*
 * leo_main.h
 * ---------------
 */

#ifndef __LEO_MAIN_H
#define __LEO_MAIN_H

typedef struct s_leo_swann_module
{
  int                         id;
  p_leo_application_cluster_t app_cluster;
  p_leo_cluster_definition_t  clu_def;
  p_leo_swann_module_t        relay;
  p_ntbx_client_t             net_client;
} leo_swann_module_t;

typedef struct s_leo_mad_module
{
  int                   id;
  char                 *cluster_id;
  p_leo_swann_module_t  relay;
  p_ntbx_client_t       net_client;
  p_leo_cluster_definition_t  clu_def;
  p_leo_application_cluster_t app_cluster;
} leo_mad_module_t;

typedef struct s_leonie
{
  int             cluster_counter;
  tbx_list_t      swann_modules;
  tbx_list_t      mad_modules;
  p_ntbx_server_t net_server;
} leonie_t;

#endif /* __LEO_MAIN_H */
