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
 * leo_pointers.h
 * ---------------
 */

#ifndef LEO_POINTERS_H
#define LEO_POINTERS_H

// .. Enums ............................................................ //
typedef enum   e_leo_loader_priority     *p_leo_loader_priority_t;

// .. Structures ....................................................... //
typedef struct s_leo_process_specific    *p_leo_process_specific_t;
typedef struct s_leo_dir_node            *p_leo_dir_node_t;
typedef struct s_leo_dir_adapter         *p_leo_dir_adapter_t;
typedef struct s_leo_dir_driver_process_specific
                                  *p_leo_dir_driver_process_specific_t;
typedef struct s_leo_dir_driver          *p_leo_dir_driver_t;
typedef struct s_leo_dir_connection
                                  *p_leo_dir_connection_t;
typedef struct s_leo_dir_channel         *p_leo_dir_channel_t;
typedef struct s_leo_dir_fchannel        *p_leo_dir_fchannel_t;
typedef struct s_leo_dir_connection_data
                                  *p_leo_dir_connection_data_t;
typedef struct s_leo_dir_vxchannel        *p_leo_dir_vxchannel_t;
typedef struct s_leo_directory            *p_leo_directory_t;
typedef struct s_leo_networks             *p_leo_networks_t;
typedef struct s_leo_spawn_group          *p_leo_spawn_group_t;
typedef struct s_leo_spawn_groups         *p_leo_spawn_groups_t;
typedef struct s_leo_loader               *p_leo_loader_t;
typedef struct s_leo_settings             *p_leo_settings_t;
typedef struct s_leonie                   *p_leonie_t;

// .. Functions ........................................................ //
typedef void (*p_leo_loader_func_t) (p_leo_settings_t,
				     p_ntbx_server_t,
				     p_tbx_slist_t);

#endif /* LEO_POINTERS_H */
