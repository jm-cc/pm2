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
 * Mad_gm.c
 * ========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gm.h>


#undef  FAILURE_CONTEXT
#define FAILURE_CONTEXT "Mad/GM: "

DEBUG_DECLARE(gm)
#undef DEBUG_NAME
#define DEBUG_NAME gm

#define USER_MSG_LENGTH 1024
#define SYS_MSG_LENGTH    64
/*
 * Enums
 * -----
 */

typedef enum e_mad_gm_link_id
{
  default_link_id = 0,
  nb_link,
} mad_gm_link_id_t, *p_mad_gm_link_id_t;

typedef enum e_mad_gm_tag
{
  sys_tag = 0,
  user_tag,
} mad_gm_tag_id_t, *p_mad_gm_tag_id_t;

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_gm_driver_specific
{
  int dummy;
} mad_gm_driver_specific_t, *p_mad_gm_driver_specific_t;

typedef struct s_mad_gm_adapter_specific
{
  int device_id;
  int next_port_id;
} mad_gm_adapter_specific_t, *p_mad_gm_adapter_specific_t;

typedef struct s_mad_gm_channel_specific
{
  unsigned int    node_id;
  int             port_id;
  struct gm_port *port;
  void 		 *recv_sys_buffer;
  unsigned int    recv_sys_buffer_size;
  void           *recv_user_buffer;
  unsigned int    recv_user_buffer_size;
  volatile int    lock;
} mad_gm_channel_specific_t, *p_mad_gm_channel_specific_t;

typedef struct s_mad_gm_connection_specific
{
  int           remote_node_id;
  int  	        remote_port_id;
  void         *send_sys_buffer;
  unsigned int  send_sys_buffer_size;
} mad_gm_connection_specific_t, *p_mad_gm_connection_specific_t;

typedef struct s_mad_gm_link_specific
{
  void         *send_user_buffer;
  unsigned int  send_user_buffer_size;
} mad_gm_link_specific_t, *p_mad_gm_link_specific_t;


/*
 * static functions
 * ----------------
 */
#undef DEBUG_NAME
#define DEBUG_NAME mad3

static
void
mad_gm_control(gm_status_t gm_status)
{
  char *msg = NULL;

  LOG_IN();
  switch (gm_status)
    {
    GM_SUCCESS:
      break;

    GM_SEND_TIMED_OUT:
      msg = "send timed out";
      break;

    GM_SEND_REJECTED:
      msg = "send rejected";
      break;

    GM_SEND_TARGET_PORT_CLOSED:
      msg = "send target port closed";
      break;

    GM_SEND_TARGET_NODE_UNREACHABLE:
      msg = "send target node unreachable";
      break;

    GM_SEND_DROPPED:
      msg = "send dropped";
      break;

    GM_SEND_PORT_CLOSED:
      msg = "send port closed";
      break;

    default:
      msg = "unknown error";
      break;
    }

  if (msg)
    FAILURE(msg);

  LOG_OUT();
}

static
void
__mad_gm_send_callback(struct gm_port *port TBX_UNUSED,
		       void           *cntxt,
		       gm_status_t     gms)
{
  p_mad_gm_channel_specific_t chs = cntxt;

  LOG_IN();
  mad_gm_control(gms);
  os->lock = 0;
  LOG_OUT();
}

static
void
mad_gm_write(p_mad_link_t   l,
	     p_mad_buffer_t b)
{
  p_mad_gm_link_specific_t       ls  = NULL;
  p_mad_connection_t             out = NULL;
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;

  LOG_IN();
  ls  = l->specific;
  out = lnk->connection;
  os  = out->specific;
  ch  = out->channel;
  chs = ch->specific;

  while (mad_more_data(b))
    {
      unsigned int len = 0;

      len = min(USER_MSG_LENGTH, b->bytes_written - b->bytes_read);

      gm_bcopy(b->buffer + b->bytes_read, ls->send_user_buffer, len);

      while(chs->lock);
      chs->lock = 1;

      if (!gm_num_send_tokens (chs->port))
	FAILURE("not enough send tokens");

      gm_send_with_callback(chs->port,
			    ls->send_user_buffer,
			    ls->send_user_buffer_size,
			    len,
			    GM_LOW_PRIORITY,
			    os->remote_node_id,
			    os->remote_port_id,
			    __mad_gm_send_callback,
			    chs);
      b->bytes_read += len;
    }

  while(chs->lock);
  LOG_OUT();
}

static
void
mad_gm_read(p_mad_link_t   l,
	    p_mad_buffer_t b)
{
  p_mad_connection_t              in   = NULL;
  p_mad_gm_connection_specific_t  is   = NULL;
  p_mad_channel_t                 ch   = NULL;
  p_mad_gm_channel_specific_t     chs  = NULL;
  struct gm_port                 *port = NULL;
  gm_recv_event_t                *e    = NULL;

  LOG_IN();
  in   = l->connection;
  is   = in->specific;
  ch   = in->channel;
  chs  = ch->specific;
  port = chs->port;

  while (!mad_buffer_full(b))
    {
      while (1)
	{
	  e = gm_receive (port);
	  switch (gm_ntohc (e->recv.type))
	    {
	    case GM_FAST_PEER_RECV_EVENT:
	    case GM_FAST_RECV_EVENT:
	      if (e->recv.buffer != chs->recv_user_buffer)
		FAILURE("unexpected message");

	      if (e->tag != user_tag)
		FAILURE("unexpected message");

	      gm_bcopy(gm_ntohp(e->recv.message),
		       b->buffer + b->bytes_written,
		       gm_ntohl(e->recv.length));

	      b->bytes_written += gm_ntohl(e->recv.length);
	      goto reception;

	      break;


	    case GM_PEER_RECV_EVENT:
	    case GM_RECV_EVENT:
	      if (e->recv.buffer != chs->recv_user_buffer)
		FAILURE("unexpected message");

	      if (e->tag != user_tag)
		FAILURE("unexpected message");

	      gm_bcopy(gm_ntohp(e->recv.buffer),
		       b->buffer + b->bytes_written,
		       gm_ntohl(e->recv.length));

	      b->bytes_written += gm_ntohl(e->recv.length);
	      goto reception;

	      break;


	    case GM_NO_RECV_EVENT:
	      break;

	    default:
	      gm_unknown (port, e);
	      break;
	    }
	}

    reception:
      if (gm_num_receive_tokens())
	{
	  gm_provide_receive_buffer_with_tag(chs->port,
					     chs->recv_user_buffer,
					     chs->recv_user_buffer_size,
					     GM_LOW_PRIORITY,
					     user_tag);
	}
      else
	FAILURE("not enough receive tokens");
    }
  LOG_OUT();
}


/*
 * Registration function
 * ---------------------
 */

void
mad_gm_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
  DEBUG_INIT(gm);
#endif // DEBUG

  LOG_IN();
  TRACE("Registering GM driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 1;
  driver->name             = tbx_strdup("gm");

  interface->driver_init                = mad_gm_driver_init;
  interface->adapter_init               = mad_gm_adapter_init;
  interface->channel_init               = mad_gm_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = mad_gm_connection_init;
  interface->link_init                  = mad_gm_link_init;
  interface->accept                     = mad_gm_accept;
  interface->connect                    = mad_gm_connect;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_gm_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = mad_gm_channel_exit;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = mad_gm_driver_exit;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_gm_new_message;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_gm_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = mad_gm_receive_message;
  interface->message_received           = mad_gm_message_received;
  interface->send_buffer                = mad_gm_send_buffer;
  interface->receive_buffer             = mad_gm_receive_buffer;
  interface->send_buffer_group          = mad_gm_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_gm_receive_sub_buffer_group;
  LOG_OUT();
}


void
mad_gm_driver_init(p_mad_driver_t d)
{
  p_mad_gm_driver_specific_t ds  = NULL;
  gm_status_t                gms = GM_SUCCESS;

  LOG_IN();
  TRACE("Initializing GM driver");
  ds          = TBX_MALLOC(sizeof(mad_gm_driver_specific_t));
  d->specific = ds;
  gms         = gm_init();
  mad_gm_control(gms);
  LOG_OUT();
}

void
mad_gm_adapter_init(p_mad_adapter_t a)
{
  p_mad_gm_adapter_specific_t as        = NULL;
  p_tbx_string_t              param_str = NULL;
  int                         device_id =    0;

  LOG_IN();
  if (tbx_streq(adapter->dir_adapter->name, "default"))
    {
      device_id = 0;
    }
  else
    {
      device_id = strtol(adapter->dir_adapter->name, NULL, 0);
    }

  as               = TBX_MALLOC(sizeof(mad_gm_adapter_specific_t));
  as->device_id    = device_id;
  as->next_port_id = 2;
  a->specific      = as;

#if defined(USER_MSG_SIZE)
  a->mtu = USER_MSG_SIZE;
#else // defined(USER_MSG_SIZE)
  a->mtu = MAD_FORWARD_MAX_MTU;
#endif // defined(USER_MSG_SIZE)

  param_str    = tbx_string_init_to_int(device_id);
  a->parameter = tbx_string_to_cstring(param_str);
  tbx_string_free(param_str);
  param_str    = NULL;
  LOG_OUT();
}

void
mad_gm_channel_init(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t  chs       = NULL;
  p_mad_adapter_t              a         = NULL;
  p_mad_gm_adapter_specific_t  as        = NULL;
  p_tbx_string_t               param_str = NULL;
  unsigned int                 node_id   =    0;
  int                          port_id   =    0;
  gm_status_t                  gms  	 = GM_SUCCESS;
  struct gm_port              *port 	 = NULL;

  LOG_IN();
  a 	  	= ch->adapter;
  as      	= a->specific;
  port_id 	= as->next_port_id++;

  gms     	= gm_open(&port, as->device_id, port_id, channel->name);
  mad_gm_control(gms);

  {
    unsigned int size_s = 0;
    unsigned int size_u = 0;

    size_s = gm_min_size_for_length(SYS_MSG_LENGTH);
    size_u = gm_min_size_for_length(USER_MSG_LENGTH);

    gms = gm_set_acceptable_sizes(port, GM_LOW_PRIORITY, size_s|size_u);
    mad_gm_control(gms);
  }

  gms           = gm_get_node_id(port, &node_id);
  mad_gm_control(gms);

  chs           = TBX_MALLOC(sizeof(mad_gm_channel_specific_t));
  chs->node_id  = node_id;
  chs->port_id  = port_id;
  chs->port     = port;
  ch->specific  = chs;

  param_str     = tbx_string_init_to_int(port_id);
  tbx_string_append_char(param_str);
  tbx_string_append_uint(node_id)
  ch->parameter = tbx_string_to_cstring(param_str);
  tbx_string_free(param_str);
  param_str     = NULL;

  chs->recv_sys_buffer      = gm_dma_malloc(chs->port, SYS_MSG_LENGTH);
  chs->recv_sys_buffer_size = gm_min_size_for_length(SYS_MSG_LENGTH);

  if (gm_num_receive_tokens())
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_sys_buffer,
					 chs->recv_sys_buffer_size,
					 GM_LOW_PRIORITY,
					 sys_tag);
    }
  else
    FAILURE("not enough receive tokens");


  chs->recv_user_buffer      = gm_dma_malloc(chs->port, USER_MSG_LENGTH);
  chs->recv_user_buffer_size = gm_min_size_for_length(USER_MSG_LENGTH);

  if (gm_num_receive_tokens())
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_user_buffer,
					 chs->recv_user_buffer_size,
					 GM_LOW_PRIORITY,
					 user_tag);
    }
  else
    FAILURE("not enough receive tokens");

  LOG_OUT();
}

void
mad_gm_connection_init(p_mad_connection_t in,
		       p_mad_connection_t out)
{
  LOG_IN();
  if (in)
    {
      p_mad_gm_connection_specific_t is = NULL;

      is           = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
      in->specific = is;
      in->nb_link  = nb_link;
    }

  if (out)
    {
      p_mad_gm_connection_specific_t os = NULL;

      os            = TBX_CALLOC(1, sizeof(mad_gm_connection_specific_t));
      out->specific = os;
      out->nb_link  = nb_link;
    }
  LOG_OUT();
}

void
mad_gm_link_init(p_mad_link_t l)
{
  p_mad_gm_link_specific_t ls = NULL;

  LOG_IN();
  ls 		 = TBX_CALLOC(1, sizeof(mad_gm_link_specific_t));
  l->specific    = ls;
  l->link_mode   = mad_link_mode_buffer;
  l->buffer_mode = mad_buffer_mode_dynamic;
  l->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_gm_accept(p_mad_connection_t   in,
	      p_mad_adapter_info_t ai)
{
  p_mad_gm_connection_specific_t is  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_adapter_t                a   = NULL;
  p_mad_gm_adapter_specific_t    as  = NULL;
  p_mad_link_t                   l   = NULL;

  LOG_IN();
  is  = in->specific;
  ch  = in->channel;
  chs = ch->specific;
  a   = ch->adapter;
  as  = a->specific;

  {
    char *ptr1 = NULL;
    char *ptr2 = NULL;

    is->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
    if ((!is->remote_port_id) && (ai->channel_parameter == ptr1))
      FAILURE("could not extract the remote port id");

    is->remote_node_id  = strtoul(ptr1, &ptr2, 0);
    if ((!is->remote_node_id) && (ptr1 == ptr2))
      FAILURE("could not extract the remote node id");
  }
  LOG_OUT();
}

void
mad_gm_connect(p_mad_connection_t   out,
	       p_mad_adapter_info_t ai)
{
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_adapter_t                a   = NULL;
  p_mad_gm_adapter_specific_t    as  = NULL;
  p_mad_link_t                   l   = NULL;

  LOG_IN();
  os  = out->specific;
  ch  = out->channel;
  chs = ch->specific;
  a   = ch->adapter;
  as  = a->specific;

  {
    char *ptr1 = NULL;
    char *ptr2 = NULL;

    is->remote_port_id  = strtol(ai->channel_parameter, &ptr1, 0);
    if ((!is->remote_port_id) && (ai->channel_parameter == ptr1))
      FAILURE("could not extract the remote port id");

    is->remote_node_id  = strtoul(ptr1, &ptr2, 0);
    if ((!is->remote_node_id) && (ptr1 == ptr2))
      FAILURE("could not extract the remote node id");
  }

  os->send_sys_buffer      = gm_dma_malloc(chs->port, SYS_MSG_LENGTH);
  os->send_sys_buffer_size = gm_min_size_for_length(SYS_MSG_LENGTH);

  l = out->link_array[default_link_id];
  {
    p_mad_gm_link_specific_t ls = NULL;

    ls                        = l->specific;
    ls->send_user_buffer      = gm_dma_malloc(chs->port, USER_MSG_LENGTH);
    ls->send_user_buffer_size = gm_min_size_for_length(USER_MSG_LENGTH);
  }
  LOG_OUT();
}

void
mad_gm_disconnect(p_mad_connection_t cnx)
{
  p_mad_gm_connection_specific_t cs  = cnx->specific;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;
  p_mad_link_t                   l   = NULL;

  LOG_IN();
  ch  = cnx->channel;
  chs = ch->specific;
  if (cnx->way == mad_incoming_connection)
    {
      gm_dma_free(chs->port, chs->recv_sys_buffer);
      chs->recv_sys_buffer = NULL;
      gm_dma_free(chs->port, chs->recv_user_buffer);
      chs->recv_user_buffer = NULL;
    }
  else if (cnx->way == mad_outgoing_connection)
    {
      gm_dma_free(chs->port, cs->send_sys_buffer);
      cs->send_sys_buffer = NULL;

      l = cnx->link_array[default_link_id];
      {
	p_mad_gm_link_specific_t ls = NULL;

	ls                   = l->specific;
	gm_dma_free(chs->port, ls->send_user_buffer);
	ls->send_user_buffer = NULL;
      }
    }
  else
    FAILURE("invalid connection way");
  LOG_OUT();
}

void
mad_gm_new_message(p_mad_connection_t out)
{
  p_mad_gm_connection_specific_t os  = NULL;
  p_mad_channel_t                ch  = NULL;
  p_mad_gm_channel_specific_t    chs = NULL;

  LOG_IN();
  os  = out->specific;
  ch  = out->channel;
  chs = chs->specific;

  while(chs->lock);
  chs->lock = 1;

  if (!gm_num_send_tokens (chs->port))
    FAILURE("not enough send tokens");

  gm_send_with_callback(chs->port,
			os->send_sys_buffer,
			os->send_sys_buffer_size,
			SYS_MSG_LENGTH,
			GM_LOW_PRIORITY,
			os->remote_node_id,
			os->remote_port_id,
			__mad_gm_send_callback,
			chs);
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_gm_poll_message(p_mad_channel_t channel)
{
  LOG_IN();
  FAILURE("unimplemented");
  LOG_OUT();
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_gm_receive_message(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t  chs       = NULL;
  p_tbx_darray_t               in_darray = NULL;
  p_mad_connection_t           in        = NULL;
  struct gm_port              *port      = NULL;
  gm_recv_event_t             *e         = NULL;

  LOG_IN();
  chs = ch->specific;
  port = chs->port;

  while (1)
    {
      e = gm_receive (port);
      switch (gm_ntohc (e->recv.type))
	{
	case GM_FAST_PEER_RECV_EVENT:
	case GM_FAST_RECV_EVENT:
	  gm_memorize_message (gm_ntohp (e->recv.buffer),
			       gm_ntohp (e->recv.message),
			       gm_ntohl (e->recv.length));
	case GM_PEER_RECV_EVENT:
	case GM_RECV_EVENT:
	  if (e->recv.buffer != chs->recv_sys_buffer)
	    FAILURE("unexpected message");

	  if (e->tag != sys_tag)
	    FAILURE("unexpected message");

	  goto reception;

	  break;

	case GM_NO_RECV_EVENT:
	  break;

	default:
	  gm_unknown (port, e);
	  break;
	}
    }

 reception:
  in_darray = ch->in_connection_darray;
  in        = tbx_darray_first_idx(in_darray, &channel_specific->last_idx);

  while (in)
    {
      p_mad_gm_connection_specific_t is = NULL;

      is = in->specific;

      if ((is->remote_node_id == e->sender_node_id)
	  && (is->remote_port_id == e->sender_port_id))
	{
	  LOG_OUT();

	  return in;
	}

      in = tbx_darray_next_idx(in_darray, &channel_specific->last_idx);
    }

  FAILURE("invalid channel state");
}

void
mad_gm_message_received(p_mad_connection_t in)
{
  p_mad_channel_t             ch  = NULL;
  p_mad_gm_channel_specific_t chs = NULL;

  LOG_IN();
  ch  = in->channel;
  chs = ch->specific;

  if (gm_num_receive_tokens())
    {
      gm_provide_receive_buffer_with_tag(chs->port,
					 chs->recv_sys_buffer,
					 chs->recv_sys_buffer_size,
					 GM_LOW_PRIORITY,
					 sys_tag);
    }
  else
    FAILURE("not enough receive tokens");
  LOG_OUT();
}

void
mad_gm_send_buffer(p_mad_link_t   l,
		   p_mad_buffer_t b)
{
  LOG_IN();
  mad_gm_write(l, b);
  LOG_OUT();
}

void
mad_gm_receive_buffer(p_mad_link_t    l,
		      p_mad_buffer_t *b)
{
  LOG_IN();
  mad_gm_read(l, *b);
  LOG_OUT();
}

void
mad_gm_send_buffer_group_1(p_mad_link_t         l,
			   p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
	  mad_gm_write(l, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group_1(p_mad_link_t         l,
				  p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
	  mad_gm_read(l, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_gm_send_buffer_group(p_mad_link_t         l,
			 p_mad_buffer_group_t bg)
{
  LOG_IN();
  mad_gm_send_buffer_group_1(l, bg);
  LOG_OUT();
}

void
mad_gm_receive_sub_buffer_group(p_mad_link_t         l,
				tbx_bool_t           first_sub_group
				TBX_UNUSED,
				p_mad_buffer_group_t bg)
{
  LOG_IN();
  mad_gm_receive_sub_buffer_group_1(l, bg);
  LOG_OUT();
}



void
mad_channel_exit(p_mad_channel_t ch)
{
  p_mad_gm_channel_specific_t chs = NULL;

  LOG_IN();
  chs = ch->specific;
  gm_close(chs->port);
  chs->port = NULL;
  TBX_FREE(chs);
  ch->specific = chs = NULL;
  LOG_OUT();
}

void
mad_gm_driver_exit(p_mad_driver_t d)
{
  p_mad_gm_driver_specific_t ds = NULL;

  LOG_IN();
  gm_finalize();
  ds = d->specific;
  TBX_FREE(ds);
  driver->specific = ds = NULL;
  LOG_OUT();
}
