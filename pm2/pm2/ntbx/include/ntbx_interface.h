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
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: ntbx_interface.h,v $
Revision 1.4  2000/11/07 17:56:47  oaumage
- ajout de commandes de lecture/ecriture resistantes sur sockets

Revision 1.3  2000/04/27 09:01:29  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.2.2.1  2000/04/03 13:46:00  oaumage
- fichier de commandes `swann'

Revision 1.2  2000/02/17 09:14:26  oaumage
- ajout du support de TCP a la net toolbox
- ajout de fonctions d'empaquetage de donnees numeriques

Revision 1.1  2000/02/08 15:25:21  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
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
ntbx_tcp_read_pack_buffer(p_ntbx_client_t      client,
			  p_ntbx_pack_buffer_t pack_buffer);

int
ntbx_tcp_write_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer);
#endif /* NTBX_TCP */

#endif /* NTBX_INTERFACE_H */
