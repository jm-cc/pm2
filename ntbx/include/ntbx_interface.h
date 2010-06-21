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
 * ntbx_interface.h
 * ================
 */

#ifndef NTBX_INTERFACE_H
#define NTBX_INTERFACE_H

/*
 * Main interface
 * --------------
 */
void
ntbx_init(int   *argc,
	  char **argv);

void
ntbx_purge_cmd_line(int   *argc,
		    char **argv);

void
ntbx_exit(void);

/*
 * Ntbx - constructors
 * -------------------
 */
p_ntbx_client_t
ntbx_client_cons(void);

p_ntbx_server_t
ntbx_server_cons(void);

p_ntbx_process_info_t
ntbx_process_info_cons(void);

p_ntbx_process_container_t
ntbx_pc_cons(void);

p_ntbx_process_t
ntbx_process_cons(void);

p_ntbx_topology_element_t
ntbx_topology_element_cons(void);

p_ntbx_topology_table_t
ntbx_topology_table_cons(void);

/*
 * Ntbx - destructors
 * ------------------
 */
void
ntbx_client_dest_ext(p_ntbx_client_t            object,
		     p_tbx_specific_dest_func_t dest_func);

void
ntbx_client_dest(p_ntbx_client_t object);

void
ntbx_server_dest_ext(p_ntbx_server_t            object,
		     p_tbx_specific_dest_func_t dest_func);

void
ntbx_server_dest(p_ntbx_server_t object);

void
ntbx_process_info_dest(p_ntbx_process_info_t      object,
		       p_tbx_specific_dest_func_t dest_func);

void
ntbx_pc_dest(p_ntbx_process_container_t object,
	     p_tbx_specific_dest_func_t dest_func);

void
ntbx_process_dest(p_ntbx_process_t           object,
		  p_tbx_specific_dest_func_t dest_func);

void
ntbx_topology_element_dest(p_ntbx_topology_element_t  object,
			   p_tbx_specific_dest_func_t dest_func);

void
ntbx_topology_table_dest(p_ntbx_topology_table_t    object,
			 p_tbx_specific_dest_func_t dest_func);


/*
 * Ntbx - misc
 * -----------
 */
void
ntbx_misc_init(void);

void
ntbx_pc_add(p_ntbx_process_container_t  pc,
	    p_ntbx_process_t            process,
	    ntbx_process_lrank_t        local_rank,
	    void                       *object,
	    const char                 *name,
	    void                       *specific);

ntbx_process_lrank_t
ntbx_pc_local_max(p_ntbx_process_container_t pc);

ntbx_process_grank_t
ntbx_pc_local_to_global(p_ntbx_process_container_t pc,
			ntbx_process_lrank_t       local_rank);

p_ntbx_process_t
ntbx_pc_get_local_process(p_ntbx_process_container_t pc,
			  ntbx_process_grank_t       local_rank);

p_ntbx_process_info_t
ntbx_pc_get_local(p_ntbx_process_container_t pc,
		  ntbx_process_lrank_t       local_rank);

void *
ntbx_pc_get_local_specific(p_ntbx_process_container_t pc,
			   ntbx_process_lrank_t       local_rank);

tbx_bool_t
ntbx_pc_first_local_rank(p_ntbx_process_container_t pc,
			 p_ntbx_process_lrank_t     local_rank);

tbx_bool_t
ntbx_pc_next_local_rank(p_ntbx_process_container_t pc,
			p_ntbx_process_lrank_t     local_rank);

ntbx_process_grank_t
ntbx_pc_global_max(p_ntbx_process_container_t pc);

ntbx_process_lrank_t
ntbx_pc_global_to_local(p_ntbx_process_container_t pc,
			ntbx_process_grank_t       global_rank);

p_ntbx_process_t
ntbx_pc_get_global_process(p_ntbx_process_container_t pc,
			   ntbx_process_grank_t       global_rank);

p_ntbx_process_info_t
ntbx_pc_get_global(p_ntbx_process_container_t pc,
		   ntbx_process_grank_t       global_rank);

void *
ntbx_pc_get_global_specific(p_ntbx_process_container_t pc,
			    ntbx_process_grank_t       global_rank);

tbx_bool_t
ntbx_pc_first_global_rank(p_ntbx_process_container_t pc,
			  p_ntbx_process_grank_t     global_rank);

tbx_bool_t
ntbx_pc_next_global_rank(p_ntbx_process_container_t pc,
			 p_ntbx_process_grank_t     global_rank);

p_ntbx_topology_table_t
ntbx_topology_table_init(p_ntbx_process_container_t  pc,
			 ntbx_topology_t             topology,
			 void                       *parameter);

void
ntbx_topology_table_set(p_ntbx_topology_table_t ttable,
			ntbx_process_lrank_t    src,
			ntbx_process_lrank_t    dst);

void
ntbx_topology_table_clear(p_ntbx_topology_table_t ttable,
			  ntbx_process_lrank_t    src,
			  ntbx_process_lrank_t    dst);

p_ntbx_topology_element_t
ntbx_topology_table_get(p_ntbx_topology_table_t ttable,
			ntbx_process_lrank_t    src,
			ntbx_process_lrank_t    dst);

void
ntbx_topology_table_exit(p_ntbx_topology_table_t ttable);

char *
ntbx_true_name(char *host_name);


/*
 * Pack/Unpack
 * -----------
 */
void
ntbx_pack_int(int                  data,
	      p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_unpack_int(p_ntbx_pack_buffer_t pack_buffer);

void
ntbx_pack_long(long                 data,
	       p_ntbx_pack_buffer_t pack_buffer);

long
ntbx_unpack_long(p_ntbx_pack_buffer_t pack_buffer);

void
ntbx_pack_unsigned_int(unsigned int         data,
		       p_ntbx_pack_buffer_t pack_buffer);

unsigned int
ntbx_unpack_unsigned_int(p_ntbx_pack_buffer_t pack_buffer);

void
ntbx_pack_unsigned_long(unsigned long        data,
			p_ntbx_pack_buffer_t pack_buffer);

unsigned long
ntbx_unpack_unsigned_long(p_ntbx_pack_buffer_t pack_buffer);

void
ntbx_pack_double(double               data,
		 p_ntbx_pack_buffer_t pack_buffer);

double
ntbx_unpack_double(p_ntbx_pack_buffer_t pack_buffer);


/*
 * TCP support
 * -----------
 */
#ifdef NTBX_TCP
void
ntbx_tcp_retry_struct_init(p_ntbx_tcp_retry_t retry);

ntbx_status_t
ntbx_tcp_retry(p_ntbx_tcp_retry_t retry);

ntbx_status_t
ntbx_tcp_timeout(p_ntbx_tcp_retry_t retry);


void
ntbx_tcp_address_fill(p_ntbx_tcp_address_t  address,
		      ntbx_tcp_port_t       port,
		      char                 *host_name);
void
ntbx_tcp_address_fill_ip(p_ntbx_tcp_address_t  address,
			 ntbx_tcp_port_t       port,
			 unsigned long        *ip);

void
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc);

void
ntbx_tcp_nb_socket_setup(ntbx_tcp_socket_t desc);

void
ntbx_tcp_socket_setup_delay(ntbx_tcp_socket_t desc);

void
ntbx_tcp_socket_setup_nodelay(ntbx_tcp_socket_t desc);

void
ntbx_tcp_client_setup_delay(p_ntbx_client_t  client);

void
ntbx_tcp_client_setup_nodelay(p_ntbx_client_t  client);

ntbx_tcp_socket_t
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port);

void
ntbx_tcp_server_init(p_ntbx_server_t server);

void
ntbx_tcp_server_init_ext(p_ntbx_server_t     server,
                         unsigned short int  port);

void
ntbx_tcp_client_init(p_ntbx_client_t client);

void
ntbx_tcp_client_reset(p_ntbx_client_t client);

void
ntbx_tcp_server_reset(p_ntbx_server_t server);

ntbx_status_t
ntbx_tcp_client_connect_ip(p_ntbx_client_t           client,
			   unsigned long             server_ip,
			   p_ntbx_connection_data_t  server_connection_data);
ntbx_status_t
ntbx_tcp_client_connect(p_ntbx_client_t           client,
			char                     *server_host_name,
			p_ntbx_connection_data_t  server_connection_data);

ntbx_status_t
ntbx_tcp_server_accept(p_ntbx_server_t server, p_ntbx_client_t client);

void
ntbx_tcp_client_disconnect(p_ntbx_client_t client);

void
ntbx_tcp_server_disconnect(p_ntbx_server_t server);

int
ntbx_tcp_read_poll(int              nb_clients,
		   p_ntbx_client_t *client_array);

int
ntbx_tcp_write_poll(int              nb_clients,
		    p_ntbx_client_t *client_array);

int
ntbx_tcp_read_block(p_ntbx_client_t  client,
		    void            *ptr,
		    size_t           length);

int
ntbx_tcp_write_block(p_ntbx_client_t  client,
		     const void      *ptr,
		     const size_t     length);

int
ntbx_tcp_read_pack_buffer(p_ntbx_client_t      client,
			  p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_tcp_write_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_tcp_read_string(p_ntbx_client_t   client,
		      char            **string);

int
ntbx_tcp_write_string(p_ntbx_client_t  client,
		       const char      *string);

void
ntbx_tcp_read(int     socket_fd,
	      void   *ptr,
	      size_t  length);

void
ntbx_tcp_write(int           socket_fd,
	       const void   *ptr,
	       const size_t  length);

ssize_t
ntbx_tcp_read_call(int		 s,
                   void		*p,
                   size_t	 l);

ssize_t
ntbx_tcp_write_call(int		 	 s,
                    const void		*p,
                    const size_t	 l);

#endif /* NTBX_TCP */


#endif /* NTBX_INTERFACE_H */




