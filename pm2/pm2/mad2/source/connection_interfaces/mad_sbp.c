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
*/

/*
 * Mad_sbp.c
 * =========
 */
/* #define DEBUG */

/*#define SOFTWARE_FLOW_CONTROL*/
#include <madeleine.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dedbuff.h>
#include <sbp.h>
#include <sbp_messages.h>

/*
 * macros and constants definition
 * -------------------------------
 */
#define MAD_SBP_INITIAL_DYNAMIC_BUFFER_POOL_SIZE 256
#define MAD_SBP_INITIAL_LIST_ELEMENT_COUNT       256

#define MAD_SBP_SBP_HEADER_SIZE SBP_HEADER_SIZE
#define MAD_SBP_MAD_HEADER_SIZE sizeof(mad_sbp_mad_header_t)
#define MAD_SBP_HEADER_SIZE  MAD_SBP_SBP_HEADER_SIZE + MAD_SBP_MAD_HEADER_SIZE
#define MAD_SBP_PAYLOAD_SIZE SBP_PAYLOAD_SIZE - MAD_SBP_MAD_HEADER_SIZE

/*
 * type definition
 * ---------------
 */
typedef unsigned long   mad_sbp_key_t,          *p_mad_sbp_key_t;
typedef unsigned long   mad_sbp_message_id_t,   *p_mad_sbp_message_id_t;
typedef unsigned char   mad_sbp_channel_id_t,   *p_mad_sbp_channel_id_t;
typedef unsigned char   mad_sbp_message_type_t, *p_mad_sbp_message_type_t;

typedef enum
{
  mad_sbp_REQUEST,
  mad_sbp_ACK,
  mad_sbp_DATA,
  mad_sbp_INTERNAL
} mad_sbp_message_type_constant_t, p_mad_sbp_message_type_constant_t;

/*
 * local structures
 * ----------------
 */
typedef struct 
{
  mad_sbp_channel_id_t   channel_id;
  mad_sbp_message_type_t message_type;  
} mad_sbp_mad_header_t, *p_mad_sbp_mad_header_t;

typedef SbpHeader_t mad_sbp_sbp_header_t, *p_mad_sbp_sbp_header_t;

typedef struct
{
  mad_sbp_sbp_header_t sbp_header;
  mad_sbp_mad_header_t mad_header;
  char                 payload[MAD_SBP_PAYLOAD_SIZE];
} mad_sbp_frame_t, *p_mad_sbp_frame_t;

typedef struct s_mad_sbp_list_element *p_mad_sbp_list_element_t;
typedef struct s_mad_sbp_list_element 
{
  p_mad_sbp_frame_t        frame;
  p_mad_sbp_list_element_t next;
} mad_sbp_list_element_t;

typedef struct
{
  p_mad_memory_t list_element_memory;
  p_mad_memory_t buffer_pool_memory;
  int            nb_adapter;
} mad_sbp_driver_specific_t, *p_mad_sbp_driver_specific_t;

typedef struct
{
  PM2_SHARED;
  mad_sbp_key_t            input_key;
  mad_sbp_key_t            output_key;
  int                      nb_in_buffers;
  int                      nb_out_buffers;
  unsigned long            vnId1;
  unsigned long            vnId2;
  p_mad_sbp_list_element_t unknown_frames_head;
  p_mad_sbp_list_element_t unknown_frames_tail;
} mad_sbp_adapter_specific_t, *p_mad_sbp_adapter_specific_t;

typedef struct
{
} mad_sbp_channel_specific_t, *p_mad_sbp_channel_specific_t;

typedef struct
{
  p_mad_sbp_list_element_t incoming_frames_head;
  p_mad_sbp_list_element_t incoming_frames_tail;
  mad_bool_t               request_status;
#ifdef SOFTWARE_FLOW_CONTROL
  mad_bool_t               acknowledgement_status;
#endif /* SOFTWARE_FLOW_CONTROL */
} mad_sbp_connection_specific_t, *p_mad_sbp_connection_specific_t;

typedef struct
{
} mad_sbp_link_specific_t, *p_mad_sbp_link_specific_t;

/*
 * static functions
 * ----------------
 */
static p_mad_sbp_frame_t
mad_sbp_get_full_frame(mad_sbp_key_t key)
{
  unsigned long frame_address = 0;

  LOG_IN();
  frame_address = pickupfullbuffer(key);
  LOG_OUT();
  return (p_mad_sbp_frame_t)frame_address;
}

static p_mad_sbp_frame_t
mad_sbp_get_empty_frame(mad_sbp_key_t key)
{
  unsigned long frame_address = 0;

  LOG_IN();
  frame_address = pickupemptybuffer(key);
  LOG_OUT();
  return (p_mad_sbp_frame_t)frame_address;
}

static void
mad_sbp_put_full_frame(p_mad_sbp_frame_t frame,
		       mad_sbp_key_t     key)
{
  LOG_IN();
  PM2_LOCK();
  returnfullbuffer(key, (unsigned long)frame);
  PM2_UNLOCK();
  LOG_OUT();
}

static void
mad_sbp_put_empty_frame(p_mad_sbp_frame_t frame,
			mad_sbp_key_t     key)
{
  LOG_IN();
  PM2_LOCK();
  returnemptybuffer(key, (unsigned long)frame);
  PM2_UNLOCK();
  LOG_OUT();
}

static p_mad_sbp_frame_t
mad_sbp_receive_sbp_frame(p_mad_connection_t connection,
			  mad_bool_t         probe_only)
{
  p_mad_sbp_connection_specific_t connection_specific = connection->specific;
  p_mad_sbp_list_element_t        element             = NULL;
  p_mad_channel_t                 channel             = connection->channel;
  p_mad_adapter_t                 adapter             = channel->adapter;
  p_mad_sbp_adapter_specific_t    adapter_specific    = adapter->specific;
  p_mad_driver_t                  driver              = adapter->driver;
  p_mad_sbp_driver_specific_t     driver_specific     = driver->specific;

  LOG_IN();
  
  while (mad_true)
    {
      PM2_LOCK_SHARED(adapter_specific);
      if ((element = connection_specific->incoming_frames_head))
	{
	  p_mad_sbp_frame_t frame = element->frame;

	  if (!probe_only)
	    {
	      connection_specific->incoming_frames_head = element->next;
	  
	      if (element == connection_specific->incoming_frames_tail)
		{
		  connection_specific->incoming_frames_tail = NULL;
		}
	  
	      mad_free(driver_specific->list_element_memory, element);
	    }

	  PM2_UNLOCK_SHARED(adapter_specific);
	  LOG_OUT();
	  return frame;
	}
      else
	{    
	  p_mad_sbp_frame_t               frame;
	  mad_host_id_t                   origin;
	  mad_sbp_channel_id_t            channel_id;
	  mad_sbp_message_type_t          message_type;
	  p_mad_channel_t                 tmp_channel;
	  p_mad_connection_t              tmp_connection;
	  p_mad_sbp_connection_specific_t tmp_connection_specific;
	  p_mad_sbp_list_element_t        element;
	  
	  frame        = mad_sbp_get_full_frame(adapter_specific->input_key);
	  PM2_LOCK_SHARED(adapter);
	  origin       = frame->sbp_header.cookie.origin;
	  channel_id   = frame->mad_header.channel_id;
	  message_type = frame->mad_header.message_type;
	  tmp_channel  = mad_get_channel(channel_id);

	  if (tmp_channel)
	    {
	      if (   (message_type == mad_sbp_REQUEST)
		  || (message_type == mad_sbp_DATA))
		{
		  tmp_connection = &(tmp_channel->input_connection[origin]);
		}
#ifdef SOFTWARE_FLOW_CONTROL
	      else if (message_type == mad_sbp_ACK)
		{
		  tmp_connection = &(tmp_channel->output_connection[origin]);
		}
#endif /* SOFTWARE_FLOW_CONTROL */
	      else
		FAILURE("invalid message type");
	  
	      if ((!probe_only) && (tmp_connection == connection))
		{
		  PM2_UNLOCK_SHARED(adapter);
		  PM2_UNLOCK_SHARED(adapter_specific);
		  LOG_OUT();
		  return frame;
		}
	      
	      tmp_connection_specific = tmp_connection->specific;
	      element = mad_malloc(driver_specific->list_element_memory);
	      element->frame = frame;
	      element->next  = NULL;
	      
	      if (tmp_connection_specific->incoming_frames_head)
		{
		  tmp_connection_specific->incoming_frames_tail->next =
		    element;
		}
	      else
		{
		  tmp_connection_specific->incoming_frames_head = element;
		}

	      tmp_connection_specific->incoming_frames_tail = element;
	    }
	  else
	    {
	      element = mad_malloc(driver_specific->list_element_memory);
	      element->frame = frame;
	      element->next  = NULL;
	      
	      if (adapter_specific->unknown_frames_tail)
		{
		  adapter_specific->unknown_frames_tail->next = element;
		}
	      else
		{
		  adapter_specific->unknown_frames_head = element;
		}
	      
	      adapter_specific->unknown_frames_tail = element;	  
	    }
	}

      PM2_UNLOCK_SHARED(adapter);
      PM2_UNLOCK_SHARED(adapter_specific);
      if (probe_only)
	{
	  LOG_OUT();
	  return NULL;
	}

      PM2_YIELD();
    }
}

/*
 * exported functions
 * ------------------
 */

/* Registration function */

void
mad_sbp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  /* Driver module registration code */
  interface = &(driver->interface);
  
  driver->connection_type = mad_unidirectional_connection;

  /* Not used for now, but might be used in the future for
     dynamic buffer allocation */
  driver->buffer_alignment = 32;
  
  interface->driver_init                = mad_sbp_driver_init;
  interface->adapter_init               = mad_sbp_adapter_init;
  interface->adapter_configuration_init = mad_sbp_adapter_configuration_init;
  interface->channel_init               = mad_sbp_channel_init;
  interface->before_open_channel        = mad_sbp_before_open_channel;
  interface->connection_init            = mad_sbp_connection_init;
  interface->link_init                  = mad_sbp_link_init;
  interface->accept                     = mad_sbp_accept;
  interface->connect                    = mad_sbp_connect;
  interface->after_open_channel         = mad_sbp_after_open_channel;
  interface->before_close_channel       = mad_sbp_before_close_channel;
  interface->disconnect                 = mad_sbp_disconnect;
  interface->after_close_channel        = mad_sbp_after_close_channel;
  interface->link_exit                  = mad_sbp_link_exit;
  interface->connection_exit            = mad_sbp_connection_exit;
  interface->channel_exit               = mad_sbp_channel_exit;
  interface->adapter_exit               = mad_sbp_adapter_exit;
  interface->driver_exit                = mad_sbp_driver_exit;
  interface->choice                     = mad_sbp_choice;
  interface->get_static_buffer          = mad_sbp_get_static_buffer;
  interface->return_static_buffer       = mad_sbp_return_static_buffer;
  interface->new_message                = mad_sbp_new_message;
  interface->receive_message            = mad_sbp_receive_message;
  interface->send_buffer                = mad_sbp_send_buffer;
  interface->receive_buffer             = mad_sbp_receive_buffer;
  interface->send_buffer_group          = mad_sbp_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_sbp_receive_sub_buffer_group;
  interface->external_spawn_init        = mad_sbp_external_spawn_init;
  interface->configuration_init         = mad_sbp_configuration_init;
  interface->send_adapter_parameter     = mad_sbp_send_adapter_parameter;
  interface->receive_adapter_parameter  = mad_sbp_receive_adapter_parameter;;
  LOG_OUT();
}

void
mad_sbp_driver_init(p_mad_driver_t driver)
{
  p_mad_sbp_driver_specific_t driver_specific;

  LOG_IN();
  /* Driver module initialization code
     Note: called only once, just after
     module registration */
  driver_specific = malloc(sizeof(mad_sbp_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;

  mad_malloc_init(&(driver_specific->buffer_pool_memory),
		  MAD_SBP_PAYLOAD_SIZE,
		  MAD_SBP_INITIAL_DYNAMIC_BUFFER_POOL_SIZE);
  mad_malloc_init(&(driver_specific->list_element_memory),
		  sizeof(mad_sbp_list_element_t),
		  MAD_SBP_INITIAL_LIST_ELEMENT_COUNT);
  LOG_OUT();
}

void
mad_sbp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver;
  p_mad_sbp_driver_specific_t    driver_specific;
  p_mad_sbp_adapter_specific_t   adapter_specific;
  
  LOG_IN();
  /* Adapter initialization code (part I)
     Note: called once for each adapter */
  driver          = adapter->driver;
  driver_specific = driver->specific;
  if (driver_specific->nb_adapter)
    FAILURE("SBP adapter already initialized");
  
  if (adapter->name == NULL)
    {
      adapter->name = malloc(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "SBP%d", driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;

  adapter_specific = malloc(sizeof(mad_sbp_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter_specific->unknown_frames_head = NULL;
  adapter_specific->unknown_frames_tail = NULL;
  adapter->specific = adapter_specific;
  PM2_INIT_SHARED(adapter_specific);

  adapter->parameter = malloc(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "-"); 
  LOG_OUT();
}

void
mad_sbp_adapter_configuration_init(p_mad_adapter_t adapter)
{
  LOG_IN();
  /* Adapter initialization code (part II)
     Note: called once for each adapter,
     after remote nodes connection information
     has been transmitted (if needed) */
  LOG_OUT();
}

void
mad_sbp_channel_init(p_mad_channel_t channel)
{
  LOG_IN();
  /* Channel initialization code
     Note: called once for each new channel */
  LOG_OUT();
}

void
mad_sbp_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_sbp_connection_specific_t in_specific;
  p_mad_sbp_connection_specific_t out_specific;
  
  LOG_IN();
  /* Point-to-point connection initialization code
     Note: called once for each connection pair during
     channel opening */
  in_specific = malloc(sizeof(mad_sbp_connection_specific_t));
  CTRL_ALLOC(in_specific);
  in_specific->incoming_frames_head = NULL;
  in_specific->incoming_frames_tail = NULL;
  in->specific = in_specific;
  in->nb_link  = 1;

  out_specific = malloc(sizeof(mad_sbp_connection_specific_t));
  CTRL_ALLOC(out_specific);
  out_specific->incoming_frames_head = NULL;
  out_specific->incoming_frames_tail = NULL;
  out->specific = out_specific;
  out->nb_link  = 1;
  LOG_OUT();
}

void 
mad_sbp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  /* Link initialization code */
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_static;
  LOG_OUT();
}

void
mad_sbp_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute before opening a new channel */

  /* Nothing */
  LOG_OUT();
}

void
mad_sbp_accept(p_mad_channel_t channel)
{
  LOG_IN();
  /* Incoming connection acceptation */

  /* Nothing */
  LOG_OUT();
}

void
mad_sbp_connect(p_mad_connection_t connection)
{
  LOG_IN();
  /* Outgoing connection establishment */

  /* Nothing */
  LOG_OUT();
}

void
mad_sbp_after_open_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t              adapter          = channel->adapter;
  p_mad_sbp_adapter_specific_t adapter_specific = adapter->specific;
  p_mad_sbp_list_element_t     element          = NULL;
  p_mad_sbp_list_element_t     *p_element       = NULL;
  
  p_mad_driver_t               driver           = adapter->driver;
  p_mad_sbp_driver_specific_t  driver_specific  = driver->specific;
  
  LOG_IN();
  /* Code to execute after opening a channel */

  p_element = &(adapter_specific->unknown_frames_head);
  element = adapter_specific->unknown_frames_head;
  
  while(element)
    {
      p_mad_sbp_frame_t frame = element->frame;

      if (frame->mad_header.channel_id == channel->id)
	{
	  p_mad_sbp_list_element_t        next_element = element->next;
	  mad_host_id_t                   origin;
	  mad_sbp_message_type_t          message_type;
	  p_mad_connection_t              tmp_connection;
	  p_mad_sbp_connection_specific_t tmp_connection_specific;
	  p_mad_sbp_list_element_t            tmp_element;

	  origin       = frame->sbp_header.cookie.origin;
	  message_type = frame->mad_header.message_type;

	  if (   (message_type == mad_sbp_REQUEST)
		  || (message_type == mad_sbp_DATA))
	    {
	      tmp_connection = &(channel->input_connection[origin]);
	    }
#ifdef SOFTWARE_FLOW_CONTROL
	  else if (message_type == mad_sbp_ACK)
	    {
	      tmp_connection = &(channel->output_connection[origin]);
	    }
#endif /* SOFTWARE_FLOW_CONTROL */
	  else
	    FAILURE("invalid message type");
	

	  tmp_connection_specific = tmp_connection->specific;
	  tmp_element = mad_malloc(driver_specific->list_element_memory);
	  tmp_element->frame = frame;
	  tmp_element->next  = NULL;
      
	  if (tmp_connection_specific->incoming_frames_head)
	    {
	      tmp_connection_specific->incoming_frames_tail->next = 
		tmp_element;
	    }
	  else
	    {
	      tmp_connection_specific->incoming_frames_head = tmp_element;
	    }
      
	  tmp_connection_specific->incoming_frames_tail = tmp_element;

	  mad_free(driver_specific->list_element_memory, element);
	  *p_element = next_element;
	  element = next_element;
	}
      else
	{
	  p_element = &(element->next);
	  element = element->next;
	}
    }
  
  if (!adapter_specific->unknown_frames_head)
    {
      adapter_specific->unknown_frames_tail = NULL;
    }  
  LOG_OUT();
}

void
mad_sbp_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute before closing a channel */
  LOG_OUT();
}

void 
mad_sbp_disconnect(p_mad_connection_t connection)
{
  p_mad_sbp_connection_specific_t connection_specific = connection->specific;
  p_mad_sbp_list_element_t        element             =
    connection_specific->incoming_frames_head;
  p_mad_sbp_adapter_specific_t     adapter_specific    =
    connection->channel->adapter->specific;
  p_mad_sbp_driver_specific_t     driver_specific     =
    connection->channel->adapter->driver->specific;
  
  LOG_IN();
  /* Code to execute to close a connection */
  while (element)
    {
      p_mad_sbp_list_element_t next_element = element->next;
      p_mad_sbp_frame_t        sbp_frame    = element->frame;
      
      mad_sbp_put_empty_frame(sbp_frame, adapter_specific->input_key);
      mad_free(driver_specific->list_element_memory, element);
      element = next_element;
    }
  
  connection_specific->incoming_frames_head =
    connection_specific->incoming_frames_tail = NULL;
  LOG_OUT();
}

void
mad_sbp_after_close_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                adapter          = channel->adapter;
  p_mad_sbp_adapter_specific_t   adapter_specific = adapter->specific;
  p_mad_driver_t                 driver           = adapter->driver;
  p_mad_sbp_driver_specific_t    driver_specific  = driver->specific;
  p_mad_sbp_list_element_t       element          =
    adapter_specific->unknown_frames_head;
  
  LOG_IN();
  /* Code to execute after a channel as been closed */
  
  while(element)
    {
      p_mad_sbp_list_element_t   next_element = element->next;
      p_mad_sbp_frame_t          sbp_frame    = element->frame;
      
      mad_sbp_put_empty_frame(sbp_frame, adapter_specific->input_key);
      mad_free(driver_specific->list_element_memory, element);
      element = next_element;
    }
  LOG_OUT();
}

void
mad_sbp_link_exit(p_mad_link_t link)
{
  LOG_IN();
  /* nothing */
  LOG_OUT();
}

void
mad_sbp_connection_exit(p_mad_connection_t in,
			p_mad_connection_t out)
{
  LOG_IN();
  free(in->specific);
  in->specific = NULL;
  free(out->specific);
  out->specific = NULL;
  LOG_OUT();
}

void
mad_sbp_channel_exit(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_sbp_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_sbp_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
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
mad_sbp_driver_exit(p_mad_driver_t driver)
{
  p_mad_sbp_driver_specific_t driver_specific = drive->specific;
  
  LOG_IN();
  mad_malloc_clean(driver_specific->list_element_memory);
  mad_malloc_clean(driver_specific->buffer_pool_memory);
  free(driver->specific);
  LOG_OUT();
}

p_mad_link_t
mad_sbp_choice(p_mad_connection_t   connection,
	       size_t               size,
	       mad_send_mode_t      send_mode,
	       mad_receive_mode_t   receive_mode)
{
  LOG_IN();
  /* Link selection function */

  LOG_OUT();
  return &(connection->link[0]);
}

void
mad_sbp_new_message(p_mad_connection_t connection)
{
  p_mad_sbp_connection_specific_t connection_specific =
    connection->specific;
  
  LOG_IN();
  /* Code to prepare a new message */

  connection_specific->request_status         = mad_false;
#ifdef SOFTWARE_FLOW_CONTROL
  connection_specific->acknowledgement_status = mad_false;
#endif /* SOFTWARE_FLOW_CONTROL */
  LOG_OUT();
}

p_mad_connection_t
mad_sbp_receive_message(p_mad_channel_t channel)
{
  p_mad_configuration_t           configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  p_mad_connection_t              connection;
  p_mad_sbp_connection_specific_t connection_specific;

  LOG_IN();
#ifndef PM2
  if (configuration->size == 2)
    {
      connection =
	&(channel->input_connection[1 - configuration->local_host_id]);
    }
  else
#endif /* PM2 */
    {
      static mad_host_id_t host_id = 0;

      /* Incoming communication detection code */      
      while (mad_true)
	{
	  if (host_id != configuration->local_host_id)
	    {
	      connection = &(channel->input_connection[host_id]);

	      if (mad_sbp_receive_sbp_frame(connection, mad_true))
		break;
	    }
	  else
	    {
	      PM2_YIELD();
	    }
	  
	  host_id = (host_id + 1) % configuration->size;
	}
    }

  connection_specific = connection->specific;
  connection_specific->request_status         = mad_false;
#ifdef SOFTWARE_FLOW_CONTROL
  connection_specific->acknowledgement_status = mad_false;
#endif /* SOFTWARE_FLOW_CONTROL */
  LOG_OUT();
  return connection;
}

void
mad_sbp_send_buffer(p_mad_link_t     lnk,
		    p_mad_buffer_t   buffer)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_sbp_connection_specific_t   connection_specific =
    lnk->connection->specific;
  p_mad_adapter_t                   adapter             =
    connection->channel->adapter;
  p_mad_sbp_adapter_specific_t      adapter_specific    =
    adapter->specific;
  p_mad_driver_t                    driver              = adapter->driver;
  p_mad_sbp_driver_specific_t       driver_specific     = driver->specific;
  p_mad_sbp_frame_t                 sbp_frame;
  
  LOG_IN();
  /* Code to send and free one static buffer */

  if (buffer->specific)
    {
      sbp_frame = buffer->specific;
    }
  else
    {
      sbp_frame = mad_sbp_get_empty_frame(adapter_specific->output_key);

      memcpy(sbp_frame->payload, buffer->buffer, buffer->bytes_written);
      mad_free(driver_specific->buffer_pool_memory, buffer->buffer);
    }

  sbp_frame->sbp_header.cookie.destn_node = connection->remote_host_id;
  sbp_frame->sbp_header.cookie.origin     =
    driver->madeleine->configuration.local_host_id;
  sbp_frame->sbp_header.cookie.len        =
    buffer->bytes_written + MAD_SBP_HEADER_SIZE;
  sbp_frame->mad_header.channel_id = connection->channel->id;
  
  if (!connection_specific->request_status)
    {
      connection_specific->request_status = mad_true;
      sbp_frame->mad_header.message_type  = mad_sbp_REQUEST;
    }
  else
    {
#ifdef SOFTWARE_FLOW_CONTROL
      if (!connection_specific->acknowledgement_status)
	{
	  p_mad_sbp_frame_t ack_sbp_frame =
	    mad_sbp_receive_sbp_frame(connection, mad_false);

	  if (ack_sbp_frame->mad_header.message_type != mad_sbp_ACK)
	    FAILURE("unexpected message type");
	  
	  connection_specific->acknowledgement_status = mad_true;
	}
#endif /* SOFTWARE_FLOW_CONTROL */
      sbp_frame->mad_header.message_type = mad_sbp_DATA;
    }

  mad_sbp_put_full_frame(sbp_frame, adapter_specific->output_key);
  buffer->buffer = NULL;  
  LOG_OUT();
}

void
mad_sbp_receive_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t  *buf)
{
  p_mad_connection_t                connection          =
    lnk->connection;
  p_mad_sbp_connection_specific_t   connection_specific =
    connection->specific;
  p_mad_sbp_frame_t                 sbp_frame;
  p_mad_buffer_t                    buffer;

  LOG_IN();

  /* Code to allocate and receive one static buffer */
#ifdef SOFTWARE_FLOW_CONTROL
  if (connection_specific->request_status
      && !connection_specific->acknowledgement_status)
    {
      p_mad_sbp_frame_t ack_sbp_frame =
	mad_sbp_get_empty_frame(adapter_specific->output_key);

      ack_sbp_frame->sbp_header.cookie.destn_node = connection->remote_host_id;
      ack_sbp_frame->sbp_header.cookie.origin     =
	adapter->driver->madeleine->configuration.local_host_id;
      ack_sbp_frame->sbp_header.cookie.len        = MAD_SBP_HEADER_SIZE;
      ack_sbp_frame->mad_header.channel_id   = connection->channel->id;
      ack_sbp_frame->mad_header.message_type = mad_sbp_ACK;
      mad_sbp_put_full_frame(ack_sbp_frame, adapter_specific->output_key);

      connection_specific->acknowledgement_status = mad_true;
    }
#endif /* SOFTWARE_FLOW_CONTROL */
  sbp_frame = mad_sbp_receive_sbp_frame(connection, mad_false);

  if (!connection_specific->request_status)
    {
      if (sbp_frame->mad_header.message_type != mad_sbp_REQUEST)
	FAILURE("unexpected message type");
      
      connection_specific->request_status = mad_true;
    }
  else if (sbp_frame->mad_header.message_type != mad_sbp_DATA)
    FAILURE("unexpected message type");
      
  buffer = mad_alloc_buffer_struct();
  
  buffer->buffer        = sbp_frame->payload;
  buffer->length        = MAD_SBP_PAYLOAD_SIZE;
  buffer->bytes_written =
    sbp_frame->sbp_header.cookie.len - MAD_SBP_HEADER_SIZE;
  buffer->bytes_read    = 0;
  buffer->type          = mad_static_buffer;
  buffer->specific      = sbp_frame;

  *buf = buffer;
  LOG_OUT();
}

void
mad_sbp_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of static buffers */
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      mad_list_reference_t              ref;

      mad_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_sbp_send_buffer(lnk, mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_sbp_receive_sub_buffer_group(p_mad_link_t           lnk,
				 mad_bool_t             first_sub_group,
				 p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Note: static buffers may only be received one at a time */
  FAILURE("grouped buffer reception is not implemented");
  LOG_OUT();
}

p_mad_buffer_t
mad_sbp_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_connection_t           connection       = lnk->connection;
  p_mad_sbp_adapter_specific_t adapter_specific =
    connection->channel->adapter->specific;
  p_mad_sbp_driver_specific_t  driver_specific  =
    connection->channel->adapter->driver->specific;
  p_mad_buffer_t               buffer;
  
  LOG_IN();
  buffer = mad_alloc_buffer_struct();

  if (connection->delayed_send)
    {
      buffer->buffer   = mad_malloc(driver_specific->buffer_pool_memory);
      buffer->specific = NULL;
    }
  else
    {
      p_mad_sbp_frame_t sbp_frame =
	mad_sbp_get_empty_frame(adapter_specific->output_key);
      
      buffer->buffer   = sbp_frame->payload;
      buffer->specific = sbp_frame;
    }

  buffer->length        = MAD_SBP_PAYLOAD_SIZE;
  buffer->bytes_written = 0;
  buffer->bytes_read    = 0;
  buffer->type          = mad_static_buffer;

  LOG_OUT();
  return buffer;
}

void
mad_sbp_return_static_buffer(p_mad_link_t     lnk,
			     p_mad_buffer_t   buffer)
{
  p_mad_sbp_adapter_specific_t adapter_specific =
    lnk->connection->channel->adapter->specific;
  p_mad_sbp_frame_t sbp_frame = buffer->specific;
  
  LOG_IN();
  if (!sbp_frame)
    FAILURE("invalid static buffer");
  
  mad_sbp_put_empty_frame(sbp_frame, adapter_specific->input_key);
  buffer->specific = NULL;
  LOG_OUT();
}

/* External spawn support functions */

void
mad_sbp_external_spawn_init(p_mad_adapter_t spawn_adapter,
			    int *argc, char **argv)
{
  /* Nothing */
}

void
mad_sbp_configuration_init(p_mad_adapter_t       spawn_adapter,
			   p_mad_configuration_t configuration)
{
  p_mad_sbp_adapter_specific_t spawn_adapter_specific =
    spawn_adapter->specific;  
  int sbp_configuration_size;
  int sbp_local_host_id;
  
  LOG_IN();
  PM2_LOCK();
#ifdef PM2  
  stop_timer();
#endif /* PM2 */

  sbp_initialize(0, /* !verbose */
		 MAD_SBP_HEADER_SIZE + MAD_SBP_PAYLOAD_SIZE, 
		 &(spawn_adapter_specific->nb_in_buffers),
		 &(spawn_adapter_specific->nb_out_buffers),
		 &sbp_configuration_size,
		 &sbp_local_host_id,
		 &(spawn_adapter_specific->vnId1),
		 &(spawn_adapter_specific->vnId2),
		 &(spawn_adapter_specific->input_key),
		 &(spawn_adapter_specific->output_key));

#ifdef PM2
  marcel_settimeslice(20000);
#endif /* PM2 */
  PM2_UNLOCK();
  /* Note: host names are currently not available */
  configuration->size          = sbp_configuration_size;
  configuration->local_host_id = sbp_local_host_id;
  configuration->host_name     = NULL;
  LOG_OUT();
}

void
mad_sbp_send_adapter_parameter(p_mad_adapter_t   spawn_adapter,
			       mad_host_id_t     remote_host_id,
			       char             *parameter)
{
  p_mad_sbp_adapter_specific_t spawn_adapter_specific =
    spawn_adapter->specific;  
  p_mad_sbp_frame_t            sbp_frame              =
    mad_sbp_get_empty_frame(spawn_adapter_specific->output_key);
  size_t                       length                 =
    strlen(parameter) + 1;
  
  LOG_IN();
  if (length >= MAD_SBP_PAYLOAD_SIZE)
    FAILURE("parameter string too long");
  
  sbp_frame->sbp_header.cookie.destn_node   = remote_host_id;
  sbp_frame->sbp_header.cookie.origin       = 0;
  sbp_frame->sbp_header.cookie.len          = MAD_SBP_HEADER_SIZE + length;
  sbp_frame->mad_header.channel_id   = 0xFF;
  sbp_frame->mad_header.message_type = mad_sbp_INTERNAL;
  memcpy(sbp_frame->payload, parameter, length);
  
  mad_sbp_put_full_frame(sbp_frame, spawn_adapter_specific->output_key);
  LOG_OUT();
}

void
mad_sbp_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter,
				  char            **parameter)
{
  p_mad_sbp_adapter_specific_t spawn_adapter_specific =
    spawn_adapter->specific;
  p_mad_sbp_driver_specific_t  spawn_driver_specific  =
    spawn_adapter->driver->specific;
  p_mad_sbp_frame_t            sbp_frame              = NULL;  
  size_t                       length;
    
  LOG_IN();
  while (mad_true)
    {
      p_mad_sbp_list_element_t element;
      
      sbp_frame = mad_sbp_get_full_frame(spawn_adapter_specific->input_key);
      if (sbp_frame->mad_header.channel_id == 0XFF)
	break;

      element = mad_malloc(spawn_driver_specific->list_element_memory);
      element->frame = sbp_frame;
      element->next  = NULL;

      if (spawn_adapter_specific->unknown_frames_tail)
	{
	  spawn_adapter_specific->unknown_frames_tail->next = element;
	}
      else
	{
	  spawn_adapter_specific->unknown_frames_head = element;
	}

      spawn_adapter_specific->unknown_frames_tail = element;	  
    }
  
  length = sbp_frame->sbp_header.cookie.len - MAD_SBP_HEADER_SIZE;
  *parameter = malloc(length);
  memcpy(*parameter, sbp_frame->payload, length);
  mad_sbp_put_empty_frame(sbp_frame, spawn_adapter_specific->input_key);
  LOG_OUT();
}
