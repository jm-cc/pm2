
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
 * Mad_tcp.c
 * =========
 */

//#define USE_MARCEL_POLL
//#define DEBUG
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
  int  connection_socket;
  int  connection_port;
  int *remote_connection_port;
} mad_tcp_adapter_specific_t, *p_mad_tcp_adapter_specific_t;

typedef struct
{
  int    max_fds;
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
static
void
mad_tcp_sync_in_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                 adapter       = channel->adapter;
  p_mad_driver_t                  driver        = adapter->driver;
  p_mad_configuration_t           configuration =
    driver->madeleine->configuration;
  ntbx_host_id_t                  size          = configuration->size;
  ntbx_host_id_t                  rank          = configuration->local_host_id;
  p_mad_connection_t              connection    = NULL;
  p_mad_tcp_connection_specific_t connection_specific = NULL;
  ntbx_host_id_t                  i;

  LOG_IN();
  for (i = 0; i < size; i++)
    {
      if (i == rank)
	{
	  /* Receive */
	  ntbx_host_id_t j;

	  for (j = 0; j < size; j++)
	    {
	      ntbx_pack_buffer_t pack_buffer;
	      mad_channel_id_t   channel_id;
	      
	      if (j == rank)
		{
		  continue;
		}

#ifdef MAD_FORWARDING
	      if(!channel->output_connection[j].active)
		{
		  continue;
		}
#endif /* MAD_FORWARDING */

	      connection = &(channel->input_connection[j]);
	      connection_specific = connection->specific;

	      ntbx_tcp_read(connection_specific->socket, 
			   &pack_buffer,
			   sizeof(ntbx_pack_buffer_t));
	      channel_id = (mad_channel_id_t)ntbx_unpack_int(&pack_buffer);
	      
	      if (channel_id != channel->id)
		FAILURE("wrong channel id");

	      ntbx_pack_int((int)rank, &pack_buffer);
	      ntbx_tcp_write(connection_specific->socket,
			     &pack_buffer,
			     sizeof(ntbx_pack_buffer_t));
	    }
	}
      else
	{
	  /* send */
	  ntbx_pack_buffer_t pack_buffer;
	  ntbx_host_id_t     host_id;

	  connection          = &(channel->output_connection[i]);
	  connection_specific = connection->specific;

#ifdef MAD_FORWARDING
	  if(!channel->output_connection[i].active)
	    {
	      continue;
	    }
#endif /* MAD_FORWARDING */

	  ntbx_pack_int((int)channel->id, &pack_buffer);
	  ntbx_tcp_write(connection_specific->socket,
			 &pack_buffer,
			 sizeof(ntbx_pack_buffer_t));
	      
	  ntbx_tcp_read(connection_specific->socket,
			&pack_buffer,
			sizeof(ntbx_pack_buffer_t));
	  host_id = (ntbx_host_id_t)ntbx_unpack_int(&pack_buffer);
	  
	  if (host_id != i)
	    FAILURE("wrong host id");
	}
    }
  LOG_OUT();
}

static
void
mad_tcp_sync_out_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                 adapter       = channel->adapter;
  p_mad_driver_t                  driver        = adapter->driver;
  p_mad_configuration_t           configuration =
    driver->madeleine->configuration;
  ntbx_host_id_t                  size          = configuration->size;
  ntbx_host_id_t                  rank          = configuration->local_host_id;
  p_mad_connection_t              connection    = NULL;
  p_mad_tcp_connection_specific_t connection_specific = NULL;
  ntbx_host_id_t                  i;

  LOG_IN();
  for (i = 0; i < size; i++)
    {
      if (i == rank)
	{
	  /* Send */
	  ntbx_host_id_t j;

	  for (j = 0; j < size; j++)
	    {
	      ntbx_pack_buffer_t pack_buffer;
	      ntbx_host_id_t     host_id;
	      
	      if (j == rank)
		{
		  continue;
		}

#ifdef MAD_FORWARDING
	      if(!channel->output_connection[j].active)
		{
		  continue;
		}
#endif /* MAD_FORWARDING */

	      connection          = &(channel->output_connection[j]);
	      connection_specific = connection->specific;

	      ntbx_pack_int((int)channel->id, &pack_buffer);
	      ntbx_tcp_write(connection_specific->socket,
			     &pack_buffer,
			     sizeof(ntbx_pack_buffer_t));

	      ntbx_tcp_read(connection_specific->socket,
			    &pack_buffer,
			    sizeof(ntbx_pack_buffer_t));
	      host_id = (ntbx_host_id_t)ntbx_unpack_int(&pack_buffer);

	      if (host_id != j)
		FAILURE("wrong host id");
	    }
	}
      else
	{
	  /* Receive */
	  ntbx_pack_buffer_t pack_buffer;
	  mad_channel_id_t channel_id;	      

#ifdef MAD_FORWARDING
	  if(!channel->output_connection[i].active)
	    {
	      continue;
	    }
#endif /* MAD_FORWARDING */

	  connection          = &(channel->input_connection[i]);
	  connection_specific = connection->specific;

	  ntbx_tcp_read(connection_specific->socket, 
			&pack_buffer,
			sizeof(ntbx_pack_buffer_t));
	  channel_id = (mad_channel_id_t)ntbx_unpack_int(&pack_buffer);
	      
	  if (channel_id != channel->id)
	    FAILURE("wrong channel id");

	  ntbx_pack_int((int)rank, &pack_buffer);
	  ntbx_tcp_write(connection_specific->socket,
			 &pack_buffer,
			 sizeof(ntbx_pack_buffer_t));
	}
    }
  LOG_OUT();
}

static
void
mad_tcp_write(int            sock,
	      p_mad_buffer_t buffer)
{
  LOG_IN();
  while (mad_more_data(buffer))
    {
      ssize_t result;

      SYSTEST(result = write(sock,
			     (const void*)(buffer->buffer +
					   buffer->bytes_read),
			     buffer->bytes_written - buffer->bytes_read));

      if (result > 0)
	{
	  buffer->bytes_read += result;
	}
      else
	FAILURE("connection closed");
      
#ifdef MARCEL
      if (mad_more_data(buffer)) TBX_YIELD();
#endif /* MARCEL */
    }
  LOG_OUT();
}

static
void
mad_tcp_read(int            sock, 
	     p_mad_buffer_t buffer)
{  
  LOG_IN();
  while (!mad_buffer_full(buffer))
    {
      ssize_t result;

      SYSTEST(result = read(sock,
			    (void*)(buffer->buffer +
				    buffer->bytes_written),
			    buffer->length - buffer->bytes_written));

      if (result > 0)
	{
	  buffer->bytes_written += result;
	}
      else
	FAILURE("connection closed");
     
#ifdef MARCEL
      if (!mad_buffer_full(buffer))
	{
	  TBX_YIELD();
	}
#endif /* MARCEL */
    }
  LOG_OUT();
}

/*
 * Registration function
 * ---------------------
 */
void
mad_tcp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  interface = &(driver->interface);
  
  driver->connection_type  = mad_bidirectional_connection;
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
  interface->before_close_channel       = mad_tcp_before_close_channel;
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
  p_mad_tcp_driver_specific_t driver_specific = NULL;

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
  p_mad_driver_t                 driver           = NULL;
  p_mad_tcp_driver_specific_t    driver_specific  = NULL;
  p_mad_tcp_adapter_specific_t   adapter_specific = NULL;
  ntbx_tcp_address_t             address;
  
  LOG_IN();
  driver          = adapter->driver;
  driver_specific = driver->specific;

  adapter_specific  = TBX_MALLOC(sizeof(mad_tcp_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter->specific = adapter_specific;

  if (driver_specific->nb_adapter)
    FAILURE("TCP adapter already initialized");

  if (!adapter->name)
    {
      adapter->name = TBX_MALLOC(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "TCP%d", driver_specific->nb_adapter);
    }
  else
    FAILURE("Adapter selection currently unimplemented on top of TCP");

  driver_specific->nb_adapter++;  
  adapter_specific->remote_connection_port = NULL;
  adapter_specific->connection_socket = ntbx_tcp_socket_create(&address, 0);
  
  SYSCALL(listen(adapter_specific->connection_socket,
		 driver->madeleine->configuration->size - 1));
  
  adapter_specific->connection_port = (ntbx_tcp_port_t)ntohs(address.sin_port);

  adapter->parameter = TBX_MALLOC(10);
  CTRL_ALLOC(adapter->parameter);

  sprintf(adapter->parameter, "%d", adapter_specific->connection_port);
  LOG_OUT();
}

#ifdef MAD_FORWARDING
extern tbx_bool_t adapter_is_present[3][2];
#endif /* MAD_FORWARDING */

void
mad_tcp_adapter_configuration_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t               driver           = adapter->driver;
  p_mad_configuration_t        configuration    =
    driver->madeleine->configuration;
  p_mad_tcp_adapter_specific_t adapter_specific = adapter->specific;
  ntbx_host_id_t               size             = configuration->size;
  ntbx_host_id_t               rank             = configuration->local_host_id;
  ntbx_host_id_t               i;

  LOG_IN();

  adapter_specific->remote_connection_port = TBX_MALLOC(size * sizeof(int));
  CTRL_ALLOC(adapter_specific->remote_connection_port);

  if (!rank)
    {
      /* Master */
      int                desc_array[size];
      int                desc;
      ntbx_host_id_t     remote_host_id;
      ntbx_pack_buffer_t pack_buffer;
      
      for(i = 1; i < size; i++)
	{
	  SYSCALL(desc = accept(adapter_specific->connection_socket,
				NULL, NULL));
      
	  SYSCALL(read(desc, &pack_buffer, sizeof(ntbx_pack_buffer_t)));
	  remote_host_id = (ntbx_host_id_t)ntbx_unpack_int(&pack_buffer);
	  desc_array[remote_host_id] = desc;

	  SYSCALL(read(desc_array[remote_host_id],
		       &pack_buffer, sizeof(ntbx_pack_buffer_t)));
	  adapter_specific->remote_connection_port[remote_host_id] = 
	    (ntbx_tcp_port_t)ntbx_unpack_int(&pack_buffer);
	}
      
      for(i = 1; i < size; i++)
#ifdef MAD_FORWARDING
	if(adapter_is_present[i][1])
#endif /* MAD_FORWARDING */
	{
	  int j;
	  
	  for (j = 1; j < size; j++)
	    {
	      if (j != i)
		{
		  ntbx_pack_int((int)adapter_specific->
				remote_connection_port[j], 
				&pack_buffer);
		  SYSCALL(write(desc_array[i],
				&pack_buffer, sizeof(ntbx_pack_buffer_t)));
		}
	    }
	  SYSCALL(close(desc_array[i]));
	}
    }
  else
    {
      /* Slave */
      ntbx_tcp_address_t server_address;
      ntbx_tcp_socket_t  desc;
      ntbx_pack_buffer_t pack_buffer;
      
      adapter_specific->remote_connection_port[0] =
	atoi(adapter->master_parameter);
      
      desc = ntbx_tcp_socket_create(NULL, 0);
      ntbx_tcp_address_fill(&server_address,
			    adapter_specific->remote_connection_port[0],
			    adapter->driver->madeleine->
			    configuration->host_name[0]);
      
      SYSCALL(connect(desc, (struct sockaddr*)&server_address,
		      sizeof(ntbx_tcp_address_t)));
      
      ntbx_pack_int((int)rank, &pack_buffer);
      SYSCALL(write(desc, &pack_buffer, sizeof(ntbx_pack_buffer_t)));

      ntbx_pack_int((int)adapter_specific->connection_port, &pack_buffer);
      SYSCALL(write(desc, &pack_buffer, sizeof(ntbx_pack_buffer_t)));
      
      for (i = 1; i < size; i++)
#ifdef MAD_FORWARDING
	if(adapter_is_present[i][1])
#endif
	{
	  if (i != rank)
	    {
	      SYSCALL(read(desc, &pack_buffer, sizeof(ntbx_pack_buffer_t)));
	      adapter_specific->remote_connection_port[i] = 
		(ntbx_tcp_port_t)ntbx_unpack_int(&pack_buffer);
	    }
	}

      close(desc);
    }
  LOG_OUT();
}

void
mad_tcp_channel_init(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific = NULL;
  
  LOG_IN();
  channel_specific = TBX_MALLOC(sizeof(mad_tcp_channel_specific_t));
  CTRL_ALLOC(channel_specific);
  channel->specific = channel_specific;
  LOG_OUT();
}

void
mad_tcp_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_tcp_connection_specific_t specific = NULL;
  
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
  lnk->id          = 0;
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
  p_mad_adapter_t                 adapter             = channel->adapter;
  p_mad_tcp_adapter_specific_t    adapter_specific    = adapter->specific;
  p_mad_tcp_connection_specific_t connection_specific =              NULL; 
  ntbx_tcp_socket_t               desc                =                -1;
  ntbx_host_id_t                  remote_host_id;  
  
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
			configuration->host_name[connection->remote_host_id]);

  SYSCALL(connect(desc, (struct sockaddr *)&server_address, 
		  sizeof(ntbx_tcp_address_t)));

  ntbx_tcp_socket_setup(desc);
  SYSCALL(write(desc,
		&(connection->channel->adapter->driver->madeleine->
		  configuration->local_host_id), 
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
    channel->adapter->driver->madeleine->configuration;
  
  LOG_IN();
  mad_tcp_sync_in_channel(channel);

  for (host = 0; host < configuration->size; host++)
#ifdef MAD_FORWARDING
    if(channel->input_connection[host].active)
#endif
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
  adapter_specific->remote_connection_port = NULL;
  
  TBX_FREE(adapter_specific);
  adapter->specific = NULL;

  TBX_FREE(adapter->parameter);
  adapter->parameter = NULL;

  if (adapter->master_parameter)
    {
      TBX_FREE(adapter->master_parameter);
      adapter->master_parameter = NULL;
    }

  TBX_FREE(adapter->name);
  adapter->name = NULL;
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_tcp_poll_message(p_mad_channel_t channel)
{
  p_mad_configuration_t        configuration    = 
    channel->adapter->driver->madeleine->configuration;
  p_mad_tcp_channel_specific_t channel_specific = channel->specific;
  fd_set rfds;
  int n;
  int i;

  LOG_IN();
  do
    {
      struct timeval timeout;

      rfds = channel_specific->read_fds;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;
		
      n = select(channel_specific->max_fds + 1,
		 &rfds, NULL,
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
#ifdef MAD_FORWARDING
    if(channel->input_connection[i].active)
#endif 
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
  p_mad_configuration_t configuration = 
    channel->adapter->driver->madeleine->configuration;

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
			 &rfds, NULL, NULL, NULL);
#endif /* MARCEL */
	    }
	  while(n <= 0);

	  for (i = 0; i < configuration->size; i++)
#ifdef MAD_FORWARDING
	    if(channel->input_connection[i].active)
#endif //MAD_FORWARDING
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

