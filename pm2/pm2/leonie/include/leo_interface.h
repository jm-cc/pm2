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
 * leo_interface.h
 * ---------------
 */

#ifndef LEO_INTERFACE_H
#define LEO_INTERFACE_H

// Objects initialisation
//------------------------/////////////////////////////////////////////////

p_leo_settings_t  
leo_settings_init(void);

p_leo_process_specific_t
leo_process_specific_init(void);

p_ntbx_server_t
leo_net_server_init(void);

p_tbx_htable_t
leo_htable_init(void);

p_leo_directory_t
leo_directory_init(void);

p_leo_dir_adapter_t
leo_dir_adapter_init(void);

p_leo_dir_driver_process_specific_t
leo_dir_driver_process_specific_init(void);

p_leo_dir_node_t
leo_dir_node_init(void);

p_leo_dir_driver_t
leo_dir_driver_init(void);

p_leo_dir_channel_process_specific_t
leo_dir_channel_process_specific_init(void);

p_leo_dir_channel_t
leo_dir_channel_init(void);

p_leo_dir_vchannel_process_routing_table_t
leo_dir_vchannel_process_routing_table_init(void);

p_leo_dir_vchannel_process_specific_t
leo_dir_vchannel_process_specific_init(void);

p_leo_dir_fchannel_t
leo_dir_fchannel_init(void);

p_leo_dir_vchannel_t
leo_dir_vchannel_init(void);

p_leo_networks_t
leo_networks_init(void);

p_leo_spawn_groups_t
leo_spawn_groups_init(void);

p_leo_spawn_group_t
leo_spawn_group_init(void);

p_leo_loader_t
leo_loader_init(void);

p_leonie_t
leonie_init(void);


// Include files processing
//--------------------------///////////////////////////////////////////////
void
process_network_include_file(p_leo_networks_t networks,
			     p_tbx_htable_t   include_htable);

void
include_network_files(p_leo_networks_t networks,
		      p_tbx_slist_t    include_files_slist);


// Loaders
//---------////////////////////////////////////////////////////////////////
p_tbx_htable_t
leo_loaders_register(void);


// Communications
//----------------/////////////////////////////////////////////////////////
void
leo_send_int(p_ntbx_client_t client,
	     const int       data);

int
leo_receive_int(p_ntbx_client_t client);

void
leo_send_unsigned_int(p_ntbx_client_t    client,
		      const unsigned int data);

unsigned int
leo_receive_unsigned_int(p_ntbx_client_t client);

void
leo_send_string(p_ntbx_client_t  client,
		const char      *string);

char *
leo_receive_string(p_ntbx_client_t client);

// File Processing
//-----------------////////////////////////////////////////////////////////
void
process_application(p_leonie_t leonie);

// Processes Spawning
//--------------------/////////////////////////////////////////////////////
void
spawn_processes(p_leonie_t leonie);

void
send_directory(p_leonie_t leonie);

// Session Control
//-----------------////////////////////////////////////////////////////////
void
init_drivers(p_leonie_t leonie);

void
init_channels(p_leonie_t leonie);

void
exit_session(p_leonie_t leonie);

// Clean-up
//----------///////////////////////////////////////////////////////////////
void
dir_vchannel_disconnect(p_leonie_t leonie);

void
directory_exit(p_leonie_t leonie);


// Usage
//-------//////////////////////////////////////////////////////////////////
void 
leo_usage(void) TBX_NORETURN;

void 
leo_help(void) TBX_NORETURN;

void 
leo_terminate(const char *msg) TBX_NORETURN;

void 
leo_error(const char *command) TBX_NORETURN;

void
leonie_failure_cleanup(void);

void
leonie_processes_cleanup(void);

#endif /* LEO_INTERFACE_H */
