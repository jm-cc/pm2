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
 * Mad_vrp.c
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
#include <VRP.h>

DEBUG_DECLARE(vrp)

#undef DEBUG_NAME
#define DEBUG_NAME vrp

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_vrp_driver_specific
{
  int dummy;
} mad_vrp_driver_specific_t, *p_mad_vrp_driver_specific_t;

typedef struct s_mad_vrp_adapter_specific
{
  int  connection_socket;
  int  connection_port;
} mad_vrp_adapter_specific_t, *p_mad_vrp_adapter_specific_t;

typedef struct s_mad_vrp_channel_specific
{
  int dummy;
} mad_vrp_channel_specific_t, *p_mad_vrp_channel_specific_t;

typedef struct s_mad_vrp_in_connection_specific
{
  int            socket;
  int            port;
  vrp_incoming_t vrp_in;
} mad_vrp_in_connection_specific_t, *p_mad_vrp_in_connection_specific_t;

typedef struct s_mad_vrp_out_connection_specific
{
  int            socket;
  int            port;
  vrp_outgoing_t vrp_out;
} mad_vrp_out_connection_specific_t, *p_mad_vrp_out_connection_specific_t;

typedef struct s_mad_vrp_link_specific
{
  int dummy;
} mad_vrp_link_specific_t, *p_mad_vrp_link_specific_t;


/*
 * static functions
 * ----------------
 */

/** none **/


#undef DEBUG_NAME
#define DEBUG_NAME mad3


/*
 * Registration function
 * ---------------------
 */

void
mad_vrp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
  DEBUG_INIT(vrp);
#endif // DEBUG

  LOG_IN();
  TRACE("Registering VRP driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("vrp");

  interface->driver_init                = mad_vrp_driver_init;
  interface->adapter_init               = mad_vrp_adapter_init;
  interface->channel_init               = mad_vrp_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = mad_vrp_connection_init;
  interface->link_init                  = mad_vrp_link_init;
  interface->accept                     = mad_vrp_accept;
  interface->connect                    = mad_vrp_connect;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_vrp_disconnect;
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
  interface->poll_message               = mad_vrp_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = mad_vrp_receive_message;
  interface->message_received           = NULL;
  interface->send_buffer                = mad_vrp_send_buffer;
  interface->receive_buffer             = mad_vrp_receive_buffer;
  interface->send_buffer_group          = mad_vrp_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_vrp_receive_sub_buffer_group;
  LOG_OUT();
}


void
mad_vrp_driver_init(p_mad_driver_t d)
{
  p_mad_vrp_driver_specific_t ds = NULL;

  LOG_IN();
  TRACE("Initializing VRP driver");
  ds          = TBX_MALLOC(sizeof(mad_vrp_driver_specific_t));
  ds->dummy   = 0;
  d->specific = ds;
  LOG_OUT();
}

void
mad_vrp_adapter_init(p_mad_adapter_t a)
{
  p_mad_vrp_adapter_specific_t as    = NULL;
  p_tbx_string_t               param = NULL;
  ntbx_tcp_address_t           addr;

  LOG_IN();
  if (strcmp(a->dir_adapter->name, "default"))
    FAILURE("unsupported adapter");

  as  = TBX_MALLOC(sizeof(mad_vrp_adapter_specific_t));
  as->connection_port   = -1;
  as->connection_socket = -1;
  a->specific = as;

#ifdef SSIZE_MAX
  a->mtu = min(SSIZE_MAX, MAD_FORWARD_MAX_MTU);
#else // SSIZE_MAX
  a->mtu = MAD_FORWARD_MAX_MTU;
#endif // SSIZE_MAX

  as->connection_socket = ntbx_tcp_socket_create(&addr, 0);
  SYSCALL(listen(as->connection_socket, min(5, SOMAXCONN)));

  as->connection_port = (ntbx_tcp_port_t)ntohs(addr.sin_port);

  param = tbx_string_init_to_int(as->connection_port);
  a->parameter = tbx_string_to_cstring(param);
  tbx_string_free(param);
  param = NULL;
  LOG_OUT();
}

void
mad_vrp_channel_init(p_mad_channel_t c)
{
  p_mad_vrp_channel_specific_t cs = NULL;

  LOG_IN();
  cs          = TBX_MALLOC(sizeof(mad_vrp_channel_specific_t));
  cs->dummy   = 0;
  c->specific = cs;
  LOG_OUT();
}

void
mad_vrp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_vrp_in_connection_specific_t  is = NULL;
  p_mad_vrp_out_connection_specific_t os = NULL;

  LOG_IN();
  is = TBX_MALLOC(sizeof(mad_vrp_in_connection_specific_t));

  is->socket =   -1;
  is->port   =   -1;
  is->vrp_in = NULL;

  in->specific = is;
  in->nb_link  =  1;

  os = TBX_MALLOC(sizeof(mad_vrp_out_connection_specific_t));

  os->socket  =   -1;
  os->port    =   -1;
  os->vrp_out = NULL;

  out->specific = os;
  out->nb_link  =  1;
  LOG_OUT();
}

void
mad_vrp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  lnk->link_mode   = mad_link_mode_buffer_group;
  /* lnk->link_mode   = mad_link_mode_buffer; */
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_aggregate;
  LOG_OUT();
}

void
mad_vrp_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t ai TBX_UNUSED)
{
  p_mad_vrp_adapter_specific_t       as = NULL;
  p_mad_vrp_in_connection_specific_t is = NULL;
  ntbx_tcp_socket_t                  s  =   -1;

  LOG_IN();
  is = in->specific;
  as = in->channel->adapter->specific;

  SYSCALL(s = accept(as->connection_socket, NULL, NULL));

  /* ajouter appel à vrp_incoming_construct */
  LOG_OUT();

}

void
mad_vrp_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t ai)
{
  p_mad_vrp_out_connection_specific_t os        = NULL;
  p_mad_dir_node_t                    r_node    = NULL;
  p_mad_dir_adapter_t                 r_adapter = NULL;
  ntbx_tcp_port_t                     r_port    =    0;
  ntbx_tcp_socket_t                   s         =   -1;
  ntbx_tcp_address_t                  r_addr;

  LOG_IN();
  os        = out->specific;
  r_node    = ai->dir_node;
  r_adapter = ai->dir_adapter;
  r_port    = atoi(r_adapter->parameter);
  s         = ntbx_tcp_socket_create(NULL, 0);

#ifndef LEO_IP
  ntbx_tcp_address_fill(&r_addr, r_port, r_node->name);
#else // LEO_IP
  ntbx_tcp_address_fill_ip(&r_addr, r_port, &r_node->ip);
#endif // LEO_IP

  SYSCALL(connect(s, (struct sockaddr *)&r_addr,
		  sizeof(ntbx_tcp_address_t)));

  ntbx_tcp_socket_setup(s);

  /* ajouter appel à vrp_outgoing_construct */
  LOG_OUT();
}

void
mad_vrp_disconnect(p_mad_connection_t c)
{
  LOG_IN();
  if (c->way == mad_incoming_connection)
    {
      p_mad_vrp_in_connection_specific_t is = NULL;

      is = c->specific;

      /* ajouter code de deconnexion VRP */
    }
  else if (c->way == mad_outgoing_connection)
    {
      p_mad_vrp_out_connection_specific_t os = NULL;

      os = c->specific;

      /* ajouter code de deconnexion VRP */
    }
  else
    FAILURE("invalid connection way");

  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_vrp_poll_message(p_mad_channel_t ch)
{
  p_mad_vrp_channel_specific_t chs       = NULL;
  p_tbx_darray_t               in_darray = NULL;
  p_mad_connection_t           in        = NULL;

  LOG_IN();
  chs       = ch->specific;
  in_darray = ch->in_connection_darray;

  /* TODO */

  LOG_OUT();

  return NULL;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_vrp_receive_message(p_mad_channel_t ch)
{
  p_mad_vrp_channel_specific_t chs       = NULL;
  p_tbx_darray_t               in_darray = NULL;
  p_mad_connection_t           in        = NULL;

  LOG_IN();
  chs       = ch->specific;
  in_darray = ch->in_connection_darray;

  /* TODO */

  LOG_OUT();

  return in;
}

void
mad_vrp_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t b)
{
  p_mad_vrp_out_connection_specific_t os = NULL;

  LOG_IN();
  os = lnk->connection->specific;
  FAILURE("unimplemented");
  LOG_OUT();
}

void
mad_vrp_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *p_b)
{
  p_mad_vrp_in_connection_specific_t is = NULL;

  LOG_IN();
  is = lnk->connection->specific;
  FAILURE("unimplemented");
  LOG_OUT();
}

void
mad_vrp_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      p_mad_vrp_out_connection_specific_t os = NULL;
      tbx_list_reference_t                ref;

      os = lnk->connection->specific;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
          FAILURE("unimplemented");
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_vrp_receive_sub_buffer_group(p_mad_link_t         lnk,
                                 tbx_bool_t           first TBX_UNUSED,
				 p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      p_mad_vrp_in_connection_specific_t is = NULL;
      tbx_list_reference_t               ref;

      is = lnk->connection->specific;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
          FAILURE("unimplemented");
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

