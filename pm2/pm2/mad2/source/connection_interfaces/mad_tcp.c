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
$Log: mad_tcp.c,v $
Revision 1.6  2000/01/05 09:44:00  oaumage
- initialisation du nouveau champs `group_mode' dans link_init
- mad_sbp.c: suppression des fonctions vides

Revision 1.5  2000/01/04 16:50:49  oaumage
- mad_mpi.c: premiere version fonctionnelle du driver
- mad_sbp.c: nouvelle correction de la transmission des noms d'hote a
  l'initialisation
- mad_tcp.c: remplacement des appels `exit' par des macros FAILURES

Revision 1.4  2000/01/04 09:18:52  oaumage
- ajout de la commande de log de CVS
- phase d'initialisation `external-spawn' terminee pour mad_mpi.c
- recuperation des noms de machines dans la phase
  d'initialisation `external-spawn' de mad_sbp.c


______________________________________________________________________________
*/

/*
 * Mad_tcp.c
 * =========
 */

#define USE_MARCEL_POLL
/* #define DEBUG */
#include <madeleine.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
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
 * local structures
 * ----------------
 */
typedef struct
{
  int nb_adapter;
} mad_tcp_driver_specific_t, *p_mad_tcp_driver_specific_t;

typedef struct
{
  int connection_socket;
  int connection_port;
  int *remote_connection_port;
} mad_tcp_adapter_specific_t, *p_mad_tcp_adapter_specific_t;

typedef struct
{
  int max_fds;
  fd_set read_fds;
} mad_tcp_channel_specific_t, *p_mad_tcp_channel_specific_t;

typedef struct
{
  int socket;
} mad_tcp_connection_specific_t, *p_mad_tcp_connection_specific_t;

typedef struct
{
} mad_tcp_link_specific_t, *p_mad_tcp_link_specific_t;


/*
 * static functions
 * ----------------
 */
static void
mad_tcp_sync_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                   adapter       = channel->adapter;
  p_mad_driver_t                    driver        = adapter->driver;
  p_mad_configuration_t             configuration =
    &(driver->madeleine->configuration);
  p_mad_connection_t                connection;
  p_mad_tcp_connection_specific_t   connection_specific;
  mad_host_id_t                     i;

  LOG_IN();
  for (i = 0;
       i < configuration->size;
       i++)
    {
      if (i == configuration->local_host_id)
	{
	  /* Receive */
	  mad_host_id_t j;

	  for (j = 0;
	       j < configuration->size;
	       j++)
	    {
	      mad_channel_id_t channel_id;	      
	      
	      if (j == configuration->local_host_id)
		{
		  continue;
		}

	      connection = &(channel->input_connection[j]);
	      connection_specific = connection->specific;

	      read(connection_specific->socket, 
		   &channel_id,
		   sizeof(mad_channel_id_t));
	      
	      if (channel_id != channel->id)
		{
		  FAILURE("wrong channel id");
		}
	      
	      write(connection_specific->socket,
		    &(configuration->local_host_id),
		    sizeof(mad_host_id_t));	      
	    }
	}
      else
	{
	  /* send */
	  mad_host_id_t host_id;

	  connection = &(channel->output_connection[i]);
	  connection_specific = connection->specific;

	  write(connection_specific->socket,
	       &(channel->id),
	       sizeof(mad_channel_id_t));	      
	      
	  read(connection_specific->socket,
	       &(host_id),
	       sizeof(mad_host_id_t));	      
	  
	  if (host_id != i)
	    {
	      FAILURE("wrong host id");
	    }
	}
    }
  LOG_OUT();
}

static void
mad_tcp_fill_sockaddr(struct sockaddr_in  *sockaddr, 
		      p_mad_adapter_t      adapter,
		      mad_host_id_t        rank)
{
  p_mad_tcp_adapter_specific_t   adapter_specific = adapter->specific;
  struct hostent                *host;

  LOG_IN();
  LOG_STR("mad_tcp_fill_sockaddr: host", adapter->
	  driver->
	  madeleine->
	  configuration.host_name[rank]);
  
  host = gethostbyname(adapter->
		       driver->
		       madeleine->
		       configuration.host_name[rank]);
  
  if (host == NULL)
    {
      fprintf(stderr,
	      "ERROR: Cannot find internet address of %s",
	      adapter->
	      driver->
	      madeleine->
	      configuration.host_name[rank]);
      exit(1);
    }
      
  sockaddr->sin_family = AF_INET;
  sockaddr->sin_port   =
    htons(adapter_specific->remote_connection_port[rank]);
  memcpy(&sockaddr->sin_addr.s_addr, host->h_addr, host->h_length);
  LOG_OUT();
}

static void
mad_tcp_setup_socket(int sock)
{
  int             un     = 1;
  int             packet = 0x8000;
  struct linger   ling   = { 1, 50 };
  int             status;

  LOG_IN();      
  status = setsockopt(sock,
		      IPPROTO_TCP,
		      TCP_NODELAY,
		      (char*)&un,
		      sizeof(int));
      
  if (status == -1)
    {
      perror("setsockopt failed");
      exit(1);
    }

  status = setsockopt(sock,
		      SOL_SOCKET,
		      SO_LINGER,
		      (char *)&ling,
		      sizeof(struct linger));
      
  if (status == -1)
    {
      perror("setsockopt failed");
      exit(1);
    }
  
  status = setsockopt(sock,
		      SOL_SOCKET,
		      SO_SNDBUF,
		      (char *)&packet,
		      sizeof(int));
      
  if (status == -1)
    {
      perror("setsockopt failed");
      exit(1);
    }

  status = setsockopt(sock,
		      SOL_SOCKET,
		      SO_RCVBUF,
		      (char *)&packet,
		      sizeof(int));
  if (status == -1)
    {
      perror("setsockopt failed");
      exit(1);
    }
  LOG_OUT();
}

static int creer_socket(int type, int port, struct sockaddr_in *adresse)
{
  struct sockaddr_in temp;
  int status;
  int desc;
  int length = sizeof(struct sockaddr_in);
  /*int packet = 0x8000;
  struct linger ling = { 1, 50 };
  int un = 1;*/

  LOG_IN();
  desc = socket(AF_INET, type, 0);
  if (desc == -1)
    {
      perror("ERROR: Cannot create socket\n");
      return -1;
    }
  
  temp.sin_family      = AF_INET;
  temp.sin_addr.s_addr = htonl(INADDR_ANY);
  temp.sin_port        = htons(port);

  status = bind(desc, (struct sockaddr *)&temp, sizeof(struct sockaddr_in));
  
  if (status == -1)
    {
      perror("ERROR: Cannot bind socket\n");
      close(desc);
      return -1;
    }

  if (adresse != NULL)
    {
      status = getsockname(desc, (struct sockaddr *)adresse, &length);
      
      if (status == -1)
	{
	  perror("ERROR: Cannot get socket name\n");
	  return -1;
	}
    }

  LOG_OUT();
  return desc;
}

static void
mad_tcp_write(int              sock,
	      p_mad_buffer_t   buffer)
{
  ssize_t result;
  
  LOG_IN();
  while (mad_more_data(buffer))
    {
      PM2_LOCK();
      result = write(sock,
		     (const void*)(buffer->buffer +
				   buffer->bytes_read),
		     buffer->bytes_written - buffer->bytes_read);
      PM2_UNLOCK();
      if (result == -1)
	{
	  perror("write");
	  FAILURE("write failed");
	}
      buffer->bytes_read += result;
#ifdef PM2
      if (mad_more_data(buffer))
	{
	  PM2_YIELD();
	}
#endif /* PM2 */
    }
  LOG_OUT();
}

static void
mad_tcp_read(int              sock, 
	     p_mad_buffer_t   buffer)
{  
  LOG_IN();
  while (!mad_buffer_full(buffer))
    {
      ssize_t result;

      PM2_LOCK();
      result = read(sock,
		    (void*)(buffer->buffer +
			    buffer->bytes_written),
		    buffer->length - buffer->bytes_written);
      PM2_UNLOCK();
      if (result == -1)
	{
	  perror("read");
	  FAILURE("read failed");
	}
      buffer->bytes_written += result;
#ifdef PM2
      if (!mad_buffer_full(buffer))
	{
	  PM2_YIELD();
	}
#endif /* PM2 */
    }
  LOG_OUT();
}


/* Registration function */

void
mad_tcp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  interface = &(driver->interface);
  
  driver->connection_type = mad_bidirectional_connection;
  driver->buffer_alignment = 32;
  
  interface->driver_init                = mad_tcp_driver_init;
  interface->adapter_init               = mad_tcp_adapter_init;
  interface->adapter_configuration_init = mad_tcp_adapter_configuration_init;
  interface->channel_init               = mad_tcp_channel_init;
  interface->before_open_channel        = mad_tcp_before_open_channel;
  interface->connection_init            = mad_tcp_connection_init;
  interface->link_init                  = mad_tcp_link_init;
  interface->accept                     = mad_tcp_accept;
  interface->connect                    = mad_tcp_connect;
  interface->after_open_channel         = mad_tcp_after_open_channel;
  interface->before_close_channel       = mad_tcp_before_close_channel;
  interface->disconnect                 = mad_tcp_disconnect;
  interface->after_close_channel        = mad_tcp_after_close_channel;
  interface->link_exit                  = mad_tcp_link_exit;
  interface->connection_exit            = mad_tcp_connection_exit;
  interface->channel_exit               = mad_tcp_channel_exit;
  interface->adapter_exit               = mad_tcp_adapter_exit;
  interface->driver_exit                = mad_tcp_driver_exit;
  interface->choice                     = mad_tcp_choice;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_tcp_new_message;
  interface->receive_message            = mad_tcp_receive_message;
  interface->send_buffer                = mad_tcp_send_buffer;
  interface->receive_buffer             = mad_tcp_receive_buffer;
  interface->send_buffer_group          = mad_tcp_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_tcp_receive_sub_buffer_group;
  interface->external_spawn_init        = NULL;
  interface->configuration_init         = NULL;
  interface->send_adapter_parameter     = NULL;
  interface->receive_adapter_parameter  = NULL;
  LOG_OUT();
}

void
mad_tcp_driver_init(p_mad_driver_t driver)
{
  p_mad_tcp_driver_specific_t driver_specific;

  LOG_IN();
  driver_specific = malloc(sizeof(mad_tcp_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;
  LOG_OUT();
}

void
mad_tcp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver;
  p_mad_tcp_driver_specific_t    driver_specific;
  p_mad_tcp_adapter_specific_t   adapter_specific;
  struct sockaddr_in             address;
  
  LOG_IN();
  driver          = adapter->driver;
  driver_specific = driver->specific;

  adapter_specific  = malloc(sizeof(mad_tcp_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter->specific = adapter_specific;

  if (driver_specific->nb_adapter)
    FAILURE("TCP adapter already initialized");

  if (adapter->name == NULL)
    {
      adapter->name     = malloc(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "TCP%d", driver_specific->nb_adapter);
    }
  else
    FAILURE("Adapter selection currently unimplemented on top of TCP");

  driver_specific->nb_adapter++;  

  adapter_specific->remote_connection_port =
    malloc(driver->madeleine->configuration.size * sizeof(int));
  CTRL_ALLOC(adapter_specific->remote_connection_port);

  adapter_specific->connection_socket =
    creer_socket(SOCK_STREAM, 0, &address);
  
  if (listen(adapter_specific->connection_socket,
	     driver->madeleine->configuration.size - 1) == -1)
    {
      perror("mad_tcp_adapter_init: listen");
      exit(1);
    }
  
  adapter_specific->connection_port = (int)ntohs(address.sin_port);
  LOG_VAL("mad_tcp_adapter_init: connection port",
	      adapter_specific->connection_port);

  adapter->parameter = malloc(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter,
	  "%d", adapter_specific->connection_port);
  LOG_OUT();
}

void
mad_tcp_adapter_configuration_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver           = adapter->driver;
  p_mad_configuration_t          configuration    =
    &(driver->madeleine->configuration);
  p_mad_tcp_adapter_specific_t   adapter_specific = adapter->specific;
  mad_host_id_t                  i;

  LOG_IN();
  if (configuration->local_host_id == 0)
    {
      /* Master */
      int                            sock[configuration->size];
      int                            fd;
      mad_host_id_t                  rank;
      
      LOG("mad_tcp_adapter_configuration_init: master");
      for(i = 1;
	  i < configuration->size;
	  i++)
	{
	  do
	    {
	      fd =
		accept(adapter_specific->connection_socket, NULL, NULL);
	      if (    (fd == -1)
		      && (errno != EINTR))
		{
		  perror("master'accept");
		  FAILURE("accept");
		}
	    }
	  while(fd == -1);
      
	  read(fd, &rank, sizeof(mad_host_id_t));
	  sock[rank] = fd;
	  read(sock[rank],
	       &(adapter_specific->remote_connection_port[rank]),
	       sizeof(int));
	}
      
      LOG("mad_tcp_adapter_configuration_init: phase 1 terminee");
      for(i = 1;
	  i < configuration->size;
	  i++)
	{
	  int j;
	  
	  for (j = 1;
	       j < configuration->size;
	       j++)
	    {
	      if (j == i)
		{
		  continue;
		}
	      
	      write(sock[i],
		    &(adapter_specific->remote_connection_port[j]),
		    sizeof(int));
	    }
	  close(sock[i]);
	}
      LOG("mad_tcp_adapter_configuration_init: phase 2 terminee");
    }
  else
    {
      /* Slave */
      struct sockaddr_in server;
      int sock;
      int status;
      
      LOG("mad_tcp_adapter_configuration_init: slave");
      adapter_specific->remote_connection_port[0] =
	atoi(adapter->master_parameter);
      LOG_VAL("mad_tcp_adapter_configuration_init: master port",
	      adapter_specific->remote_connection_port[0]);
      
      sock = creer_socket(SOCK_STREAM, 0, NULL);
      if (sock == -1)
	{
	  FAILURE("mad_tcp_adapter_configuration_init: creer_socket");
	}
      
      mad_tcp_fill_sockaddr(&server, adapter, 0);
      
      status = connect(sock,
		       (struct sockaddr*)&server,
		       sizeof(struct sockaddr_in));

      if (status == -1)
	{
	      perror("mad_tcp_adapter_configuration_init: connect");
	      FAILURE("Connect");
	}
      
      write(sock,
	    &(configuration->local_host_id),
	    sizeof(mad_host_id_t));
      write(sock,
		&(adapter_specific->connection_port),
	    sizeof(int));
      
      LOG("mad_tcp_adapter_configuration_init: phase 1 terminee");
      for (i = 1;
	   i < configuration->size;
	   i++)
	{
	  if (i == configuration->local_host_id)
	    {
	      continue;
	    }
	  
	  read(sock,
	       &(adapter_specific->remote_connection_port[i]),
	       sizeof(int));
	}

      close(sock);
      LOG("mad_tcp_adapter_configuration_init: phase 2 terminee");
    }
  LOG_OUT();
}

void
mad_tcp_channel_init(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific;
  
  LOG_IN();
  channel_specific = malloc(sizeof(mad_tcp_channel_specific_t));
  CTRL_ALLOC(channel_specific);
  channel->specific = channel_specific;
  LOG_OUT();
}

void
mad_tcp_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_tcp_connection_specific_t specific;
  
  LOG_IN();
  specific = malloc(sizeof(mad_tcp_connection_specific_t));
  CTRL_ALLOC(specific);
  in->specific = out->specific = specific;
  in->nb_link = 1;
  out->nb_link = 1;
  LOG_OUT();
}

void 
mad_tcp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  lnk->specific    = NULL;
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_aggregate;
  LOG_OUT();
}

void
mad_tcp_before_open_channel(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific;

  LOG_IN();
  channel_specific = channel->specific;
  channel_specific->max_fds = 0;  
  FD_ZERO(&(channel_specific->read_fds));  
  LOG_OUT();
}

void
mad_tcp_accept(p_mad_channel_t channel)
{
  p_mad_adapter_t                   adapter          = channel->adapter;
  p_mad_tcp_adapter_specific_t      adapter_specific = adapter->specific;
  mad_host_id_t                     remote_host_id;  
  p_mad_tcp_connection_specific_t   connection_specific; 
  int                               sock;
  
  LOG_IN();
  do
    {
      sock = accept(adapter_specific->connection_socket, NULL, NULL);
      if (sock == -1)
	{
	  if (errno == EINTR)
	    {
	      continue;
	    }
	  else
	    {
	      perror("slave'accept");
	      FAILURE("Accept");
	    }
	}
      mad_tcp_setup_socket(sock);
    }
  while (sock == -1);

  read(sock,
       &remote_host_id,
       sizeof(mad_host_id_t));
  channel->input_connection[remote_host_id].remote_host_id =
    remote_host_id;
  channel->output_connection[remote_host_id].remote_host_id =
    remote_host_id;
  
  connection_specific =
    channel->input_connection[remote_host_id].specific;
  /* Note:
     The `specific' field of tcp connections is shared by input
     and output connections */
  connection_specific->socket = sock;
  LOG_OUT();
}

void
mad_tcp_connect(p_mad_connection_t connection)
{
  p_mad_tcp_connection_specific_t   connection_specific =
    connection->specific;
  p_mad_connection_t                reverse             =
    connection->reverse;
  struct sockaddr_in                adresse;
  struct sockaddr_in                server;
  int                               sock;
  int                               status;
  
  LOG_IN();
  sock = creer_socket(SOCK_STREAM, 0, &adresse);
  if(sock == -1)
    {
      FAILURE("mad_tcp_connect: creer_socket");
    }

  mad_tcp_fill_sockaddr(&server,
			connection->channel->adapter,
			connection->remote_host_id);

  status =
    connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
  if(status == -1)
    {
      perror("mad_tcp_connect: connect");
      FAILURE("Connect");
    }

  write(sock,
	&(connection->
	  channel->
	  adapter->
	  driver->
	  madeleine->
	  configuration.local_host_id),
	sizeof(mad_host_id_t));
  mad_tcp_setup_socket(sock);

  reverse->remote_host_id = connection->remote_host_id;
  /* Note:
     The `specific' field of tcp connections is shared by input
     and output connections */
  connection_specific->socket = sock;
  LOG_OUT();
}

void
mad_tcp_after_open_channel(p_mad_channel_t channel)
{
  mad_host_id_t                  host;
  p_mad_tcp_channel_specific_t   channel_specific = channel->specific;
  p_mad_configuration_t          configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  
  LOG_IN();
  mad_tcp_sync_channel(channel);

  for (host = 0 ;
       host < configuration->size ;
       host++)
    {
      p_mad_tcp_connection_specific_t connection_specific
	= channel->input_connection[host].specific;
      
      if (host == configuration->local_host_id)
	{
	  continue;
	}
      
      FD_SET(connection_specific->socket, &(channel_specific->read_fds));
      channel_specific->max_fds
	= max(connection_specific->socket, channel_specific->max_fds);
    }
  
  LOG_OUT();
}

void
mad_tcp_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  mad_tcp_sync_channel(channel);
  LOG_OUT();
}

void 
mad_tcp_disconnect(p_mad_connection_t connection)
{
  p_mad_tcp_connection_specific_t connection_specific = connection->specific;
  int status;
  
  LOG_IN();
  status = close(connection_specific->socket);
  if (status == -1)
    {
      perror("mad_tcp_disconnect: close");
      exit(EXIT_FAILURE);
    }
  
  connection_specific->socket = -1;
  LOG_OUT();
}

void
mad_tcp_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

void
mad_tcp_link_exit(p_mad_link_t link)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

void
mad_tcp_connection_exit(p_mad_connection_t in,
			p_mad_connection_t out)
{
  LOG_IN();
  free(in->specific);
  in->specific = out->specific = NULL;
  LOG_OUT();
}

void
mad_tcp_channel_exit(p_mad_channel_t channel)
{
  LOG_IN();
  free(channel->specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_tcp_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_tcp_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
  close(adapter_specific->connection_socket);
  free(adapter_specific->remote_connection_port);
  free(adapter_specific);
  adapter->specific = NULL;
  free(adapter->parameter);
  if (adapter->master_parameter)
    {
      free(adapter->master_parameter);
      adapter->master_parameter = NULL;
    }
  free(adapter->name);
  LOG_OUT();
}

void
mad_tcp_driver_exit(p_mad_driver_t driver)
{
  LOG_IN();
  free(driver->specific);
  LOG_OUT();
}

p_mad_link_t
mad_tcp_choice(p_mad_connection_t   connection,
	       size_t               size,
	       mad_send_mode_t      send_mode,
	       mad_receive_mode_t   receive_mode)
{
  LOG_IN();
  LOG_OUT();
  return &(connection->link[0]);
}

void
mad_tcp_new_message(p_mad_connection_t connection)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t channel)
{
  p_mad_configuration_t          configuration    = 
    &(channel->adapter->driver->madeleine->configuration);

#ifndef PM2
  LOG_IN();
  if (configuration->size == 2)
    {
      LOG_OUT();
      return &(channel->
	       input_connection[1 - configuration->local_host_id]);
    }
  else
#endif /* PM2 */
    {
      p_mad_tcp_channel_specific_t channel_specific = channel->specific;
      fd_set rfds;
      int n;
      int i;

      do
	{
	  do
	    {
	      rfds = channel_specific->read_fds;
#ifdef PM2
#ifdef USE_MARCEL_POLL
	      n = marcel_select(channel_specific->max_fds + 1, &rfds, NULL);
	      
#else /* USE_MARCEL_POLL */
	      n = tselect(channel_specific->max_fds + 1, &rfds, NULL, NULL);
	      if(n <= 0)
		{
		  PM2_YIELD();
		}
#endif /* USE_MARCEL_POLL */  
#else /* PM2 */
	      n = select(channel_specific->max_fds + 1,
			 &rfds,
			 NULL,
			 NULL,
			 NULL);
#endif /* PM2 */
	    }
	  while(n <= 0);

	  for (i = 0;
	       i < configuration->size;
	       i++)
	    {
	      p_mad_tcp_connection_specific_t connection_specific
		= channel->input_connection[i].specific;
	  
	      if (i == configuration->local_host_id)
		{
		  continue;
		}
	  
	      if (FD_ISSET(connection_specific->socket, &rfds))
		{
		  break;
		}
	    }
	}
      while (n == 0); /* Should be verified !!! */

      LOG_OUT();
      return &(channel->input_connection[i]);
    }
}

void
mad_tcp_send_buffer(p_mad_link_t     lnk,
		    p_mad_buffer_t   buffer)
{
  p_mad_tcp_connection_specific_t   connection_specific =
    lnk->connection->specific;
  LOG_IN();
  mad_tcp_write(connection_specific->socket, buffer);
  LOG_OUT();
}

void
mad_tcp_receive_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t  *buffer)
{
  p_mad_tcp_connection_specific_t   connection_specific =
    lnk->connection->specific;
  LOG_IN();
  mad_tcp_read(connection_specific->socket, *buffer);
  LOG_OUT();
}

void
mad_tcp_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      p_mad_tcp_connection_specific_t   connection_specific =
	lnk->connection->specific;
      mad_list_reference_t              ref;

      mad_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_tcp_write(connection_specific->socket,
			mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_tcp_receive_sub_buffer_group(p_mad_link_t           lnk,
				 mad_bool_t             first_sub_group,
				 p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      p_mad_tcp_connection_specific_t   connection_specific =
	lnk->connection->specific;
      mad_list_reference_t              ref;

      mad_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_tcp_read(connection_specific->socket,
		       mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}
