/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * leo_types.h
 * -----------
 */

#ifndef LEO_TYPES_H
#define LEO_TYPES_H

// Directory
//-----------//////////////////////////////////////////////////////////////
typedef enum e_leo_loader_priority
{
  leo_loader_priority_undefined = -1,
  leo_loader_priority_low       =  0,
  leo_loader_priority_medium    =  1,
  leo_loader_priority_high      =  2,
  leo_loader_priority_count
} leo_loader_priority_t;

typedef struct s_leo_process_specific
{
  p_ntbx_client_t        client;
  char                  *current_loader_name;
  char                  *current_spawn_group_name;
  leo_loader_priority_t  current_loader_priority;
} leo_process_specific_t;

typedef struct s_leo_dir_node
{
  char                       *name;
  p_tbx_htable_t              device_host_names;
  p_tbx_htable_t              channel_host_names;
  unsigned long               ip;  // network form !
  p_ntbx_process_container_t  pc;
} leo_dir_node_t;

typedef struct s_leo_dir_adapter
{
  char         *name;
  char         *selector;
  char         *parameter;
  unsigned int  mtu;
} leo_dir_adapter_t;

typedef struct s_leo_dir_driver_process_specific
{
  p_tbx_htable_t  adapter_htable;
  p_tbx_slist_t   adapter_slist;
  char           *parameter;
} leo_dir_driver_process_specific_t;

typedef struct s_leo_dir_driver
{
  char                       *network_name;
  char                       *device_name;
  p_ntbx_process_container_t  pc;
} leo_dir_driver_t;


typedef struct s_leo_dir_connection
{
  char                       *adapter_name;
  p_ntbx_process_container_t  pc;
  char                       *parameter;
  p_tbx_darray_t              in_connection_parameter_darray;
  p_tbx_darray_t              out_connection_parameter_darray;
} leo_dir_connection_t;

typedef struct s_leo_dir_channel
{
  char                       *name;
  p_ntbx_process_container_t  pc;
  p_leo_dir_driver_t          driver;
  tbx_bool_t                  public;
  p_ntbx_topology_table_t     ttable;
} leo_dir_channel_t;

typedef struct s_leo_dir_fchannel
{
  char                       *name;
  char                       *channel_name;
  p_ntbx_process_container_t  pc;
} leo_dir_fchannel_t;

typedef struct s_leo_dir_connection_data
{
  char                 *channel_name;
  ntbx_process_grank_t  destination_rank;
} leo_dir_connection_data_t;

typedef struct s_leo_dir_vxchannel
{
  char                       *name;
  p_tbx_slist_t               dir_channel_slist;
  p_tbx_slist_t               dir_fchannel_slist;
  p_tbx_slist_t               sub_channel_name_slist;
  p_ntbx_process_container_t  pc;
} leo_dir_vxchannel_t;

typedef struct s_leo_directory
{
  p_tbx_slist_t  process_slist;

  p_tbx_htable_t node_htable;
  p_tbx_slist_t  node_slist;

  p_tbx_htable_t driver_htable;
  p_tbx_slist_t  driver_slist;

  p_tbx_htable_t channel_htable;
  p_tbx_slist_t  channel_slist;

  p_tbx_htable_t fchannel_htable;
  p_tbx_slist_t  fchannel_slist;

  p_tbx_htable_t vchannel_htable;
  p_tbx_slist_t  vchannel_slist;

  p_tbx_htable_t xchannel_htable;
  p_tbx_slist_t  xchannel_slist;
} leo_directory_t;

// Networks
//----------///////////////////////////////////////////////////////////////
typedef struct s_leo_networks
{
  p_tbx_htable_t htable;
  p_tbx_slist_t  slist;
} leo_networks_t;

// Processes groups
//------------------///////////////////////////////////////////////////////
typedef struct s_leo_spawn_group
{
  char          *loader_name;
  p_tbx_slist_t  process_slist;
} leo_spawn_group_t;

typedef struct s_leo_spawn_groups
{
  p_tbx_htable_t htable; // indexed by network name
  p_tbx_slist_t  slist;
} leo_spawn_groups_t;

// Loaders
//---------////////////////////////////////////////////////////////////////
typedef struct s_leo_loader
{
  char                *name;
  p_leo_loader_func_t  loader_func;
} leo_loader_t;

typedef struct s_leo_settings
{
  char              *name;
  char              *flavor;
  char              *leonie_host;
  p_tbx_arguments_t  args;
  char              *config_file;
  char              *env; //GM 
  char              *wd;
  tbx_bool_t         cd_mode;
  p_tbx_slist_t      network_file_slist;
  tbx_bool_t         gdb_mode;
  tbx_bool_t         xterm_mode;
  tbx_bool_t         log_mode;
  tbx_bool_t         pause_mode;
  tbx_bool_t         smp_mode;
  tbx_bool_t         export_mode;
  tbx_bool_t         wait_mode;
  tbx_bool_t         env_mode; //GM
  tbx_bool_t         valgrind_mode;
  tbx_bool_t         numactl_mode;
  char               *numactl_value;
  tbx_bool_t         x11_mode;
} leo_settings_t;

typedef struct s_leonie
{
 // Topology
  p_leo_networks_t     networks;
  p_leo_directory_t    directory;
  p_leo_spawn_groups_t spawn_groups;

  // Net server
  p_ntbx_server_t      net_server;

  // Loaders
  p_tbx_htable_t       loaders;

  // Command line options
  p_leo_settings_t     settings;

  // Application information
  p_tbx_htable_t       application_htable;
  p_tbx_htable_t       application_networks;
} leonie_t;

#endif /* LEO_TYPES_H */
