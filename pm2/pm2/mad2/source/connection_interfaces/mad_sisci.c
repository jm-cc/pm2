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
$Log: mad_sisci.c,v $
Revision 1.6  2000/02/03 17:37:38  oaumage
- mad_channel.c : correction de la liberation des donnees specifiques aux
                  connections
- mad_sisci.c   : support DMA avec double buffering

Revision 1.5  2000/01/21 17:28:16  oaumage
- preversion du support DMA
- !!! attention: cette version du driver n'a pas encore ete testee


______________________________________________________________________________
*/

/*
 * Mad_sisci.c
 * ===========
 */
/* #define DEBUG */
#include <madeleine.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "sisci_api.h"
#include "sisci_types.h"
#include "sisci_error.h"

/* 
 * constants
 * ---------
 */

/* SHMem */
#define MAD_SISCI_BUFFER_SIZE (65536 - sizeof(mad_sisci_connection_status_t))

/* DMA */
#define MAD_SISCI_DMA_BUFFER_SIZE 64000
#define MAD_SISCI_DMA_MIN         (MAD_SISCI_DMA_BUFFER_SIZE * 3)


/*
 * macros
 * ------
 */
/* status
 * note: 's' is supposed to be a pointer on a status structure
 */
#define MAD_SISCI_DISPLAY_ERROR(s) DISP("SISCI failure: " s "\n")
#define mad_sisci_set(s)    tbx_set((s)->flag)
#define mad_sisci_clear(s)  tbx_clear((s)->flag)
#define mad_sisci_toggle(s) tbx_toggle((s)->flag)
#define mad_sisci_test(s)   tbx_test((s)->flag)
#define mad_sisci_control() if (sisci_error != SCI_ERR_OK)\
 {mad_sisci_display_error(sisci_error);FAILURE("Aborting");} else {}

/*
 * local types
 * -----------
 */
typedef unsigned int   mad_sisci_adapter_id_t, *p_mad_sisci_adapter_id_t;
typedef int            mad_sisci_node_id_t,    *p_mad_sisci_node_id_t;
typedef unsigned int   mad_sisci_segment_id_t, *p_mad_sisci_segment_id_t;
typedef unsigned int   mad_sisci_segment_offset_t,
                                             *p_mad_sisci_segment_offset_t;
typedef unsigned int   mad_sisci_segment_size_t,
                                             *p_mad_sisci_segment_size_t;
typedef volatile void *mad_sisci_map_addr_t,   *p_mad_sisci_map_addr_t;

/*
 * local structures
 * ----------------
 */
typedef char mad_sisci_64B_block_t[64];

typedef volatile union
{
  int                   count;
  mad_sisci_64B_block_t padding;
} mad_sisci_buffer_written_t, *p_mad_sisci_buffer_written_t;

typedef volatile union
{
  tbx_flag_t            flag;
  mad_sisci_64B_block_t buffer;
} mad_sisci_status_t, *p_mad_sisci_status_t;

typedef struct
{
  mad_sisci_status_t           read;
  mad_sisci_status_t           write;
  mad_sisci_buffer_written_t   buffers_written;
} mad_sisci_connection_status_t, *p_mad_sisci_connection_status_t ;

typedef volatile struct
{
  mad_sisci_connection_status_t   status;
  mad_sisci_node_id_t             node_id;  
} mad_sisci_internal_segment_data_t, *p_mad_sisci_internal_segment_data_t;

typedef volatile struct
{
  mad_sisci_connection_status_t   status;
  char                            buffer[MAD_SISCI_BUFFER_SIZE];  
} mad_sisci_user_segment_data_t, *p_mad_sisci_user_segment_data_t;

typedef struct 
{
  sci_local_segment_t           segment;  
  sci_map_t                     map;
  mad_sisci_map_addr_t          map_addr;
  mad_sisci_segment_id_t        id;
  mad_sisci_segment_offset_t    offset;
  mad_sisci_segment_size_t      size;
} mad_sisci_local_segment_t, *p_mad_sisci_local_segment_t;

typedef struct 
{
  sci_remote_segment_t          segment;
  sci_map_t                     map;
  mad_sisci_map_addr_t          map_addr;
  mad_sisci_segment_id_t        id;
  mad_sisci_segment_offset_t    offset;
  mad_sisci_segment_size_t      size;
  sci_sequence_t                sequence;
} mad_sisci_remote_segment_t, *p_mad_sisci_remote_segment_t;

typedef struct
{
  int nb_adapter;
} mad_sisci_driver_specific_t, *p_mad_sisci_driver_specific_t;

typedef struct
{
  mad_sisci_adapter_id_t   local_adapter_id;  
  mad_sisci_node_id_t      local_node_id;
  p_mad_sisci_node_id_t    remote_node_id;
} mad_sisci_adapter_specific_t, *p_mad_sisci_adapter_specific_t;

typedef struct
{
} mad_sisci_channel_specific_t, *p_mad_sisci_channel_specific_t;

typedef struct
{
  /* SHMem */
  sci_desc_t                   sd;
  mad_sisci_local_segment_t    local_segment;
  mad_sisci_remote_segment_t   remote_segment;
  size_t                       offset;
  int                          buffers_read;
  tbx_bool_t                   write_flag_flushed;

  /* DMA send */
  sci_desc_t                   dma_send_sd[2];
  sci_dma_queue_t              dma_send_queue[2];
  mad_sisci_local_segment_t    local_dma_send_segment[2];
  mad_sisci_remote_segment_t   remote_dma_send_segment[2];
  
  /* DMA recv */
  sci_desc_t                   dma_recv_sd[2];
  mad_sisci_local_segment_t    local_dma_recv_segment[2];
} mad_sisci_connection_specific_t, *p_mad_sisci_connection_specific_t;

typedef struct
{
} mad_sisci_link_specific_t, *p_mad_sisci_link_specific_t;

/*
 * static functions
 * ----------------
 */

static void
mad_sisci_display_error(sci_error_t error)
{
  fprintf(stderr, "SISCI error code : %u, %X\n", error, error);
  fprintf(stderr,
	  "SISCI stripped error code : %u, %X\n",
	  error & ~SCI_ERR_MASK,
	  error & ~SCI_ERR_MASK);
  
  switch (error)
    {
    case SCI_ERR_OK:
      {
	MAD_SISCI_DISPLAY_ERROR("OK, should not have stopped here ...");
	break;
      }
    case SCI_ERR_BUSY:
      {
	MAD_SISCI_DISPLAY_ERROR("busy");
	break;
      }
    case SCI_ERR_FLAG_NOT_IMPLEMENTED:
      {
	MAD_SISCI_DISPLAY_ERROR("flag not implemented");
	break;
      }
    case SCI_ERR_ILLEGAL_FLAG:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal flag");
	break;
      }
    case SCI_ERR_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space");
	break;
      }
    case SCI_ERR_API_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space (API)");
	break;
      }
    case SCI_ERR_HW_NOSPC:
      {
	MAD_SISCI_DISPLAY_ERROR("no space (hardware)");
	break;
      }
    case SCI_ERR_NOT_IMPLEMENTED:
      {
	MAD_SISCI_DISPLAY_ERROR("not implemented");
	break;
      }
    case SCI_ERR_ILLEGAL_ADAPTERNO:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal adapter no");
	break;
      }
    case SCI_ERR_NO_SUCH_ADAPTERNO:
      {
	MAD_SISCI_DISPLAY_ERROR("no such adapter no");
	break;
      }
    case SCI_ERR_TIMEOUT:
      {
	MAD_SISCI_DISPLAY_ERROR("timeout");
	break;
      }
    case SCI_ERR_OUT_OF_RANGE:
      {
	MAD_SISCI_DISPLAY_ERROR("out of range");
	break;
      }
    case SCI_ERR_NO_SUCH_SEGMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("no such segment");
	break;
      }
    case SCI_ERR_ILLEGAL_NODEID:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal node id");
	break;
      }
    case SCI_ERR_CONNECTION_REFUSED:
      {
	MAD_SISCI_DISPLAY_ERROR("connection refused");
	break;
      }
    case SCI_ERR_SEGMENT_NOT_CONNECTED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment not connected");
	break;
      }
    case SCI_ERR_SIZE_ALIGNMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("size alignment");
	break;
      }
    case SCI_ERR_OFFSET_ALIGNMENT:
      {
	MAD_SISCI_DISPLAY_ERROR("offset alignment");
	break;
      }
    case SCI_ERR_ILLEGAL_PARAMETER:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal parameter");
	break;
      }
    case SCI_ERR_MAX_ENTRIES:
      {
	MAD_SISCI_DISPLAY_ERROR("mad entries");
	break;
      }
    case SCI_ERR_SEGMENT_NOT_PREPARED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment not prepared");
	break;
      }
    case SCI_ERR_ILLEGAL_ADDRESS:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal address");
	break;
      }
    case SCI_ERR_ILLEGAL_OPERATION:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal operation");
	break;
      }
    case SCI_ERR_ILLEGAL_QUERY:
      {
	MAD_SISCI_DISPLAY_ERROR("illegal query");
	break;
      }
    case SCI_ERR_SEGMENTID_USED:
      {
	MAD_SISCI_DISPLAY_ERROR("segment id used");
	break;
      }
    case SCI_ERR_SYSTEM:
      {
	MAD_SISCI_DISPLAY_ERROR("system");
	break;
      }
    case SCI_ERR_CANCELLED:
      {
	MAD_SISCI_DISPLAY_ERROR("cancelled");
	break;
      }
    case SCI_ERR_NO_SUCH_NODEID:
      {
	MAD_SISCI_DISPLAY_ERROR("no such node id");
	break;
      }
    case SCI_ERR_NODE_NOT_RESPONDING:
      {
	MAD_SISCI_DISPLAY_ERROR("node not responding");
	break;
      }
    case SCI_ERR_NO_REMOTE_LINK_ACCESS:
      {
	MAD_SISCI_DISPLAY_ERROR("no remote link access");
	break;
      }
    case SCI_ERR_NO_LINK_ACCESS:
      {
	MAD_SISCI_DISPLAY_ERROR("no link access");
	break;
      }
    case SCI_ERR_TRANSFER_FAILED:
      {
	MAD_SISCI_DISPLAY_ERROR("transfer failed");
	break;
      }
    default:
      {
	MAD_SISCI_DISPLAY_ERROR("Unknown error");
	break;
      }      
    }
}


static void
mad_sisci_flush(p_mad_sisci_remote_segment_t segment)
{
  sci_error_t sisci_error = SCI_ERR_OK;

  SCIStoreBarrier(segment->sequence, 0, &sisci_error);
  mad_sisci_control();
}

/* mad_sisci_get_node_id: query an adapter for the local SCI node id
 * --------------------- */
static mad_sisci_node_id_t 
mad_sisci_get_node_id(mad_sisci_adapter_id_t adapter_id)
{
  sci_desc_t                 descriptor;
  sci_error_t                sisci_error;
  struct sci_query_adapter   query_adapter;
  mad_sisci_node_id_t        node_id ;

  LOG_IN();
  query_adapter.subcommand     = SCI_Q_ADAPTER_NODEID; 
  query_adapter.localAdapterNo = adapter_id;
  query_adapter.data           = &node_id;
  
  SCIOpen(&descriptor, 0, &sisci_error);
  mad_sisci_control();
  SCIQuery(SCI_Q_ADAPTER, &query_adapter, 0, &sisci_error);
  mad_sisci_control();
  SCIClose(descriptor, 0, &sisci_error);
  mad_sisci_control();
  LOG_VAL("mad_sisci_get_node_id: val ", node_id);
  LOG_OUT();
  return node_id;
}


/*
 * exported functions
 * ------------------
 */

void
mad_sisci_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  interface = &driver->interface;
  
  driver->connection_type = mad_bidirectional_connection;
  driver->buffer_alignment = 64;
  
  interface->driver_init                = mad_sisci_driver_init;
  interface->adapter_init               = mad_sisci_adapter_init;
  interface->adapter_configuration_init = mad_sisci_adapter_configuration_init;
  interface->channel_init               = mad_sisci_channel_init;
  interface->before_open_channel        = mad_sisci_before_open_channel;
  interface->connection_init            = mad_sisci_connection_init;
  interface->link_init                  = mad_sisci_link_init;
  interface->accept                     = mad_sisci_accept;
  interface->connect                    = mad_sisci_connect;
  interface->after_open_channel         = mad_sisci_after_open_channel;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_sisci_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = NULL;
  interface->adapter_exit               = mad_sisci_adapter_exit;
  interface->driver_exit                = NULL;
  interface->choice                     = mad_sisci_choice;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
  interface->receive_message            = mad_sisci_receive_message;
  interface->send_buffer                = mad_sisci_send_buffer;
  interface->receive_buffer             = mad_sisci_receive_buffer;
  interface->send_buffer_group          = mad_sisci_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_sisci_receive_sub_buffer_group;
  interface->external_spawn_init        = NULL;
  interface->configuration_init         = NULL;
  interface->send_adapter_parameter     = NULL;
  interface->receive_adapter_parameter  = NULL;
  LOG_OUT();
}

void
mad_sisci_driver_init(p_mad_driver_t driver)
{
  p_mad_sisci_driver_specific_t driver_specific;

  LOG_IN();
  driver_specific = malloc(sizeof(mad_sisci_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;
  LOG_OUT();
}

void
mad_sisci_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                   driver;
  p_mad_sisci_driver_specific_t    driver_specific;
  p_mad_sisci_adapter_specific_t   adapter_specific;
  
  LOG_IN();
  driver          = adapter->driver;
  driver_specific = driver->specific;

  adapter_specific  = malloc(sizeof(mad_sisci_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter->specific = adapter_specific;

  if (adapter->name == NULL)
    {
      adapter_specific->local_adapter_id = driver_specific->nb_adapter;
      adapter_specific->local_node_id =
	mad_sisci_get_node_id(driver_specific->nb_adapter);
      adapter->name     = malloc(10);
      CTRL_ALLOC(adapter->name);  
      sprintf(adapter->name, "SISCI%u", adapter_specific->local_adapter_id);
    }      
  else
    {
      adapter_specific->local_adapter_id = atoi(adapter->name);
      adapter_specific->local_node_id =
	mad_sisci_get_node_id(driver_specific->nb_adapter);
    }

  driver_specific->nb_adapter++;  

  adapter->parameter = malloc(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "%d", adapter_specific->local_node_id);
  LOG_OUT();
}

void
mad_sisci_adapter_configuration_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                    driver           = adapter->driver;
  p_mad_configuration_t             configuration    =
    &driver->madeleine->configuration;
  p_mad_sisci_adapter_specific_t    adapter_specific = adapter->specific;
  mad_sisci_connection_specific_t   connection[configuration->size];
  sci_error_t                       sisci_error      = SCI_ERR_OK;
  mad_host_id_t                     i;
  
  LOG_IN();
  adapter_specific->remote_node_id =
    malloc(configuration->size * sizeof(mad_sisci_node_id_t));
  CTRL_ALLOC(adapter_specific->remote_node_id);
  adapter_specific->remote_node_id[configuration->local_host_id] =
    adapter_specific->local_node_id;

  if (configuration->local_host_id == 0)
    {
      /* Master */
      LOG("mad_sisci_adapter_configuration_init: master");
      for(i = 1; i < configuration->size; i++)
	{
	  p_mad_sisci_connection_specific_t   connection_specific =
	    &connection[i];
	  p_mad_sisci_local_segment_t         local_segment       =
	    &connection_specific->local_segment;
	  p_mad_sisci_remote_segment_t        remote_segment      =
	    &connection_specific->remote_segment;
	  p_mad_sisci_internal_segment_data_t data                = NULL;
	  
	  /* Local segment */
	  local_segment->size   = sizeof(mad_sisci_internal_segment_data_t);
	  local_segment->offset = 0;
	  local_segment->id = 
	      0
	    | (configuration->local_host_id  << 8)
	    | (i << 16)
	    | (1 << 24);
	  SCIOpen(&connection_specific->sd, 0, &sisci_error);
	  mad_sisci_control();

	  SCICreateSegment(connection_specific->sd,
			   &local_segment->segment,
			   local_segment->id,
			   local_segment->size,
			   0, NULL, 0, &sisci_error);
	  mad_sisci_control();

	  SCIPrepareSegment(local_segment->segment,
			    adapter_specific->local_adapter_id,
			    0, &sisci_error);
	  mad_sisci_control();

	  local_segment->map_addr =
	    SCIMapLocalSegment(local_segment->segment,
			       &local_segment->map,
			       local_segment->offset,
			       local_segment->size,
			       NULL, 0, &sisci_error);
	  mad_sisci_control();
	  
	  data = local_segment->map_addr;
	  data->status.read.flag  = tbx_flag_clear;
	  data->status.write.flag = tbx_flag_clear;
	  SCISetSegmentAvailable(local_segment->segment,
				 adapter_specific->local_adapter_id,
				 0, &sisci_error);
	  mad_sisci_control();
	  
	  mad_sisci_set(&data->status.write);
	  while (!mad_sisci_test(&data->status.read));
	  mad_sisci_clear(&data->status.read);

	  adapter_specific->remote_node_id[i] = data->node_id;

	  /* Remote segment */
	  remote_segment->id = 
	      0
	    | (i << 8)
	    | (configuration->local_host_id  << 16)
	    | (1 << 24);
	  remote_segment->size   = sizeof(mad_sisci_internal_segment_data_t);
	  remote_segment->offset = 0;

	  do
	    {
	      SCIConnectSegment(connection_specific->sd,
				&remote_segment->segment,
				adapter_specific->
				remote_node_id[i],
				remote_segment->id,
				adapter_specific->local_adapter_id,
				0,
				NULL,
				SCI_INFINITE_TIMEOUT,
				0,
				&sisci_error);
	    }
	  while (sisci_error != SCI_ERR_OK);
	  
	  remote_segment->map_addr =
	   SCIMapRemoteSegment(remote_segment->segment,
						      &remote_segment->map,
						      remote_segment->offset,
						      remote_segment->size,
						      NULL,
						      0,
						      &sisci_error);
	  mad_sisci_control();
	  
	  SCICreateMapSequence(remote_segment->map,
			       &remote_segment->sequence,
			       0,
			       &sisci_error);
	  mad_sisci_control();
	}
      
      LOG("mad_sisci_adapter_configuration_init: phase 1 terminee");
      for(i = 1; i < configuration->size; i++)
	{
	  p_mad_sisci_connection_specific_t   connection_specific =
	    &connection[i];
	  p_mad_sisci_local_segment_t         local_segment       =
	    &connection_specific->local_segment;
	  p_mad_sisci_remote_segment_t        remote_segment      =
	    &connection_specific->remote_segment;
	  mad_host_id_t j;	  
	  p_mad_sisci_internal_segment_data_t data                =
	    remote_segment->map_addr;
	  
	  for (j = 0; j < configuration->size; j++)
	    {
	      /* Note: master's node id is deliberately sent again
		 to allow for synchronisation */

	      if (j != i)
		{
		  while (!mad_sisci_test(&data->status.write));
		  mad_sisci_clear(&data->status.write);
		  data->node_id = adapter_specific->local_node_id;
		  mad_sisci_flush(remote_segment);
		  mad_sisci_set(&data->status.read);
		  mad_sisci_flush(remote_segment);
		}
	    }
	  
	  SCIUnmapSegment(remote_segment->map, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIDisconnectSegment(remote_segment->segment, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIRemoveSequence(remote_segment->sequence, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCISetSegmentUnavailable(local_segment->segment,
				   adapter_specific->local_adapter_id,
				   0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIUnmapSegment(local_segment->map, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIRemoveSegment(local_segment->segment, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIClose(connection_specific->sd, 0, &sisci_error);
	  mad_sisci_control();
	}
      LOG("mad_sisci_adapter_configuration_init: phase 2 terminee");
    }
  else
    {
      /* Slave */
      p_mad_sisci_connection_specific_t   connection_specific =
	&connection[0];
      p_mad_sisci_local_segment_t         local_segment       =
	&connection_specific->local_segment;
      p_mad_sisci_remote_segment_t        remote_segment      =
	&connection_specific->remote_segment;
      p_mad_sisci_internal_segment_data_t data                = NULL;

      LOG("mad_sisci_adapter_configuration_init: slave");
      adapter_specific->remote_node_id[0] = atoi(adapter->master_parameter);

      /* Local segment */
      local_segment->size   = sizeof(mad_sisci_internal_segment_data_t);
      local_segment->offset = 0;
      local_segment->id = 
	0
	| (configuration->local_host_id  << 8)
	| (0 << 16)
	| (1 << 24);
      SCIOpen(&connection_specific->sd, 0, &sisci_error);
      mad_sisci_control();
      
      SCICreateSegment(connection_specific->sd,
		       &local_segment->segment,
		       local_segment->id,
		       local_segment->size,
		       0, NULL, 0, &sisci_error);
      mad_sisci_control();
      
      SCIPrepareSegment(local_segment->segment,
			adapter_specific->local_adapter_id,
			0, &sisci_error);
      mad_sisci_control();
      
      local_segment->map_addr =
	SCIMapLocalSegment(local_segment->segment,
			   &local_segment->map,
			   local_segment->offset,
			   local_segment->size,
			   NULL, 0, &sisci_error);
      mad_sisci_control();
      
      data = local_segment->map_addr;

      data->status.read.flag  = tbx_flag_clear;
      data->status.write.flag = tbx_flag_clear;
      SCISetSegmentAvailable(local_segment->segment,
			     adapter_specific->local_adapter_id,
			     0, &sisci_error);
      mad_sisci_control();
      
      /* Remote segment */
      remote_segment->id = 
	0
	| (0 << 8)
	| (configuration->local_host_id  << 16)
	| (1 << 24);
      remote_segment->size   = sizeof(mad_sisci_internal_segment_data_t);
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(connection_specific->sd,
			    &remote_segment->segment,
			    adapter_specific->
			    remote_node_id[0],
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0, NULL, SCI_INFINITE_TIMEOUT, 0, &sisci_error);
	}
      while (sisci_error != SCI_ERR_OK);
      data = remote_segment->map_addr =
	SCIMapRemoteSegment(remote_segment->segment,
			    &remote_segment->map,
			    remote_segment->offset,
			    remote_segment->size,
			    NULL, 0, &sisci_error);
      mad_sisci_control();
      
      SCICreateMapSequence(remote_segment->map,
			   &remote_segment->sequence,
			   0, &sisci_error);
      mad_sisci_control();
      
      while (!mad_sisci_test(&data->status.write));
      mad_sisci_clear(&data->status.write);
      data->node_id = adapter_specific->local_node_id;
      mad_sisci_flush(remote_segment);
      mad_sisci_set(&data->status.read);
      mad_sisci_flush(remote_segment);

      LOG("mad_sisci_adapter_configuration_init: phase 1 terminee");
	  
      data = local_segment->map_addr;
      for (i = 0; i < configuration->size; i++)
	{
	  if (i != configuration->local_host_id)
	    {
	      mad_sisci_set(&data->status.write);
	      while (!mad_sisci_test(&data->status.read));
	      mad_sisci_clear(&data->status.read);

	      adapter_specific->remote_node_id[i] =
		data->node_id;
	    }
	}

      SCIUnmapSegment(remote_segment->map, 0, &sisci_error);
      mad_sisci_control();
      
      SCIDisconnectSegment(remote_segment->segment, 0, &sisci_error);
      mad_sisci_control();
      
      SCIRemoveSequence(remote_segment->sequence, 0, &sisci_error);
      mad_sisci_control();
      
      SCISetSegmentUnavailable(local_segment->segment,
			       adapter_specific->local_adapter_id,
			       0, &sisci_error);
      mad_sisci_control();
      
      SCIUnmapSegment(local_segment->map, 0, &sisci_error);
      mad_sisci_control();
      
      SCIRemoveSegment(local_segment->segment, 0, &sisci_error);
      mad_sisci_control();
      
      SCIClose(connection_specific->sd, 0, &sisci_error);
      mad_sisci_control();
      
      LOG("mad_sisci_adapter_configuration_init: phase 2 terminee");
    }
  LOG_OUT();
}

void
mad_sisci_channel_init(p_mad_channel_t channel)
{
  LOG_IN();
  channel->specific = NULL; /* Nothing */
  LOG_OUT();
}

void
mad_sisci_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_sisci_connection_specific_t specific;
  
  LOG_IN();
  specific = malloc(sizeof(mad_sisci_connection_specific_t));
  CTRL_ALLOC(specific);
  
  in->specific = specific;
  in->nb_link = 2;
  out->specific = specific;
  out->nb_link = 2;
  LOG_OUT();
}

void 
mad_sisci_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  /* lnk->link_mode   = mad_link_mode_buffer_group; */
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_sisci_before_open_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                     adapter          = channel->adapter;
  p_mad_sisci_adapter_specific_t      adapter_specific = adapter->specific;
  p_mad_driver_t                      driver           = adapter->driver;
  p_mad_configuration_t               configuration    =
    &driver->madeleine->configuration;
  sci_error_t                         sisci_error      = SCI_ERR_OK;
  mad_host_id_t host_id;

  LOG_IN();

  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      p_mad_connection_t                  connection          =
	&channel->input_connection[host_id];
      p_mad_sisci_connection_specific_t   connection_specific =
	connection->specific;
      p_mad_sisci_local_segment_t         local_segment       =
	&connection_specific->local_segment;

      connection->remote_host_id = host_id;
      local_segment->id = 
	  channel->id
	| (configuration->local_host_id << 8)
	| (connection->remote_host_id   << 16);
      
      if (host_id == configuration->local_host_id)
	{
	  LOG("mad_sisci_before_open_channel: preparing 'accept' segment");
	  local_segment->size =
	    configuration->size * sizeof(mad_sisci_status_t);
	}
      else
	{
	  LOG_VAL("mad_sisci_before_open_channel: "
                  "preparing segment for host id", host_id);
	  local_segment->size = sizeof(mad_sisci_user_segment_data_t);
	}
      
      local_segment->offset = 0;
      SCIOpen(&connection_specific->sd, 0, &sisci_error);
      mad_sisci_control();
      
      SCICreateSegment(connection_specific->sd,
		       &local_segment->segment,
		       local_segment->id,
		       local_segment->size,
		       0, NULL, 0, &sisci_error);
      mad_sisci_control();
      
      SCIPrepareSegment(local_segment->segment,
			adapter_specific->local_adapter_id,
			0, &sisci_error);
      mad_sisci_control();
      
      local_segment->map_addr =
	SCIMapLocalSegment(local_segment->segment,
			   &local_segment->map,
			   local_segment->offset,
			   local_segment->size,
			   NULL, 0, &sisci_error);    
      mad_sisci_control();
      
      if (host_id == configuration->local_host_id)
	{
	  mad_host_id_t        remote_host_id;
	  p_mad_sisci_status_t status = local_segment->map_addr;

	  for (remote_host_id = 0;
	       remote_host_id < configuration->size;
	       remote_host_id++)
	    {
	      mad_sisci_clear(status);
	      status++;
	    }
	}
      else
	{
	  p_mad_sisci_user_segment_data_t local_data =
	    local_segment->map_addr;
	  int i;
	  
	  LOG_VAL("local_segment_id", local_segment->id);
	  for (i = 0; i < 64; i++)
	    {
	      local_data->status.read.buffer[i] = 0;
	    }
	  local_data->status.read.flag             = tbx_flag_clear;
	  local_data->status.write.flag            = tbx_flag_clear;
	  local_data->status.buffers_written.count = 0;
	}

      SCISetSegmentAvailable(local_segment->segment,
			     adapter_specific->local_adapter_id,
			     0, &sisci_error);
      mad_sisci_control();
      
      LOG("mad_sisci_before_open_channel: segment is ready");
    }


  /* DMA segments */
  for (host_id = 0;
       host_id < configuration->size;
       host_id++)
    {
      if (host_id != configuration->local_host_id)
	{	
	  p_mad_connection_t                  connection          =
	    &channel->input_connection[host_id];
	  p_mad_sisci_connection_specific_t   connection_specific =
	    connection->specific;
	  int                                 k;
	  
	  for (k = 0; k < 2; k++)
	    {	      
	      p_mad_sisci_local_segment_t local_segment       =
		&connection_specific->local_dma_send_segment[k];
      
	      connection->remote_host_id = host_id;
	      local_segment->id = 
		channel->id
		| (configuration->local_host_id << 8)
		| (connection->remote_host_id   << 16)
	        | ((k + 2) << 28);
      
	      local_segment->size = MAD_SISCI_DMA_BUFFER_SIZE;
	      
	      local_segment->offset = 0;
	      SCIOpen(&connection_specific->dma_send_sd[k], 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCICreateSegment(connection_specific->dma_send_sd[k],
			       &local_segment->segment,
			       local_segment->id,
			       local_segment->size,
			       0, NULL, 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCIPrepareSegment(local_segment->segment,
				adapter_specific->local_adapter_id,
				0, &sisci_error);
	      mad_sisci_control();
	      
	      local_segment->map_addr =
		SCIMapLocalSegment(local_segment->segment,
				   &local_segment->map,
				   local_segment->offset,
				   local_segment->size,
				   NULL, 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCISetSegmentAvailable(local_segment->segment,
				     adapter_specific->local_adapter_id,
				     0, &sisci_error);
	      mad_sisci_control();

	      SCICreateDMAQueue(connection_specific->dma_send_sd[k],
				&connection_specific->dma_send_queue[k],
				adapter_specific->local_adapter_id,
				1, 0, &sisci_error);
	      mad_sisci_control();
	      LOG("mad_sisci_before_open_channel: DMA segment is ready");
	    }

	  for (k = 0; k < 2; k++)
	    {	      
	      p_mad_sisci_local_segment_t local_segment       =
		&connection_specific->local_dma_recv_segment[k];
      
	      connection->remote_host_id = host_id;
	      local_segment->id = 
		channel->id
		| (configuration->local_host_id << 8)
		| (connection->remote_host_id   << 16)
	        | ((k + 2) << 24);
      
	      local_segment->size   = MAD_SISCI_BUFFER_SIZE;
	      local_segment->offset = 0;

	      SCIOpen(&connection_specific->dma_recv_sd[k], 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCICreateSegment(connection_specific->dma_recv_sd[k],
			       &local_segment->segment,
			       local_segment->id,
			       local_segment->size,
			       0, NULL, 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCIPrepareSegment(local_segment->segment,
				adapter_specific->local_adapter_id,
				0, &sisci_error);
	      mad_sisci_control();
	      
	      local_segment->map_addr =
		SCIMapLocalSegment(local_segment->segment,
				   &local_segment->map,
				   local_segment->offset,
				   local_segment->size,
				   NULL, 0, &sisci_error);
	      mad_sisci_control();
	      
	      SCISetSegmentAvailable(local_segment->segment,
				     adapter_specific->local_adapter_id,
				     0, &sisci_error);
	      mad_sisci_control();
	      LOG("mad_sisci_before_open_channel: DMA segment is ready");
	    }
	}
    } 
  LOG_OUT();
}

void
mad_sisci_accept(p_mad_channel_t channel)
{
  mad_host_id_t                       host_id                    = -1;
  p_mad_adapter_t                     adapter                    =
    channel->adapter;
  p_mad_sisci_adapter_specific_t      adapter_specific           =
    adapter->specific;
  p_mad_driver_t                      driver                     =
    adapter->driver;
  p_mad_configuration_t               configuration              =
    &driver->madeleine->configuration;
  p_mad_connection_t                  accept_connection          =
    &channel->input_connection[configuration->local_host_id];
  p_mad_sisci_connection_specific_t   accept_connection_specific =
    accept_connection->specific;
  p_mad_sisci_local_segment_t         accept_local_segment       =
  &accept_connection_specific->local_segment;
  p_mad_connection_t                  connection;
  p_mad_sisci_connection_specific_t   connection_specific;
  p_mad_sisci_remote_segment_t        remote_segment;
  sci_error_t                         sisci_error                = SCI_ERR_OK;
  int                                 k;
  
  LOG_IN();

  do
    {
      mad_host_id_t          remote_host_id;
      p_mad_sisci_status_t   accept_local_data =
	accept_local_segment->map_addr;
      
      for (remote_host_id = 0;
	   remote_host_id < configuration->size;
	   remote_host_id++)
	{
	  if (remote_host_id != configuration->local_host_id)
	    {
	      if (mad_sisci_test(accept_local_data + remote_host_id))
		{
		  LOG("mad_sisci_accept: incoming connection request");

		  mad_sisci_clear(accept_local_data + remote_host_id);
		  host_id = remote_host_id;
		  break;
		}
	      exit;
	    }
	}
    }
  while(host_id == -1);

  connection = &channel->input_connection[host_id];
  connection_specific = connection->specific;

  remote_segment = &connection_specific->remote_segment;

  remote_segment->id = 
      0
    | (connection->remote_host_id << 8)
    | (configuration->local_host_id << 16);
  

  remote_segment->size = sizeof(mad_sisci_user_segment_data_t);
  remote_segment->offset = 0;

  do
    {
      SCIConnectSegment(connection_specific->sd,
			&remote_segment->segment,
			adapter_specific->
			remote_node_id[connection->remote_host_id],
			remote_segment->id,
			adapter_specific->local_adapter_id,
			0,
			NULL,
			SCI_INFINITE_TIMEOUT,
			0,
			&sisci_error);
    }
  while (sisci_error != SCI_ERR_OK);
  remote_segment->map_addr =
    SCIMapRemoteSegment(remote_segment->segment,
			&remote_segment->map,
			remote_segment->offset,
			remote_segment->size,
			NULL,
			0,
			&sisci_error);
  mad_sisci_control();
  
  SCICreateMapSequence(remote_segment->map,
		       &remote_segment->sequence,
		       0,
		       &sisci_error);
  mad_sisci_control();

  /* DMA */
  for (k = 0; k < 2; k++)
    {
      remote_segment = &connection_specific->remote_dma_send_segment[k];

      remote_segment->id = 
	0
	| (connection->remote_host_id << 8)
	| (configuration->local_host_id << 16)
	| ((k + 2) << 24);
  

      remote_segment->size   = MAD_SISCI_DMA_BUFFER_SIZE;
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(connection_specific->dma_send_sd[k],
			    &remote_segment->segment,
			    adapter_specific->
			    remote_node_id[connection->remote_host_id],
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0,
			    NULL,
			    SCI_INFINITE_TIMEOUT,
			    0,
			    &sisci_error);
	}
      while (sisci_error != SCI_ERR_OK);
    }
  
  LOG_OUT();
}

void
mad_sisci_connect(p_mad_connection_t connection)
{  
  p_mad_channel_t                     channel                     =
    connection->channel;
  p_mad_adapter_t                     adapter                     =
    channel->adapter;
  p_mad_sisci_adapter_specific_t      adapter_specific            =
    adapter->specific;
  p_mad_driver_t                      driver                      =
    adapter->driver;
  p_mad_configuration_t               configuration               =
    &driver->madeleine->configuration;
  p_mad_sisci_connection_specific_t   connection_specific         =
    connection->specific;
  p_mad_sisci_remote_segment_t        remote_segment              =
    &connection_specific->remote_segment;
  p_mad_connection_t                  connect_connection          =
    &channel->input_connection[configuration->local_host_id];
  p_mad_sisci_connection_specific_t   connect_connection_specific =
    connect_connection->specific;
  p_mad_sisci_remote_segment_t        connect_remote_segment      =
    &connect_connection_specific->remote_segment;
  p_mad_sisci_status_t                connect_remote_data         =
    NULL;
  sci_error_t                         sisci_error                 =
    SCI_ERR_OK;
  int                                 k;

  LOG_IN();

  LOG_VAL("mad_sisci_connect: trying to connect to host",
	  connection->remote_host_id);
  
  /* Preparation du 'remote segment' de connexion */
  LOG("mad_sisci_connect: Preparing 'connection' segment");
  connect_remote_segment->id = 
      0
    | (connection->remote_host_id << 16)
    | (connection->remote_host_id << 8);

  connect_remote_segment->size =
    configuration->size * sizeof(mad_sisci_status_t);
  connect_remote_segment->offset = 0;

  do
    {
      SCIConnectSegment(connect_connection_specific->sd,
			&connect_remote_segment->segment,
			adapter_specific->
			remote_node_id[connection->remote_host_id],
			connect_remote_segment->id,
			adapter_specific->local_adapter_id,
			0,
			NULL,
			SCI_INFINITE_TIMEOUT,
			0,
			&sisci_error);
    }
  while (sisci_error != SCI_ERR_OK);
  LOG("`connection' segment connected");

  connect_remote_data = connect_remote_segment->map_addr =
    SCIMapRemoteSegment(connect_remote_segment->segment,
			&connect_remote_segment->map,
			connect_remote_segment->offset,
			connect_remote_segment->size,
			NULL,
			0,
			&sisci_error);
  mad_sisci_control();
  LOG("mad_sisci_connect: 'connection' segment ready");

  /* Preparation des segments de communication */
  LOG("mad_sisci_connect: Preparing 'communication' segment");
  remote_segment->id = 
      0
    | (connection->remote_host_id << 8)
    | (configuration->local_host_id << 16);
  remote_segment->size = sizeof(mad_sisci_user_segment_data_t);
  remote_segment->offset = 0;
  LOG_VAL("remote_segment_id", remote_segment->id);
  
  do
    {
      SCIConnectSegment(connection_specific->sd,
			&remote_segment->segment,
			adapter_specific->
			remote_node_id[connection->remote_host_id],
			remote_segment->id,
			adapter_specific->local_adapter_id,
			0,
			NULL,
			SCI_INFINITE_TIMEOUT,
			0,
			&sisci_error);
    }
  while (sisci_error != SCI_ERR_OK);
  LOG("`communication' segment connected");
  remote_segment->map_addr =
    SCIMapRemoteSegment(remote_segment->segment,
			&remote_segment->map,
			remote_segment->offset,
			remote_segment->size,
			NULL,
			0,
			&sisci_error);
  mad_sisci_control();
  
  SCICreateMapSequence(remote_segment->map,
		       &remote_segment->sequence,
		       0, &sisci_error);
  mad_sisci_control();
  LOG("mad_sisci_connect: 'communication' segment ready");  
  
  /* Envoi du ack sur le segment de connexion */
  mad_sisci_set(connect_remote_data + configuration->local_host_id);
  while (mad_sisci_test(connect_remote_data + configuration->local_host_id));
  LOG("mad_sisci_connect: acknowledgement sent");
  
  /* Liberation du segment de connexion */
  SCIUnmapSegment(connect_remote_segment->map, 0, &sisci_error);
  mad_sisci_control();
  
  SCIDisconnectSegment(connect_remote_segment->segment, 0, &sisci_error);
  mad_sisci_control();

  /* DMA */
  for (k = 0; k < 2; k++)
    {
      remote_segment = &connection_specific->remote_dma_send_segment[k];

      remote_segment->id = 
	0
	| (connection->remote_host_id << 8)
	| (configuration->local_host_id << 16)
	| ((k + 2) << 24);
  

      remote_segment->size   = MAD_SISCI_BUFFER_SIZE;
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(connection_specific->dma_send_sd[k],
			    &remote_segment->segment,
			    adapter_specific->
			    remote_node_id[connection->remote_host_id],
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0,
			    NULL,
			    SCI_INFINITE_TIMEOUT,
			    0,
			    &sisci_error);
	}
      while (sisci_error != SCI_ERR_OK);
    }
  LOG("mad_sisci_connect: connection established");
  LOG_OUT();
}

void
mad_sisci_after_open_channel(p_mad_channel_t channel)
{
  p_mad_adapter_t                   adapter          = channel->adapter;
  p_mad_sisci_adapter_specific_t    adapter_specific = adapter->specific;
  p_mad_driver_t                    driver           = adapter->driver;
  p_mad_configuration_t             configuration    =
    &driver->madeleine->configuration;
  const mad_host_id_t               rank             =
    configuration->local_host_id;
  mad_host_id_t                     host_id;
  sci_error_t                         sisci_error                 =
    SCI_ERR_OK;
  
  LOG_IN();
  /* mettre a jour les flags d'ecriture */
  for (host_id = 0; host_id < configuration->size; host_id++)
    {
      if (host_id != rank)
	{
	  p_mad_connection_t                   connection          =
	    &channel->input_connection[host_id];
	  p_mad_sisci_connection_specific_t    connection_specific =
	    connection->specific;
	  p_mad_sisci_remote_segment_t         remote_segment      =
	    &connection_specific->remote_segment;
	  p_mad_sisci_user_segment_data_t      remote_data         =
	    remote_segment->map_addr;
      
	  mad_sisci_set(&remote_data->status.write);
	  mad_sisci_flush(remote_segment);
	  connection_specific->write_flag_flushed = tbx_true;
	  LOG("mad_sisci_after_open_channel: write authorization sent");
	}
      else
	{
	  /* Fermeture du segment d'acceptation */
	  p_mad_connection_t                  connection          =
	    &channel->input_connection[host_id];
	  p_mad_sisci_connection_specific_t   connection_specific =
	    connection->specific;
	  p_mad_sisci_local_segment_t         local_segment       =
	    &connection_specific->local_segment;

	  SCISetSegmentUnavailable(local_segment->segment,
				   adapter_specific->local_adapter_id,
				   0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIUnmapSegment(local_segment->map, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIRemoveSegment(local_segment->segment, 0, &sisci_error);
	  mad_sisci_control();
	  
	  SCIClose(connection_specific->sd, 0, &sisci_error);
	  mad_sisci_control();
	}
    }
  
  LOG_OUT();
}

void
mad_sisci_disconnect(p_mad_connection_t connection)
{
  p_mad_sisci_connection_specific_t   connection_specific =
    connection->specific;
  p_mad_channel_t                     channel             =
    connection->channel;
  p_mad_adapter_t                     adapter             = channel->adapter;
  p_mad_sisci_adapter_specific_t      adapter_specific    = adapter->specific;
  p_mad_sisci_local_segment_t         local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t        remote_segment      =
    &connection_specific->remote_segment;
  sci_error_t                         sisci_error         = SCI_ERR_OK;
  int k;

  LOG_IN();
  SCIUnmapSegment(remote_segment->map, 0, &sisci_error);
  mad_sisci_control();
  
  SCIDisconnectSegment(remote_segment->segment, 0, &sisci_error);
  mad_sisci_control();
  
  SCIRemoveSequence(remote_segment->sequence, 0, &sisci_error);
  mad_sisci_control();
  
  SCISetSegmentUnavailable(local_segment->segment,
			   adapter_specific->local_adapter_id,
			   0, &sisci_error);
  mad_sisci_control();
  
  SCIUnmapSegment(local_segment->map, 0, &sisci_error);
  mad_sisci_control();
  
  SCIRemoveSegment(local_segment->segment, 0, &sisci_error);
  mad_sisci_control();
  
  SCIClose(connection_specific->sd, 0, &sisci_error);
  mad_sisci_control();
  
  for (k = 0; k < 2; k++)
    {
      p_mad_sisci_local_segment_t local_dma_send_segment =
	&connection_specific->local_dma_send_segment[k];      
      p_mad_sisci_local_segment_t local_dma_recv_segment =
	&connection_specific->local_dma_recv_segment[k];
      p_mad_sisci_remote_segment_t remote_dma_send_segment =
	&connection_specific->remote_dma_send_segment[k];
      
      /* Send */
      SCIDisconnectSegment(remote_dma_send_segment->segment, 0, &sisci_error);
      mad_sisci_control();
  
      SCISetSegmentUnavailable(local_dma_send_segment->segment,
			       adapter_specific->local_adapter_id,
			       0, &sisci_error);
      mad_sisci_control();
      
      SCIUnmapSegment(local_dma_send_segment->map, 0, &sisci_error);
      mad_sisci_control();
      
      SCIRemoveSegment(local_dma_send_segment->segment, 0, &sisci_error);
      mad_sisci_control();
  
      SCIRemoveDMAQueue(connection_specific->dma_send_queue[k],
			0, &sisci_error);
      mad_sisci_control();
  
      SCIClose(connection_specific->dma_send_sd[k], 0, &sisci_error);
      mad_sisci_control();

      /* Recv */
      SCISetSegmentUnavailable(local_dma_recv_segment->segment,
			       adapter_specific->local_adapter_id,
			       0, &sisci_error);
      mad_sisci_control();
      
      SCIUnmapSegment(local_dma_recv_segment->map, 0, &sisci_error);
      mad_sisci_control();
      
      SCIRemoveSegment(local_dma_recv_segment->segment, 0, &sisci_error);
      mad_sisci_control();
  
      SCIClose(connection_specific->dma_recv_sd[k], 0, &sisci_error);
      mad_sisci_control();
    }
  LOG_OUT();
}

p_mad_link_t
mad_sisci_choice(p_mad_connection_t connection,
		 size_t             size,
		 mad_send_mode_t    send_mode    __attribute__ ((unused)),
		 mad_receive_mode_t receive_mode __attribute__ ((unused)))
{
  if (size < MAD_SISCI_DMA_MIN)
    {
      return &connection->link[0];
    }
  else
    {
      return &connection->link[1];
    }
}


void
mad_sisci_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_sisci_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
  free(adapter_specific->remote_node_id);
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

p_mad_connection_t 
mad_sisci_receive_message(p_mad_channel_t channel)
{
  static mad_host_id_t                     host_id          = 0;
         p_mad_adapter_t                   adapter          =
	   channel->adapter;
	 p_mad_driver_t                    driver           =
	   adapter->driver;
	 p_mad_configuration_t             configuration    =
	   &driver->madeleine->configuration;
	 const mad_host_id_t               rank             =
	   configuration->local_host_id;
	 mad_host_id_t                     remote_host_id;
	 
  LOG_IN();

  if (configuration->size == 2)
    {
      return &channel->input_connection[1 - rank];
    }

  while(tbx_true)
    {     
      for (remote_host_id = 1; /* not 0 */
	   remote_host_id < configuration->size;
	   remote_host_id++)
	{
	  do
	    {
	      host_id = (host_id + 1) % configuration->size;
	    }
	  while(host_id == rank);
	  
	  {
	    p_mad_connection_t                   connection          =
	      &channel->input_connection[host_id];
	    p_mad_sisci_connection_specific_t    connection_specific =
	      connection->specific;
	    p_mad_sisci_local_segment_t          local_segment       =
	      &connection_specific->local_segment;
	    p_mad_sisci_remote_segment_t         remote_segment      =
	      &connection_specific->remote_segment;
	    p_mad_sisci_user_segment_data_t      local_data          =
	      local_segment->map_addr;
	    p_mad_sisci_status_t                 read                =
	      &local_data->status.read;

	    if (!connection_specific->write_flag_flushed)
	      {
		connection_specific->write_flag_flushed = tbx_true;
		mad_sisci_flush(remote_segment);
	      }
	    
	    if (mad_sisci_test(read))
	      {
		LOG_OUT();
		return connection;
	      }
	  }
	}
#ifdef PM2
      PM2_YIELD();
#endif /* PM2 */
    }  
}


static void
mad_sisci_send_sci_buffer(p_mad_link_t   link,
		       p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  char                             *source              =
    buffer->buffer + buffer->bytes_read;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  volatile p_mad_sisci_status_t     read                =
    &remote_data->status.read;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;

  LOG_IN();

  while (mad_more_data(buffer))
    {
      size_t   size        =
	min(buffer->bytes_written - buffer->bytes_read,
	    remote_segment->size - sizeof(mad_sisci_connection_status_t));
      size_t   mod_4       = size % 4;
      sci_error_t sisci_error;

      buffer->bytes_read += size;
      
#ifdef PM2
      while (!mad_sisci_test(write))
	PM2_YIELD();
#else /* PM2 */
      while (!mad_sisci_test(write));
#endif /* PM2 */

      PM2_LOCK();
      SCIMemCopy(source,
		 remote_segment->map,
		 sizeof(mad_sisci_connection_status_t),
		 size - mod_4,
		 0,
		 &sisci_error);
      mad_sisci_control();
      PM2_UNLOCK();

      if (mod_4)
	{
	  volatile char    *destination =
	    remote_data->buffer + (size - mod_4);
	  source += size - mod_4;

	  while (mod_4--)
	    {
	      *destination++ = *source++;
	    }
	}
      else
	{
	  source += size;
	}

      PM2_LOCK();
      mad_sisci_flush(remote_segment);
      PM2_UNLOCK();
      mad_sisci_clear(write);
      mad_sisci_set(read);
      PM2_LOCK();
      mad_sisci_flush(remote_segment);
      PM2_UNLOCK();
      connection_specific->write_flag_flushed = tbx_true;
    }

  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer(p_mad_link_t   link, 
			     p_mad_buffer_t buffer)
{  
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  char                             *destination         =
    buffer->buffer + buffer->bytes_written;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              read                =
    &local_data->status.read;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  LOG_IN();
  
  while (!mad_buffer_full(buffer))
    {
      char    *source = local_data->buffer;
      size_t   size   =
	min(buffer->length - buffer->bytes_written,
	    local_segment->size - sizeof(mad_sisci_connection_status_t));

      if (!connection_specific->write_flag_flushed)
	{
	  connection_specific->write_flag_flushed = tbx_true;
	  mad_sisci_flush(remote_segment);
	}
      
#ifdef PM2
      while (!mad_sisci_test(read))
	PM2_YIELD();
#else /* PM2 */
      while (!mad_sisci_test(read));
#endif /* PM2 */

      memcpy(destination, source, size);
      mad_sisci_clear(read);
      mad_sisci_set(write);
      connection_specific->write_flag_flushed = tbx_false;
      buffer->bytes_written +=size;
      destination += size;
    }
  LOG_OUT();
}

static void
mad_sisci_send_sci_buffer_group(p_mad_link_t         link,
				p_mad_buffer_group_t buffer_group)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  volatile p_mad_sisci_status_t     read                =
    &remote_data->status.read;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  volatile int                     *buffers_written     =
    &remote_data->status.buffers_written.count;
  tbx_list_reference_t              ref;
  volatile char                    *destination         =
    remote_data->buffer;
  size_t                            offset              = 0;
  size_t                            destination_size    =
    remote_segment->size - sizeof(mad_sisci_connection_status_t);
  int                               buffer_counter      = 0;
  
  LOG_IN();
  
  if (tbx_empty_list(&buffer_group->buffer_list))
    {
      return ;
    }

  tbx_list_reference_init(&ref, &buffer_group->buffer_list); 
  
#ifdef PM2
  while (!mad_sisci_test(write))
    PM2_YIELD();
#else /* PM2 */
  while (!mad_sisci_test(write));
#endif /* PM2 */
  
  do
    {
      p_mad_buffer_t buffer = tbx_get_list_reference_object(&ref);
      char          *source = buffer->buffer + buffer->bytes_read;

      buffer_counter++;
      while (mad_more_data(buffer))
	{
	  size_t         size         =
	    min(buffer->bytes_written - buffer->bytes_read,
	    destination_size - offset);
	  size_t         mod_4        = size % 4;
	  size_t         aligned_size = size - mod_4;
	  sci_error_t    sisci_error;

	  buffer->bytes_read += size;
	  
	  PM2_LOCK();
	  SCIMemCopy(source,
		     remote_segment->map,
		     sizeof(mad_sisci_connection_status_t) + offset,
		     aligned_size,
		     0,
		     &sisci_error);
	  mad_sisci_control();
	  PM2_UNLOCK();

	  offset += aligned_size;
	  source += aligned_size;
	  
	  while (mod_4--)
	    {
	      *(destination+(offset++)) = *source++;
	    }

	  while (offset & 63)
	    {
	      *(destination+(offset++)) = 0;
	    }
	  
	  if (offset >= destination_size)
	    {
	      PM2_LOCK();
	      mad_sisci_flush(remote_segment);
	      PM2_UNLOCK();
	      *buffers_written = buffer_counter;
	      mad_sisci_clear(write);
	      mad_sisci_set(read);
	      PM2_LOCK();
	      mad_sisci_flush(remote_segment);
	      PM2_UNLOCK();
	      connection_specific->write_flag_flushed = tbx_true;
#ifdef PM2
	      while (!mad_sisci_test(write))
		PM2_YIELD();
#else /* PM2 */
	      while (!mad_sisci_test(write));
#endif /* PM2 */
	      offset = 0;
	      if (mad_more_data(buffer))
		{
		  buffer_counter = 1;
		}
	      else
		{
		  buffer_counter = 0;
		}
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (buffer_counter)
    {
      PM2_LOCK();
      mad_sisci_flush(remote_segment);
      PM2_UNLOCK();
      *buffers_written = buffer_counter;
      mad_sisci_clear(write);
      mad_sisci_set(read);
      PM2_LOCK();
      mad_sisci_flush(remote_segment);
      PM2_UNLOCK();
      connection_specific->write_flag_flushed = tbx_true;
    }
  
  LOG_OUT();
}

static void 
mad_sisci_receive_sci_buffer_group(p_mad_link_t         link,
				   tbx_bool_t           first_sub_group,
				   p_mad_buffer_group_t buffer_group)
{  
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              read                =
    &local_data->status.read;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  tbx_list_reference_t              ref;
  volatile char                    *source              =
    local_data->buffer;
  size_t                            offset              =
    (first_sub_group)?0:connection_specific->offset;
  int                               buffer_counter      =
    (first_sub_group)?0:connection_specific->buffers_read;
  size_t                            source_size         =
    local_segment->size - sizeof(mad_sisci_connection_status_t);
  volatile int                     *buffers_written     =
    &local_data->status.buffers_written.count;
  
  LOG_IN();
  
  if (tbx_empty_list(&buffer_group->buffer_list))
    {
      return ;
    }
  tbx_list_reference_init(&ref, &buffer_group->buffer_list);
  
  if (first_sub_group || !buffer_counter)
    {
      if (!connection_specific->write_flag_flushed)
	{
	  connection_specific->write_flag_flushed = tbx_true;
	  mad_sisci_flush(remote_segment);
	}

#ifdef PM2
      while (!mad_sisci_test(read))
	PM2_YIELD();
#else /* PM2 */
      while (!mad_sisci_test(read));
#endif /* PM2 */
    }
  
  do
    {
      p_mad_buffer_t buffer      = tbx_get_list_reference_object(&ref);
      char          *destination =
	buffer->buffer + buffer->bytes_written;

      buffer_counter++;
      
      while (!mad_buffer_full(buffer))
	{
	  size_t         size         =
	    min(buffer->length - buffer->bytes_written,
		source_size - offset);

	  buffer->bytes_written += size;
	  memcpy(destination, (const void *)(source + offset), size);

	  offset      += tbx_aligned(size, 64);
	  destination += size;

	  if (offset >= source_size)
	    {
	      *buffers_written = 0;
	      mad_sisci_clear(read);
	      mad_sisci_set(write);
	      PM2_LOCK();
	      mad_sisci_flush(remote_segment);
	      PM2_UNLOCK();
	      connection_specific->write_flag_flushed = tbx_true;
#ifdef PM2
	      while (!mad_sisci_test(read))
		PM2_YIELD();
#else /* PM2 */
	      while (!mad_sisci_test(read));
#endif /* PM2 */
	      offset = 0;
	      if (!mad_buffer_full(buffer))
		{
		  buffer_counter = 1;
		}
	      else
		{
		  buffer_counter = 0;
		}
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if ((offset > 0) && (buffer_counter >= *buffers_written))
    {
      *buffers_written = 0;
      mad_sisci_clear(read);
      mad_sisci_set(write);
      connection_specific->write_flag_flushed = tbx_false;
    }
  
  connection_specific->offset       = offset;
  connection_specific->buffers_read = buffer_counter;
  LOG_OUT();
}

/*
 * DMA transfer subroutines
 * -------------------------
 */
static void
mad_sisci_send_dma_buffer(p_mad_link_t   link,
			  p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  char                             *source              =
    buffer->buffer + buffer->bytes_read;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  volatile p_mad_sisci_status_t     read                =
    &remote_data->status.read;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  unsigned int                      iter                = 0;
  sci_error_t                       sisci_error;

  LOG_IN();
  while (mad_more_data(buffer))
    {
      unsigned int                    dma_num             = iter & 1;
      p_mad_sisci_local_segment_t     local_dma_segment   =
	&connection_specific->local_dma_send_segment[dma_num];
      p_mad_sisci_user_segment_data_t local_dma_send_data =
	local_dma_segment->map_addr;
      p_mad_sisci_remote_segment_t    remote_dma_send_segment =
	&connection_specific->remote_dma_send_segment[dma_num];
      size_t                          size                =
	min(buffer->bytes_written - buffer->bytes_read,
	    MAD_SISCI_DMA_BUFFER_SIZE);

      buffer->bytes_read += size;      
      memcpy((void *)local_dma_send_data, source, size);

      if (iter)
	{
	  SCIWaitForDMAQueue(connection_specific->dma_send_queue[1 - dma_num],
			     SCI_INFINITE_TIMEOUT, 0, &sisci_error);

	  mad_sisci_clear(write);
	  mad_sisci_set(read);
	  mad_sisci_flush(remote_segment);

	  SCIResetDMAQueue(connection_specific->dma_send_queue[1 - dma_num],
			   0, &sisci_error);
	}
      
      while (!mad_sisci_test(write));
      
      SCIEnqueueDMATransfer(connection_specific->dma_send_queue[dma_num],
			    local_dma_segment->segment,
			    remote_dma_send_segment->segment,
			    0, 0, tbx_aligned(size, 64), 0, &sisci_error);

      SCIPostDMAQueue(connection_specific->dma_send_queue[dma_num],
		      0, NULL, 0, &sisci_error);
      source += size;
      iter++;
    }

    {
      unsigned dma_num = iter & 1;

      SCIWaitForDMAQueue(connection_specific->dma_send_queue[1 - dma_num],
			 SCI_INFINITE_TIMEOUT, 0, &sisci_error);

      mad_sisci_clear(write);
      mad_sisci_set(read);
      mad_sisci_flush(remote_segment);

      SCIResetDMAQueue(connection_specific->dma_send_queue[1 - dma_num],
		       0, &sisci_error);
    }

  connection_specific->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void
mad_sisci_receive_dma_buffer(p_mad_link_t   link, 
			     p_mad_buffer_t buffer)
{  
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment;
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment;
  char                             *destination         =
    buffer->buffer + buffer->bytes_written;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              read                =
    &local_data->status.read;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  int                               iter                = 0;
  
  LOG_IN();
  

  while (!mad_buffer_full(buffer))
    {
      unsigned int                     dma_num        = iter & 1;
      p_mad_sisci_user_segment_data_t  local_dma_data =
	connection_specific->local_dma_recv_segment[dma_num].map_addr;
      volatile void                   *source         = local_dma_data;
      size_t                           size           =
	min(buffer->length - buffer->bytes_written, MAD_SISCI_DMA_BUFFER_SIZE);      
      if (!connection_specific->write_flag_flushed)
	{
	  connection_specific->write_flag_flushed = tbx_true;
	  mad_sisci_flush(remote_segment);
	}

      while (!mad_sisci_test(read));

      memcpy(destination, source, size);
      mad_sisci_clear(read);
      mad_sisci_set(write);
      connection_specific->write_flag_flushed = tbx_false;

      buffer->bytes_written +=size;
      destination += size;
      iter++;
    }

  LOG_OUT();
}

static void
mad_sisci_send_dma_buffer_group(p_mad_link_t           lnk,
				p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&buffer_group->buffer_list))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &buffer_group->buffer_list);
      do
	{
	  mad_sisci_send_dma_buffer(lnk,
				    tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static void
mad_sisci_receive_dma_buffer_group(p_mad_link_t           lnk,
				   p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&buffer_group->buffer_list))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &buffer_group->buffer_list);
      do
	{
	  mad_sisci_receive_dma_buffer(lnk,
				       tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/*
 * send/receive interface
 * -----------------------
 */
void
mad_sisci_send_buffer(p_mad_link_t   lnk,
		      p_mad_buffer_t buffer)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_sisci_send_sci_buffer(lnk, buffer);
    }
  else if (lnk->id == 1)
    {
      mad_sisci_send_dma_buffer(lnk, buffer);
    }
  else
    FAILURE("Invalid link ID");
  
  LOG_OUT();
}

void 
mad_sisci_receive_buffer(p_mad_link_t   lnk,
			 p_mad_buffer_t *buffer)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_sisci_receive_sci_buffer(lnk, *buffer);
    }
  else if (lnk->id == 1)
    {
      mad_sisci_receive_dma_buffer(lnk, *buffer);
    }
  else
    FAILURE("Invalid link ID");
  
  LOG_OUT();
  
}

void
mad_sisci_send_buffer_group(p_mad_link_t         lnk,
			    p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_sisci_send_sci_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == 1)
    {
      mad_sisci_send_dma_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  
  LOG_OUT();
}

void
mad_sisci_receive_sub_buffer_group(p_mad_link_t           lnk,
				   tbx_bool_t             first_sub_group
				   __attribute__ ((unused)),
				   p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (lnk->id == 0)
    {
      mad_sisci_receive_sci_buffer_group(lnk, first_sub_group, buffer_group);
    }
  else if (lnk->id == 1)
    {
      mad_sisci_receive_dma_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  
  LOG_OUT();
}
