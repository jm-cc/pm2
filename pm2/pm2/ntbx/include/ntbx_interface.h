
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
ntbx_init(int    argc,
	  char **argv);

void
ntbx_purge_cmd_line(int   *argc,
		    char **argv);

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
ntbx_pack_double(double               data,
		 p_ntbx_pack_buffer_t pack_buffer);

int
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
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc);

void
ntbx_tcp_nb_socket_setup(ntbx_tcp_socket_t desc);

ntbx_tcp_socket_t
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port);

void
ntbx_tcp_server_init(p_ntbx_server_t server);

void
ntbx_tcp_client_init(p_ntbx_client_t client);

void
ntbx_tcp_client_reset(p_ntbx_client_t client);

void
ntbx_tcp_server_reset(p_ntbx_server_t server);

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
		     void            *ptr,
		     size_t           length);

int
ntbx_btcp_read_block(p_ntbx_client_t  client,
		     void            *ptr,
		     size_t           length);

int
ntbx_btcp_write_block(p_ntbx_client_t  client,
		     void            *ptr,
		     size_t           length);

int
ntbx_tcp_read_pack_buffer(p_ntbx_client_t      client,
			  p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_tcp_write_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_btcp_read_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_btcp_write_pack_buffer(p_ntbx_client_t      client,
			    p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_btcp_read_string(p_ntbx_client_t   client,
		      char            **string);

int
ntbx_btcp_write_string(p_ntbx_client_t  client,
		       char            *string);

void
ntbx_tcp_read(int     socket,
	      void   *ptr,
	      size_t  length);

void
ntbx_tcp_write(int     socket,
	       void   *ptr,
	       size_t  length);

#endif /* NTBX_TCP */

#endif /* NTBX_INTERFACE_H */
