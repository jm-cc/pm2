
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

/*
 * swann_interface.h
 * -----------------
 */

#ifndef __SWANN_INTERFACE_H
#define __SWANN_INTERFACE_H


/*
 * File management
 * ---------------.........................................................
 */
swann_status_t
swann_file_init(p_swann_file_t  file,
		char           *pathname);

swann_status_t
swann_file_open(p_swann_file_t file);

swann_status_t
swann_file_create(p_swann_file_t file);

swann_status_t
swann_file_close(p_swann_file_t file);

swann_status_t
swann_file_destroy(p_swann_file_t file);

size_t
swann_file_read_block(p_swann_file_t  file,
		      void           *ptr,
		      size_t          length);

size_t
swann_file_write_block(p_swann_file_t  file,
		      void            *ptr,
		      size_t           length);

swann_status_t
swann_file_send_block(p_swann_net_client_t client,
		      p_swann_file_t       file);

swann_status_t
swann_file_receive_block(p_swann_net_client_t client,
			 p_swann_file_t       file);

/*
 * Process management
 * ------------------......................................................
 */
swann_status_t
swann_run_command(p_swann_file_t  file,
		  char           *argv[],
		  char           *envp[],
		  int            *return_code);

p_swann_command_t
swann_run_async_command(p_swann_file_t  file,
			char           *argv[],
			char           *envp[],
			p_swann_file_t  file_in,
			p_swann_file_t  file_out,
			p_swann_file_t  file_err);


/*
 * Network management
 * ------------------
 */
p_swann_net_server_t
swann_net_server_init(swann_net_server_id_t     id,
		      char                     *master_host_name,
		      p_ntbx_connection_data_t  connection_data);

void
swann_net_send_int(p_ntbx_client_t client,
		   int             data);

int
swann_net_receive_int(p_ntbx_client_t client);

void
swann_net_send_long(p_ntbx_client_t client,
		    long            data);

long
swann_net_receive_long(p_ntbx_client_t client);

void
swann_net_send_double(p_ntbx_client_t client,
		      double          data);

double
swann_net_receive_double(p_ntbx_client_t client);

void
swann_net_send_string(p_ntbx_client_t  client,
		      char            *data);

char *
swann_net_receive_string(p_ntbx_client_t client);


#endif /* __SWANN_INTERFACE_H */
