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
$Log: mad_mpi.c,v $
Revision 1.3  2000/01/04 16:50:48  oaumage
- mad_mpi.c: premiere version fonctionnelle du driver
- mad_sbp.c: nouvelle correction de la transmission des noms d'hote a
  l'initialisation
- mad_tcp.c: remplacement des appels `exit' par des macros FAILURES

Revision 1.2  2000/01/04 09:18:49  oaumage
- ajout de la commande de log de CVS
- phase d'initialisation `external-spawn' terminee pour mad_mpi.c
- recuperation des noms de machines dans la phase
  d'initialisation `external-spawn' de mad_sbp.c


______________________________________________________________________________
*/

/*
 * Mad_mpi.c
 * =========
 */
/* #define DEBUG */
/* #define USE_MARCEL_POLL */

/*
 * headerfiles
 * -----------
 */

/* MadII header files */
#include <madeleine.h>

/* system header files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> /* for `gethostname' */
#include <netdb.h>  /* for `MAXHOSTNAMELEN' */

/* protocol specific header files */
#include <mpi.h>

/*
 * macros and constants definition
 * -------------------------------
 */
#define MAD_MPI_SERVICE_TAG  0
#define MAD_MPI_TRANSFER_TAG 1
#define MAD_MPI_FLOW_CONTROL_TAG 2

/*
 * type definition
 * ---------------
 */


/*
 * local structures
 * ----------------
 */
typedef struct
{
  int nb_adapter;
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
  driver_specific = malloc(sizeof(mad_mpi_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;

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
      adapter->name = malloc(10);
      CTRL_ALLOC(adapter->name);
      sprintf(adapter->name, "MPI%d", driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;

  adapter_specific = malloc(sizeof(mad_mpi_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);

  adapter->parameter = malloc(10);
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
  channel_specific = malloc(sizeof(mad_mpi_channel_specific_t));
  CTRL_ALLOC(channel_specific);

  MPI_Comm_dup(MPI_COMM_WORLD, &channel_specific->communicator);
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
  LOG_OUT();
}

void
mad_mpi_channel_exit(p_mad_channel_t channel)
{  
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  
  LOG_IN();
  /* Code to execute to clean up a channel */
  MPI_Comm_free(&channel_specific->communicator);
  free(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_mpi_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_mpi_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
  /* Code to execute to clean up an adapter */
  free(adapter->parameter);
  free(adapter->name);
  free(adapter->specific);
  LOG_OUT();
}

void
mad_mpi_driver_exit(p_mad_driver_t driver)
{
  p_mad_mpi_driver_specific_t driver_specific = driver->specific;
  
  LOG_IN();
  /* Code to execute to clean up a driver */
  MPI_Finalize();
  LOG_OUT();
}


void
mad_mpi_new_message(p_mad_connection_t connection)
{
  p_mad_channel_t              channel          = connection->channel;
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  p_mad_configuration_t        configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  LOG_IN();
  /* Code to prepare a new message */
#ifndef PM2
  if (configuration->size == 2)
    return;
#endif /* PM2 */
  MPI_Send(&configuration->local_host_id,
	   1,
	   MPI_INT,
	   connection->remote_host_id,
	   MAD_MPI_SERVICE_TAG,
	   channel_specific->communicator);
  LOG_OUT();
}

p_mad_connection_t
mad_mpi_receive_message(p_mad_channel_t channel)
{
  p_mad_mpi_channel_specific_t channel_specific = channel->specific;
  p_mad_configuration_t configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  p_mad_connection_t    connection       = NULL;

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
      /* Incoming communication detection code */      
      int remote_host_id;
      MPI_Status status;
      
      MPI_Recv(&remote_host_id,
	       1,
	       MPI_INT,
	       MPI_ANY_SOURCE,
	       MAD_MPI_SERVICE_TAG,
	       channel_specific->communicator,
	       &status);
      connection = &(channel->input_connection[remote_host_id]);
    }
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
  
  LOG_IN();
  /* Code to send one buffer */

  MPI_Send(buffer->buffer,
	   buffer->length,
	   MPI_CHAR,
	   connection->remote_host_id,
	   MAD_MPI_TRANSFER_TAG,
	   channel_specific->communicator);
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
  MPI_Recv(buffer->buffer,
	   buffer->length,
	   MPI_CHAR,
	   connection->remote_host_id,
	   MAD_MPI_TRANSFER_TAG,
	   channel_specific->communicator,
	   &status);
  buffer->bytes_written = buffer->length;
  LOG_OUT();
}

void
mad_mpi_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /* Code to send a group of buffers */
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      mad_list_reference_t ref;

      mad_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_mpi_send_buffer(lnk, mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_mpi_receive_sub_buffer_group(p_mad_link_t           lnk,
				 mad_bool_t             first_sub_group,
				 p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  /*
   *  Code to receive a part of a group of buffers
   *                    ====
   */

  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      mad_list_reference_t ref;

      mad_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = mad_get_list_reference_object(&ref);
	  mad_mpi_receive_buffer(lnk, &buffer);
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/* External spawn support functions */

void
mad_mpi_external_spawn_init(p_mad_adapter_t spawn_adapter,
			    int *argc, char **argv)
{
  LOG_IN();
  /* External spawn initialization */
  MPI_Init(argc, &argv);
  LOG_OUT();
}

void
mad_mpi_configuration_init(p_mad_adapter_t       spawn_adapter,
			   p_mad_configuration_t configuration)
{
  p_mad_mpi_adapter_specific_t spawn_adapter_specific =
    spawn_adapter->specific;
  mad_host_id_t                host_id;
  int                          rank;
  int                          size;

  LOG_IN();
  /* Code to get configuration information from the external spawn adapter */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  configuration->local_host_id = (mad_host_id_t)rank;
  configuration->size          = (mad_configuration_size_t)size;
  configuration->host_name     =
    malloc(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      configuration->host_name[host_id] = malloc(MAXHOSTNAMELEN + 1);
      CTRL_ALLOC(configuration->host_name[host_id]);

      if (host_id == rank)
	{
	  mad_host_id_t remote_host_id;
	  
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
mad_mpi_send_adapter_parameter(p_mad_adapter_t   spawn_adapter,
			       mad_host_id_t     remote_host_id,
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
mad_mpi_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter,
				  char            **parameter)
{
  MPI_Status status;
  int len;
  
  LOG_IN();
  /* Code to receive a string from the master node */
  MPI_Recv(&len,
	   1,
	   MPI_INT,
	   0,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD,
	   &status);
  *parameter = malloc(len);
  MPI_Recv(*parameter,
	   len + 1,
	   MPI_CHAR,
	   0,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD,
	   &status);
  LOG_OUT();
}
