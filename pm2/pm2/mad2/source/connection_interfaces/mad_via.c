
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
 * Mad_via.c
 * =========
 */

#include "madeleine.h"
#include <vipl.h>
#include <unistd.h>

/* #define BOOST */

/*
 * Types definition
 * -----------------
 */

/* *** Credits ........................................................ *** */
typedef int mad_via_credit_t;

typedef struct
{
  mad_via_credit_t max_credits;
  mad_via_credit_t available_credits;
  mad_via_credit_t returned_credits;
  mad_via_credit_t alert;  
} mad_via_credits_t, *p_mad_via_credits_t;

/* *** vi abstraction ................................................. *** */
typedef struct
{
  unsigned char adapter_id;
  unsigned char channel_id;
  unsigned char host_id;
  unsigned char vi_id;
} mad_via_discriminator_t, *p_mad_via_discriminator_t;

typedef struct
{
  VIP_DESCRIPTOR  *descriptor;
  VIP_MEM_HANDLE   handle;
  size_t           number;
  size_t           size;
  /* Added for keeping track of allocated buffers for control and ack */
  void            *buffer;
  VIP_MEM_HANDLE   buffer_handle;
} mad_via_descriptor_t, *p_mad_via_descriptor_t;

typedef struct
{
  p_mad_via_descriptor_t descriptor;
  p_mad_buffer_t         buffer;
} mad_via_way_t, *p_mad_via_way_t;

typedef struct
{
  VIP_VI_HANDLE           handle;
  mad_via_discriminator_t discriminator;
  mad_via_way_t           in;
  mad_via_way_t           out;
  mad_via_credits_t       credits;
} mad_via_vi_t, *p_mad_via_vi_t;


/* *** MadII's driver specific data structures ........................ *** */
typedef struct
{
  int nb_adapter;
} mad_via_driver_specific_t, *p_mad_via_driver_specific_t;

typedef struct
{
  VIP_NIC_HANDLE        nic_handle;
  VIP_NIC_ATTRIBUTES    nic_attributes;
  VIP_PROTECTION_HANDLE ptag;
  VIP_MEM_ATTRIBUTES    memory_attributes;
} mad_via_adapter_specific_t, *p_mad_via_adapter_specific_t;

typedef struct
{
  VIP_CQ_HANDLE completion_queue;
} mad_via_channel_specific_t, *p_mad_via_channel_specific_t;

typedef struct
{
  p_mad_via_vi_t     vi;
  tbx_bool_t       posted; /* ack reception ready ? */
  tbx_bool_t       new_message;
} mad_via_connection_specific_t, *p_mad_via_connection_specific_t;

typedef struct
{
  mad_via_vi_t vi;
} mad_via_link_specific_t, *p_mad_via_link_specific_t;


/*
 * Macros
 * -------
 */

#define VIA_CTRL(op) \
{\
  VIP_RETURN mad_via_status;\
  if ((mad_via_status = (op)) != (VIP_SUCCESS)) \
    {\
      fprintf(stderr, "%s failed: \nFILE: %s\nLINE: %d\nError code :", \
              #op, __FILE__, __LINE__); \
      mad_via_disp_status(mad_via_status); \
      abort(); }}


#ifdef PM2DEBUG
#define VIA_VERIFY(op) \
{\
  VIP_RETURN mad_via_status;\
  if ((mad_via_status = (op)) != (VIP_SUCCESS)) \
    {\
      fprintf(stderr, "%s failed: \nFILE: %s\nLINE: %d\nError code :", \
              #op, __FILE__, __LINE__); \
      mad_via_disp_status(mad_via_status); \
      exit(1);}}

#else /* PM2DEBUG */
#define VIA_VERIFY(op) ((void)(op))
#endif /* PM2DEBUG */

/*
 * Implementation macros
 * ---------------------
 */

#define MAD_VIA_CREDIT_LINK                   0
#define MAD_VIA_MESSAGE_LINK                  1
#define MAD_VIA_RDMA_LINK                     2

#define MAD_VIA_SHORT_BUFFER_LIMIT           32768
//#define MAD_VIA_SHORT_BUFFER_LIMIT         5120 /* 5KB*/
#define MAD_VIA_LONG_BUFFER_LIMIT         32768 /* 32KB = VIA minimum MTU */

#define MAD_VIA_NB_CREDITS                   32
#define MAD_VIA_NB_CONNECTION_CREDITS        64
#define MAD_VIA_STATIC_BUFFER_SIZE           64

#define MAD_VIA_INITIAL_STATIC_BUFFER_NUM (MAD_VIA_NB_CREDITS * 2)
#define CONTROL_BUFFER_SIZE                 512
#define MAD_VIA_BUFFER_ALIGNMENT             64
#define MAD_VIA_MAX_TRANSFER_SIZE         32768
#define MAD_VIA_MAX_DATA_SEGMENT            252

#define MAD_VIA_LINK_CREDIT_NUMBER MAD_VIA_NB_CREDITS
#define MAD_VIA_ALIGNED(x, a) (((unsigned)(x) + (a) - 1) & ~(unsigned)((a) - 1))
#define MAD_VIA_CONNECTION_DESCRIPTOR_SIZE \
  MAD_VIA_ALIGNED((sizeof(VIP_DESCRIPTOR) \
                   + sizeof(VIP_DATA_SEGMENT)), \
                  VIP_DESCRIPTOR_ALIGNMENT)
#define MAD_VIA_CREDIT_DESCRIPTOR_SIZE \
  MAD_VIA_ALIGNED((sizeof(VIP_DESCRIPTOR) \
                   + sizeof(VIP_DATA_SEGMENT)), \
                  VIP_DESCRIPTOR_ALIGNMENT)
#define MAD_VIA_MESSAGE_DESCRIPTOR_SIZE \
  MAD_VIA_ALIGNED((sizeof(VIP_DESCRIPTOR) \
                   + MAD_VIA_MAX_DATA_SEGMENT * sizeof(VIP_DATA_SEGMENT)), \
                  VIP_DESCRIPTOR_ALIGNMENT)
#define MAD_VIA_RDMA_DESCRIPTOR_SIZE \
  MAD_VIA_ALIGNED((sizeof(VIP_DESCRIPTOR) \
                   + sizeof(VIP_ADDRESS_SEGMENT) \
                   + sizeof(VIP_DATA_SEGMENT)), \
                  VIP_DESCRIPTOR_ALIGNMENT)

#define MAD_VIA_VI_DISCRIMINATOR_LEN sizeof(mad_via_discriminator_t)
#define MAD_VIA_VI_DISCRIMINATOR(adr) (*(mad_via_discriminator_t *) ((adr)->HostAddress + (adr)->HostAddressLen))

/*
 * static variables
 * ----------------
 */
#ifdef PM2
static marcel_mutex_t __pm2_mutex;
#define PM2_VIA_LOCK() marcel_mutex_lock(&__pm2_mutex)
#define PM2_VIA_UNLOCK() marcel_mutex_unlock(&__pm2_mutex)
#else /* PM2 */
#define PM2_VIA_LOCK()
#define PM2_VIA_UNLOCK()
#endif /* PM2 */


/* VIA management
 * --------------
 */

/* mad_via_disp_status: display string corresponding 
   to VIA status value */
static void 
mad_via_disp_status(VIP_RETURN status)
{
  switch (status)
    {
    case VIP_SUCCESS:
      DISP("OK"); break;
    case VIP_NOT_DONE:
      DISP("VIP_NOT_DONE"); break;
    case VIP_INVALID_PARAMETER:
      DISP("VIP_INVALID_PARAMETER"); break;
    case VIP_ERROR_RESOURCE:
      DISP("VIP_ERROR_RESOURCE"); break;
    case VIP_TIMEOUT:
      DISP("VIP_TIMEOUT"); break;
    case VIP_REJECT:
      DISP("VIP_REJECT"); break;
    case VIP_INVALID_RELIABILITY_LEVEL:
      DISP("VIP_INVALID_RELIABILITY_LEVEL"); break;
    case VIP_INVALID_MTU:
      DISP("VIP_INVALID_MTU"); break;
    case VIP_INVALID_QOS:
      DISP("VIP_INVALID_QOS"); break;
    case VIP_INVALID_PTAG:
      DISP("VIP_INVALID_PTAG"); break;
    case VIP_INVALID_RDMAREAD:
      DISP("VIP_INVALID_RDMAREAD"); break;
    case VIP_DESCRIPTOR_ERROR:
      DISP("VIP_DESCRIPTOR_ERROR"); break;
    case VIP_INVALID_STATE:
      DISP("VIP_INVALID_STATE"); break;
    case VIP_ERROR_NAMESERVICE:
      DISP("VIP_ERROR_NAMESERVICE"); break;
    case VIP_NO_MATCH:
      DISP("VIP_NO_MATCH"); break;
    default:
      DISP("Unknwon error code"); break;
    }
}


/* mad_via_error_callback: callback called on via asynchronous error
   to display associated error message */
static void __attribute__ ((noreturn))
mad_via_error_callback (VIP_PVOID Context,
			VIP_ERROR_DESCRIPTOR * ErrorDesc)
{

  if (ErrorDesc->ResourceCode == VIP_RESOURCE_VI &&
      ErrorDesc->ErrorCode == VIP_ERROR_CONN_LOST) {
    /* most of the time everything is ok */
    /* we should test here if there are no lost messages */
    //fprintf(stderr,"error callback\n");
    exit(0);

  } else {

    fflush(stderr);
    //sleep(1);
    fflush(stderr);
    fprintf (stderr, "Asynchronous error\nResource code : ");
    
    switch (ErrorDesc->ResourceCode)
      {
      default:
	fprintf (stderr, "Inconnu\n");
	break;
      case VIP_RESOURCE_NIC:
	fprintf (stderr, "VIP_RESOURCE_NIC\n");
	break;
      case VIP_RESOURCE_VI:
	fprintf (stderr, "VIP_RESOURCE_VI\n");
	break;
      case VIP_RESOURCE_CQ:
	fprintf (stderr, "VIP_RESOURCE_CQ\n");
	break;
      case VIP_RESOURCE_DESCRIPTOR:
	fprintf (stderr, "VIP_RESOURCE_DESCRIPTOR\n");
	break;
      }
    
    fprintf (stderr, "Descriptor error code : ");
    switch (ErrorDesc->ErrorCode)
      {
      default:
	fprintf (stderr, "Inconnu\n");
	break;
      case VIP_ERROR_POST_DESC:
	fprintf (stderr, "VIP_ERROR_POST_DESC\n");
	break;
      case VIP_ERROR_CONN_LOST:
	fprintf (stderr, "VIP_ERROR_CONN_LOST\n");
	break;
      case VIP_ERROR_RECVQ_EMPTY:
	fprintf (stderr, "VIP_ERROR_RECVQ_EMPTY\n");
	break;
      case VIP_ERROR_VI_OVERRUN:
	fprintf (stderr, "VIP_ERROR_VI_OVERRUN\n");
	break;
      case VIP_ERROR_RDMAW_PROT:
	fprintf (stderr, "VIP_ERROR_RDMAW_PROT\n");
	break;
      case VIP_ERROR_RDMAW_DATA:
	fprintf (stderr, "VIP_ERROR_RDMAW_DATA\n");
	break;
      case VIP_ERROR_RDMAW_ABORT:
	fprintf (stderr, "VIP_ERROR_RDMAW_ABORT\n");
	break;
      case VIP_ERROR_RDMAR_PROT:
	fprintf (stderr, "VIP_ERROR_RDMAR_PROT\n");
	break;
      case VIP_ERROR_COMP_PROT:
	fprintf (stderr, "VIP_ERROR_COMP_PROT\n");
	break;
      case VIP_ERROR_RDMA_TRANSPORT:
	fprintf (stderr, "VIP_ERROR_RDMA_TRANSPORT\n");
	break;
      case VIP_ERROR_CATASTROPHIC:
	fprintf (stderr, "VIP_ERROR_CATASTROPHIC\n");
	break;
      }

    fprintf (stderr, "Exiting ...\n");
    exit (EXIT_FAILURE);
  }
}

/* -------------------------------------------------------------------------*/


/* -------------------------------------------------------------------------*/



/* -------------------------------------------------------------------------*/

/*
 * Functions used by Registered functions
 * ---------------------------------------
 */

/* --- mad_via_adapter_configuration_init --- */
static void
mad_via_open_nic(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipOpenNic(adapter->name, &(adapter_specific->nic_handle)));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_set_error_callback(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipErrorCallback(adapter_specific->nic_handle,
			    NULL,
			    mad_via_error_callback));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_ns_init(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipNSInit(adapter_specific->nic_handle, NULL));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_query_nic(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipQueryNic(adapter_specific->nic_handle,
		       &(adapter_specific->nic_attributes)));
  PM2_VIA_UNLOCK();

#ifdef PM2DEBUG
  mad_via_display_nic_attributes(device);
#endif /* PM2DEBUG */

  LOG_OUT();
}

static void
mad_via_create_ptag(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipCreatePtag(adapter_specific->nic_handle,
			 &(adapter_specific->ptag)));
  PM2_VIA_UNLOCK();
  adapter_specific->memory_attributes.Ptag            =
    adapter_specific->ptag;
  adapter_specific->memory_attributes.EnableRdmaWrite = VIP_TRUE;
  adapter_specific->memory_attributes.EnableRdmaRead  = VIP_FALSE;
  LOG_OUT();
}

/* --- mad_via_channel_init --- */

static void
mad_via_create_completion_queue(p_mad_channel_t channel)
{
  p_mad_adapter_t              adapter          = channel->adapter;
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;
  p_mad_driver_t               driver           = adapter->driver;
  p_mad_configuration_t        configuration    = driver->madeleine->configuration;

  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipCreateCQ(adapter_specific->nic_handle,
		       configuration->size,
		       &(
			 (
			  (p_mad_via_channel_specific_t) channel->specific 
			 )->completion_queue 
			)
		       ));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

/* --- mad_via_connection_init --- */

static void
mad_via_create_input_connection_vi(p_mad_connection_t connection,
				   p_mad_via_vi_t     vi)
{
  p_mad_channel_t                   channel             = connection->channel;
  p_mad_via_channel_specific_t      channel_specific    = channel->specific;
  p_mad_adapter_t                   adapter             = channel->adapter;
  p_mad_via_adapter_specific_t      adapter_specific    = adapter->specific;
  VIP_VI_ATTRIBUTES attributes;

  LOG_IN();
  attributes.ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  attributes.Ptag             = adapter_specific->ptag;
  attributes.EnableRdmaWrite  = VIP_TRUE;
  attributes.EnableRdmaRead   = VIP_FALSE;
  attributes.QoS              = 0;
  attributes.MaxTransferSize  = MAD_VIA_MAX_TRANSFER_SIZE;

  PM2_VIA_LOCK();
  VIA_CTRL(VipCreateVi(adapter_specific->nic_handle,
		       &attributes,
		       NULL,
		       channel_specific->completion_queue,
		       &(vi->handle)));
  PM2_VIA_UNLOCK();
  
  LOG_OUT();
}

static void
mad_via_create_vi(p_mad_connection_t connection,
		  p_mad_via_vi_t     vi)
{
  /*p_mad_via_connection_specific_t connection_specific = connection->specific;*/
  p_mad_channel_t                 channel             = connection->channel;
  p_mad_adapter_t                 adapter             = channel->adapter;
  p_mad_via_adapter_specific_t        adapter_specific    = adapter->specific;
  VIP_VI_ATTRIBUTES attributes;
  
  LOG_IN();
  attributes.ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  attributes.Ptag             = adapter_specific->ptag;
  attributes.EnableRdmaWrite  = VIP_TRUE;
  attributes.EnableRdmaRead   = VIP_FALSE;
  attributes.QoS              = 0;
  attributes.MaxTransferSize  = MAD_VIA_MAX_TRANSFER_SIZE;

  PM2_VIA_LOCK();
  //fprintf(stderr,"VipCreateVi\n");
  VIA_CTRL(VipCreateVi(adapter_specific->nic_handle,
		       &attributes,
		       NULL,
		       NULL,
		       &(vi->handle)));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_init_credits(p_mad_via_credits_t credits,
		     mad_via_credit_t    nb_credits)
{
  credits->max_credits       = nb_credits;
  credits->available_credits = credits->max_credits;
  credits->returned_credits  = 0;
  credits->alert             = 2;
}

static p_mad_via_descriptor_t
mad_via_allocate_descriptor_array(p_mad_connection_t connection,
				  size_t             descriptor_size,
				  size_t             array_size)
{
  p_mad_channel_t           channel          = connection->channel;
  p_mad_adapter_t           adapter          = channel->adapter;
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;
  p_mad_via_descriptor_t    descriptor_array;
  int                       i;
  p_mad_via_descriptor_t descriptor;

  void *descriptor_buffer;

  LOG_IN();
  descriptor_array = malloc(array_size * sizeof(mad_via_descriptor_t));

  descriptor_buffer = tbx_aligned_malloc(array_size*descriptor_size, 64);

  descriptor = descriptor_array;

  PM2_VIA_LOCK();
  VIA_CTRL(VipRegisterMem(adapter_specific->nic_handle,
			  descriptor_buffer,
			  descriptor_size*array_size,
			  &(adapter_specific->memory_attributes),
			  &(descriptor->handle)));
  PM2_VIA_UNLOCK();

  for (i = 0; i < array_size; i++)
    {
      descriptor = descriptor_array + i;

      /*
      descriptor->descriptor = tbx_aligned_malloc(descriptor_size, 64);
      CTRL_ALLOC(descriptor->descriptor);  
      memset(descriptor->descriptor,0,descriptor_size);
      */      

      descriptor->descriptor = descriptor_buffer + i * 64;
      descriptor->size = descriptor_size;      
      /* reference to allocated bloc of data for control and ack */
      descriptor->buffer = NULL;

      descriptor->handle = descriptor_array->handle;

      /*
      PM2_VIA_LOCK();
      VIA_CTRL(VipRegisterMem(adapter_specific->nic_handle,
			      descriptor->descriptor,
			      descriptor->size,
			      &(adapter_specific->memory_attributes),
			      &(descriptor->handle)));
      PM2_VIA_UNLOCK();
      */
    }

  LOG_OUT();
  return descriptor_array;
}

/* -------------------------------------------------------------------------*/

/*
 * Register
 * ---------
 */

void
mad_via_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  interface = &(driver->interface);

  //driver->connection_type = mad_bidirectional_connection;
  driver->connection_type = mad_unidirectional_connection;
  driver->buffer_alignment = 32;

  interface->driver_init                = mad_via_driver_init;
  interface->adapter_init               = mad_via_adapter_init;
  interface->adapter_configuration_init = mad_via_adapter_configuration_init;
  interface->channel_init               = mad_via_channel_init;
  interface->before_open_channel        = mad_via_before_open_channel;
  interface->connection_init            = mad_via_connection_init;
  interface->link_init                  = mad_via_link_init;
  interface->accept                     = mad_via_accept;
  interface->connect                    = mad_via_connect;
  interface->after_open_channel         = mad_via_after_open_channel;
  interface->before_close_channel       = mad_via_before_close_channel;
  interface->disconnect                 = mad_via_disconnect;
  interface->after_close_channel        = mad_via_after_close_channel;
  interface->choice                     = mad_via_choice;
  interface->get_static_buffer          = mad_via_get_static_buffer;
  interface->return_static_buffer       = mad_via_return_static_buffer;
  interface->new_message                = mad_via_new_message;
  interface->receive_message            = mad_via_receive_message;
  interface->send_buffer                = mad_via_send_buffer;
  interface->receive_buffer             = mad_via_receive_buffer;
  interface->send_buffer_group          = mad_via_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_via_receive_sub_buffer_group;
  interface->external_spawn_init        = NULL;
  interface->configuration_init         = NULL;
  interface->send_adapter_parameter     = NULL;
  interface->receive_adapter_parameter  = NULL;

  LOG_OUT();
  //return settings;
}

/* -------------------------------------------------------------------------*/

/*
 * Registered Functions
 * ---------------------
 */

void
mad_via_driver_init(p_mad_driver_t driver)
{
  p_mad_via_driver_specific_t driver_specific;
  
  LOG_IN();
  driver_specific = malloc(sizeof(mad_via_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;
#ifdef PM2
  marcel_mutex_init(&__pm2_mutex, NULL);
#endif /* PM2 */
  driver->name = malloc(4);
  CTRL_ALLOC(driver->name);
  strcpy(driver->name, "via");
  LOG_OUT();
}

void
mad_via_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver           = adapter->driver;
  p_mad_via_driver_specific_t    driver_specific  = driver->specific;
  p_mad_via_adapter_specific_t   adapter_specific;

  LOG_IN();
  adapter_specific = malloc(sizeof(mad_via_adapter_specific_t));
  CTRL_ALLOC(adapter_specific);
  adapter->specific = adapter_specific;

  if (adapter->name == NULL)
    {
      FAILURE("VIA adapter selector must not be NULL");
    }

  driver_specific->nb_adapter++;

  adapter->parameter = malloc(10);
  CTRL_ALLOC(adapter->parameter);
  sprintf(adapter->parameter, "-"); 

  LOG_OUT();
}

void
mad_via_adapter_configuration_init(p_mad_adapter_t adapter)
{
  /* Note: with MVia 0.9.3, Nic initialization must occur after
     mad slave processes spawn */  
  LOG_IN();
  mad_via_open_nic(adapter);
  mad_via_set_error_callback(adapter);
  mad_via_ns_init(adapter);
  mad_via_query_nic(adapter);
  mad_via_create_ptag(adapter);
  //mad_via_static_buffers_init(adapter);
  LOG_OUT();
}

void mad_via_channel_init(p_mad_channel_t channel)
{
  p_mad_via_channel_specific_t channel_specific ;

  LOG_IN();

  channel->specific = NULL;
  channel_specific  = malloc(sizeof(mad_via_channel_specific_t));
  CTRL_ALLOC(channel_specific);
  channel->specific = channel_specific;
  
  mad_via_create_completion_queue(channel);
  LOG_OUT();
}

void
mad_via_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_via_connection_init(p_mad_connection_t in, p_mad_connection_t out)
{
  p_mad_via_connection_specific_t  in_specific;
  p_mad_via_connection_specific_t out_specific;
  
  p_mad_via_adapter_specific_t adapter_specific; 
  
  LOG_IN();

  adapter_specific = (p_mad_via_adapter_specific_t) in->channel->adapter->specific;

  in_specific = malloc(sizeof(mad_via_connection_specific_t));
  CTRL_ALLOC(in);
  in->specific = in_specific;
  in->nb_link  = 3;
  in_specific->vi = malloc(sizeof(mad_via_vi_t));
  in_specific->posted     = tbx_false;
  mad_via_create_input_connection_vi(in, in_specific->vi);
  //mad_via_init_credits(&(in_specific->vi->credits),1);
  in_specific->vi->in.descriptor = 
    mad_via_allocate_descriptor_array(in,MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,1);
  in_specific->vi->out.descriptor =
    mad_via_allocate_descriptor_array(in,MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,1);

  in_specific->vi->out.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
  in_specific->vi->in.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
   
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  in_specific->vi->out.descriptor->buffer,
			  MAD_VIA_SHORT_BUFFER_LIMIT,
			  0,
			  &(in_specific->vi->out.descriptor->buffer_handle)
			  ));
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  in_specific->vi->in.descriptor->buffer,
			  MAD_VIA_SHORT_BUFFER_LIMIT,
			  0,
			  &(in_specific->vi->in.descriptor->buffer_handle)
			  ));
  


  adapter_specific = (p_mad_via_adapter_specific_t) out->channel->adapter->specific;

  out_specific = malloc(sizeof(mad_via_connection_specific_t));
  CTRL_ALLOC(out);
  out->specific = out_specific;
  out->nb_link  = 3;
  out_specific->vi = malloc(sizeof(mad_via_vi_t));
  out_specific->posted     = tbx_false;
  mad_via_create_vi(out, out_specific->vi); 
  //mad_via_init_credits(&(out_specific->vi->credits),1);
  out_specific->vi->in.descriptor =
    mad_via_allocate_descriptor_array(out,MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,1);
  out_specific->vi->out.descriptor =
    mad_via_allocate_descriptor_array(out,MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,1);

  out_specific->vi->out.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
  out_specific->vi->in.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
  memcpy(out_specific->vi->out.descriptor->buffer,"old!",4);
  memcpy(out_specific->vi->in.descriptor->buffer,"old!",4);

  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  out_specific->vi->out.descriptor->buffer,
			  MAD_VIA_SHORT_BUFFER_LIMIT,
			  0,
			  &(out_specific->vi->out.descriptor->buffer_handle)
			  ));
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  out_specific->vi->in.descriptor->buffer,
			  MAD_VIA_SHORT_BUFFER_LIMIT,
			  0,
			  &(out_specific->vi->in.descriptor->buffer_handle)
			  ));

  LOG_OUT();
}

void
mad_via_link_init(p_mad_link_t lnk)
{
  p_mad_connection_t         connection = lnk->connection;
  p_mad_via_link_specific_t  lnk_specific;

  p_mad_via_adapter_specific_t adapter_specific;
  
  LOG_IN();

  lnk_specific = malloc(sizeof(mad_via_link_specific_t));
  CTRL_ALLOC(lnk_specific);
  lnk->specific = lnk_specific;

  adapter_specific =
      (p_mad_via_adapter_specific_t) lnk->connection->channel->adapter->specific;

  if (lnk->id == MAD_VIA_MESSAGE_LINK)
   {
    lnk->link_mode   = mad_link_mode_buffer;
    lnk->buffer_mode = mad_buffer_mode_dynamic;
    lnk->group_mode = mad_group_mode_split;
    
    mad_via_create_vi(connection, &(lnk_specific->vi));    

    lnk_specific->vi.in.descriptor = 
      mad_via_allocate_descriptor_array(lnk->connection,
					MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
					1);
    lnk_specific->vi.out.descriptor = 
      mad_via_allocate_descriptor_array(lnk->connection,
					MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
					1);

    lnk_specific->vi.in.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
    lnk_specific->vi.out.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
    
    VIA_CTRL(VipRegisterMem(
			    adapter_specific->nic_handle,
			    lnk_specific->vi.in.descriptor->buffer,
			    MAD_VIA_SHORT_BUFFER_LIMIT,
			    0,
			    &(lnk_specific->vi.in.descriptor->buffer_handle)
			    ));
    VIA_CTRL(VipRegisterMem(
			    adapter_specific->nic_handle,
			    lnk_specific->vi.out.descriptor->buffer,
			    MAD_VIA_SHORT_BUFFER_LIMIT,
			    0,
			    &(lnk_specific->vi.out.descriptor->buffer_handle)
			    ));
  }
  else if (lnk->id == MAD_VIA_CREDIT_LINK)
    {
      lnk->link_mode   = mad_link_mode_buffer;
      lnk->buffer_mode = mad_buffer_mode_dynamic;
      lnk->group_mode = mad_group_mode_split;
      
      mad_via_create_vi(connection, &(lnk_specific->vi));
      mad_via_init_credits(&(lnk_specific->vi.credits), MAD_VIA_NB_CREDITS);
      
      if (lnk->connection->way == mad_incoming_connection)
	{
	  
	  
	  lnk_specific->vi.in.descriptor = 
	    mad_via_allocate_descriptor_array(lnk->connection,
					      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
					      MAD_VIA_NB_CREDITS);
	  lnk_specific->vi.in.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_NB_CREDITS*MAD_VIA_SHORT_BUFFER_LIMIT,64);
	  
	  VIA_CTRL(VipRegisterMem(
				  adapter_specific->nic_handle,
				  lnk_specific->vi.in.descriptor->buffer,
				  MAD_VIA_NB_CREDITS*MAD_VIA_SHORT_BUFFER_LIMIT,
				  0,
				  &(lnk_specific->vi.in.descriptor->buffer_handle)
				  ));
	  
	}
      else
	{
	  
	  lnk_specific->vi.in.descriptor = 
	    mad_via_allocate_descriptor_array(lnk->connection,
					      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
					      1);
	  lnk_specific->vi.in.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
	  
	  VIA_CTRL(VipRegisterMem(
				  adapter_specific->nic_handle,
				  lnk_specific->vi.in.descriptor->buffer,
				  MAD_VIA_SHORT_BUFFER_LIMIT,
				  0,
				  &(lnk_specific->vi.in.descriptor->buffer_handle)
				  ));
	}
      
      
      lnk_specific->vi.out.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
					  1);
      
      
      lnk_specific->vi.out.descriptor->buffer = tbx_aligned_malloc(MAD_VIA_SHORT_BUFFER_LIMIT,64);
      
      VIA_CTRL(VipRegisterMem(
			      adapter_specific->nic_handle,
			      lnk_specific->vi.out.descriptor->buffer,
			      MAD_VIA_SHORT_BUFFER_LIMIT,
			      0,
			      &(lnk_specific->vi.out.descriptor->buffer_handle)
			      ));
    }
  
  LOG_OUT();
  return;
  
}

void
mad_via_accept(p_mad_channel_t channel)
{
  VIP_NET_ADDRESS *local, *remote;

  p_mad_via_adapter_specific_t adapter_specific = 
    (p_mad_via_adapter_specific_t) channel->adapter->specific;
  p_mad_configuration_t configuration = 
    channel->adapter->driver->madeleine->configuration;
  
  VIP_NIC_ATTRIBUTES nic_attributes = adapter_specific->nic_attributes;
  VIP_NIC_HANDLE nic_handle = adapter_specific->nic_handle;

  VIP_CONN_HANDLE conn = NULL;
  VIP_VI_ATTRIBUTES remote_attribs;
  p_mad_via_vi_t mad_via_vi;
  
  VIP_VI_HANDLE vi;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;

  mad_via_discriminator_t discriminator;

  int i;

  LOG_IN();
  
  /* Local Address filling */
  /* Allocation */
  local = malloc (sizeof (VIP_NET_ADDRESS)
		  + nic_attributes.NicAddressLen
		  + MAD_VIA_VI_DISCRIMINATOR_LEN);

  /* Filling HostAddressLen and DiscriminatorLen */
  local->HostAddressLen = nic_attributes.NicAddressLen;
  local->DiscriminatorLen = MAD_VIA_VI_DISCRIMINATOR_LEN;
  memcpy (local->HostAddress,
	  nic_attributes.LocalNicAddress,
	  nic_attributes.NicAddressLen);


  /* Remote Address Allocation */
  remote = malloc (sizeof (VIP_NET_ADDRESS)
		   + nic_attributes.NicAddressLen
		   + MAD_VIA_VI_DISCRIMINATOR_LEN);
  memset (remote,
	  0,
	  sizeof (VIP_NET_ADDRESS)
	  + nic_attributes.NicAddressLen
	  + MAD_VIA_VI_DISCRIMINATOR_LEN);
  remote->DiscriminatorLen = MAD_VIA_VI_DISCRIMINATOR_LEN;


  
  /* Accept for Control Connection */

  /* Filling Local Discriminator */
  discriminator.adapter_id = (unsigned char)channel->adapter->id;
  discriminator.channel_id = (unsigned char)channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 0;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator;

  /* Waiting for incoming connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectWait(nic_handle,
			  local, VIP_INFINITE, remote, &remote_attribs, &conn));
  PM2_VIA_UNLOCK();

  /* Select the structures for incoming control connection */
  discriminator = MAD_VIA_VI_DISCRIMINATOR (remote);
  mad_via_vi  = ((p_mad_via_connection_specific_t)
		 ((channel->input_connection + discriminator.host_id)->specific))->vi;
  
  vi = mad_via_vi->handle;
  desc = mad_via_vi->in.descriptor->descriptor;
  mh =  mad_via_vi->in.descriptor->handle;
  
  /* PrePosting a Descriptor */
  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.Status = 0;
  desc->Data[0].Handle = mad_via_vi->in.descriptor->buffer_handle;
  desc->Control.Reserved = 0;
  

  VIA_CTRL(VipPostRecv(
		       vi,
		       desc,
		       mh
		       ));

  /* Accepting a connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectAccept(conn, vi));
  PM2_VIA_UNLOCK();

    


  /* Accept for Message Link */

  /* Filling local discriminator */
  discriminator.adapter_id = (unsigned char)channel->adapter->id;
  discriminator.channel_id = (unsigned char)channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 2;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator;

  /* Waiting for incoming connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectWait(nic_handle,
			  local, VIP_INFINITE, remote, &remote_attribs, &conn));
  PM2_VIA_UNLOCK();


  /* Selects the structure for message link connection */
  discriminator = MAD_VIA_VI_DISCRIMINATOR(remote);  
  mad_via_vi = &((
		  (p_mad_via_link_specific_t)
		  ((channel->input_connection + discriminator.host_id)
		   ->link[MAD_VIA_MESSAGE_LINK].specific)
		  )->vi);
  vi = mad_via_vi->handle;

  /* No prepost for MSG link on the accept side! */
  
  /* Accepting a connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectAccept(conn, vi));
  PM2_VIA_UNLOCK();




  //Accept for Credit Link
  
  // Filling Local Discriminator
  discriminator.adapter_id = (unsigned char)channel->adapter->id;
  discriminator.channel_id = (unsigned char)channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 4;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator;

  // Waiting for incoming connection
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectWait(nic_handle,
			  local, VIP_INFINITE, remote, &remote_attribs, &conn));
  PM2_VIA_UNLOCK();



  // Selects the structure for credit link connection
  discriminator = MAD_VIA_VI_DISCRIMINATOR(remote);  
  mad_via_vi = &((
		  (p_mad_via_link_specific_t)
		  ((channel->input_connection + discriminator.host_id)
		   ->link[MAD_VIA_CREDIT_LINK].specific)
		  )->vi);
  
  vi = mad_via_vi->handle;
  

  // PrePosting the descriptors
  for (i=0;i<MAD_VIA_NB_CREDITS;i++)
    {
      desc = (mad_via_vi->in.descriptor + i)->descriptor;
      mh =  (mad_via_vi->in.descriptor + i)->handle;
      
      desc->Data[0].Data.Address = (mad_via_vi->in.descriptor->buffer) + (MAD_VIA_SHORT_BUFFER_LIMIT*i);
      desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
      desc->Control.SegCount = 1;
      desc->Control.Control = 0;
      desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
      desc->Control.Status = 0;
      desc->Data[0].Handle = mad_via_vi->in.descriptor->buffer_handle;
      desc->Control.Reserved = 0;

  
      VIA_CTRL(VipPostRecv(
			   vi,
			   desc,
			   mh
			   ));
      
    }
  
  // Accepting a connection
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectAccept(conn, vi));
  PM2_VIA_UNLOCK();
  

  /* cleaning addresses */
  free(local);
  free(remote);

  LOG_OUT();
}

void
mad_via_connect(p_mad_connection_t connection)
{
  VIP_NET_ADDRESS *local;
  VIP_NET_ADDRESS *remote;
  p_mad_via_adapter_specific_t specific = 
    (p_mad_via_adapter_specific_t) connection->channel->adapter->specific;
  p_mad_configuration_t configuration = 
    connection->channel->adapter->driver->madeleine->configuration;

  VIP_NIC_ATTRIBUTES nic_attributes = specific->nic_attributes;
  VIP_NIC_HANDLE nic_handle = specific->nic_handle;

  mad_via_discriminator_t discriminator;
  VIP_VI_ATTRIBUTES remote_attribs;

  p_mad_via_vi_t mad_via_vi;
  VIP_VI_HANDLE vi;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;

  LOG_IN();

  /* Local Address filling */

  /* Allocation */
  local = malloc (sizeof (VIP_NET_ADDRESS)
		  + nic_attributes.NicAddressLen
		  + MAD_VIA_VI_DISCRIMINATOR_LEN);
  
  /* HostAddressLen and DiscriminatorLen */
  local->HostAddressLen = nic_attributes.NicAddressLen;
  local->DiscriminatorLen = MAD_VIA_VI_DISCRIMINATOR_LEN;
  memcpy (local->HostAddress,
	  nic_attributes.LocalNicAddress,
	  nic_attributes.NicAddressLen);
  


  /* remote */
  remote = malloc (sizeof (VIP_NET_ADDRESS)
		   + nic_attributes.NicAddressLen
		   + MAD_VIA_VI_DISCRIMINATOR_LEN);
  memset (remote,
	  0,
	  sizeof (VIP_NET_ADDRESS)
	  + nic_attributes.NicAddressLen
	  + MAD_VIA_VI_DISCRIMINATOR_LEN);
  remote->DiscriminatorLen = MAD_VIA_VI_DISCRIMINATOR_LEN;

  PM2_VIA_LOCK();
  VIA_CTRL(VipNSGetHostByName(nic_handle,
			      configuration->host_name[connection->remote_host_id],
			      remote, 0));
  PM2_VIA_UNLOCK();
  


  /* Connect for Control Connection */
  
  /* Filling Local Discriminator */
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 1;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator ;
 
  /* remote */
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)connection->remote_host_id;
  discriminator.vi_id = 0;
  MAD_VIA_VI_DISCRIMINATOR (remote) = discriminator ;    

  /* Select the structures for control connection */ 
  mad_via_vi = ((p_mad_via_connection_specific_t) connection->specific)->vi;
  vi = mad_via_vi->handle;
  desc = mad_via_vi->in.descriptor->descriptor;
  mh =  mad_via_vi->in.descriptor->handle;

  /* Preposting a Descriptor */

  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.Status = 0;
  desc->Data[0].Handle = mad_via_vi->in.descriptor->buffer_handle;
  desc->Control.Reserved = 0;
  
  VIA_CTRL(VipPostRecv(
		       vi,
		       desc,
		       mh
		       ));
  

  /* Requesting connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectRequest(vi, local, remote, VIP_INFINITE, &remote_attribs));
  PM2_VIA_UNLOCK();

  /* Connect For message link */

  /* Filling local discriminator */
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 3;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator ;
   
  /* Filling remote discriminator */
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)connection->remote_host_id;
  discriminator.vi_id = 2;
  MAD_VIA_VI_DISCRIMINATOR (remote) = discriminator ;    

  /* Selects the structure for message link connection */
  mad_via_vi = &((
		  (p_mad_via_link_specific_t)
		  (connection->link[MAD_VIA_MESSAGE_LINK].specific)
		  )->vi);
  vi = mad_via_vi->handle;
  desc = mad_via_vi->in.descriptor->descriptor;
  mh =  mad_via_vi->in.descriptor->handle;

  /* Preposting a Descriptor for msg link */

  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.Status = 0;
  desc->Data[0].Handle = mad_via_vi->in.descriptor->buffer_handle;
  desc->Control.Reserved = 0;

  VIA_CTRL(VipPostRecv(
		       vi,
		       desc,
		       mh
		       ));
  

  /* Requesting connection */
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectRequest(vi, local, remote, VIP_INFINITE, &remote_attribs));
  PM2_VIA_UNLOCK();




  /* Connect For credit link */
  
  // Filling local discriminator
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)configuration->local_host_id;
  discriminator.vi_id = 5;
  MAD_VIA_VI_DISCRIMINATOR (local) = discriminator ;
   
  // Filling remote discriminator
  discriminator.adapter_id = (unsigned char)connection->channel->adapter->id;
  discriminator.channel_id = (unsigned char)connection->channel->id;
  discriminator.host_id = (unsigned char)connection->remote_host_id;
  discriminator.vi_id = 4;
  MAD_VIA_VI_DISCRIMINATOR (remote) = discriminator ;    

  // Selects the structure for message link connection
  mad_via_vi = &((
		  (p_mad_via_link_specific_t)
		  (connection->link[MAD_VIA_CREDIT_LINK].specific)
		  )->vi);

  vi = mad_via_vi->handle;
  desc = mad_via_vi->in.descriptor->descriptor;
  mh =  mad_via_vi->in.descriptor->handle;


  // Preposting a Descriptor for credit link
  
  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
  desc->Control.Status = 0;
  desc->Data[0].Handle = mad_via_vi->in.descriptor->buffer_handle;
  desc->Control.Reserved = 0;
  
  VIA_CTRL(VipPostRecv(
		       vi,
		       desc,
		       mh
		       ));
  

  // Requesting connection
  PM2_VIA_LOCK();
  VIA_CTRL(VipConnectRequest(vi, local, remote, VIP_INFINITE, &remote_attribs));
  PM2_VIA_UNLOCK();
  

  /* cleaning addresses */
  free(local);
  free(remote);
  
  LOG_OUT();
}

void
mad_via_after_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_via_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  /* Nothing */
  LOG_OUT();
}

void
mad_via_disconnect(p_mad_connection_t connection)
{
  VIP_RETURN via_status;
  VIP_VI_HANDLE vi = 
    ((p_mad_via_connection_specific_t) connection->specific)->vi->handle;

  LOG_IN();
  
  via_status = VipDisconnect(vi);

  LOG_OUT();
}

void
mad_via_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();


  LOG_OUT();
}

p_mad_link_t
mad_via_choice(p_mad_connection_t   connection,
	       size_t               buffer_length,
	       mad_send_mode_t      send_mode,
	       mad_receive_mode_t   receive_mode)
{
  LOG_IN();

  /*
  LOG_OUT();
  return &(connection->link[MAD_VIA_MESSAGE_LINK]);
  */

  if (buffer_length <= MAD_VIA_SHORT_BUFFER_LIMIT)
    {
      LOG_OUT();
      return &(connection->link[MAD_VIA_CREDIT_LINK]);
    }
  else
    {
      LOG_OUT();
      return &(connection->link[MAD_VIA_MESSAGE_LINK]);
    }
  
  /*
  if (buffer_length <= MAD_VIA_SHORT_BUFFER_LIMIT)
    {
      LOG_OUT();
      return &(connection->link[MAD_VIA_CREDIT_LINK]);
    }
  else if (buffer_length <= MAD_VIA_LONG_BUFFER_LIMIT)
    {
      LOG_OUT();
      return &(connection->link[MAD_VIA_MESSAGE_LINK]);
    }
  else
    {
      LOG_OUT();
      return &(connection->link[MAD_VIA_RDMA_LINK]);
    }
  */

}

void
mad_via_new_message(p_mad_connection_t connection)
{  
  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;

  LOG_IN();

  specific->new_message = tbx_true;

  LOG_OUT();
}

p_mad_connection_t
mad_via_receive_message(p_mad_channel_t channel)
{
  VIP_VI_HANDLE vi_handle;
  VIP_BOOLEAN direction;

  p_mad_connection_t connection = NULL;
  p_mad_via_connection_specific_t specific;

  int remote_host_id;

  LOG_IN();

  VIA_CTRL( VipCQWait( ( (p_mad_via_channel_specific_t) channel->specific )->completion_queue,
		       VIP_INFINITE,
		       &vi_handle,
		       &direction));

  for ( remote_host_id = 0;
	remote_host_id < channel->adapter->driver->madeleine->configuration->size;
	remote_host_id++ ) {
    
    if (remote_host_id == 
	channel->adapter->driver->madeleine->configuration->local_host_id)
      {
	continue;
      }
    
    connection = (
		  (p_mad_connection_t)channel->input_connection
		  + remote_host_id
		  );
    specific = connection->specific;
    
    if (vi_handle == specific->vi->handle)
      {
	specific->new_message = tbx_true;
	break;
      }
  }
  

  LOG_OUT();

  return connection;
}

void
mad_via_send_message_buffer(p_mad_link_t     link,
			    p_mad_buffer_t   buffer)
{
  VIP_RETURN via_status;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh, mh_buffer;

  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;

  p_mad_connection_t connection = link->connection;

  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;
  
  p_mad_via_vi_t vi;

  p_mad_via_adapter_specific_t adapter =
    (
     (p_mad_via_adapter_specific_t)
     (connection->channel->adapter->specific)
     );

  
  LOG_IN();

  if (specific->new_message == tbx_true)
    {
      
      /* CTRL VI */
      vi = specific->vi;

      if (specific->posted == tbx_true)
	{
	  VIA_CTRL(VipSendWait(
			       vi->handle,
			       VIP_INFINITE,
			       &desc));
	}

      /* Init descriptor needed ! */
      desc = vi->out.descriptor->descriptor;
      mh = vi->out.descriptor->handle;

      memcpy(vi->out.descriptor->buffer,"Control!",8);
      
      VIA_CTRL(VipPostSend(
			   vi->handle,
			   desc,
			   mh));
      
      specific->new_message = tbx_false;
    }


  /* MSG VI */
  vi = &(link_specific->vi);
  
  VIA_CTRL(VipRegisterMem(
			  adapter->nic_handle,
			  buffer->buffer,
			  buffer->length,
			  0,
			  &mh_buffer
			  ));

  while (mad_more_data(buffer)) {

    size_t current_length = 0;
    current_length = min (MAD_VIA_MAX_TRANSFER_SIZE, (buffer->bytes_written - buffer->bytes_read));

    /* RecvWait Ack */
    via_status = VipRecvWait(
			     vi->handle,
			     VIP_INFINITE,
			     &desc);
    VIA_CTRL(via_status);
 

    /* PostRecv Next Ack */
    mh = vi->in.descriptor->handle;
    VIA_CTRL(VipPostRecv(
			 vi->handle,
			 desc,
			 mh
			 ));

    
    /* PostSend Buffer */
    desc = vi->out.descriptor->descriptor;
    mh = vi->out.descriptor->handle;
    
    desc->Data[0].Data.Address = (buffer->buffer) + (buffer->bytes_read);
    desc->Data[0].Length = current_length;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = current_length;
    desc->Control.Status = 0;
    desc->Data[0].Handle = mh_buffer;
    
    VIA_CTRL(VipPostSend(
			 vi->handle,
			 vi->out.descriptor->descriptor,
			 vi->out.descriptor->handle));
    
    
    /* SendWait Buffer */
    via_status = VipSendWait(
			     vi->handle,
			     VIP_INFINITE,
			     &desc
			     );

    VIA_CTRL(via_status);

    buffer->bytes_read += current_length;
  }

  VIA_CTRL(VipDeregisterMem(
			  adapter->nic_handle,
			  buffer->buffer,
			  mh_buffer
			  ));

  specific->posted = tbx_true;
  LOG_OUT();
}

void
mad_via_send_credit_buffer(p_mad_link_t     link,
			   p_mad_buffer_t   buffer)
{
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;

  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;

  p_mad_connection_t connection = link->connection;

  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;
  
  p_mad_via_vi_t vi;

  LOG_IN();

  if (specific->new_message == tbx_true)
    {
      
      /* CTRL VI */
      vi = specific->vi;
      
      if (specific->posted == tbx_true) {
	
	VIA_CTRL(VipSendWait(
			     vi->handle,
			     VIP_INFINITE,
			     &desc));
	
	
	VIA_CTRL(VipRecvWait(
			     vi->handle,
			     VIP_INFINITE,
			     &desc));

	mh = vi->in.descriptor->handle;
	
	VIA_CTRL(VipPostRecv(
			     vi->handle,
			     desc,
			     mh
			     ));  
      }
      
      /* Init descriptor needed ! */
      desc = vi->out.descriptor->descriptor;
      mh = vi->out.descriptor->handle;

      desc->Data[0].Data.Address = vi->out.descriptor->buffer;
      //desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
      desc->Data[0].Length = buffer->length;
      desc->Control.SegCount = 1;
      desc->Control.Control = 0;
      desc->Control.Length = buffer->length;
      desc->Control.Status = 0;
      desc->Data[0].Handle = vi->out.descriptor->buffer_handle;

      memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,buffer->buffer,buffer->length);
      
      VIA_CTRL(VipPostSend(
			   vi->handle,
			   vi->out.descriptor->descriptor,
			   vi->out.descriptor->handle));
      
      
      specific->new_message = tbx_false;
    }
  else
    {
      vi = &(link_specific->vi);

      if (vi->credits.available_credits <= vi->credits.alert)
	{
	  VIA_CTRL(VipRecvWait(
			       vi->handle,
			       VIP_INFINITE,
			       &desc));
	      
	      if (desc->Control.ImmediateData == 0)
		{
		  fprintf(stderr,"BIG PROBLEM: no more credits\n");
		}
	      
	      vi->credits.available_credits += desc->Control.ImmediateData;
	      
	      mh = vi->in.descriptor->handle;

	      VIA_CTRL(VipPostRecv(
				   vi->handle,
				   desc,
				   mh
				   ));
	}

      
      desc = vi->out.descriptor->descriptor;
      mh = vi->out.descriptor->handle;

      desc->Data[0].Data.Address = vi->out.descriptor->buffer;
      desc->Data[0].Length = buffer->length;
      desc->Control.SegCount = 1;
      desc->Control.Control = 0;
      desc->Control.Length = buffer->length;
      desc->Control.Status = 0;
      desc->Data[0].Handle = vi->out.descriptor->buffer_handle;

      memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,buffer->buffer,buffer->length);

      desc->Control.ImmediateData = (
				     (p_mad_via_link_specific_t)
				     (connection->reverse->link[MAD_VIA_CREDIT_LINK]).specific
				     )->vi.credits.returned_credits;

      ((p_mad_via_link_specific_t)(connection->reverse->link[MAD_VIA_CREDIT_LINK]).specific)
	->vi.credits.available_credits += 
	((p_mad_via_link_specific_t)(connection->reverse->link[MAD_VIA_CREDIT_LINK]).specific)
	->vi.credits.returned_credits;
      ((p_mad_via_link_specific_t)(connection->reverse->link[MAD_VIA_CREDIT_LINK]).specific)
	->vi.credits.returned_credits = 0;
      
      vi->credits.available_credits --;

      VIA_CTRL(VipPostSend(
			   vi->handle,
			   vi->out.descriptor->descriptor,
			   vi->out.descriptor->handle));
  
      VIA_CTRL(VipSendWait(
			   vi->handle,
			   VIP_INFINITE,
			   &desc));
    }
  
  specific->posted = tbx_true;

  LOG_OUT();
}


void
mad_via_send_buffer(p_mad_link_t     link,
		    p_mad_buffer_t   buffer)
{
  LOG_IN();

  if (link->id == MAD_VIA_MESSAGE_LINK)
    {
      mad_via_send_message_buffer(link, buffer);
    }
  else if (link->id == MAD_VIA_CREDIT_LINK)
    {
      mad_via_send_credit_buffer(link, buffer);
    }

  LOG_OUT();
}



void
mad_via_receive_message_buffer(p_mad_link_t     link,
			       p_mad_buffer_t  *buffer)
{
  VIP_RETURN via_status;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh, mh_buffer;

  p_mad_via_vi_t vi;
    
  p_mad_connection_t connection = link->connection;

  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;

  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;
  
  p_mad_via_adapter_specific_t adapter =
    (p_mad_via_adapter_specific_t) link->connection->channel->adapter->specific;


  LOG_IN();

  /* CTRL VI */
  vi = specific->vi;

  if (specific->new_message == tbx_true)
    {
      VIA_CTRL(VipRecvDone(
			   vi->handle,
			   &desc
			   ));

      /* We can verify that we have received a CONTROL! message here */
      //fprintf(stderr,"It should be Control!:%s\n", ((char*)(desc->Data[0].Data.Address)));
      
      mh = vi->in.descriptor->handle;
     
      VIA_CTRL(VipPostRecv(
			   vi->handle,
			   desc,
			   mh
			   ));
      
      specific->posted = tbx_true;
      specific->new_message = tbx_false;
    }
      
      
  /* MSG VI */
  vi = &(link_specific->vi);
  
  VIA_CTRL(VipRegisterMem(
			  adapter->nic_handle,
			  (*buffer)->buffer,
			  (*buffer)->length,
			  0,
			  &mh_buffer
			  ));

  while (!mad_buffer_full(*buffer)) {
    size_t current_length = 0;
    
    current_length = min (MAD_VIA_MAX_TRANSFER_SIZE, ((*buffer)->length - (*buffer)->bytes_written));    

    /* PostRecv Buffer */
    desc = vi->in.descriptor->descriptor;
    mh = vi->in.descriptor->handle;
    
    desc->Data[0].Data.Address = ((*buffer)->buffer + (*buffer)->bytes_written);
    desc->Data[0].Length = current_length;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = current_length;
    desc->Control.Status = 0;
    desc->Data[0].Handle = mh_buffer;
    desc->Control.Reserved = 0;
    
    VIA_CTRL(VipPostRecv(
			 vi->handle,
			 desc,
			 mh
			 ));
    

    /* PostSend Acknowledge */
    desc = vi->out.descriptor->descriptor;
    mh = vi->out.descriptor->handle;
    
    desc->Data[0].Data.Address = vi->out.descriptor->buffer;
    desc->Data[0].Length = 4;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = 4;
    desc->Control.Status = 0;
    desc->Data[0].Handle = vi->out.descriptor->buffer_handle;
    
    memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,"Ack!",4);
    
    VIA_CTRL(VipPostSend(
			 vi->handle,
			 vi->out.descriptor->descriptor,
			 vi->out.descriptor->handle));

    /* SendWait Ack */
    via_status = VipSendWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc);

    VIA_CTRL(via_status);
    
    /* RecvWait Buffer */    
    via_status = VipRecvWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc);
    VIA_CTRL(via_status);

    (*buffer)->bytes_written += current_length;
  }

  VIA_CTRL(VipDeregisterMem(
			  adapter->nic_handle,
			  (*buffer)->buffer,
			  mh_buffer
			  ));

  LOG_OUT();
}

void
mad_via_receive_credit_buffer(p_mad_link_t     link,
			      p_mad_buffer_t  *buffer)
{
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;
  
  p_mad_via_vi_t vi;
  
  p_mad_connection_t connection = link->connection;
  
  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;
  
  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;
  
  LOG_IN();

  /* CTRL VI */
  vi = specific->vi;

  if (specific->new_message == tbx_true)
    {
      
      if (specific->posted == tbx_true) {
	
	VIA_CTRL(VipSendWait(
			     vi->handle,
			     VIP_INFINITE,
			     &desc));  
      }
      
      VIA_CTRL(VipRecvDone(
			   vi->handle,
			   &desc
			   ));
            
      memcpy((*buffer)->buffer,desc->Data[0].Data.Address,(*buffer)->length);

      /* post next reception */
      desc = vi->in.descriptor->descriptor;
      mh = vi->in.descriptor->handle;
      
      
      desc->Data[0].Data.Address = vi->in.descriptor->buffer;
      desc->Data[0].Length = MAD_VIA_SHORT_BUFFER_LIMIT;
      desc->Control.SegCount = 1;
      desc->Control.Control = 0;
      desc->Control.Length = MAD_VIA_SHORT_BUFFER_LIMIT;
      desc->Control.Status = 0;
      desc->Data[0].Handle = vi->in.descriptor->buffer_handle;
      desc->Control.Reserved = 0;
      
      
      VIA_CTRL(VipPostRecv(
			   vi->handle,
			   desc,
			   mh
			   ));
    

      /* post acknowledge */
      desc = vi->out.descriptor->descriptor;
      mh = vi->out.descriptor->handle;

      
      desc->Data[0].Data.Address = vi->out.descriptor->buffer;
      desc->Data[0].Length = 4;
      desc->Control.SegCount = 1;
      desc->Control.Control = 0;
      desc->Control.Length = 4;
      desc->Control.Status = 0;
      desc->Data[0].Handle = vi->out.descriptor->buffer_handle;
      

      memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,"Ack!",4);
      
      VIA_CTRL(VipPostSend(
			   vi->handle,
			   vi->out.descriptor->descriptor,
			   vi->out.descriptor->handle));      

      specific->new_message = tbx_false;
    }
  else
    {
      vi = &(link_specific->vi);

      if (vi->credits.available_credits <= vi->credits.alert)
	{
	  desc = vi->out.descriptor->descriptor;
	  mh = vi->out.descriptor->handle;

	  desc->Data[0].Data.Address = vi->out.descriptor->buffer;
	  desc->Data[0].Length = 4;
	  desc->Control.SegCount = 1;
	  desc->Control.Control = 0;
	  desc->Control.Length = 4;
	  desc->Control.Status = 0;
	  desc->Data[0].Handle = vi->out.descriptor->buffer_handle;

	  memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,"Ack!",4);
	  
	  desc->Control.ImmediateData = vi->credits.returned_credits;
	  vi->credits.available_credits += vi->credits.returned_credits;
	  vi->credits.returned_credits = 0;
	  
	  VIA_CTRL(VipPostSend(
			       vi->handle,
			       vi->out.descriptor->descriptor,
			       vi->out.descriptor->handle));
	  
	  VIA_CTRL(VipSendWait(
			       vi->handle,
			       VIP_INFINITE,
			       &desc));

	}
	  
      vi->credits.available_credits --;
      VIA_CTRL(VipRecvWait(
			   vi->handle,
			   VIP_INFINITE,
			   &desc));
      
      ((p_mad_via_link_specific_t) (connection->reverse->link[MAD_VIA_CREDIT_LINK]).specific)
	->vi.credits.available_credits += desc->Control.ImmediateData;

      memcpy((*buffer)->buffer, desc->Data[0].Data.Address, (*buffer)->length);
        
      // it is always the same memory handle so the first one is ok!
      mh = vi->in.descriptor->handle;
      
      VIA_CTRL(VipPostRecv(
			   vi->handle,
			   desc,
			   mh
			   ));

      vi->credits.returned_credits ++;
    }  
  
  specific->posted = tbx_true;
  
  LOG_OUT();
}

void
mad_via_receive_buffer(p_mad_link_t     link,
		       p_mad_buffer_t  *buffer)
{
  LOG_IN();

  if (link->id == MAD_VIA_MESSAGE_LINK)
    {
      mad_via_receive_message_buffer(link, buffer);
    }
  else if (link->id == MAD_VIA_CREDIT_LINK)
    {
      mad_via_receive_credit_buffer(link, buffer);
    }

  LOG_OUT();
}

void
mad_via_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;
      
      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_via_send_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }

  LOG_OUT();
}

void
mad_via_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_buffer_group,
				 p_mad_buffer_group_t buffer_group)
{
  LOG_IN();

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;
      
      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer;
	  
	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_receive_buffer(lnk, &buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }

  LOG_OUT();
}

p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t lnk)
{
  fprintf(stderr,"GET_STATIC_BUFFER\n");
  return NULL;
}

void
mad_via_return_static_buffer(p_mad_link_t     lnk,
			     p_mad_buffer_t   buffer)

{
  fprintf(stderr,"RETURN_STATIC_BUFFER\n");
}

/*
p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_via_adapter_specific_t adapter_specific
    = lnk->connection->channel->adapter->specific;
  p_mad_buffer_t buffer;

  LOG_IN();
  buffer                = mad_alloc_buffer_struct();
  buffer->length        = MAD_VIA_STATIC_BUFFER_SIZE;
  buffer->bytes_written = 0;
  buffer->bytes_read    = 0;
  buffer->type          = mad_static_buffer;
  mad_via_buffer_alloc(adapter_specific->memory, buffer);
  LOG_OUT();

  return buffer;
}

void
mad_via_return_static_buffer(p_mad_link_t     lnk,
			     p_mad_buffer_t   buffer)
{
  p_mad_via_adapter_specific_t adapter_specific
    = lnk->connection->channel->adapter->specific;
  LOG_IN();
  mad_via_buffer_free(adapter_specific->memory, buffer);
  LOG_OUT();
}
*/
