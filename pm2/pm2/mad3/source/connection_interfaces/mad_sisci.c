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
 * Mad_sisci.c
 * ===========
 */
//#define USE_MARCEL_POLL
#define MAD_SISCI_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)

#undef PM2DEBUG
#ifdef MARCEL
#include "marcel.h"
#endif
#include "madeleine.h"
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

/* Optimized SHMem */
#define MAD_SISCI_OPT_MAX  112
/* #define MAD_SISCI_OPT_MAX  0 */

/* SHMem */
#define MAD_SISCI_BUFFER_SIZE (65536 - sizeof(mad_sisci_connection_status_t))
//#define MAD_SISCI_BUFFER_SIZE (16384 - sizeof(mad_sisci_connection_status_t))
#define MAD_SISCI_MIN_SEG_SIZE 8192

/* Links */
#define MAD_SISCI_LINK_OPT      0
#define MAD_SISCI_LINK_REGULAR  1

/*
 * macros
 * ------
 */
/* status
 * note: 's' is supposed to be a pointer on a status structure
 */
#define MAD_SISCI_DISPLAY_ERROR(s) DISP("SISCI failure: " s "\n")
#define mad_sisci_set(s)    tbx_set(&((s)->flag))
#define mad_sisci_clear(s)  tbx_clear(&((s)->flag))
#define mad_sisci_toggle(s) tbx_toggle(&((s)->flag))
#define mad_sisci_test(s)   tbx_test(&((s)->flag))
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
typedef char mad_sisci_1024B_block_t[1024];

typedef volatile union
{
  tbx_flag_t            flag;
  mad_sisci_64B_block_t buffer;
} mad_sisci_status_t, *p_mad_sisci_status_t;

typedef struct
{
  mad_sisci_status_t      read;
  mad_sisci_1024B_block_t padding;
  mad_sisci_status_t      write;
} mad_sisci_connection_status_t, *p_mad_sisci_connection_status_t ;

typedef volatile struct
{
  mad_sisci_connection_status_t status;
  mad_sisci_node_id_t           node_id;  
} mad_sisci_internal_segment_data_t, *p_mad_sisci_internal_segment_data_t;

typedef volatile struct
{
  mad_sisci_connection_status_t status;
  char                          buffer[MAD_SISCI_BUFFER_SIZE];  
} mad_sisci_user_segment_data_t, *p_mad_sisci_user_segment_data_t;

typedef struct 
{
  sci_local_segment_t        segment;  
  sci_map_t                  map;
  mad_sisci_map_addr_t       map_addr;
  mad_sisci_segment_id_t     id;
  mad_sisci_segment_offset_t offset;
  mad_sisci_segment_size_t   size;
} mad_sisci_local_segment_t, *p_mad_sisci_local_segment_t;

typedef struct 
{
  sci_remote_segment_t       segment;
  sci_map_t                  map;
  mad_sisci_map_addr_t       map_addr;
  mad_sisci_segment_id_t     id;
  mad_sisci_segment_offset_t offset;
  mad_sisci_segment_size_t   size;
  sci_sequence_t             sequence;
} mad_sisci_remote_segment_t, *p_mad_sisci_remote_segment_t;

typedef struct
{
  int nb_adapter;
#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  marcel_pollid_t mad_sisci_pollid;
#endif
} mad_sisci_driver_specific_t, *p_mad_sisci_driver_specific_t;

typedef struct
{
  mad_sisci_adapter_id_t local_adapter_id;  
  mad_sisci_node_id_t    local_node_id;
  p_mad_sisci_node_id_t  remote_node_id;
} mad_sisci_adapter_specific_t, *p_mad_sisci_adapter_specific_t;

typedef struct
{
  /* Array of size configuration->size with adresse of flag for read */
  p_mad_sisci_status_t *read;
  int                   next;
  int                   max;
} mad_sisci_channel_specific_t, *p_mad_sisci_channel_specific_t;

typedef enum
{
  mad_sisci_poll_channel,
  mad_sisci_poll_flag,
} mad_sisci_poll_op_t, *p_mad_sisci_poll_op_t;

typedef struct
{
  p_mad_channel_t    channel;
  p_mad_connection_t connection;  
} mad_sisci_poll_channel_data_t, *p_mad_sisci_poll_channel_data_t;

typedef struct
{
  p_mad_sisci_status_t flag;
} mad_sisci_poll_flag_data_t, *p_mad_sisci_poll_flag_data_t;

typedef union
{
  mad_sisci_poll_channel_data_t channel_op;
  mad_sisci_poll_flag_data_t    flag_op;
} mad_sisci_poll_data_t, *p_mad_sisci_poll_data_t;

typedef struct
{
  mad_sisci_poll_op_t   op;
  mad_sisci_poll_data_t data;
} mad_sisci_marcel_poll_cell_arg_t, *p_mad_sisci_marcel_poll_cell_arg_t;

typedef struct
{
  sci_desc_t                 sd[2];
  mad_sisci_local_segment_t  local_segment[2];
  mad_sisci_remote_segment_t remote_segment[2];
  int                        buffers_read;
  volatile tbx_bool_t        write_flag_flushed;
} mad_sisci_connection_specific_t, *p_mad_sisci_connection_specific_t;

typedef struct
{
  int dummy;
} mad_sisci_link_specific_t, *p_mad_sisci_link_specific_t;

/*
 * static functions
 * ----------------
 */

static void
mad_sisci_display_error(sci_error_t error)
{
  DISP("SISCI error code : %u, %X", error, error);
  DISP("SISCI stripped error code : %u, %X",
       error & ~SCI_ERR_MASK, error & ~SCI_ERR_MASK);
  
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

  LOG_IN();
  SCIStoreBarrier(segment->sequence, 0, &sisci_error);
  mad_sisci_control();
  LOG_OUT();
}

/* mad_sisci_get_node_id: query an adapter for the local SCI node id
 * --------------------- */
static mad_sisci_node_id_t 
mad_sisci_get_node_id(mad_sisci_adapter_id_t adapter_id)
{
  mad_sisci_node_id_t      node_id = 0;
  sci_desc_t               descriptor;
  sci_error_t              sisci_error;
  struct sci_query_adapter query_adapter;

  LOG_IN();
  SCIOpen(&descriptor, 0, &sisci_error);
  mad_sisci_control();

  query_adapter.subcommand     = SCI_Q_ADAPTER_NODEID; 
  query_adapter.localAdapterNo = adapter_id;
  query_adapter.data           = &node_id;
  
  SCIQuery(SCI_Q_ADAPTER, &query_adapter, 0, &sisci_error);
  mad_sisci_control();

  SCIClose(descriptor, 0, &sisci_error);
  mad_sisci_control();

  LOG_VAL("mad_sisci_get_node_id: val ", node_id);
  LOG_OUT();

  return node_id;
}

/* For marcel polling
 * --------------------- */
#if defined(MARCEL) && defined(USE_MARCEL_POLL)
static void
mad_sisci_marcel_group(marcel_pollid_t id)
{
  return;
}

inline static int
mad_sisci_do_poll(p_mad_sisci_marcel_poll_cell_arg_t info)
{
  int status = 0;
 
  LOG_IN();
  if (info->op == mad_sisci_poll_flag)
    {
      status = mad_sisci_test(info->data.flag_op.flag);
    }
  else if (info->op == mad_sisci_poll_channel)
    {
      p_mad_channel_t                channel          = NULL;
      p_mad_sisci_channel_specific_t channel_specific = NULL;
      p_mad_connection_t             in               = NULL;
      p_tbx_darray_t                 in_darray        = NULL;
      int                            next             =    0;
      int                            max              =    0;
      int                            i                =    0;

      channel          = info->data.channel_op.channel;
      channel_specific = channel->specific;
      in_darray        = channel->in_connection_darray;
      max              = channel_specific->max;
      next             = channel_specific->next;

      i = max;
      
      while (i--)
	{
	  next = (next + 1) % max;
	  in   = tbx_darray_get(in_darray, next);

	  if (in)
	    {
	      p_mad_sisci_connection_specific_t in_specific = NULL;
	  
	      in_specific = in->specific;

	      if (!in_specific->write_flag_flushed)
		{
		  p_mad_sisci_remote_segment_t remote_segment = NULL;

		  remote_segment = &(in_specific->remote_segment[0]);

		  mad_sisci_flush(remote_segment);
		  in_specific->write_flag_flushed = tbx_true;
		}

	      if (mad_sisci_test(channel_specific->read[next]))
		{
		  info->data.channel_op.connection = in;
		  status                           =  1;
		  break;
		}
	    }
	}

      channel_specific->next = next;
    }
  else
    FAILURE("unknown polling operation");

  LOG_OUT();
  
  return status;
}

static void *
mad_sisci_marcel_fast_poll(marcel_pollid_t id,
			   any_t           arg,
			   boolean         first_call)
{
  void *status = MARCEL_POLL_FAILED;

  LOG_IN();
  if (mad_sisci_do_poll((p_mad_sisci_marcel_poll_cell_arg_t) arg))
    {
      status = MARCEL_POLL_SUCCESS_FOR(arg);
    }
  LOG_OUT();

  return status;
}

static void *
mad_sisci_marcel_poll(marcel_pollid_t id,
		      unsigned        active, 
		      unsigned        sleeping,
		      unsigned        blocked)
{
  p_mad_sisci_marcel_poll_cell_arg_t  my_arg = NULL;
  void                               *status = MARCEL_POLL_FAILED;

  LOG_IN();
  FOREACH_POLL(id, my_arg) 
    {
      if (mad_sisci_do_poll((p_mad_sisci_marcel_poll_cell_arg_t) my_arg)) 
	{
	  status = MARCEL_POLL_SUCCESS(id);
	  goto found;
	}
    }

found:
  LOG_OUT();

  return status;
}

inline static
void
mad_sisci_wait_for(p_mad_link_t         link, 
		   p_mad_sisci_status_t flag)
{
  LOG_IN();
  if (!mad_sisci_test(flag))
    {
      p_mad_sisci_driver_specific_t    driver_specific = NULL;
      mad_sisci_marcel_poll_cell_arg_t arg;

      driver_specific = link->connection->channel->adapter->driver->specific;

      arg.op                = mad_sisci_poll_flag;
      arg.data.flag_op.flag = flag;

      marcel_poll(driver_specific->mad_sisci_pollid, &arg);
    }
  LOG_OUT();
}

#else // MARCEL && USE_MARCEL_POLL
inline static
void
mad_sisci_wait_for(p_mad_link_t         link,
		   p_mad_sisci_status_t flag)
{
  while (!mad_sisci_test(flag))
    TBX_YIELD();
}

#endif // MARCEL && USE_MARCEL_POLL

/*
 * exported functions
 * ------------------
 */

void
mad_sisci_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  TRACE("Registering SISCI driver");
  interface = driver->interface;
  
  driver->connection_type = mad_bidirectional_connection;
  driver->buffer_alignment = 64;  
  driver->name = tbx_strdup("sisci");
  
  interface->driver_init                = mad_sisci_driver_init;
  interface->adapter_init               = mad_sisci_adapter_init;
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
  interface->channel_exit               = mad_sisci_channel_exit;
  interface->adapter_exit               = mad_sisci_adapter_exit;
  interface->driver_exit                = NULL;
  interface->choice                     = mad_sisci_choice;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = NULL;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_sisci_poll_message;
#endif /* MAD_MESSAGE_POLLING */
  interface->receive_message            = mad_sisci_receive_message;
  interface->message_received           = NULL;
  interface->send_buffer                = mad_sisci_send_buffer;
  interface->receive_buffer             = mad_sisci_receive_buffer;
  interface->send_buffer_group          = mad_sisci_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_sisci_receive_sub_buffer_group;
  LOG_OUT();
}

void
mad_sisci_driver_init(p_mad_driver_t driver)
{
  p_mad_sisci_driver_specific_t driver_specific = NULL;

  LOG_IN();
  TRACE("Initializing SISCI driver");
  driver_specific  = TBX_MALLOC(sizeof(mad_sisci_driver_specific_t));
  driver->specific = driver_specific;
 
#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  driver_specific->mad_sisci_pollid =
    marcel_pollid_create(mad_sisci_marcel_group,
			 mad_sisci_marcel_poll,
			 mad_sisci_marcel_fast_poll,
			 MAD_SISCI_POLLING_MODE);
#endif // MARCEL && USE_MARCEL_POLL
  LOG_OUT();
}

void
mad_sisci_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_sisci_adapter_specific_t adapter_specific = NULL;
  p_tbx_string_t                 parameter_string = NULL;
  
  LOG_IN();
  adapter_specific  = TBX_MALLOC(sizeof(mad_sisci_adapter_specific_t));
  adapter->specific = adapter_specific;

  adapter->mtu      = 0xFFFFFFFFUL;
 
  if (!strcmp(adapter->dir_adapter->name, "default"))
    {
      adapter_specific->local_adapter_id = 0;
    }
  else
    {
      adapter_specific->local_adapter_id = atoi(adapter->dir_adapter->name);
    }

  adapter_specific->local_node_id =
    mad_sisci_get_node_id(adapter_specific->local_adapter_id);  

  parameter_string   =
    tbx_string_init_to_int(adapter_specific->local_node_id);
  adapter->parameter = tbx_string_to_c_string(parameter_string);
  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_sisci_channel_init(p_mad_channel_t channel)
{
  p_mad_dir_channel_t            dir_channel = NULL;
  p_mad_sisci_channel_specific_t specific    = NULL;
  ntbx_process_lrank_t           size        =   -1;

  LOG_IN();
  specific          = TBX_MALLOC(sizeof(mad_sisci_channel_specific_t));
  channel->specific = specific;
  dir_channel       = channel->dir_channel;
  size              = ntbx_pc_local_max(dir_channel->pc);
  specific->read    = TBX_MALLOC(sizeof(p_mad_sisci_status_t) * size);
  specific->next    = 0;
  specific->max     = size + 1;
  LOG_OUT();
}

void
mad_sisci_connection_init(p_mad_connection_t in,
			  p_mad_connection_t out)
{
  p_mad_channel_t                   channel          = NULL;
  p_mad_sisci_channel_specific_t    channel_specific = NULL;
  p_mad_adapter_t                   adapter          = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific = NULL;
  p_mad_sisci_connection_specific_t specific         = NULL;
  ntbx_process_lrank_t              lrank_l          =   -1;
  ntbx_process_lrank_t              lrank_r          =   -1;
  int                               segment          =    0;
  int                               ch_id            =    0;
  
  LOG_IN();
  specific      = TBX_MALLOC(sizeof(mad_sisci_connection_specific_t));
  in->specific  = specific;
  out->specific = specific;
  in->nb_link   = 2;
  out->nb_link  = 2;

  channel          = in->channel;
  channel_specific = channel->specific;
  adapter          = channel->adapter;
  adapter_specific = adapter->specific;

  ch_id = channel->dir_channel->id;
  
  if (channel->type == mad_channel_type_forwarding)
    {
      ch_id |= 0x80;
    }
  
  lrank_l = channel->process_lrank;
  lrank_r = in->remote_rank;
  
  for (segment = 0; segment < 2; segment++)
    {	  
      p_mad_sisci_local_segment_t     local_segment = NULL;
      p_mad_sisci_user_segment_data_t local_data    = NULL;
      sci_error_t                     sisci_error   = SCI_ERR_OK;
      int i                                         =    0;
      
      local_segment = &specific->local_segment[segment];
 
      local_segment->id = 
	ch_id
	| (lrank_l << 8)
	| (lrank_r << 16)
	| (segment << 28);

      local_segment->size   = sizeof(mad_sisci_user_segment_data_t);      
      local_segment->offset = 0;

      SCIOpen(&specific->sd[segment], 0, &sisci_error);
      mad_sisci_control();
      
      SCICreateSegment(specific->sd[segment],
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
	  
     local_data = local_segment->map_addr;

      for (i = 0; i < MAD_SISCI_OPT_MAX; i++)
	{
	  local_data->status.read.buffer[i] = 0;
	}

      local_data->status.read.flag  = tbx_flag_clear;
      local_data->status.write.flag = tbx_flag_clear;


      if (!segment)
	{
	  channel_specific->read[lrank_r] = &(local_data->status.read);
	}

      SCISetSegmentAvailable(local_segment->segment,
			     adapter_specific->local_adapter_id,
			     0, &sisci_error);
      mad_sisci_control();
    }  
  LOG_OUT();
}

void 
mad_sisci_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  pm2debug_flush();
  lnk->link_mode   = mad_link_mode_buffer_group;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_sisci_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_sisci_connect(p_mad_connection_t   out,
		  p_mad_adapter_info_t ai)
{  
  p_mad_channel_t                   channel            = NULL;
  p_mad_adapter_t                   adapter            = NULL;
  p_mad_dir_adapter_t               dir_remote_adapter = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific   = NULL;
  p_mad_sisci_connection_specific_t out_specific       = NULL;
  ntbx_process_lrank_t              lrank_l            =   -1;
  ntbx_process_lrank_t              lrank_r            =   -1;
  sci_error_t                       sisci_error        = SCI_ERR_OK;
  int                               channel_id         =    0;
  int                               segment            =    0;
  int                               remote_node_sci_id =    0;

  LOG_IN();
  channel            = out->channel;
  adapter            = channel->adapter;
  adapter_specific   = adapter->specific;
  out_specific       = out->specific;
  channel_id         = channel->dir_channel->id;
  dir_remote_adapter = ai->dir_adapter;
  remote_node_sci_id = atoi(dir_remote_adapter->parameter);
  
  if (channel->type == mad_channel_type_forwarding)
    {
      channel_id |= 0x80;
    }  

  lrank_l = channel->process_lrank;
  lrank_r = out->remote_rank;

   for (segment = 0; segment < 2; segment++)
    {
      p_mad_sisci_remote_segment_t    remote_segment = NULL;
      p_mad_sisci_user_segment_data_t remote_data    = NULL;
      
      remote_segment = &(out_specific->remote_segment[segment]);
    
      remote_segment->id = 
	channel_id
	| (lrank_r << 8)
	| (lrank_l << 16)
	| (segment << 28);

      remote_segment->size   = sizeof(mad_sisci_user_segment_data_t);
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(out_specific->sd[segment],
			    &remote_segment->segment,
			    remote_node_sci_id,
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0, NULL, SCI_INFINITE_TIMEOUT, 0, &sisci_error);

	  if (sisci_error != SCI_ERR_OK)
	    {
	      mad_sisci_display_error(sisci_error);
	      DISP("mad_sisci: could not connect, sleeping ...");
#ifdef MARCEL
	      marcel_delay(1000);
#else // MARCEL
	      sleep(1);
#endif // MARCEL	      
	      DISP("mad_sisci: could not connect, waking up");
	    }
	}
      while (sisci_error != SCI_ERR_OK);

      remote_segment->map_addr =
	SCIMapRemoteSegment(remote_segment->segment,
			    &remote_segment->map,
			    remote_segment->offset,
			    remote_segment->size,
			    NULL, 0, &sisci_error);
      mad_sisci_control();
  
      SCICreateMapSequence(remote_segment->map, &remote_segment->sequence,
			   0, &sisci_error);
      mad_sisci_control();

      remote_data = remote_segment->map_addr;
      mad_sisci_set(&remote_data->status.write);
      mad_sisci_flush(remote_segment);
      out_specific->write_flag_flushed = tbx_true;
    }
  LOG_OUT();
}

void
mad_sisci_accept(p_mad_connection_t   in,
		 p_mad_adapter_info_t ai)
{
  p_mad_channel_t                   channel            = NULL;
  p_mad_adapter_t                   adapter            = NULL;
  p_mad_dir_adapter_t               dir_remote_adapter = NULL;
  p_mad_sisci_adapter_specific_t    adapter_specific   = NULL;
  p_mad_sisci_connection_specific_t in_specific        = NULL;
  ntbx_process_lrank_t              lrank_l            =   -1;
  ntbx_process_lrank_t              lrank_r            =   -1;
  sci_error_t                       sisci_error        = SCI_ERR_OK;
  int                               channel_id         =    0;
  int                               segment            =    0;
  int                               remote_node_sci_id =    0;

  LOG_IN();
  channel            = in->channel;
  adapter            = channel->adapter;
  adapter_specific   = adapter->specific;
  in_specific        = in->specific;
  channel_id         = channel->dir_channel->id;
  dir_remote_adapter = ai->dir_adapter;
  remote_node_sci_id = atoi(dir_remote_adapter->parameter);

  if (channel->type == mad_channel_type_forwarding)
    {
      channel_id |= 0x80;
    }  

  lrank_l = channel->process_lrank;
  lrank_r = in->remote_rank;

  for (segment = 0; segment < 2; segment++)
    {      
      p_mad_sisci_remote_segment_t    remote_segment = NULL;
      p_mad_sisci_user_segment_data_t remote_data    = NULL;
     
      remote_segment = &(in_specific->remote_segment[segment]);

      remote_segment->id = 
	channel_id
	| (lrank_r << 8)
	| (lrank_l << 16)
	| (segment << 28);  

      remote_segment->size   = sizeof(mad_sisci_user_segment_data_t);
      remote_segment->offset = 0;

      do
	{
	  SCIConnectSegment(in_specific->sd[segment],
			    &remote_segment->segment,
			    remote_node_sci_id,
			    remote_segment->id,
			    adapter_specific->local_adapter_id,
			    0, NULL, SCI_INFINITE_TIMEOUT, 0, &sisci_error);

	  if (sisci_error != SCI_ERR_OK)
	    {
	      mad_sisci_display_error(sisci_error);
	      DISP("mad_sisci: could not connect, sleeping ...");
#ifdef MARCEL
	      marcel_delay(1000);
#else // MARCEL
	      sleep(1);
#endif // MARCEL	      
	      DISP("mad_sisci: could not connect, waking up");
	    }
	}
      while (sisci_error != SCI_ERR_OK);

      remote_segment->map_addr =
	SCIMapRemoteSegment(remote_segment->segment,
			    &remote_segment->map,
			    remote_segment->offset,
			    remote_segment->size,
			    NULL, 0, &sisci_error);
      mad_sisci_control();
  
      SCICreateMapSequence(remote_segment->map, &remote_segment->sequence,
			   0, &sisci_error);
      mad_sisci_control();

      remote_data = remote_segment->map_addr;
      mad_sisci_set(&remote_data->status.write);
      mad_sisci_flush(remote_segment);
      in_specific->write_flag_flushed = tbx_true;
    }
  LOG_OUT();
}

void
mad_sisci_after_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
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
  sci_error_t                         sisci_error         = SCI_ERR_OK;
  int k;

  LOG_IN();
  for (k = 0; k < 2; k++)
    {
      p_mad_sisci_local_segment_t  local_segment  =
	&(connection_specific->local_segment[k]);
      p_mad_sisci_remote_segment_t remote_segment =
	&(connection_specific->remote_segment[k]);

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
  
      SCIClose(connection_specific->sd[k], 0, &sisci_error);
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
  if (size <= MAD_SISCI_OPT_MAX)
    {
      return connection->link_array[MAD_SISCI_LINK_OPT];
    }
  else
    {
      return connection->link_array[MAD_SISCI_LINK_REGULAR];
    }
}


void
mad_sisci_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_sisci_adapter_specific_t adapter_specific = adapter->specific;
  
  LOG_IN();
  TBX_FREE(adapter_specific->remote_node_id);
  TBX_FREE(adapter_specific);
  adapter->specific = NULL;
  LOG_OUT();
}

void
mad_sisci_channel_exit(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = channel->specific;
  
  LOG_IN();
  TBX_FREE(channel_specific->read);
  TBX_FREE(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t 
mad_sisci_poll_message(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = NULL;
  p_mad_connection_t             in               = NULL;
  p_tbx_darray_t                 in_darray        = NULL;
  int                            next             =    0;
  int                            max              =    0;
  int                            i                =    0;
 
  LOG_IN();
  channel_specific = channel->specific;
  in_darray        = channel->in_connection_darray;
  max              = channel_specific->max;
  next             = channel_specific->next;
  i                = max;
      
  while (i--)
    {
      next = (next + 1) % max;
      in   = tbx_darray_get(in_darray, next);

      if (in)
	{
	  p_mad_sisci_connection_specific_t in_specific = NULL;
	  
	  in_specific = in->specific;

	  if (!in_specific->write_flag_flushed)
	    {
	      p_mad_sisci_remote_segment_t remote_segment = NULL;

	      remote_segment = &(in_specific->remote_segment[0]);

	      in_specific->write_flag_flushed = tbx_true;
	      mad_sisci_flush(remote_segment);
	    }

	  if (mad_sisci_test(channel_specific->read[next]))
	    goto found;
	}
    }

  in = NULL;

found:
  channel_specific->next = next;
  
  LOG_OUT();

  return in;
}
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t 
mad_sisci_receive_message(p_mad_channel_t channel)
{
  p_mad_sisci_channel_specific_t channel_specific = NULL;
  p_mad_connection_t             in               = NULL;
  p_tbx_darray_t                 in_darray        = NULL;
  int                            next             =    0;
  int                            max              =    0;
 
  LOG_IN();
  channel_specific = channel->specific;
  in_darray        = channel->in_connection_darray;
  max              = channel_specific->max;
  next             = channel_specific->next;

#if defined(MARCEL) && defined(USE_MARCEL_POLL)
  {
    p_mad_sisci_driver_specific_t    driver_specific = NULL;
    mad_sisci_marcel_poll_cell_arg_t arg;

    driver_specific = channel->adapter->driver->specific;

    arg.op                          = mad_sisci_poll_channel;
    arg.data.channel_op.channel     = channel;
    arg.data.channel_op.connection  = NULL;

    marcel_poll(driver_specific->mad_sisci_pollid, &arg);

    in = arg.data.channel_op.connection;
  }
#else // MARCEL && USE_MARCEL_POLL
  while (tbx_true)
    {     
      int i = 0;

      i = max;
      
      while (i--)
	{
	  next = (next + 1) % max;
	  in   = tbx_darray_get(in_darray, next);

	  if (in)
	    {
	      p_mad_sisci_connection_specific_t in_specific = NULL;
	  
	      in_specific = in->specific;

	      if (!in_specific->write_flag_flushed)
		{
		  p_mad_sisci_remote_segment_t remote_segment = NULL;

		  remote_segment = &(in_specific->remote_segment[0]);

		  mad_sisci_flush(remote_segment);
		  in_specific->write_flag_flushed = tbx_true;
		}

	      if (mad_sisci_test(channel_specific->read[next]))
		goto found;
	    }
	}
      TBX_YIELD();
    }  

found:
  channel_specific->next = next;
#endif // MARCEL && USE_MARCEL_POLL
  
  LOG_OUT();

  return in;
}


static void
mad_sisci_send_sci_buffer(p_mad_link_t   link,
		       p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  int k                                                 = 0;
  size_t segment_size                                   = 
    connection_specific->remote_segment[0].size
    - sizeof(mad_sisci_connection_status_t);

  LOG_IN();

  if ((buffer->bytes_written - buffer->bytes_read) < (segment_size << 2))
    {
      segment_size = (buffer->bytes_written - buffer->bytes_read) >> 2;
      if (segment_size < MAD_SISCI_MIN_SEG_SIZE)
	segment_size = MAD_SISCI_MIN_SEG_SIZE;
    }

  if (mad_more_data(buffer))
    {
      do 
	{
	  p_mad_sisci_local_segment_t       local_segment       =
	    &(connection_specific->local_segment[k]);
	  p_mad_sisci_remote_segment_t      remote_segment      =
	    &(connection_specific->remote_segment[k]);
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
	  size_t   size        =
	    min(buffer->bytes_written - buffer->bytes_read, segment_size);
	  size_t   mod_4       = size % 4;
	  sci_error_t sisci_error;

	  buffer->bytes_read += size;

	  mad_sisci_wait_for(link, write);

	  SCIMemCopy(source,
		     remote_segment->map,
		     sizeof(mad_sisci_connection_status_t),
		     size - mod_4,
		     0,
		     &sisci_error);
	  //mad_sisci_control();

	  if (mod_4)
	    {
	      volatile char    *destination =
		remote_data->buffer + (size - mod_4);
	      source += size - mod_4;
	      size += 4 - mod_4;

	      *(unsigned int *)destination = 0;

	      while (mod_4--)
		{
		  *destination++ = *source++;
		}
	    }
	  else
	    {
	      source += size;
	    }

	  if (size & 63)
	    {
	      volatile int *destination =
		(volatile void *)remote_data->buffer + size;
	  
	      do
		{
		  *destination++ = 0;
		  size += 4;
		}
	      while (size & 63);
	    }
      
	  mad_sisci_flush(remote_segment);
	  mad_sisci_clear(write);
	  mad_sisci_set(read);
	  mad_sisci_flush(remote_segment);
	  k ^= 1;
	}
      while (mad_more_data(buffer));

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
  int k = 0;
  size_t segment_size                                   = 
    connection_specific->remote_segment[0].size
    - sizeof(mad_sisci_connection_status_t);

  if ((buffer->length - buffer->bytes_written) < (segment_size << 2))
    {
      segment_size = (buffer->length - buffer->bytes_written) >> 2;
      if (segment_size < MAD_SISCI_MIN_SEG_SIZE)
	segment_size = MAD_SISCI_MIN_SEG_SIZE;
    }
  

  LOG_IN();
  
  if (!mad_buffer_full(buffer))
    {
      size_t  size;

      {
	p_mad_sisci_local_segment_t       local_segment       =
	  &(connection_specific->local_segment[k]);
	p_mad_sisci_remote_segment_t      remote_segment      =
	  &(connection_specific->remote_segment[k]);
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
	char   *source = local_data->buffer;
	size = min(buffer->length - buffer->bytes_written, segment_size);

	if (!connection_specific->write_flag_flushed)
	  {
	    connection_specific->write_flag_flushed = tbx_true;
	    mad_sisci_flush(remote_segment);
	  }
      
	mad_sisci_wait_for(link, read);

	memcpy(destination, source, size);
	mad_sisci_clear(read);
	mad_sisci_set(write);
	buffer->bytes_written +=size;
	destination += size;
	k = 1;
      }

      while (!mad_buffer_full(buffer))
	{
	  p_mad_sisci_local_segment_t       local_segment       =
	    &(connection_specific->local_segment[k]);
	  p_mad_sisci_remote_segment_t      remote_segment      =
	    &(connection_specific->remote_segment[k]);
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
	  char   *source = local_data->buffer;
	  size = min(buffer->length - buffer->bytes_written, segment_size);
	  mad_sisci_flush(remote_segment);
	  mad_sisci_wait_for(link, read);
	  memcpy(destination, source, size);
	  mad_sisci_clear(read);
	  mad_sisci_set(write);
	  buffer->bytes_written +=size;
	  destination += size;
	  k ^= 1;
	}      

      connection_specific->write_flag_flushed = tbx_false;
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
    &(connection_specific->local_segment[0]);
  p_mad_sisci_remote_segment_t      remote_segment      =
    &(connection_specific->remote_segment[0]);
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  volatile p_mad_sisci_status_t     read                =
    &remote_data->status.read;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  tbx_list_reference_t              ref;
  volatile char                    *destination         =
    remote_data->buffer;
  size_t                            offset              = 0;
  size_t                            destination_size    =
    remote_segment->size - sizeof(mad_sisci_connection_status_t);
  
  LOG_IN();
  tbx_list_reference_init(&ref, &buffer_group->buffer_list); 
  
  mad_sisci_wait_for(link, write);
  
  do
    {
      p_mad_buffer_t buffer = tbx_get_list_reference_object(&ref);
      char          *source = buffer->buffer + buffer->bytes_read;

      while (mad_more_data(buffer))
	{
	  size_t         size         =
	    min(buffer->bytes_written - buffer->bytes_read,
	    destination_size - offset);
	  size_t         mod_4        = size % 4;
	  size_t         aligned_size = size - mod_4;
	  sci_error_t    sisci_error;

	  buffer->bytes_read += size;
	  
	  SCIMemCopy(source,
		     remote_segment->map,
		     sizeof(mad_sisci_connection_status_t) + offset,
		     aligned_size,
		     0,
		     &sisci_error);
	  mad_sisci_control();

	  offset += aligned_size;
	  source += aligned_size;
	  
	  while (mod_4--)
	    {
	      *(destination+(offset++)) = *source++;
	    }
	  
	  while (offset & 0x03)
	    {
	      *(destination+(offset++)) = 0;
	    }	  

	  if (tbx_aligned(offset, 64) >= destination_size)
	    {
	      while (offset & 0x30)
		{
		  *(unsigned int *)(destination+offset) = 0;
		  offset += 4;
		}	  

	      mad_sisci_flush(remote_segment);

	      mad_sisci_clear(write);
	      mad_sisci_set(read);
	      mad_sisci_flush(remote_segment);

	      mad_sisci_wait_for(link, write);

	      offset = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      while (offset & 0x30)
	{
	  *(unsigned int *)(destination+offset) = 0;
	  offset += 4;
	}	  

      mad_sisci_flush(remote_segment);

      mad_sisci_clear(write);
      mad_sisci_set(read);
      mad_sisci_flush(remote_segment);
    }

  connection_specific->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void 
mad_sisci_receive_sci_buffer_group(p_mad_link_t         link,
				   p_mad_buffer_group_t buffer_group)
{  
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &(connection_specific->local_segment[0]);
  p_mad_sisci_remote_segment_t      remote_segment      =
    &(connection_specific->remote_segment[0]);
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
  size_t                            offset              = 0;
  size_t                            source_size         =
    local_segment->size - sizeof(mad_sisci_connection_status_t);
  
  LOG_IN();
  
  tbx_list_reference_init(&ref, &buffer_group->buffer_list);

  if (!connection_specific->write_flag_flushed)
    {
      connection_specific->write_flag_flushed = tbx_true;

      if (!mad_sisci_test(read))
	{
	  mad_sisci_flush(remote_segment);

	  mad_sisci_wait_for(link, read);
	}
    }
  else
    {  
      mad_sisci_wait_for(link, read);
    }
      
  do
    {
      p_mad_buffer_t buffer      = tbx_get_list_reference_object(&ref);
      char          *destination =
	buffer->buffer + buffer->bytes_written;

      while (!mad_buffer_full(buffer))
	{
	  size_t         size         =
	    min(buffer->length - buffer->bytes_written,
		source_size - offset);

	  buffer->bytes_written += size;
	  memcpy(destination, (const void *)(source + offset), size);

	  offset += tbx_aligned(size, 4);
	  destination += size;

	  if (tbx_aligned(offset, 64) >= source_size)
	    {
	      mad_sisci_clear(read);
	      mad_sisci_set(write);
	      mad_sisci_flush(remote_segment);
	      connection_specific->write_flag_flushed = tbx_true;
	      mad_sisci_wait_for(link, read);
	      offset = 0;
	    }
	}
    }
  while (tbx_forward_list_reference(&ref));

  if (offset > 0)
    {
      mad_sisci_clear(read);
      mad_sisci_set(write);
      connection_specific->write_flag_flushed = tbx_false;
    }  
  LOG_OUT();
}

/*
 * Optimized routines
 * ------------------
 */
static void
mad_sisci_send_sci_buffer_opt(p_mad_link_t   link,
			      p_mad_buffer_t buffer)
{
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment[0];
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment[0];
  unsigned char                    *source              =
    buffer->buffer + buffer->bytes_read;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  volatile p_mad_sisci_status_t     write               =
    &local_data->status.write;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  register volatile unsigned int   *data_remote_ptr     =
    (volatile void *)remote_data->status.read.buffer;
  volatile unsigned int            *remote_ptr          = NULL;
  register unsigned int             descriptor          = 1;
  register unsigned int             mask_1              = 2;
  register size_t                   bread               =
    buffer->bytes_read;
  const size_t                      bwrite              =
    buffer->bytes_written;
  const size_t                      bwrite3             = bwrite - 3;
  int                               j                   = 0;

  LOG_IN();
  mad_sisci_wait_for(link, write);
  mad_sisci_clear(write);

  remote_ptr = data_remote_ptr++;
  if (bwrite > 3)
    while (bread < bwrite3)
      {
	bread += 4;

	if (*(unsigned int *)source)
	  {
	    *data_remote_ptr++ = *(unsigned int *)source;
	    descriptor |= mask_1;
	  }

	source += 4;
	mask_1 <<= 1;

	j++;

	if (j >= 7)
	  {
	    *remote_ptr = descriptor;
	    descriptor  = 1;
	    mask_1      = 2;
	    j           = 0;
	  
	    if (bread < bwrite)
	      {
		remote_ptr = data_remote_ptr++;
	      }
	  }
      }

  if (bread < bwrite)
    {
      unsigned int temp_buffer = 0;
      int          i           = 0;

      do
	{
	  temp_buffer += ((unsigned int)*source) << i;
	  source++;
	  i += 8;
	  bread++;
	}
      while (bread < bwrite);

      if (temp_buffer)
	{
	  *data_remote_ptr++ = temp_buffer;
	  descriptor |= mask_1;
	}
      *remote_ptr = descriptor;
    }
  else if (j)
    {
      *remote_ptr = descriptor;
    }    
  
  while (((unsigned int)data_remote_ptr) & 0x30)
    {
      *data_remote_ptr++ = 0;
    }

  mad_sisci_flush(remote_segment);
  buffer->bytes_read = bread;
  connection_specific->write_flag_flushed = tbx_true;
  LOG_OUT();
}

static void
mad_sisci_receive_sci_buffer_opt(p_mad_link_t   link, 
				   p_mad_buffer_t buffer)
{  
  p_mad_connection_t                connection          = link->connection;
  p_mad_sisci_connection_specific_t connection_specific =
    connection->specific;
  p_mad_sisci_local_segment_t       local_segment       =
    &connection_specific->local_segment[0];
  p_mad_sisci_remote_segment_t      remote_segment      =
    &connection_specific->remote_segment[0];
  unsigned char                    *destination         =
    buffer->buffer + buffer->bytes_written;
  p_mad_sisci_user_segment_data_t   local_data          =
    local_segment->map_addr;
  p_mad_sisci_user_segment_data_t   remote_data         =
    remote_segment->map_addr;
  p_mad_sisci_status_t              write               =
    &remote_data->status.write;
  volatile unsigned int            *local_ptr           =
    (volatile void *)local_data->status.read.buffer;
  unsigned int                      descriptor          = 0;
  unsigned int                      mask_1              = 2;
  int                               j                   = 0;
  const size_t                      blen                =
    buffer->length;
  const size_t                      blen3               =
    blen - 3;
  register size_t                   bwrite              =
    buffer->bytes_written;
  
  LOG_IN();
  if (!connection_specific->write_flag_flushed)
    {
      connection_specific->write_flag_flushed = tbx_true;
      mad_sisci_flush(remote_segment);
    }

#ifdef MARCEL
  while (!(descriptor = *local_ptr)) TBX_YIELD();
#endif /* MARCEL */

  if (blen > 3)
    while (bwrite < blen3)
      {
	
	if (!j)
	  {
	    while (!(descriptor = *local_ptr));
	    *local_ptr++ = 0;
	    mask_1       = 2;
	    j++;
	  }
	
	if (descriptor & mask_1)
	  {
	    unsigned int int_buffer;

	    while (!(int_buffer = *local_ptr));      
	    *(unsigned int *)destination = int_buffer;
	    *local_ptr++ = 0;
	  }
	else
	  {
	    *(unsigned int *)destination = 0;
	  }

	mask_1 <<= 1;
	j = (j + 1) & 0x7;
	destination += 4;
	bwrite      += 4;
      }  

  if (bwrite < blen)
    {
      unsigned int temp_buffer = 0;
      int          i           = 0;
      
      if (!j)
	{
	  while (!(descriptor = *local_ptr));
	  *local_ptr++ = 0;
	  mask_1       = 2;
	}
    
      if (descriptor & mask_1)
	{
	  while (!(temp_buffer = *local_ptr));
	  *local_ptr++ = 0;
	}
      else
	{
	  temp_buffer = 0;
	}

      do
	{
	  *destination++ = (unsigned char)(temp_buffer >> i);
	  i += 8;
	  bwrite++;
	}
      while (bwrite < blen);
    }

  mad_sisci_set(write);
  connection_specific->write_flag_flushed = tbx_false;
  //connection_specific->write_flag_flushed = tbx_true;
  //mad_sisci_flush(remote_segment);
  buffer->bytes_written = bwrite;
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
  if (lnk->id == MAD_SISCI_LINK_OPT)
    {
      mad_sisci_send_sci_buffer_opt(lnk, buffer);
    }
  else if (lnk->id == MAD_SISCI_LINK_REGULAR)
    {
      mad_sisci_send_sci_buffer(lnk, buffer);
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
  if (lnk->id == MAD_SISCI_LINK_OPT)
    {
      mad_sisci_receive_sci_buffer_opt(lnk, *buffer);
    }
  else if (lnk->id == MAD_SISCI_LINK_REGULAR)
    {
      mad_sisci_receive_sci_buffer(lnk, *buffer);
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
  if (   (lnk->id == MAD_SISCI_LINK_OPT)
      || (lnk->id == MAD_SISCI_LINK_REGULAR))
    {
      mad_sisci_send_sci_buffer_group(lnk, buffer_group);
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
  if (   (lnk->id == MAD_SISCI_LINK_OPT)
      || (lnk->id == MAD_SISCI_LINK_REGULAR))
    {
      mad_sisci_receive_sci_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("Invalid link ID");
  LOG_OUT();
}

