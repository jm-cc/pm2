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
$Log: ntbx_tcp.c,v $
Revision 1.9  2000/11/08 08:16:16  oaumage
*** empty log message ***

Revision 1.8  2000/05/18 11:36:19  oaumage
- Remplacement des types `tableau' par des types `structure de tableau'

Revision 1.7  2000/04/27 08:59:19  oaumage
- fusion de la branche pm2_mad2_multicluster


Revision 1.6  2000/04/25 14:33:55  vdanjean
Nouveaux Makefiles :-)

Revision 1.5.2.4  2000/04/15 15:23:29  oaumage
- corrections diverses

Revision 1.5.2.3  2000/03/30 14:31:51  oaumage
- correction d'une faute de frappe

Revision 1.5.2.2  2000/03/30 14:29:53  oaumage
- retablissement de l'etat `connecte' en sortie des fonctions de transmission

Revision 1.5.2.1  2000/03/30 14:14:56  oaumage
- initialisation des champs local/remote_host

Revision 1.5  2000/03/08 17:17:00  oaumage
- utilisation de TBX_MALLOC

Revision 1.4  2000/03/07 15:43:48  oaumage
- suppression de l'usage de socketlen_t au profit de int (probleme sur info)

Revision 1.3  2000/03/01 11:03:37  oaumage
- mise a jour des #includes ("")

Revision 1.2  2000/02/17 09:14:40  oaumage
- ajout du support de TCP a la net toolbox

Revision 1.1  2000/02/08 15:25:30  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * tbx_tcp.c
 * ---------
 */
/* #define DEBUG */
#include "tbx.h"
#include "ntbx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

/*
 * Macro and constant definition
 * ========================================================================
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

/*
 * Data Structures
 * ========================================================================
 */
typedef struct
{
  ntbx_tcp_socket_t descriptor;

} ntbx_tcp_client_specific_t, *p_ntbx_tcp_client_specific_t;


typedef struct
{
  ntbx_tcp_socket_t descriptor;
  ntbx_tcp_port_t   port;
} ntbx_tcp_server_specific_t, *p_ntbx_tcp_server_specific_t;


/*
 * Functions
 * ========================================================================
 */

/*
 * General Purpose functions
 * -------------------------
 */
void
ntbx_tcp_retry_struct_init(p_ntbx_tcp_retry_t retry)
{
  LOG_IN();
  retry->count         = 0;
  retry->sleep_count   = 0;
  retry->timeout_count = 0;
  LOG_OUT();
}

ntbx_status_t
ntbx_tcp_retry(p_ntbx_tcp_retry_t retry)
{
  LOG_IN();
  if (retry->count < NTBX_TCP_MAX_RETRY_BEFORE_SLEEP)
    {
      retry->count++;
    }
  else
    {
      if (retry->sleep_count < NTBX_TCP_MAX_SLEEP_RETRY)
	{
	  retry->sleep_count++;
	  retry->count = 0;
	  sleep(NTBX_TCP_SLEEP_DELAY);
	}
      else
	{
	  LOG_OUT();
	  return ntbx_failure;
	}
    }
  
  LOG_OUT();
  return ntbx_success;
}

ntbx_status_t
ntbx_tcp_timeout(p_ntbx_tcp_retry_t retry)
{
  LOG_IN();
  if (retry->timeout_count < NTBX_TCP_MAX_TIMEOUT_RETRY)
    {
      retry->timeout_count ++;
      retry->count       = 0;
      retry->sleep_count = 0;
      sleep(NTBX_TCP_SLEEP_DELAY);
    }
  else
    {
      LOG_OUT();
      return ntbx_failure;
     }
  
  LOG_OUT();
  return ntbx_success;
}


/*
 * Setup functions
 * ---------------
 */
void
ntbx_tcp_address_fill(p_ntbx_tcp_address_t   address, 
		      ntbx_tcp_port_t        port,
		      char                  *host_name)
{
  struct hostent *host_entry;

  LOG_IN();
  if (!(host_entry = gethostbyname(host_name)))
    FAILURE("ERROR: Cannot find host internet address");
      
  address->sin_family = AF_INET;
  address->sin_port   = htons(port);
  memcpy(&address->sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);
  LOG_OUT();
}

void
ntbx_tcp_socket_setup(ntbx_tcp_socket_t desc)
{
  int           val    = 1;
  int           packet = 0x8000;
  struct linger ling   = { 1, 50 };
  int           len    = sizeof(int);
  
  LOG_IN();      
  SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_LINGER, (char *)&ling,
		     sizeof(struct linger)));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_SNDBUF, (char *)&packet, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_RCVBUF, (char *)&packet, len));
  LOG_OUT();
}

void
ntbx_tcp_nb_socket_setup(ntbx_tcp_socket_t desc)
{
  int           val    = 1;
  int           packet = 0x8000;
  struct linger ling   = { 1, 50 };
  int           len    = sizeof(int);
  
  LOG_IN(); 
  SYSCALL(fcntl(desc, F_SETFL, O_NONBLOCK));
  SYSCALL(setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_LINGER, (char *)&ling,
		     sizeof(struct linger)));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_SNDBUF, (char *)&packet, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_RCVBUF, (char *)&packet, len));
  LOG_OUT();
}

ntbx_tcp_socket_t 
ntbx_tcp_socket_create(p_ntbx_tcp_address_t address,
		       ntbx_tcp_port_t      port)
{
  int                len  = sizeof(ntbx_tcp_address_t);
  ntbx_tcp_address_t temp;
  int                desc;

  LOG_IN();
  SYSCALL(desc = socket(AF_INET, SOCK_STREAM, 0));
  
  temp.sin_family      = AF_INET;
  temp.sin_addr.s_addr = htonl(INADDR_ANY);
  temp.sin_port        = htons(port);

  SYSCALL(bind(desc, (struct sockaddr *)&temp, len));

  if (address)
    SYSCALL(getsockname(desc, (struct sockaddr *)address, &len));

  LOG_OUT();
  return desc;
}

/*
 * Communication functions
 * -----------------------
 */


/*...Initialisation.....................*/

/* Setup a server socket */
void
ntbx_tcp_server_init(p_ntbx_server_t server)
{
  p_ntbx_tcp_server_specific_t server_specific = NULL;
  ntbx_tcp_address_t           address;
  
  LOG_IN();
  if (server->state)
    FAILURE("server already initialized");

  server->local_host = malloc(MAXHOSTNAMELEN + 1);
  CTRL_ALLOC(server->local_host);
  gethostname(server->local_host, MAXHOSTNAMELEN);

  server_specific = TBX_MALLOC(sizeof(ntbx_tcp_server_specific_t));
  CTRL_ALLOC(server_specific);
  server->specific = server_specific;
  
  server_specific->descriptor = ntbx_tcp_socket_create(&address, 0);
  SYSCALL(listen(server_specific->descriptor, min(5, SOMAXCONN)));
  ntbx_tcp_nb_socket_setup(server_specific->descriptor);
  
  server_specific->port = (ntbx_tcp_port_t)ntohs(address.sin_port);
  sprintf(server->connection_data.data, "%d", server_specific->port);
  
  server->state = ntbx_server_state_initialized;
  LOG_OUT();
}


/* Setup a client socket */
void
ntbx_tcp_client_init(p_ntbx_client_t client)
{
  p_ntbx_tcp_client_specific_t client_specific = NULL;
  ntbx_tcp_address_t           address;
  
  LOG_IN();
  if (client->state)
    FAILURE("client already initialized");

  client->local_host = malloc(MAXHOSTNAMELEN + 1);
  CTRL_ALLOC(client->local_host);
  gethostname(client->local_host, MAXHOSTNAMELEN);
  client->remote_host = NULL;

  client_specific = TBX_MALLOC(sizeof(ntbx_tcp_client_specific_t));
  CTRL_ALLOC(client_specific);
  client->specific = client_specific;  
  
  client_specific->descriptor = ntbx_tcp_socket_create(&address, 0);
  ntbx_tcp_nb_socket_setup(client_specific->descriptor);
  client->state = ntbx_client_state_initialized;
  LOG_OUT();
}


/*...Reset..............................*/

/* Reset a client object */
void
ntbx_tcp_client_reset(p_ntbx_client_t client)
{
  LOG_IN();
  if (client->state)
    {
      if (client->specific == NULL)
	FAILURE("invalid client data");
      
      SYSCALL(close(((p_ntbx_tcp_client_specific_t)client->specific)->descriptor));
      TBX_FREE(client->specific);
      client->specific = NULL;
      client->state    = ntbx_client_state_uninitialized;
      free(client->remote_host);
      client->remote_host = NULL;
    }
  else
    FAILURE("invalid client state");
  LOG_OUT();
}


/* Reset a server object */
void
ntbx_tcp_server_reset(p_ntbx_server_t server)
{
  LOG_IN();
  if (server->state)
    {
      if (server->specific == NULL)
	FAILURE("invalid server data");
      
      SYSCALL(close(((p_ntbx_tcp_server_specific_t)server->specific)
		    ->descriptor));
      TBX_FREE(server->specific);
      server->specific = NULL;
      server->state    = ntbx_server_state_uninitialized;
    }
  else
    FAILURE("invalid server state");
  LOG_OUT();
}


/*...Connection.........................*/

/* Connect a client socket to a server */
ntbx_status_t
ntbx_tcp_client_connect(p_ntbx_client_t           client,
			char                     *server_host_name,
			p_ntbx_connection_data_t  server_connection_data)
{
  p_ntbx_tcp_client_specific_t client_specific = client->specific;
  ntbx_tcp_port_t              server_port     =
    atoi((char *)server_connection_data);
  ntbx_tcp_address_t           server_address;
  ntbx_tcp_retry_t             retry;

  LOG_IN();
  if (client->state != ntbx_client_state_initialized)
    FAILURE("invalid client state");

  ntbx_tcp_address_fill(&server_address, server_port, server_host_name);

  ntbx_tcp_retry_struct_init(&retry);

  while(connect(client_specific->descriptor,
		(struct sockaddr *)&server_address, 
		sizeof(ntbx_tcp_address_t)) == -1)
    {
      if (errno == EINPROGRESS)
	continue;
      else if (errno == EINTR)
	{
	  if (ntbx_tcp_retry(&retry) != ntbx_success)
	    {
	      ntbx_tcp_client_reset(client);
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else if (errno == EALREADY)
	{
	  if (ntbx_tcp_retry(&retry) != ntbx_success)
	    {
	      ntbx_tcp_client_reset(client);
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else if (errno == EWOULDBLOCK)
	{
	  if (ntbx_tcp_retry(&retry) != ntbx_success)
	    {
	      ntbx_tcp_client_reset(client);
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else if (errno == ETIMEDOUT)
	{
	  if (ntbx_tcp_timeout(&retry) != ntbx_success)
	    {
	      ntbx_tcp_client_reset(client);
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else
	{
	  perror("connect");
	  FAILURE("ntbx_tcp_client_connect failed");
	}
    }

  client->state = ntbx_client_state_connected;
  client->remote_host = malloc(strlen(server_host_name) + 1);
  CTRL_ALLOC(client->remote_host);
  strcpy(client->remote_host, server_host_name);
  LOG_OUT();
  return ntbx_success;
}


/* Accept an incoming client connection request */
ntbx_status_t
ntbx_tcp_server_accept(p_ntbx_server_t server, p_ntbx_client_t client)
{
  p_ntbx_tcp_server_specific_t server_specific = server->specific;
  p_ntbx_tcp_client_specific_t client_specific = NULL;
  ntbx_tcp_socket_t            descriptor;
  ntbx_tcp_retry_t             retry;

  LOG_IN();
  if (client->state != ntbx_client_state_initialized)
    FAILURE("invalid client state");

  ntbx_tcp_retry_struct_init(&retry);

  while ((descriptor = accept(server_specific->descriptor, NULL, NULL)) == -1)
    {
      if (errno == EINTR)
	{
	  if (ntbx_tcp_retry(&retry) != ntbx_success)
	    {
	      client->state = ntbx_client_state_uninitialized;
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else if (errno == EWOULDBLOCK)
	{ 
	  if (ntbx_tcp_retry(&retry) != ntbx_success)
	    {
	      client->state = ntbx_client_state_uninitialized;
	      LOG_OUT();
	      return ntbx_failure;
	    }
	}
      else
	{
	  perror("accept");
	  FAILURE("ntbx_tcp_server_accept");
	}
    }

  client_specific = TBX_MALLOC(sizeof(ntbx_tcp_client_specific_t));
  CTRL_ALLOC(client_specific);
  client->specific = client_specific;  
  
  client_specific->descriptor = descriptor;
  client->state = ntbx_client_state_connected;
  LOG_OUT();
  return ntbx_success;
}


/*...Disconnection......................*/

/* Disconnect a client socket */
void
ntbx_tcp_client_disconnect(p_ntbx_client_t client)
{
  p_ntbx_tcp_client_specific_t client_specific = client->specific;
  
  LOG_IN();
  if (client->state == ntbx_client_state_uninitialized)
    FAILURE("client not initialized");

  if (client->state == ntbx_client_state_shutdown)
    FAILURE("client already shut down");

  if (client->state == ntbx_client_state_closed)
    FAILURE("client already closed");

  if (client->state == ntbx_client_state_data_ready)
    FAILURE("unread data remained while closing client");

  if (   (client->state == ntbx_client_state_connected)
      || (client->state == ntbx_client_state_write_ready)
      || (client->state == ntbx_client_state_peer_closed))
    {
      SYSCALL(shutdown(client_specific->descriptor, 2));
      client->state = ntbx_client_state_shutdown;
    }
  
  if (   (client->state == ntbx_client_state_shutdown)
      || (client->state == ntbx_client_state_initialized))
    {
      SYSCALL(close(client_specific->descriptor));
      client->state = ntbx_client_state_closed;
    }
  else
    FAILURE("invalid client state");

  TBX_FREE(client->specific);
  client_specific  = client->specific = NULL;
  client->state    = ntbx_client_state_uninitialized;
  free(client->remote_host);
  client->remote_host = NULL;

  LOG_OUT();
}


/* Disconnect a server */
void
ntbx_tcp_server_disconnect(p_ntbx_server_t server)
{
  p_ntbx_tcp_server_specific_t server_specific = server->specific;
  
  LOG_IN();
  if (server->state == ntbx_server_state_uninitialized)
    FAILURE("server not initialized");
  
  if (server->state == ntbx_server_state_connection_ready)
    FAILURE("incoming connection not processed by server");

  if (server->state == ntbx_server_state_shutdown)
    FAILURE("server already shut down");

  if (server->state == ntbx_server_state_closed)
    FAILURE("server already closed");

  if (   (server->state == ntbx_server_state_initialized)
      || (server->state == ntbx_server_state_accepting))
    {
      SYSCALL(close(server_specific->descriptor));
      server->state = ntbx_server_state_closed;
    }
  else
    FAILURE("invalid server state");

  TBX_FREE(server->specific);
  server_specific  = server->specific = NULL;
  server->state    = ntbx_server_state_uninitialized;

  LOG_OUT();
}


/*...Polling............................*/

/* data ready to read */
int
ntbx_tcp_read_poll(int              nb_clients,
		   p_ntbx_client_t *client_array)
{  
  fd_set           read_fds;
  int              max_fds = 0;
  int              i;
  ntbx_tcp_retry_t retry;

  LOG_IN();
  ntbx_tcp_retry_struct_init(&retry);

  FD_ZERO(&read_fds);
  for (i = 0; i < nb_clients; i++)
    {
      p_ntbx_tcp_client_specific_t client_specific =
	client_array[i]->specific;
	  
      if (client_array[i]->state != ntbx_client_state_connected)
	FAILURE("invalid client state");

      FD_SET(client_specific->descriptor, &read_fds);
      max_fds = max(max_fds, client_specific->descriptor);
    }
      
  while(tbx_true)
    {
      fd_set         local_read_fds = read_fds;
      struct timeval timeout;
      int            status;

      timeout.tv_sec  = NTBX_TCP_SELECT_TIMEOUT;
      timeout.tv_usec = 0;

      status = select(max_fds + 1,
		      &local_read_fds,
		      NULL,
		      NULL,
		      &timeout);

      if (status == -1)
	{
	  if (errno == EINTR)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  return ntbx_failure;
		}

	      continue;
	    }
	  else
	    {
	      perror("select");
	      FAILURE("ntbx_tcp_read_poll failed");
	    }	  
	}
      else if (status == 0)
	{
	  if (ntbx_tcp_timeout(&retry) != ntbx_success)
	    {
	      return ntbx_failure;
	    }
	  
	  continue;
	}
      else
	{
	  for (i = 0; i < nb_clients; i++)
	    {
	      p_ntbx_client_t              client          =
		client_array[i];
	      p_ntbx_tcp_client_specific_t client_specific =
		client->specific;

	      if (FD_ISSET(client_specific->descriptor, &local_read_fds))
		{
		  client->state = ntbx_client_state_data_ready;
		}
	    }
	  
	  return status;
	}
    }
  LOG_OUT();
}


/* data ready to write */
int
ntbx_tcp_write_poll(int              nb_clients,
		    p_ntbx_client_t *client_array)
{
  fd_set           write_fds;
  int              max_fds = 0;
  int              i;
  ntbx_tcp_retry_t retry;

  LOG_IN();
  ntbx_tcp_retry_struct_init(&retry);

  FD_ZERO(&write_fds);
  for (i = 0; i < nb_clients; i++)
    {
      p_ntbx_tcp_client_specific_t client_specific =
	client_array[i]->specific;
	  
      if (client_array[i]->state != ntbx_client_state_connected)
	FAILURE("invalid client state");

      FD_SET(client_specific->descriptor, &write_fds);
      max_fds = max(max_fds, client_specific->descriptor);
    }
      
  while(tbx_true)
    {
      fd_set         local_write_fds = write_fds;
      struct timeval timeout;
      int            status;

      timeout.tv_sec  = NTBX_TCP_SELECT_TIMEOUT;
      timeout.tv_usec = 0;

      status = select(max_fds + 1,
		      NULL,
		      &local_write_fds,
		      NULL,
		      &timeout);

      if (status == -1)
	{
	  if (errno == EINTR)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  LOG_OUT();
		  return ntbx_failure;
		}

	      continue;
	    }
	  else
	    {
	      perror("select");
	      FAILURE("ntbx_tcp_write_poll failed");
	    }
	}
      else if (status == 0)
	{
	  if (ntbx_tcp_timeout(&retry) != ntbx_success)
	    {
	      LOG_OUT();
	      return ntbx_failure;
	    }
	  
	  continue;
	}
      else
	{
	  for (i = 0; i < nb_clients; i++)
	    {
	      p_ntbx_client_t              client          =
		client_array[i];
	      p_ntbx_tcp_client_specific_t client_specific =
		client->specific;

	      if (FD_ISSET(client_specific->descriptor, &local_write_fds))
		{
		  client->state = ntbx_client_state_write_ready;
		}
	    }
	  
	  LOG_OUT();
	  return status;
	}
    }
  LOG_OUT();
}


/*...Read/Write...(block)...............*/

/* read a block */
int
ntbx_tcp_read_block(p_ntbx_client_t  client,
		    void            *ptr,
		    size_t           length)
{
  p_ntbx_tcp_client_specific_t client_specific = client->specific;
  size_t                       bytes_read      = 0;
  ntbx_tcp_retry_t             retry;

  LOG_IN();
  if (client->state != ntbx_client_state_data_ready)
    FAILURE("invalid client state");
  ntbx_tcp_retry_struct_init(&retry);

  while (bytes_read < length)
    {
      int status;

      status = read(client_specific->descriptor,
		    ptr + bytes_read,
		    length - bytes_read);
      
      if (status == -1)
	{
	  if (errno == EAGAIN)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  ntbx_tcp_client_reset(client);
		  LOG_OUT();
		  return ntbx_failure;
		}
	      
	      continue;
	    }
	  else if (errno == EINTR)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  ntbx_tcp_client_reset(client);
		  LOG_OUT();
		  return ntbx_failure;
		}
	      
	      continue;	      
	    }
	  else
	    {
	      perror("read");
	      FAILURE("ntbx_tcp_read_block");
	    }
	}
      else if (status == 0)
	{
	  client->state = ntbx_client_state_peer_closed;
	  return ntbx_failure;
	}
      else
	{
	  ntbx_tcp_retry_struct_init(&retry);
	  bytes_read += status;
	} 
    }
  client->state = ntbx_client_state_connected;
  LOG_OUT();
  return ntbx_success;
}


/* write a block */
int
ntbx_tcp_write_block(p_ntbx_client_t  client,
		     void            *ptr,
		     size_t           length)
{
  p_ntbx_tcp_client_specific_t client_specific = client->specific;
  size_t                       bytes_written   = 0;
  ntbx_tcp_retry_t             retry;

  LOG_IN();
  if (client->state != ntbx_client_state_write_ready)
    FAILURE("invalid client state");

  ntbx_tcp_retry_struct_init(&retry);

  while (bytes_written < length)
    {
      int status;

      status = write(client_specific->descriptor,
		    ptr + bytes_written,
		    length - bytes_written);
      
      if (status == -1)
	{
	  if (errno == EAGAIN)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  ntbx_tcp_client_reset(client);
		  return ntbx_failure;
		}
	      
	      continue;
	    }
	  else if (errno == EINTR)
	    {
	      if (ntbx_tcp_retry(&retry) != ntbx_success)
		{
		  ntbx_tcp_client_reset(client);
		  return ntbx_failure;
		}
	      
	      continue;	      
	    }
	  else
	    {
	      perror("write");
	      FAILURE("ntbx_tcp_write_block");
	    }
	}
      else if (status == 0)
	{
	  client->state = ntbx_client_state_peer_closed;
	  return ntbx_failure;
	}
      else
	{
	  ntbx_tcp_retry_struct_init(&retry);
	  bytes_written += status;
	} 
    }
  client->state = ntbx_client_state_connected;
  LOG_OUT();
  return ntbx_success;
}


/*...Read/Write...(pack.buffer).........*/

/* read a pack buffer */
int
ntbx_tcp_read_pack_buffer(p_ntbx_client_t      client,
			  p_ntbx_pack_buffer_t pack_buffer)
{
  LOG_IN();
  LOG_OUT();
  return ntbx_tcp_read_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
}


/* write a pack buffer */
int
ntbx_tcp_write_pack_buffer(p_ntbx_client_t      client,
			   p_ntbx_pack_buffer_t pack_buffer)
{
  LOG_IN();
  LOG_OUT();
  return ntbx_tcp_write_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
}

/* ...Read/Write services ..............*/

/* read a block */
void
ntbx_tcp_read(int     socket,
	      void   *ptr,
	      size_t  length)
{
  size_t bytes_read = 0;

  LOG_IN();
  while (bytes_read < length)
    {
      int status;

      status = read(socket, ptr + bytes_read, length - bytes_read);
      
      if (status == -1)
	{
	  if (errno == EAGAIN)
	    {
	      continue;
	    }
	  else if (errno == EINTR)
	    {
	      continue;	      
	    }
	  else
	    ERROR("read");
	}
      else if (status == 0)
	FAILURE("connection closed");
      else
	{
	  bytes_read += status;
	} 
    }
  LOG_OUT();
}


/* write a block */
void
ntbx_tcp_write(int     socket,
	       void   *ptr,
	       size_t  length)
{
  size_t bytes_written = 0;

  LOG_IN();
  while (bytes_written < length)
    {
      int status;

      status = write(socket, ptr + bytes_written, length - bytes_written);
      
      if (status == -1)
	{
	  if (errno == EAGAIN)
	    {
	      continue;
	    }
	  else if (errno == EINTR)
	    {
	      continue;	      
	    }
	  else
	    ERROR("write");
	}
      else if (status == 0)
	FAILURE("connection closed");
      else
	{
	  bytes_written += status;
	} 
    }
  LOG_OUT();
}
