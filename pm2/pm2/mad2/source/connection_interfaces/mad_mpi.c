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
#define MAD_MPI_SERVICE_TAG 0

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
  int            nb_adapter;
} mad_mpi_driver_specific_t, *p_mad_mpi_driver_specific_t;

typedef struct
{
} mad_mpi_adapter_specific_t, *p_mad_mpi_adapter_specific_t;

typedef struct
{
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
  interface->adapter_configuration_init = mad_mpi_adapter_configuration_init;
  interface->channel_init               = mad_mpi_channel_init;
  interface->before_open_channel        = mad_mpi_before_open_channel;
  interface->connection_init            = mad_mpi_connection_init;
  interface->link_init                  = mad_mpi_link_init;
  interface->accept                     = mad_mpi_accept;
  interface->connect                    = mad_mpi_connect;
  interface->after_open_channel         = mad_mpi_after_open_channel;
  interface->before_close_channel       = mad_mpi_before_close_channel;
  interface->disconnect                 = mad_mpi_disconnect;
  interface->after_close_channel        = mad_mpi_after_close_channel;
  interface->choice                     = mad_mpi_choice;
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
mad_mpi_adapter_configuration_init(p_mad_adapter_t adapter)
{
  LOG_IN();
  /* Adapter initialization code (part II)
     Note: called once for each adapter,
     after remote nodes connection information
     has been transmitted (if needed) */
  LOG_OUT();
}

void
mad_mpi_channel_init(p_mad_channel_t channel)
{
  LOG_IN();
  /* Channel initialization code
     Note: called once for each new channel */
  LOG_OUT();
}

void
mad_mpi_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_mpi_connection_specific_t in_specific;
  p_mad_mpi_connection_specific_t out_specific;
  
  LOG_IN();
  /* Point-to-point connection initialization code
     Note: called once for each connection pair during
     channel opening */
  in_specific = malloc(sizeof(mad_mpi_connection_specific_t));
  CTRL_ALLOC(in_specific);
  in->specific = in_specific;
  in->nb_link  = 1;

  out_specific = malloc(sizeof(mad_mpi_connection_specific_t));
  CTRL_ALLOC(out_specific);
  out->specific = out_specific;
  out->nb_link  = 1;
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
mad_mpi_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute before opening a new channel */
  LOG_OUT();
}

void
mad_mpi_accept(p_mad_channel_t channel)
{
  LOG_IN();
  /* Incoming connection acceptation */
  LOG_OUT();
}

void
mad_mpi_connect(p_mad_connection_t connection)
{
  LOG_IN();
  /* Outgoing connection establishment */
  LOG_OUT();
}

void
mad_mpi_after_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute after opening a channel */
  LOG_OUT();
}

void
mad_mpi_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute before closing a channel */
  LOG_OUT();
}

void 
mad_mpi_disconnect(p_mad_connection_t connection)
{
  LOG_IN();
  /* Code to execute to close a connection */
  LOG_OUT();
}

void
mad_mpi_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Code to execute after a channel as been closed */
  LOG_OUT();
}

p_mad_link_t
mad_mpi_choice(p_mad_connection_t   connection,
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
mad_mpi_new_message(p_mad_connection_t connection)
{
  LOG_IN();
  /* Code to prepare a new message */
  LOG_OUT();
}

p_mad_connection_t
mad_mpi_receive_message(p_mad_channel_t channel)
{
  p_mad_configuration_t           configuration    = 
    &(channel->adapter->driver->madeleine->configuration);
  p_mad_connection_t              connection       = NULL;

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
    }
  LOG_OUT();
  return connection;
}

void
mad_mpi_send_buffer(p_mad_link_t     lnk,
		    p_mad_buffer_t   buffer)
{
  LOG_IN();
  /* Code to send one buffer */
  LOG_OUT();
}

void
mad_mpi_receive_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t  *buf)
{
  p_mad_buffer_t buffer = *buf;
  LOG_IN();
  /* Code to receive one buffer */
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
	  mad_mpi_receive_buffer(lnk, mad_get_list_reference_object(&ref));
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
  p_mad_host_id_t              host_id;
  
  LOG_IN();
  /* Code to get configuration information from the external spawn adapter */
  MPI_Comm_rank(MPI_COMM_WORLD, &configuration->local_host_id);
  MPI_Comm_size(MPI_COMM_WORLD, &configuration->size);
  configuration->host_name     =
    malloc(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      configuration->host_name[host_id] = malloc(MAXHOSTNAMELEN + 1);
      CTRL_ALLOC(configuration->host_name[host_id]);
      gethostname(configuration->host_name[host_id], MAXHOSTNAMELEN);
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
	   remote_node_id,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);
  MPI_Send(parameter,
	   len + 1,
	   MPI_CHAR,
	   remote_node_id,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);
  LOG_OUT();
}

void
mad_mpi_receive_adapter_parameter(p_mad_adapter_t   spawn_adapter,
				  char            **parameter)
{
  int len;
  
  LOG_IN();
  /* Code to receive a string from the master node */
  MPI_Recv(&len,
	   1,
	   MPI_INT,
	   0,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);
  *parameter = malloc(len);
  MPI_Send(*parameter,
	   len + 1,
	   MPI_CHAR,
	   0,
	   MAD_MPI_SERVICE_TAG,
	   MPI_COMM_WORLD);
  LOG_OUT();
}
