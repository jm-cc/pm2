
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
/* #define DEBUG */
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

/* *** preregistered memory blocks manager data ....................... *** */
typedef struct
{
  void           *next;
  VIP_MEM_HANDLE  handle;
} mad_via_memory_element_t, *p_mad_via_memory_element_t;

typedef struct
{
  TBX_SHARED;

  void             *first_block;
  void             *base;
  size_t            element_size;
  long              element_number;
  void             *head;
  long              first_new_element;
  VIP_MEM_HANDLE    memory_handle;
  p_mad_adapter_t   adapter;
} mad_via_memory_t, *p_mad_via_memory_t;

/* *** MadII's driver specific data structures ........................ *** */
typedef struct
{
  int nb_adapter;
} mad_via_driver_specific_t, *p_mad_via_driver_specific_t;

typedef struct
{
  p_mad_via_memory_t    memory;
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


#ifdef DEBUG
#define VIA_VERIFY(op) \
{\
  VIP_RETURN mad_via_status;\
  if ((mad_via_status = (op)) != (VIP_SUCCESS)) \
    {\
      fprintf(stderr, "%s failed: \nFILE: %s\nLINE: %d\nError code :", \
              #op, __FILE__, __LINE__); \
      mad_via_disp_status(mad_via_status); \
      exit(1);}}

#else /* DEBUG */
#define VIA_VERIFY(op) ((void)(op))
#endif /* DEBUG */

/*
 * Implementation macros
 * ---------------------
 */

#define MAD_VIA_CREDIT_LINK                   0
#define MAD_VIA_MESSAGE_LINK                  1
#define MAD_VIA_RDMA_LINK                     2

#define MAD_VIA_SHORT_BUFFER_LIMIT         5120 /* 5KB*/
#define MAD_VIA_LONG_BUFFER_LIMIT         32768 /* 32KB = VIA minimum MTU */

#define MAD_VIA_NB_CREDITS                   32
#define MAD_VIA_NB_CONNECTION_CREDITS        64
#define MAD_VIA_STATIC_BUFFER_SIZE          512
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

/*
 * functions
 * ---------==================================================================
 */

static void niKo_debug_descriptor(VIP_DESCRIPTOR* desc, p_mad_buffer_t buffer) {

#if 0
  VIP_UINT32 erreur;
  
  VipGetError(&erreur);
  if (erreur != VIP_DETAIL_UNKNOWN)
    {
      switch (erreur)
	{
	case VIP_DETAIL_GENERIC:
	    fprintf(stderr, "Generic descriptor error\n");
	    break;
	case VIP_DETAIL_OS_SPECIFIC:
	  {
	    VIP_UINT32 syscall;
	    
	    syscall = VipReason2Syscall(erreur);
	    fprintf(stderr, "syscall: %d\n", syscall);
	    break;
	  }
	case VIP_DETAIL_VENDOR_SPECIFIC:
	  {
	    VIP_UINT32 errno;

	    errno = VipReason2Errno(erreur);
	    fprintf(stderr, "errno: %d\n", errno);
	    break;
	  }
	}
    }
#endif /* 0 */

  fprintf(stderr, "mad_buffer: %p\n", buffer);
  fprintf(stderr, "\nniKo detected an error :\ndescriptor: %p\nbuffer: %p\n",desc,desc->Data[0].Data.Address);
}


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

/* mad_via_disp_nic_attributes: display the VIA nic attributes */
static void 
mad_via_disp_nic_attributes(p_mad_via_adapter_specific_t device)
{
  fprintf (stderr, "Name: %s\n",
	   device->nic_attributes.Name);
  fprintf (stderr, "Hardware version: %lu\n",
	   device->nic_attributes.HardwareVersion);
  fprintf (stderr, "Provider version: %lu\n",
	   device->nic_attributes.ProviderVersion);
  fprintf (stderr, "Nic address length: %u\n",
	   device->nic_attributes.NicAddressLen);
  fprintf (stderr, "Nic address: %x\n",
	   (int) device->nic_attributes.LocalNicAddress);
  if (device->nic_attributes.ThreadSafe)
    {
      fprintf (stderr, "Thread safe: yes\n");
    }
  else
    {
      fprintf (stderr, "Thread safe: no\n");
    }
  fprintf (stderr, "Max discriminator length: %d\n",
	   (int) device->nic_attributes.MaxDiscriminatorLen);
  fprintf (stderr, "Max register byte: %lu\n",
	   device->nic_attributes.MaxRegisterBytes);
  fprintf (stderr, "Max register regions: %lu\n",
	   device->nic_attributes.MaxRegisterRegions);
  fprintf (stderr, "Max register block byte: %lu\n",
	   device->nic_attributes.MaxRegisterBlockBytes);
  fprintf (stderr, "Max VI: %lu\n",
	   device->nic_attributes.MaxVI);
  fprintf (stderr, "Max descriptors per queue: %lu\n",
	   device->nic_attributes.MaxDescriptorsPerQueue);
  fprintf (stderr, "Max segments per descriptor: %lu\n",
	   device->nic_attributes.MaxSegmentsPerDesc);
  fprintf (stderr, "Max CQ: %lu\n",
	   device->nic_attributes.MaxCQ);
  fprintf (stderr, "Max CQ entries: %lu\n",
	   device->nic_attributes.MaxCQEntries);
  fprintf (stderr, "Max transfer size: %lu\n",
	   device->nic_attributes.MaxTransferSize);
  fprintf (stderr, "Native MTU: %lu\n",
	   device->nic_attributes.NativeMTU);
  fprintf (stderr, "Max Ptags: %lu\n",
	   device->nic_attributes.MaxPtags);
  
  switch(device->nic_attributes.ReliabilityLevelSupport)
    {
    case VIP_SERVICE_UNRELIABLE:
      fprintf(stderr, "Reliability level support : unreliable\n");
      break;
    case VIP_SERVICE_RELIABLE_DELIVERY:
      fprintf(stderr, "Reliability level support : reliable delivery\n");
      break;
    case VIP_SERVICE_RELIABLE_RECEPTION:
      fprintf(stderr, "Reliability level support : reliable reception\n");
      break;
    default:
      fprintf(stderr, "Reliability level support : unknown !!!\n");
      break;      
    }
  if (device->nic_attributes.RDMAReadSupport)
    {
      fprintf(stderr, "RDMA read support: yes\n");
    }
  else
    {
      fprintf(stderr, "RDMA read support: no\n");
    }
}

/* mad_via_error_callback: callback called on via asynchronous error
   to display associated error message */
static void __attribute__ ((noreturn))
mad_via_error_callback (VIP_PVOID Context,
			VIP_ERROR_DESCRIPTOR * ErrorDesc)
{
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

/* -------------------------------------------------------------------------*/

/*
 * Memory management
 * ------------------
 */

static void *
mad_via_indirection(p_mad_via_memory_t memory,
		    size_t             index)
{
  LOG_IN();
  LOG_OUT();
  return memory->base + memory->element_size * index;
  LOG_OUT();
}

static void *
mad_via_last_element(p_mad_via_memory_t memory)
{
  LOG_IN();
  LOG_OUT();
  return mad_via_indirection(memory, memory->element_number);
  LOG_OUT();
}

static void *
mad_via_first_new_element(p_mad_via_memory_t memory)
{
  LOG_IN();
  LOG_OUT();
  return mad_via_indirection(memory, memory->first_new_element);
  LOG_OUT();
}

static void
mad_via_register_memory_block(p_mad_via_memory_t memory)
{
  p_mad_adapter_t                adapter          = memory->adapter;
  p_mad_via_adapter_specific_t   adapter_specific = adapter->specific;

  LOG_IN();
  PM2_VIA_LOCK();
  //fprintf(stderr,"VipRegisterMem\n");
  VIA_CTRL(VipRegisterMem(adapter_specific->nic_handle,
			  memory->base,
			  memory->element_size * memory->element_number
			  + sizeof(mad_via_memory_element_t),
			  &(adapter_specific->memory_attributes),
			  &(memory->memory_handle)));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_buffer_allocator_init(p_mad_adapter_t adapter,
			      size_t          buffer_len,
			      long            initial_buffer_number)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;
  p_mad_via_memory_t           memory           =
    malloc(sizeof(mad_via_memory_t));
  p_mad_via_memory_element_t   last_element     = NULL;

  LOG_IN();
  CTRL_ALLOC(memory);
  adapter_specific->memory = memory;

  TBX_INIT_SHARED(memory);
  memory->adapter = adapter;

  if (initial_buffer_number <= 0)
    {
      initial_buffer_number = MAD_VIA_INITIAL_STATIC_BUFFER_NUM;
    }

  if (buffer_len < sizeof(mad_via_memory_element_t))
    {
      buffer_len = sizeof(mad_via_memory_element_t);
    }

  memory->first_block =
    tbx_aligned_malloc(initial_buffer_number * buffer_len
		       + sizeof(mad_via_memory_element_t),
		       MAD_VIA_BUFFER_ALIGNMENT);
  CTRL_ALLOC(memory->first_block);
  memory->base              = memory->first_block;
  memory->element_size      = buffer_len;
  memory->element_number    = initial_buffer_number;
  memory->head              = NULL; 
  memory->first_new_element = 0;
  mad_via_register_memory_block(memory);
  
  last_element = mad_via_indirection(memory, memory->element_number);
  last_element->next   = NULL;
  last_element->handle = memory->memory_handle;
  LOG_OUT();
}

static void
mad_via_buffer_alloc(p_mad_via_memory_t memory,
		     p_mad_buffer_t buffer)
{
  LOG_IN();
  TBX_LOCK_SHARED(memory);
  
  if (memory->head)
    {
      p_mad_via_memory_element_t element = memory->head;

      buffer->buffer   = element;
      buffer->specific = (p_mad_driver_specific_t)element->handle;
      memory->first_block  = element->next;
    }
  else 
    {
      if (memory->first_new_element >= memory->element_number)
	{
	  p_mad_via_memory_element_t last_element;
	  void *new_base =
	    tbx_aligned_malloc(memory->element_number * memory->element_size
			       + sizeof(mad_via_memory_element_t),
			       MAD_VIA_BUFFER_ALIGNMENT);
	  CTRL_ALLOC(new_base);

	  last_element = mad_via_last_element(memory);
	  last_element->next = new_base;

	  memory->base = new_base;
	  memory->first_new_element = 0;
	  mad_via_register_memory_block(memory);

	  last_element = mad_via_last_element(memory);
	  last_element->next   = NULL;
	  last_element->handle = memory->memory_handle;
	}
      
      buffer->buffer   = mad_via_first_new_element(memory);
      buffer->specific = (p_mad_driver_specific_t) memory->memory_handle;
      memory->first_new_element++;
    }  

  buffer->length = memory->element_size;
  buffer->bytes_written = 0 ;
  buffer->bytes_read = 0 ;
  TBX_UNLOCK_SHARED(memory);
  LOG_OUT();
}

static void
mad_via_buffer_free(p_mad_via_memory_t memory,
		    p_mad_buffer_t buffer)
{
  p_mad_via_memory_element_t element = buffer->buffer;
  
  LOG_IN();
  TBX_LOCK_SHARED(memory);
  element->next   = memory->head;
  element->handle = (VIP_MEM_HANDLE) buffer->specific;
  memory->head = element;
  TBX_UNLOCK_SHARED(memory);
  LOG_OUT();
}


/* -------------------------------------------------------------------------*/

/*
 * Transmission modules
 * ---------------------
 */

/* ----- CREDIT TM ----- */


/* ----- Message TM ----- */
static void
mad_via_msg_send_buffers(p_mad_link_t  lnk,
			 p_tbx_list_reference_t ref)
{
  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) lnk->specific;
  p_mad_via_adapter_specific_t adapter_specific =
    (p_mad_via_adapter_specific_t) lnk->connection->channel->adapter->specific;
  VIP_DESCRIPTOR                   *desc                = NULL;
  VIP_RETURN                        via_status;
  size_t                            length;
  
  LOG_IN();
  do
    {
      PM2_VIA_LOCK();
      /*
      via_status = VipRecvWait(link_specific->vi.handle,
			       VIP_INFINITE,
			       &desc);
      */
      VIA_CTRL(VipRecvWait(link_specific->vi.handle,
			   VIP_INFINITE,
			   &desc));
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);
  
  length = desc->Control.ImmediateData;
  
  PM2_VIA_LOCK();
  VIA_CTRL(VipPostRecv(link_specific->vi.handle,
		       desc,
		       adapter_specific->memory->memory_handle));
  PM2_VIA_UNLOCK();
  
  desc = (VIP_DESCRIPTOR*)(link_specific->vi.out.descriptor);
  //MAD_VIA_FILL_DESC_IMMEDIATE(desc, length);
  desc->Control.Length   = 0;
  desc->Control.Status   = 0;
  desc->Control.Control  = VIP_CONTROL_OP_SENDRECV;
  desc->Control.SegCount = 0;
  desc->Control.ImmediateData = length;
  desc->Control.Control |= VIP_CONTROL_IMMEDIATE;

  do
    {
      size_t seg_length = 0 ;
      p_mad_buffer_t buffer;

      buffer = (p_mad_buffer_t)tbx_get_list_reference_object(ref);
      
      desc->Data[desc->Control.SegCount].Data.Address =
	buffer->buffer + buffer->bytes_read;
      desc->Data[desc->Control.SegCount].Handle =
	(VIP_MEM_HANDLE)buffer->specific;
      
      if ((buffer->bytes_written - buffer->bytes_read) <= length)
	{
	  seg_length = buffer->bytes_written - buffer->bytes_read;
	  tbx_forward_list_reference(ref);
	  buffer->bytes_read = buffer->bytes_written;
	}
      else
	{
	  seg_length = length;
	  buffer->bytes_read += length ;
	}

      length -= seg_length;
      desc->Data[desc->Control.SegCount].Length = seg_length;
      desc->Control.SegCount++;
    }
  while((length > 0) && (!tbx_reference_after_end_of_list(ref)));

  PM2_VIA_LOCK();
  VIA_CTRL(VipPostSend (link_specific->vi.handle,
			desc,
			adapter_specific->memory->memory_handle));
  PM2_VIA_UNLOCK();

  do
    {
      PM2_VIA_LOCK();
      via_status = VipSendWait(link_specific->vi.handle,
			       VIP_INFINITE,
			       &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);
  LOG_OUT();
}

static void 
mad_via_msg_receive_buffers(p_mad_link_t  lnk,
			    p_tbx_list_reference_t ref)
{
  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) lnk->specific;
  p_mad_via_adapter_specific_t adapter_specific =
    (p_mad_via_adapter_specific_t) lnk->connection->channel->adapter->specific;
  VIP_RETURN                        via_status;
  VIP_DESCRIPTOR                   *desc = NULL;
  size_t                            length              = 0;
  size_t                            max_length          =
    MAD_VIA_MAX_TRANSFER_SIZE;
  
  LOG_IN();
  do
    {
      length = 0 ;
      max_length = MAD_VIA_MAX_TRANSFER_SIZE;

      desc = (VIP_DESCRIPTOR*)link_specific->vi.in.descriptor;
      desc->Control.Status = 0;
      desc->Control.Control = VIP_CONTROL_OP_SENDRECV;
      desc->Control.SegCount = 0;

      do
	{
	  size_t seg_length = 0 ;
	  p_mad_buffer_t buffer;
	  
	  buffer = (p_mad_buffer_t)tbx_get_list_reference_object(ref);
	  
	  desc->Data[desc->Control.SegCount].Data.Address =
	    buffer->buffer + buffer->bytes_written;
	  desc->Data[desc->Control.SegCount].Handle =
	    (VIP_MEM_HANDLE)buffer->specific;
	  
	  if ((buffer->length - buffer->bytes_written) <= max_length)
	    {
	      seg_length = buffer->length - buffer->bytes_written;
	      buffer->bytes_written = buffer->length;
	      tbx_forward_list_reference(ref);
	    }
	  else
	    {
	      seg_length = max_length;
	      buffer->bytes_written += max_length;
	    }
	  
	  desc->Data[desc->Control.SegCount].Length = seg_length;
	  desc->Control.SegCount++;
	  length += seg_length;
	  max_length -= seg_length;
	}
      while ((max_length > 0) && !tbx_reference_after_end_of_list(ref));

      desc->Control.Length = length;

      PM2_VIA_LOCK();
      VIA_CTRL(VipPostRecv(link_specific->vi.handle,
			   desc,
			   adapter_specific->memory->memory_handle));
      PM2_VIA_UNLOCK();
      desc = (VIP_DESCRIPTOR*)link_specific->vi.out.descriptor;
      //      MAD_VIA_FILL_DESC_IMMEDIATE(desc, length);
      PM2_VIA_LOCK();
      VIA_CTRL(VipPostSend(link_specific->vi.handle,
			   desc,
			   adapter_specific->memory->memory_handle));
      PM2_VIA_UNLOCK();
      do
	{
	  PM2_VIA_LOCK();
	  via_status = VipSendWait(link_specific->vi.handle,
				   VIP_INFINITE,
				   &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
      
      do
	{
	  PM2_VIA_LOCK();
	  via_status = VipRecvWait(link_specific->vi.handle,
				   VIP_INFINITE,
				   &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
    }
  while (!tbx_reference_after_end_of_list(ref));
  LOG_OUT();
}

/* ----- RDMA TM ----- */



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

#ifdef DEBUG
  mad_via_display_nic_attributes(device);
#endif /* DEBUG */

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

  LOG_IN();
  descriptor_array = malloc(array_size * sizeof(mad_via_descriptor_t));

  for (i = 0; i < array_size; i++)
    {
      descriptor = descriptor_array + i;

      descriptor->descriptor = tbx_aligned_malloc(descriptor_size, MAD_VIA_BUFFER_ALIGNMENT);
      CTRL_ALLOC(descriptor->descriptor);  
      memset(descriptor->descriptor,0,descriptor_size);
      
      descriptor->size = descriptor_size;
      /* reference to allocated bloc of data for control and ack */
      descriptor->buffer = NULL;

      PM2_VIA_LOCK();
      VIA_CTRL(VipRegisterMem(adapter_specific->nic_handle,
			      descriptor->descriptor,
			      descriptor->size,
			      &(adapter_specific->memory_attributes),
			      &(descriptor->handle)));
      PM2_VIA_UNLOCK();
      
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

  in_specific->vi->out.descriptor->buffer = tbx_aligned_malloc(64,64);
  in_specific->vi->in.descriptor->buffer = tbx_aligned_malloc(64,64);
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  in_specific->vi->out.descriptor->buffer,
			  64,
			  0,
			  &(in_specific->vi->out.descriptor->buffer_handle)
			  ));
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  in_specific->vi->in.descriptor->buffer,
			  64,
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

  out_specific->vi->out.descriptor->buffer = tbx_aligned_malloc(64,64);
  out_specific->vi->in.descriptor->buffer = tbx_aligned_malloc(64,64);
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  out_specific->vi->out.descriptor->buffer,
			  64,
			  0,
			  &(out_specific->vi->out.descriptor->buffer_handle)
			  ));
  VIA_CTRL(VipRegisterMem(
			  adapter_specific->nic_handle,
			  out_specific->vi->in.descriptor->buffer,
			  64,
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
  

  if (lnk->id == MAD_VIA_MESSAGE_LINK) {
   
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

    lnk_specific->vi.in.descriptor->buffer = tbx_aligned_malloc(64,64);
    lnk_specific->vi.out.descriptor->buffer = tbx_aligned_malloc(64,64);
    
    adapter_specific =
      (p_mad_via_adapter_specific_t) lnk->connection->channel->adapter->specific;
    VIA_CTRL(VipRegisterMem(
			    adapter_specific->nic_handle,
			    lnk_specific->vi.in.descriptor->buffer,
			    64,
			    0,
			    &(lnk_specific->vi.in.descriptor->buffer_handle)
			    ));
    VIA_CTRL(VipRegisterMem(
			    adapter_specific->nic_handle,
			    lnk_specific->vi.out.descriptor->buffer,
			    64,
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
  VIP_RETURN via_status;
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
  //fprintf(stderr,"VI is %p; DESC is %p; MH is %p\n",vi,desc,&mh);

  
  /* PrePosting a Descriptor */
  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
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

  VIP_RETURN via_status;
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
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
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
  //fprintf(stderr,"VI is %p; DESC is %p; MH is %p\n",vi,desc,&mh);


  /* Preposting a Descriptor for msg link */

  desc->Data[0].Data.Address = mad_via_vi->in.descriptor->buffer;
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
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
  LOG_OUT();
  return &(connection->link[MAD_VIA_MESSAGE_LINK]);
  
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
  VIP_RETURN via_status;

  p_mad_via_connection_specific_t specific =
    (p_mad_via_connection_specific_t) connection->specific;

  p_mad_via_vi_t vi = specific->vi;

  p_mad_via_adapter_specific_t adapter =
    (
     (p_mad_via_adapter_specific_t)
     (connection->channel->adapter->specific)
     );

  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;

  LOG_IN();

  if (specific->posted == tbx_true) {
    
    VIA_CTRL(VipSendWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc));
    
    
    VIA_CTRL(VipRecvWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc));
    
    
    
    desc = vi->in.descriptor->descriptor;
    mh = vi->in.descriptor->handle;
    
    desc->Data[0].Data.Address = vi->in.descriptor->buffer;
    desc->Data[0].Length = 64;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = 64;
    desc->Control.Status = 0;
    desc->Data[0].Handle = vi->in.descriptor->buffer_handle;
    
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
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
  desc->Control.Status = 0;
  desc->Data[0].Handle = vi->out.descriptor->buffer_handle;
  memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,"Control!",9);

  VIA_CTRL(VipPostSend(
		       vi->handle,
		       vi->out.descriptor->descriptor,
		       vi->out.descriptor->handle));
	   
  specific->posted = tbx_true;

  LOG_OUT();
}

p_mad_connection_t
mad_via_receive_message(p_mad_channel_t channel)
{
  VIP_RETURN via_status;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh;
  
  VIP_VI_HANDLE vi_handle;
  VIP_BOOLEAN direction;

  p_mad_connection_t connection = NULL;
  p_mad_via_connection_specific_t specific;
  p_mad_via_vi_t vi;
  p_mad_via_adapter_specific_t adapter =
    (
     (p_mad_via_adapter_specific_t)
     (channel->adapter->specific)
     );

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
	//fprintf(stderr,"Remote_Host_Id is %d!\n",remote_host_id);
	break;
      }
  }
  
  specific = (p_mad_via_connection_specific_t) 
    ((channel->input_connection) + remote_host_id)->specific;
  vi = specific->vi;


  if (specific->posted == tbx_true) {

    VIA_CTRL(VipSendWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc));  
  }

  /* Replaced by CQWait at the beginning of the function
  VIA_CTRL(VipRecvWait(
		       vi->handle,
		       VIP_INFINITE,
		       &desc));
  */

  desc = NULL;
  VIA_CTRL(VipRecvDone(
		       vi->handle,
		       &desc
		       ));
  
  desc = vi->in.descriptor->descriptor;
  mh = vi->in.descriptor->handle;
    
  desc->Data[0].Data.Address = vi->in.descriptor->buffer;
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
  desc->Control.Status = 0;
  desc->Data[0].Handle = vi->in.descriptor->buffer_handle;
  desc->Control.Reserved = 0;
  
  VIA_CTRL(VipPostRecv(
		       vi->handle,
		       desc,
		       mh
		       ));
  


  desc = vi->out.descriptor->descriptor;
  mh = vi->out.descriptor->handle;
  desc->Data[0].Data.Address = vi->out.descriptor->buffer;
  desc->Data[0].Length = 64;
  desc->Control.SegCount = 1;
  desc->Control.Control = 0;
  desc->Control.Length = 64;
  desc->Control.Status = 0;
  desc->Data[0].Handle = vi->out.descriptor->buffer_handle;

  memcpy(vi->out.descriptor->descriptor->Data[0].Data.Address,"Ack!",4);

  VIA_CTRL(VipPostSend(
		       vi->handle,
		       vi->out.descriptor->descriptor,
		       vi->out.descriptor->handle));

  specific->posted = tbx_true;

  LOG_OUT();

  return (channel->input_connection) + remote_host_id;
}

void
mad_via_send_buffer(p_mad_link_t     link,
		    p_mad_buffer_t   buffer)
{
  VIP_RETURN via_status;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh, mh_buffer;

  p_mad_via_adapter_specific_t adapter =
    (p_mad_via_adapter_specific_t) link->connection->channel->adapter->specific;

  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;
  
  p_mad_via_vi_t vi = 
    &((
     (p_mad_via_link_specific_t)
     (link->specific)
     )->vi);
  

  LOG_IN();

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

    if (via_status == VIP_DESCRIPTOR_ERROR) {
      niKo_debug_descriptor(desc, buffer);
    }

    VIA_CTRL(via_status);
 
    /* PostRecv Next Ack */
    desc = vi->in.descriptor->descriptor;
    mh = vi->in.descriptor->handle;
    
    desc->Data[0].Data.Address = vi->in.descriptor->buffer;
    desc->Data[0].Length = 64;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = 64;
    desc->Control.Status = 0;
    desc->Data[0].Handle = vi->in.descriptor->buffer_handle;
    desc->Control.Reserved = 0;
    
    memcpy(desc->Data[0].Data.Address,"Ack!",4);

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

    if (via_status == VIP_DESCRIPTOR_ERROR) {
      niKo_debug_descriptor(desc, buffer);
    }
    
    VIA_CTRL(via_status);

    buffer->bytes_read += current_length;
  }

  VIA_CTRL(VipDeregisterMem(
			  adapter->nic_handle,
			  buffer->buffer,
			  mh_buffer
			  ));
	   
  LOG_OUT();
}

void
mad_via_receive_buffer(p_mad_link_t     link,
		       p_mad_buffer_t  *buffer)
{
  VIP_RETURN via_status;
  VIP_DESCRIPTOR *desc;
  VIP_MEM_HANDLE mh, mh_buffer;

  
  p_mad_via_vi_t vi = 
    &((
     (p_mad_via_link_specific_t)
     (link->specific)
     )->vi);
    
  p_mad_via_link_specific_t link_specific = 
    (p_mad_via_link_specific_t) link->specific;
  
  p_mad_via_adapter_specific_t adapter =
    (p_mad_via_adapter_specific_t) link->connection->channel->adapter->specific;


  LOG_IN();
  
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
    desc->Data[0].Length = 64;
    desc->Control.SegCount = 1;
    desc->Control.Control = 0;
    desc->Control.Length = 64;
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

    if (via_status == VIP_DESCRIPTOR_ERROR) {
      niKo_debug_descriptor(desc, *buffer);
    }    

    VIA_CTRL(via_status);
    
    /* RecvWait Buffer */
    
    via_status = VipRecvWait(
			 vi->handle,
			 VIP_INFINITE,
			 &desc);
    
    if (via_status == VIP_DESCRIPTOR_ERROR) {
      niKo_debug_descriptor(desc, *buffer);
    }

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
mad_via_send_buffer_group(p_mad_link_t           lnk,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  //fprintf(stderr,"ENTERING SEND_BUFFER_GROUP\n");

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

  //fprintf(stderr,"EXITING SEND_BUFFER_GROUP\n");
  LOG_OUT();
}

void
mad_via_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_buffer_group,
				 p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  //fprintf(stderr,"ENTERING RECEIVE_SUB_BUFFER_GROUP\n");

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

  //fprintf(stderr,"EXITING RECEIVE_SUB_BUFFER_GROUP\n");
  LOG_OUT();
}

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
