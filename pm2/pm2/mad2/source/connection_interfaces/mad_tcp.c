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
Revision 1.24  2000/06/09 18:12:01  vdanjean
cleaning debug messages

Revision 1.23  2000/06/07 08:12:08  oaumage
- Retour a des bases saines

Revision 1.22  2000/06/06 16:52:51  oaumage
- Correction synchronisation

Revision 1.21  2000/06/06 16:40:48  oaumage
- Retablissement du sync final

Revision 1.20  2000/06/06 12:13:19  oaumage
- suppression du sync terminal

Revision 1.19  2000/05/29 17:12:22  vdanjean
End of mad2 corrected

Revision 1.18  2000/05/29 09:15:15  oaumage
- Modificationde mad_tcp_sync_channel en deux fonctions mad_tcp_sync_in_channel
  (synchronisation par reception) et mad_tcp_sync_out_channel
  (synchronisation par emission) pour eviter des interferences avec les
  applications en fin de session

Revision 1.17  2000/04/17 15:47:42  oaumage
- correction de l'allocation du tableau des ports

Revision 1.16  2000/03/15 09:59:43  oaumage
- renommage du polling Nexus

Revision 1.15  2000/03/08 17:19:47  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

Revision 1.14  2000/03/02 15:46:04  oaumage
- support du polling Nexus

Revision 1.13  2000/03/02 14:51:24  oaumage
- indication du nom du protocole dans la structure driver

Revision 1.12  2000/02/28 11:06:37  rnamyst
Changed #include "" into #include <>.

Revision 1.11  2000/02/08 17:49:55  oaumage
- support de la net toolbox
- mad_tcp.c : deplacement des fonctions statiques de gestion des sockets
              vers la net toolbox

Revision 1.10  2000/01/31 15:55:01  oaumage
- mad_mpi.c : terminaison amelioree sous PM2
- mad_tcp.c : debogage de la synchronisation finale

Revision 1.9  2000/01/25 14:16:10  oaumage
- suppression (temporaire) du sync lors de la fermeture du canal pour
  resoudre un probleme de blocage a la terminaison d'une session PM2

Revision 1.8  2000/01/13 14:46:14  oaumage
- adaptation pour la prise en compte de la toolbox

Revision 1.7  2000/01/10 10:23:16  oaumage
*** empty log message ***

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
#define DEBUG
#include "madeleine.h"

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
mad_tcp_sync_in_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                   adapter       = channel->adapter;
  p_mad_driver_t                    driver        = adapter->driver;
  p_mad_configuration_t             configuration =
    &(driver->madeleine->configuration);
  p_mad_connection_t                connection;
  p_mad_tcp_connection_specific_t   connection_specific;
  ntbx_host_id_t                     i;

  LOG_IN();
  LOG_VAL("Syncing channel", channel->id);
  
  for (i = 0;
       i < configuration->size;
       i++)
    {
      if (i == configuration->local_host_id)
	{
	  /* Receive */
	  ntbx_host_id_t j;

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

	      LOG_VAL("Receiving channel id from host", j);
	      SYSCALL(read(connection_specific->socket, 
			   &channel_id,
			   sizeof(mad_channel_id_t)));
	      LOG_VAL("Received channel id from host", j);
	      LOG_VAL("Channel id", channel_id);
	      
	      if (channel_id != channel->id)
		{
		  FAILURE("wrong channel id");
		}
	      LOG_VAL("Writing local host id", configuration->local_host_id);
	      SYSCALL(write(connection_specific->socket,
			    &(configuration->local_host_id),
			    sizeof(ntbx_host_id_t)));	      
	      LOG_VAL("Wrote local host id", configuration->local_host_id);
	    }
	}
      else
	{
	  /* send */
	  ntbx_host_id_t host_id;

	  connection = &(channel->output_connection[i]);
	  connection_specific = connection->specific;

	  LOG_VAL("Writing channel id", channel->id);
	  SYSCALL(write(connection_specific->socket,
			&(channel->id),
			sizeof(mad_channel_id_t)));
	  LOG_VAL("Wrote channel id", channel->id);
	      
	  LOG_VAL("Receiving host id from host", i);
	  SYSCALL(read(connection_specific->socket,
		       &(host_id),
		       sizeof(ntbx_host_id_t)));
	  LOG_VAL("Received host id from host", i);
	  LOG_VAL("Host id", host_id);
	  
	  if (host_id != i)
	    {
	      FAILURE("wrong host id");
	    }
	}
    }
  LOG_VAL("Channel synced", channel->id);
  LOG_OUT();
}

static void
mad_tcp_sync_out_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                   adapter       = channel->adapter;
  p_mad_driver_t                    driver        = adapter->driver;
  p_mad_configuration_t             configuration =
    &(driver->madeleine->configuration);
  p_mad_connection_t                connection;
  p_mad_tcp_connection_specific_t   connection_specific;
  ntbx_host_id_t                     i;

  LOG_IN();
  LOG_VAL("Syncing channel", channel->id);
  
  for (i = 0;
       i < configuration->size;
       i++)
    {
      if (i == configuration->local_host_id)
	{
	  /* Send */
	  ntbx_host_id_t j;

	  for (j = 0;
	       j < configuration->size;
	       j++)
	    {
	      ntbx_host_id_t host_id;
	      
	      if (j == configuration->local_host_id)
		{
		  continue;
		}

	      connection = &(channel->output_connection[j]);
	      connection_specific = connection->specific;

	      LOG_VAL("Writing channel id", channel->id);
	      SYSCALL(write(connection_specific->socket,
			    &(channel->id),
			    sizeof(mad_channel_id_t)));
	      LOG_VAL("Wrote channel id", channel->id);	      	  

	      LOG_VAL("Receiving host id from host", j);
	      SYSCALL(read(connection_specific->socket,
			   &(host_id),
			   sizeof(ntbx_host_id_t)));
	      LOG_VAL("Received host id from host", j);
	      LOG_VAL("Host id", host_id);

	      if (host_id != j)
		{
		  FAILURE("wrong host id");
		}
	    }
	}
      else
	{
	  /* Receive */
	  mad_channel_id_t channel_id;	      

	  connection = &(channel->input_connection[i]);
	  connection_specific = connection->specific;

	  LOG_VAL("Receiving channel id from host", i);
	  SYSCALL(read(connection_specific->socket, 
		       &channel_id,
		       sizeof(mad_channel_id_t)));
	  LOG_VAL("Received channel id from host", i);
	  LOG_VAL("Channel id", channel_id);
	      
	  if (channel_id != channel->id)
	    {
	      FAILURE("wrong channel id");
	    }

	  LOG_VAL("Writing local host id", configuration->local_host_id);
	  SYSCALL(write(connection_specific->socket,
			&(configuration->local_host_id),
			sizeof(ntbx_host_id_t)));	      
	  LOG_VAL("Wrote local host id", configuration->local_host_id);
	}
    }
  LOG_VAL("Channel synced", channel->id);
  LOG_OUT();
}

static void
mad_tcp_write(int              sock,
	      p_mad_buffer_t   buffer)
{
  LOG_IN();
  while (mad_more_data(buffer))
    {
      ssize_t result;
      SYSTEST(result = write(sock,
			     (const void*)(buffer->buffer +
					   buffer->bytes_read),
			     buffer->bytes_written - buffer->bytes_read));

      if (result > 0) buffer->bytes_read += result;
#ifdef MARCEL
      if (mad_more_data(buffer)) TBX_YIELD();
#endif /* MARCEL */
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

      SYSTEST(result = read(sock,
			    (void*)(buffer->buffer +
				    buffer->bytes_written),
			    buffer->length - buffer->bytes_written));

      if (result > 0) buffer->bytes_written += result;
#ifdef MARCEL
      if (!mad_buffer_full(buffer))
	{
	  TBX_YIELD();
	}
#endif /* MARCEL */
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
  /* interface->before_close_channel       = NULL; */
  interface->before_close_channel    = mad_tcp_before_close_channel;
  interface->disconnect                 = mad_tcp_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = NULL;
  interface->adapter_exit               = mad_tcp_adapter_exit;
  interface->driver_exit                = NULL;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_tcp_poll_message;
#endif /* MAD_MESSAGE_POLLING */
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
  driver_specific = TBX_MALLOC(sizeof(mad_tcp_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;
  
  driver->name = TBX_MALLOC(4);
  CTRL_ALLOC(driver->name);
  strcpy(driver->name, "tcp");
  LOG_OUT();
}

void
mad_tcp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver;
  p_mad_tcp_driver_specific_t    driver_specific;
  p_mad_tcp_adapter_specific_t   adapter_specific;
  ntbx_tcp_address_t             address;
  
  LOG_IN();
  driver          = adapter->driver;
  driver_specific = driver->specific;

  adapter_specific  = TBX_MALLOC(sizeof(mad_tcp_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter->specific = adapter_specific;

  if (driver_specific->nb_adapter)
    FAILURE("TCP adapter already initialized");

  if (adapter->name == NULL)
    {
      adapter->name     = TBX_MALLOC(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "TCP%d", driver_specific->nb_adapter);
    }
  else
    FAILURE("Adapter selection currently unimplemented on top of TCP");

  driver_specific->nb_adapter++;  
  adapter_specific->remote_connection_port = NULL;
  adapter_specific->connection_socket = ntbx_tcp_socket_create(&address, 0);
  
  SYSCALL(listen(adapter_specific->connection_socket,
		 driver->madeleine->configuration.size - 1));
  
  adapter_specific->connection_port =
    (ntbx_tcp_port_t)ntohs(address.sin_port);

  adapter->parameter = TBX_MALLOC(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "%d", adapter_specific->connection_port);
  LOG_OUT();
}

void
mad_tcp_adapter_configuration_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver           = adapter->driver;
  p_mad_configuration_t          configuration    =
    &(driver->madeleine->configuration);
  p_mad_tcp_adapter_specific_t   adapter_specific = adapter->specific;
  ntbx_host_id_t                 i;

  LOG_IN();
  adapter_specific->remote_connection_port =
    TBX_MALLOC(driver->madeleine->configuration.size * sizeof(int));
  CTRL_ALLOC(adapter_specific->remote_connection_port);

  if (configuration->local_host_id == 0)
    {
      /* Master */
      int            desc_array[configuration->size];
      int            desc;
      ntbx_host_id_t rank;
      
      LOG("mad_tcp_adapter_configuration_init: master");
      for(i = 1; i < configuration->size; i++)
	{
	  SYSCALL(desc =
		  accept(adapter_specific->connection_socket, NULL, NULL));
      
	  SYSCALL(read(desc, &rank, sizeof(ntbx_host_id_t)));
	  desc_array[rank] = desc;
	  SYSCALL(read(desc_array[rank],
		       &(adapter_specific->remote_connection_port[rank]),
		       sizeof(ntbx_tcp_port_t)));
	}
      
      LOG("mad_tcp_adapter_configuration_init: phase 1 terminee");
      for(i = 1; i < configuration->size; i++)
	{
	  int j;
	  
	  for (j = 1; j < configuration->size; j++)
	    {
	      if (j != i)
		SYSCALL(write(desc_array[i],
			      &(adapter_specific->remote_connection_port[j]),
			      sizeof(ntbx_tcp_port_t)));
	    }
	  SYSCALL(close(desc_array[i]));
	}
      LOG("mad_tcp_adapter_configuration_init: phase 2 terminee");
    }
  else
    {
      /* Slave */
      ntbx_tcp_address_t server_address;
      ntbx_tcp_socket_t  desc;
      
      LOG("mad_tcp_adapter_configuration_init: slave");
      adapter_specific->remote_connection_port[0] =
	atoi(adapter->master_parameter);
      
      desc = ntbx_tcp_socket_create(NULL, 0);
      ntbx_tcp_address_fill(&server_address,
			    adapter_specific->remote_connection_port[0],
			    adapter->driver->madeleine->
			    configuration.host_name[0]);
      
      SYSCALL(connect(desc, (struct sockaddr*)&server_address,
		      sizeof(ntbx_tcp_address_t)));
      
      SYSCALL(write(desc, &(configuration->local_host_id),
		    sizeof(ntbx_host_id_t)));
      SYSCALL(write(desc, &(adapter_specific->connection_port),
		    sizeof(ntbx_tcp_port_t)));
      
      LOG("mad_tcp_adapter_configuration_init: phase 1 terminee");
      for (i = 1; i < configuration->size; i++)
	{
	  if (i != configuration->local_host_id)
	    SYSCALL(read(desc, &(adapter_specific->remote_connection_port[i]),
			 sizeof(ntbx_tcp_port_t)));
	}

      close(desc);
      LOG("mad_tcp_adapter_configuration_init: phase 2 terminee");
    }
  LOG_OUT();
}

void
mad_tcp_channel_init(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific;
  
  LOG_IN();
  channel_specific = TBX_MALLOC(sizeof(mad_tcp_channel_specific_t));
  CTRL_ALLOC(channel_specific);
  channel->specific = channel_specific;
  LOG_OUT();
}

void
mad_tcp_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_tcp_connection_specific_t specific;
  
  LOG_IN();
  specific = TBX_MALLOC(sizeof(mad_tcp_connection_specific_t));
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
  ntbx_host_id_t                    remote_host_id;  
  p_mad_tcp_connection_specific_t   connection_specific; 
  ntbx_tcp_socket_t                 desc;
  
  LOG_IN();
  SYSCALL(desc = accept(adapter_specific->connection_socket, NULL, NULL));
  ntbx_tcp_socket_setup(desc);
  SYSCALL(read(desc, &remote_host_id, sizeof(ntbx_host_id_t)));
  channel->input_connection[remote_host_id].remote_host_id = remote_host_id;
  channel->output_connection[remote_host_id].remote_host_id = remote_host_id;
  
  connection_specific = channel->input_connection[remote_host_id].specific;
  /* Note:
     The `specific' field of tcp connections is shared by input
     and output connections */
  connection_specific->socket = desc;
  LOG_OUT();
}

void
mad_tcp_connect(p_mad_connection_t connection)
{
  p_mad_tcp_connection_specific_t   connection_specific =
    connection->specific;
  p_mad_connection_t                reverse             =
    connection->reverse;
  p_mad_adapter_t                   adapter             =
    connection->channel->adapter;
  p_mad_tcp_adapter_specific_t      adapter_specific    =
    adapter->specific;
  ntbx_tcp_address_t                address;
  ntbx_tcp_address_t                server_address;
  ntbx_tcp_socket_t                 desc;

  LOG_IN();
  desc = ntbx_tcp_socket_create(&address, 0);
  ntbx_tcp_address_fill(&server_address,
			adapter_specific->
			remote_connection_port[connection->remote_host_id],
			adapter->driver->madeleine->
			configuration.host_name[connection->remote_host_id]);

  SYSCALL(connect(desc, (struct sockaddr *)&server_address, 
		  sizeof(ntbx_tcp_address_t)));

  ntbx_tcp_socket_setup(desc);
  SYSCALL(write(desc,
		&(connection->channel->adapter->driver->madeleine->
		  configuration.local_host_id), 
		sizeof(ntbx_host_id_t)));

  reverse->remote_host_id = connection->remote_host_id;
  /* Note:
     The `specific' field of tcp connections is shared by input
     and output connections */
  connection_specific->socket = desc;
  LOG_OUT();
}

void
mad_tcp_after_open_channel(p_mad_channel_t channel)
{
  ntbx_host_id_t                 host;
  p_mad_tcp_channel_specific_t   channel_specific = channel->specific;
  p_mad_configuration_t          configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  
  LOG_IN();
  mad_tcp_sync_in_channel(channel);

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
  mad_tcp_sync_out_channel(channel);
  LOG_OUT();
}

void 
mad_tcp_disconnect(p_mad_connection_t connection)
{
  p_mad_tcp_connection_specific_t connection_specific = connection->specific;
  
  LOG_IN();
  SYSCALL(close(connection_specific->socket));
  connection_specific->socket = -1;
  LOG_OUT();
}

void
mad_tcp_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_tcp_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
  SYSCALL(close(adapter_specific->connection_socket));
  TBX_FREE(adapter_specific->remote_connection_port);
  TBX_FREE(adapter_specific);
  adapter->specific = NULL;
  TBX_FREE(adapter->parameter);
  if (adapter->master_parameter)
    {
      TBX_FREE(adapter->master_parameter);
      adapter->master_parameter = NULL;
    }
  TBX_FREE(adapter->name);
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_tcp_poll_message(p_mad_channel_t channel)
{
  p_mad_configuration_t configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  p_mad_tcp_channel_specific_t channel_specific = channel->specific;
  fd_set rfds;
  int n;
  int i;

  do
    {
      struct timeval timeout;
      rfds = channel_specific->read_fds;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;
		
      n = select(channel_specific->max_fds + 1,
		 &rfds,
		 NULL,
		 NULL,
		 &timeout);
      if (!n)
	{
	  LOG_OUT();
	  return NULL;
	}
    }
  while(n < 0);

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

  LOG_OUT();
  return &(channel->input_connection[i]);
}
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t channel)
{
  p_mad_configuration_t configuration    = 
    &(channel->adapter->driver->madeleine->configuration);

#if !defined(MARCEL) && !defined(MAD_MESSAGE_POLLING)
  LOG_IN();
  if (configuration->size == 2)
    {
      LOG_OUT();
      return &(channel->
	       input_connection[1 - configuration->local_host_id]);
    }
  else
#endif /* MARCEL/MAD_MESSAGE_POLLING */
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
#ifdef MARCEL
#ifdef USE_MARCEL_POLL
	      n = marcel_select(channel_specific->max_fds + 1, &rfds, NULL);
	      
#else /* USE_MARCEL_POLL */
	      n = tselect(channel_specific->max_fds + 1, &rfds, NULL, NULL);
	      if(n <= 0)
		{
		  TBX_YIELD();
		}
#endif /* USE_MARCEL_POLL */  
#else /* MARCEL */
	      n = select(channel_specific->max_fds + 1,
			 &rfds,
			 NULL,
			 NULL,
			 NULL);
#endif /* MARCEL */
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
  p_mad_tcp_connection_specific_t connection_specific =
    lnk->connection->specific;
  LOG_IN();
  mad_tcp_write(connection_specific->socket, buffer);
  LOG_OUT();
}

void
mad_tcp_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *buffer)
{
  p_mad_tcp_connection_specific_t connection_specific =
    lnk->connection->specific;
  LOG_IN();
  mad_tcp_read(connection_specific->socket, *buffer);
  LOG_OUT();
}

void
mad_tcp_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      p_mad_tcp_connection_specific_t connection_specific =
	lnk->connection->specific;
      tbx_list_reference_t            ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_tcp_write(connection_specific->socket,
			tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_tcp_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_group
				   __attribute__ ((unused)),
				 p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      p_mad_tcp_connection_specific_t connection_specific =
	lnk->connection->specific;
      tbx_list_reference_t            ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_tcp_read(connection_specific->socket,
		       tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

