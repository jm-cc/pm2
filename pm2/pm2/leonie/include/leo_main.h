/*
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
