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
$Log: mad_via.c,v $
Revision 1.3  2000/02/28 11:46:11  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/04 09:18:52  oaumage
- ajout de la commande de log de CVS
- phase d'initialisation `external-spawn' terminee pour mad_mpi.c
- recuperation des noms de machines dans la phase
  d'initialisation `external-spawn' de mad_sbp.c


______________________________________________________________________________
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
  size_t           number
  size_t           size;
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
  mad_via_credit_t        credits;
} mad_via_vi_t, *p_mad_via_vi_t;

/* *** preregistered memory blocks manager data ....................... *** */
typedef struct
{
  void           *next;
  VIP_MEM_HANDLE  handle;
} mad_via_memory_element_t, *p_mad_via_memory_element_t;

typedef struct
{
  PM2_SHARED;
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
  VIP_NIC_HANDLE        handle;
  VIP_NIC_ATTRIBUTES    attributes;
  VIP_PROTECTION_HANDLE ptag;
  VIP_MEM_ATTRIBUTES    memory_attributes;
} mad_via_adapter_specific_t, *p_mad_via_adapter_specific_t;

typedef struct
{
  VIP_CQ_HANDLE completion_queue;
} mad_via_channel_specific_t, *p_mad_via_channel_specific_t;

typedef struct
{
  mad_via_vi_t     vi;
  mad_bool_t       posted; /* ack reception ready ? */
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
      exit(1); }}


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
#define MAD_VIA_INITIAL_STATIC_BUFFER_NUM (NB_CREDITS * 2)
#define CONTROL_BUFFER_SIZE                 512
#define MAD_VIA_BUFFER_ALIGNMENT             32
#define MAD_VIA_MAX_TRANSFER_SIZE         32768
#define MAD_VIA_MAX_DATA_SEGMENT            252


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

/* ---- */

/*
 * Memory management
 * ------------------
 */

static void
mad_via_alloc_control_buffer(p_mad_buffer_t    buffer,
			     p_mad_adapter_t   adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  buffer->buffer         = mad_aligned_malloc(CONTROL_BUFFER_SIZE,
					      MAD_VIA_BUFFER_ALIGNMENT);
  buffer->length         = CONTROL_BUFFER_SIZE;
  buffer->bytes_written  = 0;
  buffer->bytes_read     = 0;
  buffer->type           = mad_internal_buffer;
  PM2_LOCK();
  VIA_CTRL(VipRegisterMem(adapter_specific->handle,
			  buffer->buffer,
			  buffer->length,
			  &adapter_specific->memory_attributes,
			  &(buffer->specific)));
  PM2_UNLOCK();
  LOG_OUT();
}

static void
mad_via_free_control_buffer(p_mad_buffer_t    buffer,
			    p_mad_adapter_t   adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  LOG_IN();
  PM2_LOCK();
  VIA_CTRL(VipDeregisterMem(adapter_specific->handle,
			    buffer->buffer,
			    (VIP_MEM_HANDLE)(buffer->specific)));
  PM2_UNLOCK();
  mad_aligned_free(buffer->buffer, MAD_VIA_BUFFER_ALIGNMENT);
  LOG_OUT();
}

static void *
mad_via_indirection(p_mad_via_memory_t memory,
		    size_t             index)
{
  return memory->base + memory->element_size * index;
}

static void *
mad_via_last_element(p_mad_via_memory_t memory)
{
  return mad_via_indirection(memory, memory->element_number);
}

static void *
mad_via_first_new_element(p_mad_via_memory_t memory)
{
  return mad_via_indirection(memory, memory->first_new_element);
}

static void
mad_via_register_memory_block(p_mad_via_memory_t memory)
{
  p_mad_adapter_t                adapter          = memory->adapter;
  p_mad_via_adapter_specific_t   adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipRegisterMem(adapter_specific->nic_handle,
			  memory->base,
			  memory->element_size * memory->element_number
			  + sizeof(mad_via_memory_element_t),
			  &(adapter_specific->memory_attributes),
			  &(memory->memory_handle)));
  PM2_UNLOCK();  
}

static void
mad_via_buffer_allocator_init(p_mad_adapter_t adapter,
			      size_t          buffer_length,
			      long            initial_buffer_number)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;
  p_mad_via_memory_t           memory           =
    malloc(sizeof(mad_via_memory_t));
  p_mad_via_memory_element_t   last_element     = NULL;

  LOG_IN();
  CTRL_ALLOC(memory);
  adapter_specific->memory = memory;

  PM2_INIT_SHARED(memory);
  memory->adapter = adapter;

  if (initial_buffer_number <= 0)
    {
      initial_buffer_number = INITIAL_STATIC_BUFFER_NUM;
    }

  if (buffer_len < sizeof(mad_via_memory_element_t))
    {
      buffer_len = sizeof(mad_via_memory_element_t);
    }

  memory->first_block =
    mad_aligned_malloc(initial_buffer_number * buffer_len
		       + sizeof(mad_via_memory_element_t),
		       MAD_VIA_BUFFER_ALIGNMENT);
  CTRL_ALLOC(memory->first_mem);
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
mad_via_buffer_alloc(p_mad_via_memory memory,
		     p_mad_buffer buffer)
{
  LOG_IN();
  PM2_LOCK_SHARED(memory);
  
  if (memory->head)
    {
      p_mad_via_memory_element_t element = memory->head;

      buffer->buffer   = element;
      buffer->specific = element->handle;
      memory->first_free  = element->next;
    }
  else 
    {
      if (memory->first_new_element >= memory->element_number)
	{
	  p_mad_via_memory_element_t last_element;
	  void *new_base =
	    mad_aligned_malloc(memory->element_number * memory->element_size
			       + sizeof(mad_via_memory_element_t)
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
      buffer->specific = memory->memory_handle;
      memory->first_new_element++;
    }  

  buffer->length = memory->element_size;
  buffer->bytes_written = 0 ;
  buffer->bytes_read = 0 ;
  PM2_UNLOCK_SHARED(memory);
  LOG_OUT();
}

static void
mad_via_buffer_free(p_mad_via_memory memory,
		    p_mad_buffer buffer)
{
  p_mad_via_memory_element_t element = buffer->buffer;
  
  LOG_IN();
  PM2_LOCK_SHARED(memory);
  element->next   = memory->head;
  element->handle = buffer->specific;
  memory->head = element;
  PM2_UNLOCK_SHARED(memory);
  LOG_OUT();
}

static void
mad_via_buffer_alloc_clean(p_mad_via_memory memory)
{
  const size_t                   block_size       =
    memory->element_number * memory->element_size;
  void                          *block            = memory->first_block;
  p_mad_adapter_t                adapter          = memory->adapter;
  p_mad_via_adapter_specific_t   adapter_specific = adapter->specific;
  
  LOG_IN();
  PM2_LOCK_SHARED(*memory);

  while (block)
    {
      p_mad_via_memory_element_t   element    = block + block_size;
      void                        *next_block = element->next;

      PM2_LOCK();
      VIA_CTRL (VipDeregisterMem (adapter_specific->handle,
				  block,
				  element->handle);
      PM2_UNLOCK();
      mad_aligned_free(block, MAD_VIA_BUFFER_ALIGNMENT);
      block = next_block;
    }

  free(memory);
  LOG_OUT();
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
mad_via_disp_nic_attributes(p_mad_via_device_specific device)
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
static void 
mad_via_error_callback (VIP_PVOID Context,
			VIP_ERROR_DESCRIPTOR * ErrorDesc)
      __attribute__ ((noreturn))
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

static unsigned int
mad_via_aligned(const unsigned int base,
		const unsigned int alignment)
     __attribute__ ((const))
{
  return (base + alignment - 1) & ~(alignment - 1);
}

static size_t
mad_via_connection_descriptor_size()
     __attribute__ ((const))
{
  const size_t effective_size =
    sizeof(VIP_DESCRIPTOR) + sizeof(VIP_DATA_SEGMENT);
  return mad_alignment(effective_size, VIP_DESCRIPTOR_ALIGNMENT);
}

static size_t
mad_via_credit_descriptor_size()
     __attribute__ ((const))
{
  const size_t effective_size =
    sizeof(VIP_DESCRIPTOR) + sizeof(VIP_DATA_SEGMENT);
  return mad_alignment(effective_size, VIP_DESCRIPTOR_ALIGNMENT);
}

static size_t
mad_via_message_descriptor_size()
     __attribute__ ((const))
{
  const size_t effective_size =
    sizeof(VIP_DESCRIPTOR) +
    MAD_VIA_MAX_DATA_SEGMENT * sizeof(VIP_DATA_SEGMENT);
  return mad_alignment(effective_size, VIP_DESCRIPTOR_ALIGNMENT);
}

static size_t
mad_via_rdma_descriptor_size()
     __attribute__ ((const))
{
  const size_t effective_size =
    sizeof(VIP_DESCRIPTOR) +
    sizeof(VIP_ADDRESS_SEGMENT) +
    sizeof(VIP_DATA_SEGMENT);
  return mad_alignment(effective_size, VIP_DESCRIPTOR_ALIGNMENT);
}

/* -------------------------------------------------------------------------*/
static void
mad_via_fill_discriminator(p_mad_via_discriminator_t   discriminator,
			   const p_mad_channel_t       channel,
			   const unsigned char         vi)
{
  const p_mad_adapter_t                               = channel->adapter;
  const p_mad_via_adapter_specific_t adapter_specific = adapter->specific;
  
  discriminator->adapter_id = adapter->id;
  discriminator->channel_id = channel->id;
  discriminator->host_id    =
    adapter->driver->madeleine.configuration.local_host_id;
  discriminator->vi_id      = vi;
}

/* -------------------------------------------------------------------------*/
static void
mad_via_fill_read_descriptor(VIP_DESCRIPTOR        *descriptor,
			     const p_mad_buffer_t   buffer,
			     const unsigned int     immediate_data)
{
  descriptor->CS.Length        = buffer->bytes_written;
  descriptor->CS.Status        = 0;
  descriptor->CS.Control       =
    VIP_CONTROL_OP_SENDRECV|VIP_CONTROL_OP_IMMEDIATE;
  descriptor->CS.ImmediateDate = immediate_data;
  descriptor->CS.SegCount      = 1;
  descriptor->DS[0].Local.Data.Address = buffer->buffer;
  descriptor->DS[0].Local.Data.Handle  = buffer->specific;
  descriptor->DS[0].Local.Data.Length  = buffer->length;
}

static void
mad_via_fill_write_descriptor(VIP_DESCRIPTOR        *descriptor,
			      const p_mad_buffer_t   buffer,
			      const unsigned int     immediate_data)
{
  descriptor->CS.Length        = buffer->bytes_written;
  descriptor->CS.Status        = 0;
  descriptor->CS.Control       =
    VIP_CONTROL_OP_SENDRECV|VIP_CONTROL_OP_IMMEDIATE;
  descriptor->CS.ImmediateDate = immediate_data;
  descriptor->CS.SegCount      = 1;
  descriptor->DS[0].Local.Data.Address = buffer->buffer;
  descriptor->DS[0].Local.Data.Handle  = buffer->specific;
  descriptor->DS[0].Local.Data.Length  = buffer->bytes_written;
}

static void
mad_via_fill_minimal_descriptor(VIP_DESCRIPTOR *descriptor)
{
  descriptor->CS.Length   = 0;
  descriptor->CS.Status   = 0;
  descriptor->CS.Control  = VIP_CONTROL_OP_SENDRECV;
  descriptor->CS.SegCount = 0;
}

/* -------------------------------------------------------------------------*/

static void
mad_via_open_nic(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipOpenNic(adapter->name, &(adapter_specific->handle)));
  PM2_UNLOCK();  
}


static void
mad_via_set_error_callback(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipErrorCallback(adapter_specific->handle,
			    NULL,
			    mad_via_error_callback));
  PM2_UNLOCK();  
}


static void
mad_via_ns_init(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipNSInit(adapter_specific->handle, NULL));
  PM2_UNLOCK();
}

static void
mad_via_query_nic(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipQueryNic(adapter_specific->handle,
		       &(adapter_specific->attributes)));
  PM2_UNLOCK();

#ifdef DEBUG
  mad_via_display_nic_attributes(device);
#endif /* DEBUG */
}

static void
mad_via_create_ptag(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = adapter->specific;

  PM2_LOCK();
  VIA_CTRL(VipCreatePtag(adapter_specific->nic_handle,
			 &(adapter_specific->ptag)));
  PM2_UNLOCK();
  adapter_specific->memory_attributes.Ptag            =
    adapter_specific->ptag;
  adapter_specific->memory_attributes.EnableRdmaWrite = VIP_TRUE;
  adapter_specific->memory_attributes.EnableRdmaRead  = VIP_FALSE;
}

static void
mad_via_static_buffers_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t        driver        = adapter->driver;
  p_mad_configuration_t configuration = &(driver->madeleine.configuration);
  size_t                buffer_size   = STATIC_BUFFER_SIZE;
  int                   buffer_number =
    INITIAL_STATIC_BUFFER_NUM * configuration->size

  mad_via_buffer_alloc_init(adapter, buffer_size, buffer_number);
}

static void
mad_via_create_completion_queue(p_mad_channel_t channel)
{
  p_mad_adapter_t              adapter          = channel->adapter;
  p_mad_adapter_specific_t     adapter_specific = adapter->specific;
  p_mad_driver_t               driver           = adapter->driver;
  p_mad_configuration_t        configuration    =
    &(driver->madeleine.configuration);

  PM2_VIA_LOCK();
  VIA_CTRL(VipCreateCQ(adapter_specific->handle,
		       configuration->size,
		       &(channel_specific->completion_queue)));
  PM2_VIA_UNLOCK();

}

static p_mad_descriptor_t
mad_via_allocate_descriptor_array(p_mad_connection_t connection,
				  size_t             descriptor_size,
				  size_t             array_size)
{
  p_mad_channel_t           channel          = connection->channel;
  p_mad_adapter_t           adapter          = channel->adapter;
  p_mad_adapter_specific_t  adapter_specific = adapter->specific;
  p_mad_via_descriptor_t    descriptor_array;
  int                       i;
  descriptor_array = malloc(array_size * sizeof(mad_via_descriptor_t));

  for (i = 0; i < array_size; i++)
    {
      p_mad_via_descriptor_t descriptor = descriptor_array + i;

      descriptor = malloc(sizeof(mad_via_descriptor_t));
      CTRL_ALLOC(descriptor);
      descriptor->descriptor = mad_aligned_malloc(size);
      CTRL_ALLOC(descriptor->descriptor);  
      descriptor->size = descriptor_size;
      
      PM2_VIA_LOCK();
      VIA_CTRL(VipRegisterMem(adapter_specific->handle,
			      descriptor->descriptor,
			      descriptor->size,
			      &(adapter_specific->memory_attributes),
			      &(descriptor->handle)));
      PM2_VIA_UNLOCK();
      
    }
  
  return descriptor_array;
}

static void
mad_via_create_input_connection_vi(p_mad_connection_t connection,
				   p_mad_via_vi_t     vi)
{
  p_mad_via_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t                 channel             = connection->channel;
  p_mad_channel_specific_t        channel_specific    = channel->specific;
  p_mad_adapter_t                 adapter             = channel->adapter;
  p_mad_adapter_specific_t        adapter_specific    = adapter->specific;
  VIP_VI_ATTRIBUTES attributes;
  
  attributes.ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  attributes.Ptag             = adapter_specific->ptag;
  attributes.EnableRdmaWrite  = VIP_TRUE;
  attributes.EnableRdmaRead   = VIP_FALSE;
  attributes.QoS              = 0;
  attributes.MaxTransferSize  = MAD_VIA_MAX_TRANSFER_SIZE;

  PM2_VIA_LOCK();
  VIA_CTRL(VipCreateVi(adapter_specific->handle,
		       &attributes,
		       NULL,
		       channel_specific->completion_queue,
		       &(vi->handle)));
  PM2_VIA_UNLOCK();
}

static void
mad_via_create_vi(p_mad_connection_t connection,
		  p_mad_via_vi_t     vi)
{
  p_mad_via_connection_specific_t connection_specific = connection->specific;
  p_mad_channel_t                 channel             = connection->channel;
  p_mad_adapter_t                 adapter             = channel->adapter;
  p_mad_adapter_specific_t        adapter_specific    = adapter->specific;
  VIP_VI_ATTRIBUTES attributes;
  
  attributes.ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  attributes.Ptag             = adapter_specific->ptag;
  attributes.EnableRdmaWrite  = VIP_TRUE;
  attributes.EnableRdmaRead   = VIP_FALSE;
  attributes.QoS              = 0;
  attributes.MaxTransferSize  = MAD_VIA_MAX_TRANSFER_SIZE;

  PM2_VIA_LOCK();
  VIA_CTRL(VipCreateVi(adapter_specific->handle,
		       &attributes,
		       NULL,
		       NULL,,
		       &(vi->handle)));
  PM2_VIA_UNLOCK();
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

static void
mad_via_rdma_send_buffers(p_mad_link_t             lnk,
			  p_mad_list_reference_t   ref)
{
  p_mad_connection_t              connection          = lnk->connection;
  p_mad_via_connection_specific_t connection_specific = connection_specific;
  p_mad_via_vi_t                  vi                  = &(lnk->vi);
  p_mad_via_way_t                 in                  = &(vi->in);  
  p_mad_via_way_t                 out                 = &(vi->out);
  VIP_ADDRESS_SEGMENT            *address_segment;
  VIP_DESCRIPTOR                 *descriptor;
  VIP_RETURN                      via_status;
  int                             buffer_number;
  int                             buffer_counter;

  LOG_IN();
  address_segment = in->buffer->buffer;

  do
    {
      PM2_VIA_LOCK();
      via_status = VipRecvWait(vi->handle, VIP_INFINITE, &descriptor);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  PM2_YIELD();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;

  buffer_number = descriptor->CS.ImmediateData;
#error Modification stopped here
#error memory handle should not be put into mad_via_descriptor_t
  PM2_VIA_LOCK();
  VIA_CTRL(VipPostRecv(vi->handle, descriptor, vi->in.descriptor->handle));
  PM2_VIA_UNLOCK();

  for (buffer_counter = 0;
       buffer_counter < buffer_number;
       address_segment++, buffer_counter++as++)
    {
      p_mad_buffer_t buffer = mad_get_list_reference_object(ref);

      while (mad_more_data(buffer))
	{
	  size_t remaining_bytes = buffer->bytes_written - buffer->bytes_read;
	  size_t block_length    =
	    min(remaining_bytes, MAD_VIA_MAX_TRANSFER_SIZE);

	  descriptor = vi->out.descriptor->descriptor ;

	  descriptor->CS.Length  = block_length;
	  descriptor->CS.Status  = 0;
	  descriptor->CS.Control = VIP_CONTROL_OP_RDMAWRITE;

	  if (   ((buffer_counter + 1) == nb_buffers)
	      &&  (block_length == remaining_bytes))
	    {
	      desc->CS.Control       |= VIP_CONTROL_IMMEDIATE;
	      desc->CS.ImmediateData  = nb_buffers;
	    }

	  desc->CS.SegCount = 2;
	  
	  desc->DS[0].Remote = *as;
	  desc->DS[0].Remote.Data.Address += buffer->bytes_read; 
	  desc->DS[1].Local.Data.Address =
	    buffer->buffer + buffer->bytes_read;
	  desc->DS[1].Local.Handle = (VIP_MEM_HANDLE)buffer->specific;
	  desc->DS[1].Local.Length = block_length;

	  PM2_VIA_LOCK();
	  VIA_CTRL(VipPostSend (conn->rdma_vi_handle,
				desc,
				conn->memory_handle));
	  PM2_VIA_UNLOCK();

	  do
	    {
	      PM2_VIA_LOCK();
	      via_status = VipSendWait(conn->rdma_vi_handle,
			  VIP_INFINITE,
			  &desc);
	      PM2_VIA_UNLOCK();
#ifdef PM2
	      if (via_status == VIP_TIMEOUT)
		{
		  marcel_yield();
		}
#endif /* PM2 */	  
	    }
	  while (via_status == VIP_TIMEOUT) ;
	  VIA_CTRL(via_status);
	  
	  buffer->bytes_read += block_length;
	}
      mad_forward_list_reference(ref);
    }

  LOG_OUT();
}

static void
mad_via_rdma_receive_buffers(p_mad_link_t             lnk,
			     p_mad_list_reference_t   ref)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_via_connection_specific_t   connection_specific = connection_specific;
  VIP_RETURN                        via_status;
  VIP_ADDRESS_SEGMENT              *as ;
  VIP_DESCRIPTOR                   *desc                = NULL;
  int                               count               = 0 ;

  LOG_IN();
  as   = (VIP_ADDRESS_SEGMENT *)(conn->rdma_buffer_out.buffer);

  desc = conn->rdma_descriptor_in;
  MAD_VIA_FILL_DESC_MINIMAL(desc);
  PM2_VIA_LOCK();
  VIA_CTRL(VipPostRecv(conn->rdma_vi_handle,
		       desc,
		       conn->memory_handle));
  PM2_VIA_UNLOCK();

  do
    {
      p_mad_buffer buffer;

      buffer = (p_mad_buffer)mad_get_list_reference_object(ref);
      as->Data.Address = buffer->buffer;
      as->Handle = (VIP_MEM_HANDLE)buffer->specific;
      as->Reserved = 0;      
      as++;
      count++;
    }
  while (mad_forward_list_reference(ref));

  conn->rdma_buffer_out.bytes_written = count * sizeof(VIP_ADDRESS_SEGMENT) ;
  desc = conn->rdma_descriptor_out;
  MAD_VIA_FILL_DESC_WRITE_IMMEDIATE(desc,
				    &(conn->rdma_buffer_out),
				    count);

  PM2_VIA_LOCK();
  VIA_CTRL(VipPostSend(conn->rdma_vi_handle,
		       desc,
		       conn->memory_handle));
  PM2_VIA_UNLOCK();

  do
    {
      PM2_VIA_LOCK();
      via_status =
	VipSendWait(conn->rdma_vi_handle, VIP_INFINITE, &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);
 
  do
    {
      PM2_VIA_LOCK();
      via_status =
	VipRecvWait(conn->rdma_vi_handle, VIP_INFINITE, &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);
  LOG_OUT();
}
  
static void
mad_via_msg_send_buffers(p_mad_link_t             lnk,
			 p_mad_list_reference_t   ref)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_via_connection_specific_t   connection_specific = connection_specific;
  VIP_DESCRIPTOR                   *desc                = NULL;
  VIP_RETURN                        via_status;
  size_t                            length;

  LOG_IN();

  do
    {
      PM2_VIA_LOCK();
      via_status = VipRecvWait(conn->message_vi_handle,
			       VIP_INFINITE,
			       &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);

  length = desc->CS.ImmediateData;
  
  PM2_VIA_LOCK();
  VIA_CTRL(VipPostRecv(conn->message_vi_handle,
		       desc,
		       conn->memory_handle));
  PM2_VIA_UNLOCK();
  
  desc = conn->message_descriptor_out ;
  MAD_VIA_FILL_DESC_IMMEDIATE(desc, length);
  desc->CS.Length = length;
  
  do
    {
      size_t seg_length = 0 ;
      p_mad_buffer buffer;

      buffer = (p_mad_buffer)mad_get_list_reference_object(ref);
      
      desc->DS[desc->CS.SegCount].Local.Data.Address =
	buffer->buffer + buffer->bytes_read;
      desc->DS[desc->CS.SegCount].Local.Handle =
	(VIP_MEM_HANDLE)buffer->specific;
      
      if ((buffer->bytes_written - buffer->bytes_read) <= length)
	{
	  seg_length = buffer->bytes_written - buffer->bytes_read;
	  mad_forward_list_reference(ref);
	  buffer->bytes_read = buffer->bytes_written;
	}
      else
	{
	  seg_length = length;
	  buffer->bytes_read += length ;
	}

      length -= seg_length;
      desc->DS[desc->CS.SegCount].Local.Length = seg_length;
      desc->CS.SegCount++;
    }
  while((length > 0) && (!mad_reference_after_end_of_list(ref)));

  PM2_VIA_LOCK();
  VIA_CTRL(VipPostSend (conn->message_vi_handle,
			desc,
			conn->memory_handle));
  PM2_VIA_UNLOCK();

  do
    {
      PM2_VIA_LOCK();
      via_status = VipSendWait(conn->message_vi_handle,
			       VIP_INFINITE,
			       &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);
  LOG_OUT();
}

static void 
mad_via_msg_receive_buffers(p_mad_link_t             lnk,
			    p_mad_list_reference_t   ref)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_via_connection_specific_t   connection_specific = connection_specific;
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
      
      desc = conn->message_descriptor_in ;
      desc->CS.Status = 0;
      desc->CS.Control = VIP_CONTROL_OP_SENDRECV;
      desc->CS.SegCount = 0;

      do
	{
	  size_t seg_length = 0 ;
	  p_mad_buffer buffer;
	  
	  buffer = (p_mad_buffer)mad_get_list_reference_object(ref);
	  
	  desc->DS[desc->CS.SegCount].Local.Data.Address =
	    buffer->buffer + buffer->bytes_written;
	  desc->DS[desc->CS.SegCount].Local.Handle =
	    (VIP_MEM_HANDLE)buffer->specific;
	  
	  if ((buffer->length - buffer->bytes_written) <= max_length)
	    {
	      seg_length = buffer->length - buffer->bytes_written;
	      buffer->bytes_written = buffer->length;
	      mad_forward_list_reference(ref);
	    }
	  else
	    {
	      seg_length = max_length;
	      buffer->bytes_written += max_length;
	    }
	  
	  desc->DS[desc->CS.SegCount].Local.Length = seg_length;
	  desc->CS.SegCount++;
	  length += seg_length;
	  max_length -= seg_length;
	}
      while ((max_length > 0) && !mad_reference_after_end_of_list(ref));

      desc->CS.Length = length;

      PM2_VIA_LOCK();
      VIA_CTRL(VipPostRecv(conn->message_vi_handle,
			   desc,
			   conn->memory_handle));
      PM2_VIA_UNLOCK();
      desc = conn->message_descriptor_out ;
      MAD_VIA_FILL_DESC_IMMEDIATE(desc, length);
      PM2_VIA_LOCK();
      VIA_CTRL(VipPostSend(conn->message_vi_handle,
			   desc,
			   conn->memory_handle));
      PM2_VIA_UNLOCK();
      do
	{
	  PM2_VIA_LOCK();
	  via_status = VipSendWait(conn->message_vi_handle,
				   VIP_INFINITE,
				   &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif /* PM2 */	  
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
      
      do
	{
	  PM2_VIA_LOCK();
	  via_status = VipRecvWait(conn->message_vi_handle,
				   VIP_INFINITE,
				   &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif /* PM2 */	  
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
    }
  while (!mad_reference_after_end_of_list(ref));

  LOG_OUT();
}

static void 
mad_via_credit_write(p_mad_link_t     lnk,
		     p_mad_buffer_t   buffer)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_via_connection_specific_t   connection_specific = connection_specific;
  p_mad_via_credits_t               credits             = link->specific;
  VIP_DESCRIPTOR                   *desc                = NULL;  
  VIP_RETURN                        via_status;

  LOG_IN();
  if (credits->available_credits <= credits->alert)
    {
      mad_via_credit new_credits;
      
      do
	{
	  PM2_VIA_LOCK();
	  via_status =
	    VipRecvWait(conn->credit_vi_handle, VIP_INFINITE, &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif /* PM2 */	  
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
      
      new_credits = (mad_via_credit)desc->CS.ImmediateData;
      
      PM2_VIA_LOCK();
      VIA_CTRL(VipPostRecv(conn->credit_vi_handle, desc,
			   conn->memory_handle));
      PM2_VIA_UNLOCK();
      
      if ((new_credits < 1) && (credits->available_credits <= 0))
	{
	  FAILURE("No more credits");
	}
      credits->available_credits += new_credits;
    }

  desc = conn->credit_descriptor_out ;
  MAD_VIA_FILL_DESC_WRITE(desc, buffer);

  PM2_VIA_LOCK();
  VIA_CTRL(VipPostSend (conn->credit_vi_handle, desc, conn->memory_handle));
  PM2_VIA_UNLOCK();
  do
    {
      PM2_VIA_LOCK();
      via_status = VipSendWait(conn->credit_vi_handle, VIP_INFINITE, &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);

  mad_via_buffer_free(conn->device->via_memory, buffer);
  credits->available_credits--;
  LOG_OUT();
}

static void 
mad_via_credit_read(p_mad_link_t     lnk,
		    p_mad_buffer_t  *buf)
{
  p_mad_connection_t                connection          = lnk->connection;
  p_mad_via_connection_specific_t   connection_specific = connection_specific;
  p_mad_via_credits_t               credits             = link->specific;
  VIP_DESCRIPTOR                   *desc                = NULL;  
  VIP_RETURN                        via_status;
  p_mad_buffer_t                    buffer;
  
  LOG_IN();
  if (credits->available_credits <= credits->alert)
    {
      desc = conn->credit_descriptor_out ;
      MAD_VIA_FILL_DESC_IMMEDIATE(desc, credits->returned_credits);

      PM2_VIA_LOCK();
      VIA_CTRL(VipPostSend (conn->credit_vi_handle,
			    desc,
			    conn->memory_handle));
      PM2_VIA_UNLOCK();    
  
      do
	{
	  PM2_VIA_LOCK();
	  via_status = VipSendWait(conn->credit_vi_handle,
				   VIP_INFINITE,
				   &desc);
	  PM2_VIA_UNLOCK();
#ifdef PM2
	  if (via_status == VIP_TIMEOUT)
	    {
	      marcel_yield();
	    }
#endif /* PM2 */	  
	}
      while (via_status == VIP_TIMEOUT) ;
      VIA_CTRL(via_status);
      
      credits->available_credits += credits->returned_credits;
      credits->returned_credits = 0;
    }

  do
    {
      PM2_VIA_LOCK();
      via_status = VipRecvWait(conn->credit_vi_handle,
			       VIP_INFINITE,
			       &desc);
      PM2_VIA_UNLOCK();
#ifdef PM2
      if (via_status == VIP_TIMEOUT)
	{
	  marcel_yield();
	}
#endif /* PM2 */	  
    }
  while (via_status == VIP_TIMEOUT) ;
  VIA_CTRL(via_status);

  buffer = mad_alloc_buffer_struct();

  buffer->length = STATIC_BUFFER_SIZE;
  buffer->bytes_written = desc->DS[0].Local.Length ;
  buffer->bytes_read = 0 ;
  buffer->type = mad_static_buffer ;
  buffer->buffer = desc->DS[0].Local.Data.Address;
  buffer->specific = (void*)desc->DS[0].Local.Handle;
    
  {
    mad_buffer temp_buffer;

    mad_via_buffer_alloc(conn->device->via_memory,
			 &temp_buffer);
    
    MAD_VIA_FILL_DESC_READ(desc, &temp_buffer);

    PM2_VIA_LOCK();

    VIA_CTRL(VipPostRecv (conn->credit_vi_handle,
			  desc,
			  conn->memory_handle));
    PM2_VIA_UNLOCK();
  }

  *buf = buffer;
  credits->available_credits--;
  credits->returned_credits++;
  LOG_OUT();
}

static void 
mad_via_prepare_buffer(p_mad_link_t     lnk,
		       p_mad_buffer_t   buffer)
{
  LOG_IN();
  buffer->specific = NULL;
  PM2_VIA_LOCK();
  VIA_CTRL(VipRegisterMem(conn->device->nic_handle,
			  buffer->buffer,
			  buffer->length,
			  &(conn->device->memory_attributes),
			  &((VIP_MEM_HANDLE)buffer->specific)));
  PM2_VIA_UNLOCK();
  LOG_OUT();
}

static void
mad_via_prepare_buffer_group(p_mad_link_t           lnk,
			     p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      mad_list_reference ref;
      mad_init_list_reference(&ref, &(buffer_group->buffer_list));

      do
	{
	  mad_via_prepare_buffer(lnk, mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

static void 
mad_via_unprepare_buffer(p_mad_link_t     lnk,
			 p_mad_buffer_t   buffer)
{
  LOG_IN();
  PM2_VIA_LOCK();
  VIA_CTRL(VipDeregisterMem(conn->device->nic_handle,
			    buffer->buffer,
			    (VIP_MEM_HANDLE)(buffer->specific)));
  PM2_VIA_UNLOCK();
  buffer->specific = NULL;
  LOG_OUT();
}

static void 
mad_via_unprepare_buffer_group(p_mad_link_t         lnk,
			       p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!mad_empty_list(&(buffer_group->buffer_list)))
    {
      mad_list_reference ref;
      mad_init_list_reference(&ref, &(buffer_group->buffer_list));

      do
	{
	  mad_via_unprepare_buffer(lnk, mad_get_list_reference_object(&ref));
	}
      while(mad_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/* ----- */
void
mad_via_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface;

  LOG_IN();
  interface = &(driver->interface);

  driver->connection_type = mad_bidirectional_connection;
  driver->buffer_alignment = 32;

  interface->driver_init                = mad_via_driver_init;
  interface->adapter_init               = mad_via_adapter_init;
  interface->adapter_configuration_init = mad_via_adatpter_configuration_init;
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
  return settings;
}

void
mad_via_driver_init(p_mad_driver_t driver)
{
  p_mad_tcp_driver_specific_t driver_specific;

  LOG_IN();
  driver_specific = malloc(sizeof(mad_tcp_driver_specific_t));
  CTRL_ALLOC(driver_specific);
  driver->specific = driver_specific;
  driver_specific->nb_adapter = 0;
#ifdef PM2
  marcel_mutex_init(&__pm2_mutex, NULL);
#endif /* PM2 */
  LOG_OUT();
}

void
mad_via_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_driver_t                 driver           = adapter->driver;
  p_mad_tcp_driver_specific_t    driver_specific  = driver->specific;
  p_mad_tcp_adapter_specific_t   adapter_specific;

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
  mad_via_static_buffers_init(adapter);
  LOG_OUT();
}

void mad_via_channel_init(p_mad_channel_t channel)
{
  p_mad_via_channel_specific_t channel_specific ;

  LOG_IN();
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

  in_specific = malloc(sizeof(mad_via_connection_specific_t));
  CTRL_ALLOC(in);
  in->specific = in_specific;
  in->nb_link  = 3;

  in_specific->posted     = mad_false;
  mad_via_init_credits(&(in_specific->vi.credits),
		       MAD_VIA_CONNECTION_CREDIT_NUMBER);
  mad_via_create_input_connection_vi(in, &(in_specific->vi));
  in_specific->vi.in.descriptor = 
    mad_via_allocate_descriptor_array(in,
				      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
				      MAD_VIA_CONNECTION_CREDIT_NUMBER);
  in_specific->vi.out.descriptor =
    mad_via_allocate_descriptor_array(in,
				      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
				      1);

  out_specific = malloc(sizeof(mad_via_connection_specific_t));
  CTRL_ALLOC(out);
  out->specific = out_specific;
  out->nb_link  = 3;

  out_specific->posted     = mad_false;
  mad_via_init_credits(&(out_specific->vi.credits),
		       MAD_VIA_CONNECTION_CREDIT_NUMBER);
  mad_via_create_vi(out, &(out_specific->vi));
  out_specific->vi.in.descriptor =
    mad_via_allocate_descriptor_array(out,
				      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
				      1);
  out_specific->vi.out.descriptor =
    mad_via_allocate_descriptor_array(out,
				      MAD_VIA_CONNECTION_DESCRIPTOR_SIZE,
				      1);
}

void
mad_via_link_init(p_mad_link_t lnk)
{
  p_mad_connection_t         connection = lnk->connection;
  p_mad_via_link_specific_t link_specific;

  LOG_IN();
  link_specific = malloc(sizeof(mad_via_link_specific_t));
  CTRL_ALLOC(lnk_specific);
  lnk->specific = lnk_specific;
  mad_via_create_vi(connection, &(lnk_specific->vi));

  if (lnk->id == CREDIT_LINK)
    {
      lnk->link_mode   = mad_link_mode_buffer;
      lnk->buffer_mode = mad_buffer_mode_static;

      mad_via_init_credits(&(lnk_specific->vi.credits),
			   MAD_VIA_LINK_CREDIT_NUMBER);
    
      if (connection->way == mad_incoming_connection)
	{
	  lnk_specific->vi.in.descriptor =
	    mad_via_allocate_descriptor_array(lnk->connection,
					      MAD_VIA_CREDIT_DESCRIPTOR_SIZE,
					      MAD_VIA_LINK_CREDIT_NUMBER);
	}
      else
	{
	  lnk_specific->vi.in.descriptor =
	    mad_via_allocate_descriptor_array(lnk->connection,
					      MAD_VIA_CREDIT_DESCRIPTOR_SIZE,
					      1);
	}
      lnk_specific->vi.out.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_CREDIT_DESCRIPTOR_SIZE,
					  1);
    }
  else if (lnk->id == MESSAGE_LINK)
    {
      lnk->link_mode   = mad_link_mode_buffer_group;
      lnk->buffer_mode = mad_buffer_mode_dynamic;
      lnk_specific->vi.in.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_MESSAGE_DESCRIPTOR_SIZE,
					  1);
      lnk_specific->vi.out.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_MESSAGE_DESCRIPTOR_SIZE,
					  1);
    }
  else if (lnk->id)
    {
      lnk->link_mode   = mad_link_mode_buffer_group;
      lnk->buffer_mode = mad_buffer_mode_dynamic;
      lnk_specific->vi.in.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_RDMA_DESCRIPTOR_SIZE,
					  1);
      lnk_specific->vi.out.descriptor = 
	mad_via_allocate_descriptor_array(lnk->connection,
					  MAD_VIA_RDMA_DESCRIPTOR_SIZE,
					  1);
    }
  LOG_OUT();
}

void
mad_via_accept(p_mad_channel_t channel)
{
  LOG_IN();
#error unimplemented
  LOG_OUT();
}

void
mad_via_connect(p_mad_connection_t connection)
{
  LOG_IN();
#error unimplemented
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
  LOG_IN();
#error unimplemented
  LOG_OUT();
}

void
mad_via_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
#error unimplemented
  LOG_OUT();
}

p_mad_link_t
mad_via_choice(p_mad_connection_t   connection,
	       size_t               size,
	       mad_send_mode_t      send_mode,
	       mad_receive_mode_t   receive_mode)
{
  LOG_IN();
   if (buffer_length <= SHORT_BUFFER_LIMIT)
    {
      LOG_OUT();
      return &(connection->link[CREDIT_LINK]);
    }
  else if (buffer_length <= LONG_BUFFER_LIMIT)
    {
      LOG_OUT();
      return &(connection->link[MESSAGE_LINK]);
    }
  else
    {
      LOG_OUT();
      return &(connection->link[RDMA_LINK]);
    }
}

void
mad_via_new_message(p_mad_connection_t connection)
{
  LOG_IN();
#error unimplemented
  LOG_OUT();
}

p_mad_connection_t
mad_via_receive_message(p_mad_channel_t channel)
{
  LOG_IN();
#error unimplemented
  LOG_OUT();
}

void
mad_via_send_buffer(p_mad_link_t     link,
		    p_mad_buffer_t   buffer)
{
  LOG_IN();
  if (link->id == CREDIT_LINK)
    {
      mad_via_credit_write(link, buffer);
    }
  else
    FAILURE("this link only transfer grouped buffer");
  LOG_OUT();
}

void
mad_via_receive_buffer(p_mad_link_t     link,
		       p_mad_buffer_t  *buffer)
{
  LOG_IN();
  if (link->id == CREDIT_LINK)
    {
      mad_via_credit_read(link, buffer);
    }
  else
    FAILURE("this link only transfer grouped buffer");
  LOG_OUT();
}

void
mad_via_send_buffer_group(p_mad_link_t           link,
			  p_mad_buffer_group_t   buffer_group)
{
  LOG_IN();
  if (link->id == CREDIT_LINK)
    {
      if (!mad_empty_list(&(buffer_group->buffer_list)))
	{
	  mad_list_reference ref;
	  mad_init_list_reference(&ref, &(buffer_group->buffer_list));

	  do
	    {
	      mad_via_credit_write(link, mad_get_list_reference_object(&ref));
	    }
	  while (mad_forward_list_reference(&ref));
	}
    }
  else
    {
      if (!mad_empty_list(&(buffer_group->buffer_list)))
	{
	  mad_list_reference ref;
	  mad_via_prepare_buffer_group(link->connection, buffer_group);
	  mad_init_list_reference(&ref, &(buffer_group->buffer_list));

	  if (link->id == MESSAGE_LINK)
	    {
	      do
		{
		  mad_via_msg_send_buffers(link, &ref);
		}
	      while (!mad_reference_after_end_of_list(&ref));
	    }
	  else if (link->id == RDMA_LINK)
	    {
	      do
		{
		  mad_via_rdma_send_buffers(link, &ref);
		}
	      while (!mad_reference_after_end_of_list(&ref));
	    }
	  else
	    FAILURE("unknown link id");
	  
	  mad_via_unprepare_buffer_group(link->connection, buffer_group);
	}
    }
  LOG_OUT();
}

void
mad_via_receive_sub_buffer_group(p_mad_link_t         link,
				 mad_bool_t           first_sub_buffer_group,
				 p_mad_buffer_group_t buffer_group)
{
  LOG_OUT();

  if (link->id == CREDIT_LINK)
    {
      FAILURE("this link cannot receive grouped buffers");
    }
  else
    {
      if (!mad_empty_list(&(buffer_group->buffer_list)))
	{
	  mad_list_reference ref;
	  mad_via_prepare_buffer_group(link->connection, buffer_group);
	  mad_init_list_reference(&ref, &(buffer_group->buffer_list));
	  
	  if (link->id == MESSAGE_LINK)
	    {
	      mad_via_msg_receive_buffers(link, &ref);
	    }
	  else if (link->id == RDMA_LINK)
	    {
	      mad_via_rdma_receive_buffers(link, &ref);
	    }
	  else
	    FAILURE("unknown link id");
	  
	  mad_via_unprepare_buffer_group(link->connection, buffer_group);
	}
    }

  LOG_OUT();
}

p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_via_adapter_specific_t adapter_specific
    = lnk->connection->channel->adapter->device_specific;
  p_mad_buffer_t buffer;

  LOG_IN();
  buffer                = mad_alloc_buffer_struct();
  buffer->length        = STATIC_BUFFER_SIZE;
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
    = lnk->connection->channel->adapter->device_specific;
  LOG_IN();
  mad_via_buffer_free(adapter_specific->memory, buffer);
  LOG_OUT();
}
