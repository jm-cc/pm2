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
 * Mad_tcp.c
 * =========
 */

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
  int dummy;
} mad_tcp_driver_specific_t, *p_mad_tcp_driver_specific_t;

typedef struct
{
  int  connection_socket;
  int  connection_port;
} mad_tcp_adapter_specific_t, *p_mad_tcp_adapter_specific_t;

typedef struct
{
  int                max_fds;
  fd_set             read_fds;
  fd_set             active_fds;
  int                active_number;
  tbx_darray_index_t last_idx;
} mad_tcp_channel_specific_t, *p_mad_tcp_channel_specific_t;

typedef struct
{
  int socket;
} mad_tcp_connection_specific_t, *p_mad_tcp_connection_specific_t;

typedef struct
{
  int dummy;
} mad_tcp_link_specific_t, *p_mad_tcp_link_specific_t;


/*
 * static functions
 * ----------------
 */
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
#endif // MARCEL 
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
#endif // MARCEL 
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
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  TRACE("Registering TCP driver");
  interface = driver->interface;
  
  driver->connection_type  = mad_bidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("tcp");
  
  interface->driver_init                = mad_tcp_driver_init;
  interface->adapter_init               = mad_tcp_adapter_init;
  interface->channel_init               = mad_tcp_channel_init;
  interface->before_open_channel        = mad_tcp_before_open_channel;
  interface->connection_init            = mad_tcp_connection_init;
  interface->link_init                  = mad_tcp_link_init;
  interface->accept                     = mad_tcp_accept;
  interface->connect                    = mad_tcp_connect;
  interface->after_open_channel         = mad_tcp_after_open_channel;
  interface->before_close_channel       = NULL; 
  interface->disconnect                 = mad_tcp_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = NULL;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = NULL;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_tcp_poll_message;
#endif // MAD_MESSAGE_POLLING 
  interface->receive_message            = mad_tcp_receive_message;
  interface->message_received           = NULL;
  interface->send_buffer                = mad_tcp_send_buffer;
  interface->receive_buffer             = mad_tcp_receive_buffer;
  interface->send_buffer_group          = mad_tcp_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_tcp_receive_sub_buffer_group;
  LOG_OUT();
}


void
mad_tcp_driver_init(p_mad_driver_t driver)
{
  p_mad_tcp_driver_specific_t driver_specific = NULL;

  LOG_IN();
  TRACE("Initializing TCP driver");
  driver_specific  = TBX_MALLOC(sizeof(mad_tcp_driver_specific_t));
  driver->specific = driver_specific;
  LOG_OUT();
}

void
mad_tcp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_tcp_adapter_specific_t adapter_specific = NULL;
  p_tbx_string_t               parameter_string = NULL;
  ntbx_tcp_address_t           address;

  LOG_IN();
  if (strcmp(adapter->dir_adapter->name, "default"))
    FAILURE("unsupported adapter");
  
  adapter_specific  = TBX_MALLOC(sizeof(mad_tcp_adapter_specific_t));
  adapter_specific->connection_port   = -1;
  adapter_specific->connection_socket = -1;
  adapter->specific                   = adapter_specific;
#ifdef SSIZE_MAX
  adapter->mtu                        = min(SSIZE_MAX, MAD_FORWARD_MAX_MTU);
#else // SSIZE_MAX
  adapter->mtu                        = MAD_FORWARD_MAX_MTU;
#endif // SSIZE_MAX

  adapter_specific->connection_socket =
    ntbx_tcp_socket_create(&address, 0);
  SYSCALL(listen(adapter_specific->connection_socket,
		 min(5, SOMAXCONN)));
  
  adapter_specific->connection_port =
    (ntbx_tcp_port_t)ntohs(address.sin_port);

  parameter_string   =
    tbx_string_init_to_int(adapter_specific->connection_port);
  adapter->parameter = tbx_string_to_cstring(parameter_string);
  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_tcp_channel_init(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific = NULL;
  
  LOG_IN();
  channel_specific  = TBX_MALLOC(sizeof(mad_tcp_channel_specific_t));
  channel->specific = channel_specific;
  LOG_OUT();
}

void
mad_tcp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_tcp_connection_specific_t specific = NULL;
  
  LOG_IN();
  specific     = TBX_MALLOC(sizeof(mad_tcp_connection_specific_t));
  in->specific = out->specific = specific;
  in->nb_link  = 1;
  out->nb_link = 1;
  LOG_OUT();
}

void 
mad_tcp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
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
  channel_specific          = channel->specific;
  channel_specific->max_fds = 0;  
  FD_ZERO(&(channel_specific->read_fds));  
  LOG_OUT();
}

void
mad_tcp_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t adapter_info TBX_UNUSED)
{
  p_mad_channel_t                 channel             = NULL;
  p_mad_adapter_t                 adapter             = NULL;
  p_mad_tcp_adapter_specific_t    adapter_specific    = NULL;
  p_mad_tcp_connection_specific_t connection_specific = NULL; 
  ntbx_tcp_socket_t               desc                =   -1;
  
  LOG_IN();
  connection_specific = in->specific;
  channel             = in->channel;
  adapter             = channel->adapter;
  adapter_specific    = adapter->specific;

  SYSCALL(desc = accept(adapter_specific->connection_socket, NULL, NULL));
  ntbx_tcp_socket_setup(desc);
  
  connection_specific->socket = desc;
  LOG_OUT();

}

void
mad_tcp_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t adapter_info)
{
  p_mad_tcp_connection_specific_t   connection_specific   = NULL;
  p_mad_adapter_t                   adapter               = NULL;
  p_mad_tcp_adapter_specific_t      adapter_specific      = NULL;
  p_mad_dir_node_t                  remote_node           = NULL;
  p_mad_dir_adapter_t               remote_adapter        = NULL;
  ntbx_tcp_port_t                   remote_port           =    0;
  ntbx_tcp_socket_t                 sock                  =   -1;
  ntbx_tcp_address_t                remote_address;
  
  LOG_IN();
  connection_specific = out->specific;
  adapter             = out->channel->adapter;
  adapter_specific    = adapter->specific;
  remote_node         = adapter_info->dir_node;
  remote_adapter      = adapter_info->dir_adapter;
  remote_port         = atoi(remote_adapter->parameter);
  sock                = ntbx_tcp_socket_create(NULL, 0);

  ntbx_tcp_address_fill(&remote_address, remote_port, remote_node->name);

  SYSCALL(connect(sock, (struct sockaddr *)&remote_address, 
		  sizeof(ntbx_tcp_address_t)));
  
  ntbx_tcp_socket_setup(sock);
  connection_specific->socket = sock;
  LOG_OUT();
}

void
mad_tcp_after_open_channel(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t  channel_specific = NULL;
  p_mad_dir_channel_t           dir_channel      = NULL;
  p_ntbx_process_container_t    pc               = NULL;
  tbx_darray_index_t            idx              =    0;
  p_tbx_darray_t                in_darray        = NULL;
  p_mad_connection_t            in               = NULL;
  fd_set                       *read_fds         = NULL;
  int                           max_fds          =    0;

  LOG_IN();
  channel_specific = channel->specific;
  dir_channel      = channel->dir_channel;
  in_darray        = channel->in_connection_darray;
  pc               = dir_channel->pc;
  read_fds         = &(channel_specific->read_fds);

  in = tbx_darray_first_idx(in_darray, &idx);
  while (in)
    {
      p_mad_tcp_connection_specific_t in_specific = NULL;
      
      in_specific = in->specific;
      
      FD_SET(in_specific->socket, read_fds);
      max_fds = max(in_specific->socket, max_fds);

      in = tbx_darray_next_idx(in_darray, &idx);
    }
  
  channel_specific->max_fds       = max_fds;
  channel_specific->active_number =  0;
  channel_specific->last_idx      = -1;
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

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_tcp_poll_message(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific = NULL;
  p_tbx_darray_t               in_darray        = NULL;
  p_ntbx_process_container_t   pc               = NULL;
  p_mad_connection_t           in               = NULL;
  
  LOG_IN();
  channel_specific = channel->specific;
  pc               = channel->dir_channel->pc;
  in_darray        = channel->in_connection_darray;

  if (!channel_specific->active_number)
    {
      int status = -1;
      struct timeval timeout;

      channel_specific->active_fds = channel_specific->read_fds;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;

      status = select(channel_specific->max_fds + 1,
		      &channel_specific->active_fds,
		      NULL, NULL, &timeout);
      

      if ((status == -1) && (errno != EINTR))
	{
	  perror("select");
	  FAILURE("system call failed");
	}

      if (status <= 0)
	{
	  LOG_OUT();
	  return NULL;
	}
 
      channel_specific->active_number = status;

      in = tbx_darray_first_idx(in_darray, &channel_specific->last_idx);
   }
  else
    {
      in = tbx_darray_next_idx(in_darray, &channel_specific->last_idx);
    }

  while (in)
    {
      p_mad_tcp_connection_specific_t in_specific = NULL;
      
      in_specific = in->specific;
      
      if (FD_ISSET(in_specific->socket, &channel_specific->active_fds))
	{
	  channel_specific->active_number--;
	  
	  LOG_OUT();
	  
	  return in;
	}      

      in = tbx_darray_next_idx(in_darray, &channel_specific->last_idx);
    }
  
  FAILURE("invalid channel state");
}
#endif // MAD_MESSAGE_POLLING 

p_mad_connection_t
mad_tcp_receive_message(p_mad_channel_t channel)
{
  p_mad_tcp_channel_specific_t channel_specific = NULL;
  p_tbx_darray_t               in_darray        = NULL;
  p_ntbx_process_container_t   pc               = NULL;
  p_mad_connection_t           in               = NULL;
  
  LOG_IN();
  channel_specific = channel->specific;
  pc               = channel->dir_channel->pc;
  in_darray        = channel->in_connection_darray;

  if (!channel_specific->active_number)
    {
      int status = -1;

      do
	{
	  channel_specific->active_fds = channel_specific->read_fds;

#ifdef MARCEL
#ifdef USE_MARCEL_POLL
	  status = marcel_select(channel_specific->max_fds + 1,
				 &channel_specific->active_fds,
				 NULL);
              
#else // USE_MARCEL_POLL 
	  status = tselect(channel_specific->max_fds + 1,
			   &channel_specific->active_fds,
			   NULL, NULL);
	  if (status <= 0)
	    {
	      TBX_YIELD();
	    }
#endif // USE_MARCEL_POLL   
#else // MARCEL 
	  status = select(channel_specific->max_fds + 1,
			  &channel_specific->active_fds,
			  NULL, NULL, NULL);
#endif // MARCEL

	  if ((status == -1) && (errno != EINTR))
	    {
	      perror("select");
	      FAILURE("system call failed");
	    }
	}
      while (status <= 0);
 
      channel_specific->active_number = status;

      in = tbx_darray_first_idx(in_darray, &channel_specific->last_idx);
   }
  else
    {
      in = tbx_darray_next_idx(in_darray, &channel_specific->last_idx);
    }

  while (in)
    {
      p_mad_tcp_connection_specific_t in_specific = NULL;
      
      in_specific = in->specific;
      
      if (FD_ISSET(in_specific->socket, &channel_specific->active_fds))
	{
	  channel_specific->active_number--;
	  
	  LOG_OUT();
	  
	  return in;
	}      

      in = tbx_darray_next_idx(in_darray, &channel_specific->last_idx);
    }
  
  FAILURE("invalid channel state");
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

