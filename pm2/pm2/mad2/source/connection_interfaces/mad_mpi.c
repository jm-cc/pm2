
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
 * Mad_mpi.c
 * =========
 */
/* #define DEBUG */
/* #define USE_MARCEL_POLL */

/*
 * !!!!!!!!!!!!!!!!!!!!!!!! Workarounds !!!!!!!!!!!!!!!!!!!!
 * _________________________________________________________
 */
#define MARCEL_POLL_WA

/*
 * headerfiles
 * -----------
 */

/* MadII header files */
#include "madeleine.h"

/* system header files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <netdb.h>

/* protocol specific header files */
#include <mpi.h>

/*
 * macros and constants definition
 * -------------------------------
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */
#define MAD_MPI_SERVICE_TAG      0
#define MAD_MPI_TRANSFER_TAG     1
#define MAD_MPI_FLOW_CONTROL_TAG 2
#ifdef MARCEL
#define MAX_MPI_REQ 64
#endif /* MARCEL */

/*
 * type definition
 * ---------------
 */


/*
 * local structures
 * ----------------
 */
#ifdef MARCEL
typedef struct 
{
  marcel_pollinst_t pollinst;
  MPI_Request       request;
  MPI_Status        status;
} mad_mpi_poll_arg_t, *p_mad_mpi_poll_arg_t;
#endif /* MARCEL */

typedef struct
{
  TBX_SHARED;
  int nb_adapter;
#ifdef MARCEL
  marcel_pollid_t      poll_id;
  unsigned             poll_nb_req;
  MPI_Request          poll_req[MAX_MPI_REQ];
  p_mad_mpi_poll_arg_t poll_arg[MAX_MPI_REQ];
#endif /* MARCEL */
} mad_mpi_driver_specific_t, *p_mad_mpi_driver_specific_t;

typedef struct
{
} mad_mpi_adapter_specific_t, *p_mad_mpi_adapter_specific_t;

typedef struct
{
  MPI_Comm communicator;
} mad_mpi_channel_specific_t, *p_mad_mpi_channel_specific_t;

typedef struct
{
} mad_mpi_connection_specific_t, *p_mad_mpi_connection_specific_t;

typedef struct
{
} mad_mpi_link_specific_t, *p_mad_mpi_link_specific_t;

/*
 * static functions
 * ----------------
 */
#ifdef MARCEL
static p_mad_mpi_driver_specific_t mad_mpi_driver_specific;

static void mpi_io_group(marcel_pollid_t id)
{
  p_mad_mpi_poll_arg_t arg;
  unsigned             count = 0;
  
  LOG_IN();
  FOREACH_POLL(id, arg) 
    {
      arg->pollinst = GET_CURRENT_POLLINST(id);
      mad_mpi_driver_specific->poll_req[count] = arg->request;
      mad_mpi_driver_specific->poll_arg[count] = arg;
      count++;
    }
  mad_mpi_driver_specific->poll_nb_req = count;
  LOG_OUT();
}

static void *mpi_io_poll(marcel_pollid_t id,
			 unsigned        active,
			 unsigned        sleeping,
			 unsigned        blocked)
{
  LOG_IN();
  if(TBX_TRYLOCK_SHARED(mad_mpi_driver_specific))
    {
      int flag;
      int index;
      MPI_Status status;
  
      MPI_Testany(mad_mpi_driver_specific->poll_nb_req,
		  mad_mpi_driver_specific->poll_req,
		  &index,
		  &flag,
		  &status);      
      TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
      
      if(flag) 
	{
	  mad_mpi_driver_specific->poll_arg[index]->status = status;
#ifdef MARCEL_POLL_WA
	  mad_mpi_driver_specific->poll_nb_req--;
#endif /* MARCEL_POLL_WA */
	  LOG_OUT();
	  return MARCEL_POLL_SUCCESS_FOR(mad_mpi_driver_specific->
					 poll_arg[index]->pollinst);
	}
    }
  LOG_OUT();
  return MARCEL_POLL_FAILED;
}

static void mad_mpi_send(void         *buffer,
			 int           count,
			 MPI_Datatype  type,
			 int           destination,
			 int           tag,
			 MPI_Comm      communicator,
			 MPI_Status   *status)
{
  mad_mpi_poll_arg_t arg;

  TBX_LOCK_SHARED(mad_mpi_driver_specific);
  MPI_Isend(buffer,
	    count, 
	    type, 
	    destination,
	    tag,
	    communicator,
	    &arg.request);
  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
  marcel_poll(mad_mpi_driver_specific->poll_id, (any_t)&arg);
  *status = arg.status;
}

static void mad_mpi_recv(void         *buffer,
			 int           count,
			 MPI_Datatype  type,
			 int           source,
			 int           tag,
			 MPI_Comm      communicator,
			 MPI_Status   *status)
{
  mad_mpi_poll_arg_t arg;

  TBX_LOCK_SHARED(mad_mpi_driver_specific);
  MPI_Irecv(buffer,
	    count,
	    type, 
	    source,
	    tag,
	    communicator, 
	    &arg.request);
  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
  marcel_poll(mad_mpi_driver_specific->poll_id, (any_t)&arg);
  *status = arg.status;
}
#endif /* MARCEL */

/*
 * exported functions
 * ------------------
 */

/* Registration function */
void
mad_mpi_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  /* Driver module registration code */
  interface = &(driver->interface);
  
  driver->connection_type = mad_unidirectional_connection;

  /* Not used for now, but might be used in the future for
     dynamic buffer allocation */
  driver->buffer_alignment = 32;
  
  interface->driver_init                = mad_mpi_driver_init;
  interface->adapter_init               = mad_mpi_adapter_init;
  interface->adapter_configuration_init = NULL;
  interface->channel_init               = mad_mpi_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = NULL;
  interface->link_init                  = mad_mpi_link_init;
  interface->accept                     = NULL;
  interface->connect                    = NULL;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = NULL;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = mad_mpi_channel_exit;
  interface->adapter_exit               = mad_mpi_adapter_exit;
  interface->driver_exit                = mad_mpi_driver_exit;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_mpi_new_message;
  interface->receive_message            = mad_mpi_receive_message;
  interface->send_buffer                = mad_mpi_send_buffer;
  interface->receive_buffer             = mad_mpi_receive_buffer;
  interface->send_buffer_group          = mad_mpi_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_mpi_receive_sub_buffer_group;
  interface->external_spawn_init        = mad_mpi_external_spawn_init;
  interface->configuration_init         = mad_mpi_configuration_init;
  interface->send_adapter_parameter     = mad_mpi_send_adapter_parameter;
  interface->receive_adapter_parameter  = mad_mpi_receive_adapter_parameter;;
  LOG_OUT();
}

void
mad_mpi_driver_init(p_mad_driver_t driver)
{
  p_mad_mpi_driver_specific_t driver_specific;

  LOG_IN();
  /* Driver module initialization code
     Note: called only once, just after
     module registration */
  driver_specific = TBX_MALLOC(sizeof(mad_mpi_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  TBX_INIT_SHARED(driver_specific);
  driver_specific->nb_adapter = 0;

#ifdef MARCEL
  mad_mpi_driver_specific = driver_specific;
  mad_mpi_driver_specific->poll_id =
    marcel_pollid_create(mpi_io_group,
			 mpi_io_poll,
			 NULL,
			 MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD);
#endif /* MARCEL */
  driver->name = TBX_MALLOC(4);
  CTRL_ALLOC(driver->name);
  strcpy(driver->name, "mpi");
  LOG_OUT();
}

void
mad_mpi_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver;
  p_mad_mpi_driver_specific_t    driver_specific;
  p_mad_mpi_adapter_specific_t   adapter_specific;
  
  LOG_IN();
  /* Adapter initialization code (part I)
     Note: called once for each adapter */
  driver          = adapter->driver;
  driver_specific = driver->specific;
  if (driver_specific->nb_adapter)
    FAILURE("MPI adapter already initialized");
  
  if (adapter->name == NULL)
    {
      adapter->name = TBX_MALLOC(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "MPI%d", driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;

  adapter_specific = NULL;

  adapter->parameter = TBX_MALLOC(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "-"); 
  LOG_OUT();
}

void
mad_mpi_channel_init(p_mad_channel_t channel)
{
  p_mad_mpi_channel_specific_t channel_specific;
  
  LOG_IN();
  /* Channel initialization code
     Note: called once for each new channel */
  channel_specific = TBX_MALLOC(sizeof(mad_mpi_channel_specific_t));
  CTRL_ALLOC(channel_specific);

  TBX_LOCK_SHARED(mad_mpi_driver_specific);
  MPI_Comm_dup(MPI_COMM_WORLD, &channel_specific->communicator);
  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
  channel->specific = channel_specific;
  LOG_OUT();
}

void 
mad_mpi_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  /* Link initialization code */
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_mpi_channel_exit(p_mad_channel_t channel)
{  
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  
  LOG_IN();
  /* Code to execute to clean up a channel */
  TBX_LOCK_SHARED(mad_mpi_driver_specific);
  MPI_Comm_free(&channel_specific->communicator);
  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
  TBX_FREE(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_mpi_adapter_exit(p_mad_adapter_t adapter)
{
  LOG_IN();
  /* Code to execute to clean up an adapter */
  TBX_FREE(adapter->parameter);
  TBX_FREE(adapter->name);
  TBX_FREE(adapter->specific);
  LOG_OUT();
}

void
mad_mpi_driver_exit(p_mad_driver_t driver)
{
#ifdef MARCEL
  unsigned count;
#endif /* MARCEL */  
  LOG_IN();
  /* Code to execute to clean up a driver */
  TBX_LOCK_SHARED(mad_mpi_driver_specific);
#ifdef MARCEL
  for (count = 0;
       count < mad_mpi_driver_specific->poll_nb_req;
       count++)
    {
      MPI_Cancel(&(mad_mpi_driver_specific->poll_req[count]));
    }
#endif /* MARCEL */  
  MPI_Finalize();
  TBX_FREE (driver->specific);
#ifdef MARCEL
  mad_mpi_driver_specific = NULL;
#endif /* MARCEL */
  driver->specific = NULL;
  LOG_OUT();
}


void
mad_mpi_new_message(p_mad_connection_t connection)
{
  p_mad_channel_t              channel          = connection->channel;
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  p_mad_configuration_t        configuration    = 
    channel->adapter->driver->madeleine->configuration;
#ifdef MARCEL
  MPI_Status status;
#endif /* MARCEL */
  LOG_IN();
  /* Code to prepare a new message */
#ifdef MARCEL
  mad_mpi_send(&configuration->local_host_id,
	       1,
	       MPI_INT,
	       connection->remote_host_id,
	       MAD_MPI_SERVICE_TAG,
	       channel_specific->communicator,
	       &status);
#else
  if (configuration->size == 2)
    return;
  MPI_Send(&configuration->local_host_id,
	   1,
	   MPI_INT,
	   connection->remote_host_id,
	   MAD_MPI_SERVICE_TAG,
	   channel_specific->communicator);
#endif /* MARCEL */
  LOG_OUT();
}

p_mad_connection_t
mad_mpi_receive_message(p_mad_channel_t channel)
{
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
#ifndef MARCEL
  p_mad_configuration_t configuration    = 
    channel->adapter->driver->madeleine->configuration;
#endif /* MARCEL */
  p_mad_connection_t    connection       = NULL;
  int                   remote_host_id;
  MPI_Status            status;

  LOG_IN();
#ifdef MARCEL
  mad_mpi_recv(&remote_host_id,
	       1,
	       MPI_INT,
	       MPI_ANY_SOURCE,
	       MAD_MPI_SERVICE_TAG,
	       channel_specific->communicator,
	       &status);
  connection = &(channel->input_connection[remote_host_id]);
#else /* MARCEL */
  if (configuration->size == 2)
    {
      connection =
	&(channel->input_connection[1 - configuration->local_host_id]);
    }
  else
    {
      /* Incoming communication detection code */      
      MPI_Recv(&remote_host_id,
	       1,
	       MPI_INT,
	       MPI_ANY_SOURCE,
	       MAD_MPI_SERVICE_TAG,
	       channel_specific->communicator,
	       &status);
      connection = &(channel->input_connection[remote_host_id]);
    }
#endif /* MARCEL */
  LOG_OUT();
  return connection;
}

void
mad_mpi_send_buffer(p_mad_link_t     lnk,
		    p_mad_buffer_t   buffer)
{
  p_mad_connection_t           connection       = lnk->connection;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
#ifdef MARCEL
  MPI_Status status;
#endif /* MARCEL */
  
  LOG_IN();
/* Code to send one buffer */
#ifdef MARCEL
  mad_mpi_send(buffer->buffer,
	       buffer->length,
	       MPI_CHAR,
	       connection->remote_host_id,
	       MAD_MPI_TRANSFER_TAG,
	       channel_specific->communicator,
	       &status);
#else /* MARCEL */
  MPI_Send(buffer->buffer,
	   buffer->length,
	   MPI_CHAR,
	   connection->remote_host_id,
	   MAD_MPI_TRANSFER_TAG,
	   channel_specific->communicator);
#endif /* MARCEL */
  buffer->bytes_read = buffer->length;
  LOG_OUT();
}

void
mad_mpi_receive_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t  *buf)
{
  p_mad_connection_t           connection       = lnk->connection;
  p_mad_channel_t              channel          = connection->channel;
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  p_mad_buffer_t buffer = *buf;
  MPI_Status status;
  
  LOG_IN();
  /* Code to receive one buffer */
#ifdef MARCEL
  mad_mpi_recv(buffer->buffer,
	       buffer->length,
	       MPI_CHAR,
	       connection->remote_host_id,
	       MAD_MPI_TRANSFER_TAG,
	       channel_specific->communicator,
	       &status);
#else /* MARCEL */
  MPI_Recv(buffer->buffer,
	   buffer->length,
	   MPI_CHAR,
	   connection->remote_host_id,
	   MAD_MPI_TRANSFER_TAG,
	   channel_specific->communicator,
	   &status);
#endif /* MARCEL */
  buffer->bytes_written = buffer->length;
  LOG_OUT();
}

void
mad_mpi_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of buffers */
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;
      int                  buffer_count = buffer_group->buffer_list.length;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      
      if (buffer_count == 1)
	{
	  mad_mpi_send_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      else
	{
	  p_mad_connection_t           connection       = lnk->connection;
	  p_mad_channel_t              channel          = connection->channel;
	  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
	  p_mad_buffer_t               buffer;
	  int                          length[buffer_count];
	  MPI_Aint                     displacement[buffer_count];
	  MPI_Datatype                 hindexed;
	  int                          count = 0;
#ifdef MARCEL
	  MPI_Status status;
#endif /* MARCEL */

	  TBX_LOCK_SHARED(mad_mpi_driver_specific);
	  do
	    {
	      buffer = tbx_get_list_reference_object(&ref);

	      MPI_Address(buffer->buffer, displacement + count);	      
	      length[count] = buffer->length;
	      count ++;
	    }
	  while(tbx_forward_list_reference(&ref));

	  MPI_Type_hindexed(buffer_count,
			    length,
			    displacement,
			    MPI_BYTE,
			    &hindexed);
	  MPI_Type_commit(&hindexed);
	  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
#ifdef MARCEL
	  mad_mpi_send(MPI_BOTTOM,
		       1,
		       hindexed,
		       connection->remote_host_id,
		       MAD_MPI_TRANSFER_TAG,
		       channel_specific->communicator,
		       &status);
#else /* MARCEL */
	  MPI_Send(MPI_BOTTOM,
		   1,
		   hindexed,
		   connection->remote_host_id,
		   MAD_MPI_TRANSFER_TAG,
		   channel_specific->communicator);
#endif /* MARCEL */
	  TBX_LOCK_SHARED(mad_mpi_driver_specific);
	  MPI_Type_free(&hindexed);
	  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
	}
    }
  LOG_OUT();
}

void
mad_mpi_receive_sub_buffer_group(p_mad_link_t           lnk,
				 tbx_bool_t             first_sub_group,
				 p_mad_buffer_group_t   buffer_group)
{
  /* Code to receive a group of buffers */
  LOG_IN();

  if (!first_sub_group)
    FAILURE("group split error");

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;
      int                  buffer_count = buffer_group->buffer_list.length;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      
      if (buffer_count == 1)
	{
	  p_mad_buffer_t buffer = tbx_get_list_reference_object(&ref);

	  mad_mpi_receive_buffer(lnk, &buffer);
	}
      else
	{
	  p_mad_connection_t           connection       = lnk->connection;
	  p_mad_channel_t              channel          = connection->channel;
	  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
	  p_mad_buffer_t               buffer;
	  int                          length[buffer_count];
	  MPI_Aint                     displacement[buffer_count];
	  MPI_Datatype                 hindexed;
	  MPI_Status                   status;
	  int                          count = 0;
	  
	  TBX_LOCK_SHARED(mad_mpi_driver_specific);
	  do
	    {
	      buffer = tbx_get_list_reference_object(&ref);

	      MPI_Address(buffer->buffer, displacement + count);	      
	      length[count] = buffer->length;
	      count ++;
	    }
	  while(tbx_forward_list_reference(&ref));

	  MPI_Type_hindexed(buffer_count,
			    length,
			    displacement,
			    MPI_BYTE,
			    &hindexed);
	  MPI_Type_commit(&hindexed);
	  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
#ifdef MARCEL
	  mad_mpi_recv(MPI_BOTTOM,
		       1,
		       hindexed,
		       connection->remote_host_id,
		       MAD_MPI_TRANSFER_TAG,
		       channel_specific->communicator,
		       &status);
#else /* MARCEL */
	  MPI_Recv(MPI_BOTTOM,
		   1,
		   hindexed,
		   connection->remote_host_id,
		   MAD_MPI_TRANSFER_TAG,
		   channel_specific->communicator,
		   &status);	  
#endif /* MARCEL */
	  TBX_LOCK_SHARED(mad_mpi_driver_specific);
	  MPI_Type_free(&hindexed);
	  TBX_UNLOCK_SHARED(mad_mpi_driver_specific);
	}
    }
  LOG_OUT();
}

/* External spawn support functions */

void
mad_mpi_external_spawn_init(p_mad_adapter_t   spawn_adapter
			    __attribute__ ((unused)),
			    int              *argc,
			    char            **argv)
{
  LOG_IN();
  /* External spawn initialization */
  MPI_Init(argc, &argv);
  LOG_OUT();
}

void
mad_mpi_configuration_init(p_mad_adapter_t       spawn_adapter
			   __attribute__ ((unused)),
			   p_mad_configuration_t configuration)
{
  ntbx_host_id_t               host_id;
  int                          rank;
  int                          size;

  LOG_IN();
  /* Code to get configuration information from the external spawn adapter */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  configuration->local_host_id = (ntbx_host_id_t)rank;
  configuration->size          = (mad_configuration_size_t)size;
  configuration->host_name     =
    TBX_MALLOC(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      configuration->host_name[host_id] = TBX_MALLOC(MAXHOSTNAMELEN + 1);
      CTRL_ALLOC(configuration->host_name[host_id]);

      if (host_id == rank)
	{
	  ntbx_host_id_t remote_host_id;
	  
	  gethostname(configuration->host_name[host_id], MAXHOSTNAMELEN);
	  
	  for (remote_host_id = 0;
	       remote_host_id < configuration->size;
	       remote_host_id++)
	    {
	      int len;
	      
	      len = strlen(configuration->host_name[host_id]);
	      
	      if (remote_host_id != rank)
		{
		  MPI_Send(&len,
			   1,
			   MPI_INT,
			   remote_host_id,
			   MAD_MPI_SERVICE_TAG,
			   MPI_COMM_WORLD);
		  MPI_Send(configuration->host_name[host_id],
			   len + 1,
			   MPI_CHAR,
			   remote_host_id,
			   MAD_MPI_SERVICE_TAG,
			   MPI_COMM_WORLD);
		}
	    }
	}
      else
	{
	  int len;
	  MPI_Status status;

	  MPI_Recv(&len,
		   1,
		   MPI_INT,
		   host_id,
		   MAD_MPI_SERVICE_TAG,
		   MPI_COMM_WORLD,
		   &status);

	  MPI_Recv(configuration->host_name[host_id],
		   len + 1,
		   MPI_CHAR,
		   host_id,
		   MAD_MPI_SERVICE_TAG,
		   MPI_COMM_WORLD,
		   &status);
	}      
    }
  LOG_OUT();
}

void
mad_mpi_send_adapter_parameter(p_mad_adapter_t   spawn_adapter
			       __attribute__ ((unused)),
			       ntbx_host_id_t    remote_host_id,
			       char             *parameter)
{  
  int len = strlen(parameter);

  LOG_IN();
  /* Code to send a string from the master node to one slave node */
  MPI_Send(&len,
	   1,
	   MPI_INT,
	   remote_host_id,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);

  MPI_Send(parameter,
	   len + 1,
	   MPI_CHAR,
	   remote_host_id,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);
  LOG_OUT();
}

void
mad_mpi_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter
				  __attribute__ ((unused)),
				  char            **parameter)
{
  MPI_Status status;
  int len;
  
  LOG_IN();
  /* Code to receive a string from the master node */
  MPI_Recv(&len,
	   1,
	   MPI_INT,
	   -1,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD,
	   &status);

  *parameter = TBX_MALLOC(len);
  MPI_Recv(*parameter,
	   len + 1,
	   MPI_CHAR,
	   -1,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD,
	   &status);

  LOG_OUT();
}
