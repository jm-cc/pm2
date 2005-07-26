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

/*
 * Macrocommands
 * ----------------------------------------------------------------------------
 */

/* Constants*/
#define MAD_VIA_INITIAL_CHANNEL_CQ_SIZE     16
#define MAD_VIA_MAIN_BUFFER_SIZE         32768

#define MAD_VIA_MSG_THRES                 4096
#define MAD_VIA_RDMA_THRES               32768

#define MAD_VIA_RECV_MAIN_DESC_NUM           8
#define MAD_VIA_RECV_MAIN_DESC_ADDR_NUM      0
#define MAD_VIA_RECV_MAIN_DESC_DATA_NUM      8

#define MAD_VIA_SEND_CREDIT_DESC_NUM         8
#define MAD_VIA_SEND_CREDIT_DESC_ADDR_NUM    0
#define MAD_VIA_SEND_CREDIT_DESC_DATA_NUM    1
#define MAD_VIA_RECV_CREDIT_DESC_NUM         MAD_VIA_SEND_CREDIT_DESC_NUM
#define MAD_VIA_RECV_CREDIT_DESC_ADDR_NUM    MAD_VIA_SEND_CREDIT_DESC_ADDR_NUM
#define MAD_VIA_RECV_CREDIT_DESC_DATA_NUM    MAD_VIA_SEND_CREDIT_DESC_DATA_NUM
#define MAD_VIA_CREDIT_BUFFER_SIZE           MAD_VIA_MSG_THRES
#define MAD_VIA_CREDIT_NB_BUFFER             8

#define MAD_VIA_SEND_RDMA_DESC_NUM           2
#define MAD_VIA_SEND_RDMA_DESC_ADDR_NUM      1
#define MAD_VIA_SEND_RDMA_DESC_DATA_NUM      1

#ifndef VIP_DATA_ALIGNMENT
#define VIP_DATA_ALIGNMENT VIP_DESCRIPTOR_ALIGNMENT
#endif // VIP_DATA_ALIGNMENT

/* Access to the discriminator field of the VI attribute structure */
#ifdef TBX_USE_SAFE_MACROS
#define MAD_VIA_DISCRIMINATOR(a) (*({                                       \
        typedef _ta = (a);                                                  \
        _ta _a = (a);                                                       \
        (p_mad_via_discriminator_t) (_a->HostAddress + _a->HostAddressLen); \
}))
#else // TBX_USE_SAFE_MACROS
#define MAD_VIA_DISCRIMINATOR(a) \
        (*(p_mad_via_discriminator_t) ((a)->HostAddress + (a)->HostAddressLen))
#endif // TBX_USE_SAFE_MACROS

/*
 *  param a: nb of address segments
 *  param d: nb of data segments
 */
#ifdef TBX_USE_SAFE_MACROS
#define MAD_VIA_DESCRIPTOR_SIZE(a, d) ({                \
        int _a = (a);                                   \
        int _d = (d);                                   \
                                                        \
        tbx_aligned(      (sizeof(VIP_DESCRIPTOR)       \
                   + (_a) * sizeof(VIP_ADDRESS_SEGMENT) \
                   + (_d) * sizeof(VIP_DATA_SEGMENT)),  \
                  VIP_DESCRIPTOR_ALIGNMENT); })
#else // TBX_USE_SAFE_MACROS
#define MAD_VIA_DESCRIPTOR_SIZE(a, d)                  \
        tbx_aligned(      (sizeof(VIP_DESCRIPTOR)      \
                   + (a) * sizeof(VIP_ADDRESS_SEGMENT) \
                   + (d) * sizeof(VIP_DATA_SEGMENT)),  \
                  VIP_DESCRIPTOR_ALIGNMENT)
#endif // TBX_USE_SAFE_MACROS

/*
 * Enums
 * ----------------------------------------------------------------------------
 */
typedef enum e_mad_via_vi_rdma_mode
  {
    rdma_off        = 0,
    rdma_write      = 1,
    rdma_read       = 2,
    rdma_read_write = 3,
  }
mad_via_vi_rdma_mode_t, *p_mad_via_vi_rdma_mode_t;

typedef int mad_via_vi_id_base_t, *p_mad_via_vi_id_base_t;

typedef enum e_mad_via_vi_id_idx
  {
    vi_id_idx_in_main    = 0,
    vi_id_idx_out_main   = 1,
    vi_id_idx_in_credit  = 2,
    vi_id_idx_out_credit = 3,
    vi_id_idx_in_msg     = 4,
    vi_id_idx_out_msg    = 5,
    vi_id_idx_in_rdma    = 6,
    vi_id_idx_out_rdma   = 7,
  }
mad_via_vi_id_idx_t, *p_mad_via_vi_id_idx_t;

typedef enum e_mad_via_link_id
{
  cresit_link_id = 0,
  msg_link_id,
  rdma_link_id,
  nb_link,
} mad_via_link_id_t, *p_mad_via_link_id_t;

/*
 * Structures
 * ----------------------------------------------------------------------------
 */
typedef struct s_mad_via_discriminator
{
  unsigned short int vi_id_base;
  unsigned char      process_grank;
  unsigned char      vi_id_idx;
} mad_via_discriminator_t, *p_mad_via_discriminator_t;

typedef struct s_mad_via_buffer_specific
{
  VIP_MEM_HANDLE mem_handle;
} mad_via_buffer_specific_t, *p_mad_via_buffer_specific_t;

typedef struct s_mad_via_driver_specific
{
  mad_via_vi_id_base_t next_vi_id_base;
} mad_via_driver_specific_t, *p_mad_via_driver_specific_t;

typedef struct s_mad_via_adapter_specific
{
  VIP_NIC_HANDLE         nic_handle;
  VIP_NIC_ATTRIBUTES    *nic_attributes;
  VIP_PROTECTION_HANDLE  ptag;
  VIP_MEM_ATTRIBUTES    *memory_attributes;
} mad_via_adapter_specific_t, *p_mad_via_adapter_specific_t;

typedef struct s_mad_via_cq
{
  VIP_CQ_HANDLE handle;
  VIP_ULONG     entry_count;
} mad_via_cq_t, *p_mad_via_cq_t;

typedef struct s_mad_via_descriptor_set
{
  TBX_SHARED;
  VIP_MEM_HANDLE  mem_handle;
  void           *mem;
  void           *limit;
  int             nb_desc;
  int             nb_addr_seg;
  int             nb_data_seg;
  p_tbx_slist_t   next;
} mad_via_descriptor_set_t, *p_mad_via_descriptor_set_t;

typedef struct s_mad_via_buffer_area
{
  void           *mem;
  VIP_MEM_HANDLE  mem_handle;
} mad_via_buffer_area_t, *p_mad_via_buffer_area_t;

typedef struct s_mad_via_buffer_set
{
  p_tbx_slist_t   area_slist;
  p_tbx_slist_t   buffer_slist;
  int             nb_buffer;
  int             initial_nb_buffer;
  size_t          buffer_size;
  p_mad_adapter_t adapter;
} mad_via_buffer_set_t, *p_mad_via_buffer_set_t;

typedef struct s_mad_via_channel_specific
{
  p_mad_via_cq_t input_cq;
} mad_via_channel_specific_t, *p_mad_via_channel_specific_t;

typedef struct s_mad_via_vi
{
  VIP_VI_HANDLE             handle;
  VIP_VI_ATTRIBUTES        *attributes;
  VIP_DESCRIPTOR           *input_desc;
  VIP_MEM_HANDLE            input_mem_handle;
  VIP_DESCRIPTOR           *output_desc;
  VIP_MEM_HANDLE            output_mem_handle;
  mad_via_vi_id_idx_t       id;
} mad_via_vi_t, *p_mad_via_vi_t;

typedef struct s_mad_via_in_connection_specific
{
  mad_via_vi_id_base_t       vi_id_base;
  p_mad_via_vi_t             vi;
  p_mad_via_descriptor_set_t descriptor_set;
  int                        nb_consumed;
} mad_via_in_connection_specific_t, *p_mad_via_in_connection_specific_t;

typedef struct s_mad_via_out_connection_specific
{
  mad_via_vi_id_base_t vi_id_base;
  p_mad_via_vi_t       vi;
  tbx_bool_t           need_send_wait;
  int                  nb_consumed;
} mad_via_out_connection_specific_t, *p_mad_via_out_connection_specific_t;

typedef struct s_mad_via_link_specific
{
  p_mad_via_vi_t             vi;
  p_mad_via_descriptor_set_t descriptor_set;
  p_mad_via_buffer_set_t     buffer_set;
} mad_via_link_specific_t, *p_mad_via_link_specific_t;

/*
 * Static variables
 * ----------------------------------------------------------------------------
 */

TBX_CRITICAL_SECTION(vi_id_base_computation);

/*
 * Static functions
 * ----------------------------------------------------------------------------
 */

static
void
mad_via_disp_discriminator(char                    *msg,
			   mad_via_discriminator_t  d)
{
  LOG_IN();
  DISP_STR("Discriminator", msg);
  DISP_VAL("process", d.process_grank);
  DISP_VAL("id base", d.vi_id_base);
  DISP_VAL("id idx",  d.vi_id_idx);
  LOG_OUT();
}


static
VIP_UINT32
mad_via_descriptor_status(VIP_UINT32 status)
{
  VIP_UINT32 s = 0;

  LOG_IN();
  s = status & ~VIP_STATUS_RESERVED;

  if (s & VIP_STATUS_DONE)
    {
      DISP("Done");
    }

  if (s & VIP_STATUS_FORMAT_ERROR)
    {
      DISP("Format Error");
    }

  if (s & VIP_STATUS_PROTECTION_ERROR)
    {
      DISP("Protection Error");
    }

  if (s & VIP_STATUS_LENGTH_ERROR)
    {
      DISP("Length Error");
    }

  if (s & VIP_STATUS_PARTIAL_ERROR)
    {
      DISP("Partial Error");
    }

  if (s & VIP_STATUS_DESC_FLUSHED_ERROR)
    {
      DISP("Flushed Error");
    }

  if (s & VIP_STATUS_TRANSPORT_ERROR)
    {
      DISP("Transport Error");
    }

  if (s & VIP_STATUS_RDMA_PROT_ERROR)
    {
      DISP("RDMA Prot Error");
    }

  if (s & VIP_STATUS_REMOTE_DESC_ERROR)
    {
      DISP("Remote Desc Error");
    }

  if (s & VIP_STATUS_ERROR_MASK)
    {
      DISP("Error Mask");
    }

  if (s & VIP_STATUS_OP_SEND)
    {
      DISP("Op Send");
    }

  if (s & VIP_STATUS_OP_RECEIVE)
    {
      DISP("Op Receive");
    }

  if (s & VIP_STATUS_OP_RDMA_WRITE)
    {
      DISP("Op RDMA write");
    }

  if (s & VIP_STATUS_OP_REMOTE_RDMA_WRITE)
    {
      DISP("Op Remote RDMA write");
    }

  if (s & VIP_STATUS_OP_RDMA_READ)
    {
      DISP("Op RDMA Read");
    }

  if (s & VIP_STATUS_OP_MASK)
    {
      DISP("Op Mask");
    }

  if (s & VIP_STATUS_IMMEDIATE)
    {
      DISP("Immediate");
    }

  LOG_OUT();

  return status;
}


static
const char *
mad_via_status_string(VIP_RETURN status)
{
  const char *s = NULL;

  LOG_IN();
  switch (status)
    {
    default:
      s = "VIA: unknown status";
      break;
    case VIP_SUCCESS:
      s = "VIA: success";
      break;
    case VIP_NOT_DONE:
      s = "VIA: not done";
      break;
    case VIP_INVALID_PARAMETER:
      s = "VIA: invalid parameter";
      break;
    case VIP_ERROR_RESOURCE:
      s = "VIA: error - resource";
      break;
    case VIP_TIMEOUT:
      s = "VIA: timeout";
      break;
    case VIP_REJECT:
      s = "VIA: reject";
      break;
    case VIP_INVALID_RELIABILITY_LEVEL:
      s = "VIA: invalid reliability level";
      break;
    case VIP_INVALID_MTU:
      s = "VIA: invalid mtu";
      break;
    case VIP_INVALID_QOS:
      s = "VIA: invalid qos";
      break;
    case VIP_INVALID_PTAG:
      s = "VIA: invalid ptag";
      break;
    case VIP_INVALID_RDMAREAD:
      s = "VIA: invalid rdma read";
      break;
    case VIP_DESCRIPTOR_ERROR:
      s = "VIA: descriptor error";
      break;
    case VIP_INVALID_STATE:
      s = "VIA: invalid state";
      break;
    case VIP_ERROR_NAMESERVICE:
      s = "VIA: error - name service";
      break;
    case VIP_NO_MATCH:
      s = "VIA: no match";
      break;
    case VIP_NOT_REACHABLE:
      s = "VIA: not reachable";
      break;
    }
  LOG_OUT();

  return s;
}

static
void TBX_NORETURN
mad_via_error_callback(VIP_PVOID             context,
		       VIP_ERROR_DESCRIPTOR *error_descriptor)
{
  DISP("VIA asynchronous error report");

  DISP_PTR("Context pointer", context);

  DISP("Resource code");
  switch (error_descriptor->ResourceCode)
    {
    default:
      DISP("Inconnu");
      break;
    case VIP_RESOURCE_NIC:
      DISP("VIP_RESOURCE_NIC");
      break;
    case VIP_RESOURCE_VI:
      DISP("VIP_RESOURCE_VI");
      break;
    case VIP_RESOURCE_CQ:
      DISP("VIP_RESOURCE_CQ");
      break;
    case VIP_RESOURCE_DESCRIPTOR:
      DISP("VIP_RESOURCE_DESCRIPTOR");
      break;
    }

  DISP("Error code");
  switch (error_descriptor->ErrorCode)
    {
    default:
      DISP("Inconnu");
      break;
    case VIP_ERROR_POST_DESC:
      DISP("VIP_ERROR_POST_DESC");
      break;
    case VIP_ERROR_CONN_LOST:
      DISP("VIP_ERROR_CONN_LOST");
      break;
    case VIP_ERROR_RECVQ_EMPTY:
      DISP("VIP_ERROR_RECVQ_EMPTY");
      break;
    case VIP_ERROR_VI_OVERRUN:
      DISP("VIP_ERROR_VI_OVERRUN");
      break;
    case VIP_ERROR_RDMAW_PROT:
      DISP("VIP_ERROR_RDMAW_PROT");
      break;
    case VIP_ERROR_RDMAW_DATA:
      DISP("VIP_ERROR_RDMAW_DATA");
      break;
    case VIP_ERROR_RDMAW_ABORT:
      DISP("VIP_ERROR_RDMAW_ABORT");
      break;
    case VIP_ERROR_RDMAR_PROT:
      DISP("VIP_ERROR_RDMAR_PROT");
      break;
    case VIP_ERROR_COMP_PROT:
      DISP("VIP_ERROR_COMP_PROT");
      break;
    case VIP_ERROR_RDMA_TRANSPORT:
      DISP("VIP_ERROR_RDMA_TRANSPORT");
      break;
    case VIP_ERROR_CATASTROPHIC:
      DISP("VIP_ERROR_CATASTROPHIC");
      break;
    }

  FAILURE("VIA: asynchronous error");
}

static
VIP_RETURN
_(VIP_RETURN status)
{
  if (status != VIP_SUCCESS)
    {
      const char *s = NULL;

      s = mad_via_status_string(status);
      FAILURE(s);
    }

  return status;
}

// Async version
static
VIP_RETURN
__(VIP_RETURN status)
{
  if ((status != VIP_SUCCESS) && (status != VIP_TIMEOUT))
    {
      const char *s = NULL;

      s = mad_via_status_string(status);
      FAILURE(s);
    }

  return status;
}

static void
mad_via_disp_nic_attributes(VIP_NIC_ATTRIBUTES *na)
{
  DISP("Name: %s",               na->Name);
  DISP("Hardware version: %lu",  na->HardwareVersion);
  DISP("Provider version: %lu",  na->ProviderVersion);
  DISP("Nic address length: %u", na->NicAddressLen);
  DISP("Nic address: %x",        (int) na->LocalNicAddress);
  if (na->ThreadSafe)
    {
      DISP("Thread safe: yes");
    }
  else
    {
      DISP("Thread safe: no");
    }
  DISP("Max discriminator length: %d",     (int) na->MaxDiscriminatorLen);
  DISP("Max register byte: %lu",           na->MaxRegisterBytes);
  DISP("Max register regions: %lu",        na->MaxRegisterRegions);
  DISP("Max register block byte: %lu",     na->MaxRegisterBlockBytes);
  DISP("Max VI: %lu",                      na->MaxVI);
  DISP("Max descriptors per queue: %lu",   na->MaxDescriptorsPerQueue);
  DISP("Max segments per descriptor: %lu", na->MaxSegmentsPerDesc);
  DISP("Max CQ: %lu",                      na->MaxCQ);
  DISP("Max CQ entries: %lu",              na->MaxCQEntries);
  DISP("Max transfer size: %lu",           na->MaxTransferSize);
  DISP("Native MTU: %lu",                  na->NativeMTU);
  DISP("Max Ptags: %lu",                   na->MaxPtags);

  if (na->ReliabilityLevelSupport & (VIP_SERVICE_UNRELIABLE|VIP_SERVICE_RELIABLE_DELIVERY|VIP_SERVICE_RELIABLE_RECEPTION))
    {
      if (na->ReliabilityLevelSupport & VIP_SERVICE_UNRELIABLE)
	{
	  DISP("Reliability level support : unreliable");
	}

      if (na->ReliabilityLevelSupport & VIP_SERVICE_RELIABLE_DELIVERY)
	{
	  DISP("Reliability level support : reliable delivery");
	}
      
      if (na->ReliabilityLevelSupport & VIP_SERVICE_RELIABLE_RECEPTION)
	{
	  DISP("Reliability level support : reliable reception");
	}
    }
  else
    {
      DISP("Reliability level support : unknown");
    }

  if (na->RDMAReadSupport)
    {
      DISP("RDMA read support: yes");
    }
  else
    {
      DISP("RDMA read support: no");
    }
}

static
VIP_MEM_HANDLE
mad_via_register_block(p_mad_adapter_t  adapter,
		       void            *ptr,
		       size_t           length)
{
  p_mad_via_adapter_specific_t as         = NULL;
  VIP_MEM_HANDLE               mem_handle = 0;

  LOG_IN();
  as = adapter->specific;
  _(VipRegisterMem(as->nic_handle, ptr, length, as->memory_attributes,
		   &mem_handle));
  LOG_OUT();

  return mem_handle;
}

static
void
mad_via_unregister_block(p_mad_adapter_t  adapter,
			 void            *ptr,
			 VIP_MEM_HANDLE   mem_handle)
{
  p_mad_via_adapter_specific_t as = NULL;

  LOG_IN();
  as = adapter->specific;
  _(VipDeregisterMem(as->nic_handle, ptr, mem_handle));
  LOG_OUT();
}

static
p_mad_via_cq_t
mad_via_cq_init(p_mad_adapter_t adapter,
		  VIP_ULONG       entry_count)
{
  p_mad_via_adapter_specific_t as     = NULL;
  p_mad_via_cq_t               cq     = NULL;
  VIP_CQ_HANDLE                handle =    0;

  LOG_IN();
  as = adapter->specific;

  cq = TBX_CALLOC(1, sizeof(mad_via_cq_t));

  _(VipCreateCQ(as->nic_handle, entry_count, &handle));

  cq->handle      = handle;
  cq->entry_count = entry_count;
  LOG_OUT();

  return cq;
}

static
p_mad_via_vi_t
mad_via_vi_init(p_mad_adapter_t        adapter,
		mad_via_vi_rdma_mode_t rdma_mode,
		p_mad_via_cq_t         read_cq,
		p_mad_via_cq_t         write_cq,
		mad_via_vi_id_idx_t    id)
{
  p_mad_via_adapter_specific_t  as       = NULL;
  p_mad_via_vi_t                vi       = NULL;
  VIP_VI_ATTRIBUTES            *attr     = NULL;
  VIP_CQ_HANDLE                 handle_r = NULL;
  VIP_CQ_HANDLE                 handle_w = NULL;
  VIP_VI_HANDLE                 handle   =    0;

  LOG_IN();
  as = adapter->specific;

  attr = TBX_CALLOC(1, sizeof(VIP_VI_ATTRIBUTES));

  attr->ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  attr->MaxTransferSize  = adapter->mtu;
  attr->QoS              = 0;
  attr->Ptag             = as->ptag;
  attr->EnableRdmaWrite  = rdma_mode & rdma_write;
  attr->EnableRdmaRead   = rdma_mode & rdma_read;

  handle_r = read_cq  ? read_cq->handle  : NULL;
  handle_w = write_cq ? write_cq->handle : NULL;

  _(VipCreateVi(as->nic_handle, attr, handle_r, handle_w, &handle));

  vi = TBX_CALLOC(1, sizeof(mad_via_vi_t));
  vi->handle     = handle;
  vi->id         = id;
  vi->attributes = attr;

  /*
   * Allocation of the main communication descriptors
   * - each descriptor has room for a unique data segment
   *   and no address segment
   */
  {
    const size_t size = MAD_VIA_DESCRIPTOR_SIZE(0, 1);

    vi->input_desc  = tbx_aligned_malloc(size, VIP_DESCRIPTOR_ALIGNMENT);
    vi->output_desc = tbx_aligned_malloc(size, VIP_DESCRIPTOR_ALIGNMENT);

    vi->input_mem_handle  =
      mad_via_register_block(adapter, vi->input_desc, size);
    vi->output_mem_handle =
      mad_via_register_block(adapter, vi->output_desc, size);
  }
  LOG_OUT();

  return vi;
}

static
p_mad_via_descriptor_set_t
mad_via_descriptor_set_init(p_mad_adapter_t adapter,
			    int             nb_desc,
			    int             nb_addr_seg,
			    int             nb_data_seg)
{
  p_mad_via_descriptor_set_t  descriptor_set  = NULL;
  size_t                      descriptor_size =    0;
  void                       *mem             = NULL;
  p_tbx_slist_t               next            = NULL;

  LOG_IN();
  descriptor_set = TBX_CALLOC(1, sizeof(mad_via_descriptor_set_t));
  TBX_INIT_SHARED(descriptor_set);

  next = tbx_slist_nil();
  descriptor_set->next = next;
  descriptor_set->nb_desc = nb_desc;
  descriptor_set->nb_addr_seg = nb_addr_seg;
  descriptor_set->nb_data_seg = nb_data_seg;

  descriptor_size = MAD_VIA_DESCRIPTOR_SIZE(nb_addr_seg, nb_data_seg);
  mem = tbx_aligned_malloc(nb_desc * descriptor_size, VIP_DESCRIPTOR_ALIGNMENT);
  descriptor_set->mem   = mem;
  descriptor_set->limit = mem + nb_desc * descriptor_size;

  descriptor_set->mem_handle =
    mad_via_register_block(adapter, mem, nb_desc * descriptor_size);

  while (nb_desc--)
    {
      tbx_slist_enqueue(next, mem);
      mem += descriptor_size;
    }
  LOG_OUT();

  return descriptor_set;
}

static
void
mad_via_descriptor_set_exit(p_mad_adapter_t            adapter,
			    p_mad_via_descriptor_set_t set)
{
  void *mem = NULL;

  LOG_IN();
  mem = set->mem;

  mad_via_unregister_block(adapter, mem, set->mem_handle);
  tbx_aligned_free(mem, VIP_DESCRIPTOR_ALIGNMENT);
  tbx_slist_free(set->next);
  memset(set, 0, sizeof(mad_via_descriptor_set_t));

  TBX_FREE(set);
  LOG_OUT();
}

static
tbx_bool_t
mad_via_descriptor_set_desc_avail(p_mad_via_descriptor_set_t set)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  result = !tbx_slist_is_nil(set->next);
  LOG_OUT();

  return result;
}

static
VIP_DESCRIPTOR *
mad_via_descriptor_set_get_desc(p_mad_via_descriptor_set_t set)
{
  VIP_DESCRIPTOR *desc = NULL;

  LOG_IN();
  if (!mad_via_descriptor_set_desc_avail(set))
    FAILURE("descriptor set underrun");

  desc = tbx_slist_dequeue(set->next);
  LOG_OUT();

  return desc;
}

static
VIP_MEM_HANDLE
mad_via_descriptor_set_get_handle(p_mad_via_descriptor_set_t set)

{
  VIP_MEM_HANDLE mem_handle = 0;

  LOG_IN();
  mem_handle = set->mem_handle;
  LOG_OUT();

  return mem_handle;
}

static
void
mad_via_descriptor_set_return_desc(p_mad_via_descriptor_set_t  set,
				   VIP_DESCRIPTOR             *desc)
{
  LOG_IN();
  if (((void *)desc < set->mem) || ((void *)desc >= set->limit)
      || ((int)desc & (VIP_DESCRIPTOR_ALIGNMENT - 1)))
    FAILURE("unexpected descriptor");

  tbx_slist_enqueue(set->next, desc);
  LOG_OUT();
}

static
void
mad_via_buffer_set_expand(p_mad_via_buffer_set_t buffer_set)
{
  p_mad_adapter_t               adapter    = NULL;
  p_mad_via_adapter_specific_t  as         = NULL;
  p_mad_via_buffer_area_t       area       = NULL;
  void                         *mem        = NULL;
  VIP_MEM_HANDLE                mem_handle =    0;
  size_t                        size       =    0;

  LOG_IN();
  adapter = buffer_set->adapter;
  as      = adapter->specific;
  area    = TBX_CALLOC(1, sizeof(mad_via_buffer_area_t));
  size    = buffer_set->initial_nb_buffer * buffer_set->buffer_size;

  mem = tbx_aligned_malloc(size, VIP_DATA_ALIGNMENT);
  area->mem = mem;

  _(VipRegisterMem(as->nic_handle, mem, size, as->memory_attributes,
		   &mem_handle));
  area->mem_handle = mem_handle;
  tbx_slist_enqueue(buffer_set->area_slist, area);

  {
    int i = 0;

    for (i = 0; i < buffer_set->initial_nb_buffer; i++)
      {
	p_mad_buffer_t              buffer = NULL;
	p_mad_via_buffer_specific_t bs = NULL;

	bs = TBX_CALLOC(1, sizeof(mad_via_buffer_specific_t));
	bs->mem_handle = mem_handle;

	buffer                = mad_alloc_buffer_struct();
	buffer->buffer        = mem;
	buffer->length        = buffer_set->buffer_size;
	buffer->bytes_written = 0;
	buffer->bytes_read    = 0;
	buffer->type          = mad_static_buffer;
	buffer->specific      = bs;

	tbx_slist_enqueue(buffer_set->buffer_slist, buffer);

	mem += buffer_set->buffer_size;
      }
  }

  buffer_set->nb_buffer += buffer_set->initial_nb_buffer;
  LOG_OUT();
}

static
p_mad_via_buffer_set_t
mad_via_buffer_set_init(p_mad_adapter_t adapter,
			int             initial_num,
			size_t          buffer_size)
{
  p_mad_via_buffer_set_t buffer_set  = NULL;

  LOG_IN();
  buffer_size = tbx_aligned(buffer_size, VIP_DATA_ALIGNMENT);

  buffer_set = TBX_CALLOC(1, sizeof(mad_via_buffer_set_t));

  buffer_set->buffer_size       = buffer_size;
  buffer_set->nb_buffer         = 0;
  buffer_set->initial_nb_buffer = initial_num;
  buffer_set->area_slist        = tbx_slist_nil();
  buffer_set->buffer_slist      = tbx_slist_nil();
  buffer_set->adapter           = adapter;

  mad_via_buffer_set_expand(buffer_set);
  LOG_OUT();

  return buffer_set;
}

static
p_mad_buffer_t
mad_via_buffer_set_get_buffer(p_mad_via_buffer_set_t buffer_set)
{
  p_mad_buffer_t buffer = NULL;

  LOG_IN();
  if (tbx_slist_is_nil(buffer_set->buffer_slist))
    {
      mad_via_buffer_set_expand(buffer_set);
    }

  buffer = tbx_slist_dequeue(buffer_set->buffer_slist);
  LOG_OUT();

  return buffer;
}

static
void
mad_via_buffer_set_return_buffer(p_mad_via_buffer_set_t buffer_set,
				 p_mad_buffer_t         buffer)
{
  LOG_IN();
  buffer->bytes_read    = 0;
  buffer->bytes_written = 0;
  tbx_slist_enqueue(buffer_set->buffer_slist, buffer);
  LOG_OUT();
}

static
void
mad_via_register_buffer(p_mad_adapter_t adapter,
			p_mad_buffer_t  buffer)
{
  LOG_IN();
  buffer->specific =
    (void *)mad_via_register_block(adapter, buffer->buffer, buffer->length);
  LOG_OUT();
}

static
void
mad_via_unregister_buffer(p_mad_adapter_t adapter,
			p_mad_buffer_t  buffer)
{
  LOG_IN();
  mad_via_unregister_block(adapter, buffer->buffer,
			   (VIP_MEM_HANDLE)buffer->specific);
  buffer->specific = NULL;
  LOG_OUT();
}


/*
 * Exported functions
 * ----------------------------------------------------------------------------
 */
void
mad_via_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  TRACE("Registering VIA driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("via");

  interface->driver_init                = mad_via_driver_init;
  interface->adapter_init               = mad_via_adapter_init;
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
  interface->link_exit                  = mad_via_link_exit;
  interface->connection_exit            = mad_via_connection_exit;
  interface->channel_exit               = mad_via_channel_exit;
  interface->adapter_exit               = mad_via_adapter_exit;
  interface->driver_exit                = mad_via_driver_exit;
  interface->choice                     = mad_via_choice;
  interface->get_static_buffer          = mad_via_get_static_buffer;
  interface->return_static_buffer       = mad_via_return_static_buffer;
  interface->new_message                = mad_via_new_message;
  interface->finalize_message           = mad_via_finalize_message;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_via_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = mad_via_receive_message;
  interface->message_received           = mad_via_message_received;
  interface->send_buffer                = mad_via_send_buffer;
  interface->receive_buffer             = mad_via_receive_buffer;
  interface->send_buffer_group          = mad_via_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_via_receive_sub_buffer_group;
  LOG_OUT();
}

void
mad_via_driver_init(p_mad_driver_t driver, int *argc, char ***argv)
{
  p_mad_via_driver_specific_t driver_specific = NULL;

  LOG_IN();
  TRACE("Initializing VIA driver");
  driver_specific                  = TBX_CALLOC(1, sizeof(mad_via_driver_specific_t));
  driver_specific->next_vi_id_base = 0;

  driver->specific = driver_specific;
  LOG_OUT();
}

void
mad_via_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t  as          = NULL;
  p_mad_dir_adapter_t           dir_adapter = NULL;
  VIP_NIC_HANDLE                nic         =    0;
  VIP_NIC_ATTRIBUTES           *nic_attr    = NULL;
  VIP_PROTECTION_HANDLE         ptag        =    0;
  VIP_MEM_ATTRIBUTES           *mem_attr    = NULL;

  LOG_IN();
  dir_adapter = adapter->dir_adapter;

  if (strcmp(dir_adapter->name, "default") == 0)
    {
      _(VipOpenNic("/dev/via_lo", &nic));
    }
  else
    FAILURE("unsupported VIA nic");

  _(VipErrorCallback(nic, NULL, mad_via_error_callback));
  _(VipNSInit(nic, NULL));

  nic_attr = TBX_CALLOC(1, sizeof(VIP_NIC_ATTRIBUTES));
  _(VipQueryNic(nic, nic_attr));
  mad_via_disp_nic_attributes(nic_attr);

  _(VipCreatePtag(nic, &ptag));
  mem_attr = TBX_CALLOC(1, sizeof(VIP_MEM_ATTRIBUTES));
  mem_attr->Ptag            = ptag;
  mem_attr->EnableRdmaWrite = VIP_TRUE;
  mem_attr->EnableRdmaRead  = VIP_FALSE;

  as = TBX_CALLOC(1, sizeof(mad_via_adapter_specific_t));
  as->nic_handle        = nic;
  as->nic_attributes    = nic_attr;
  as->ptag              = ptag;
  as->memory_attributes = mem_attr;

  adapter->parameter = tbx_strdup("-");
  adapter->specific  = as;
  adapter->mtu       = min(nic_attr->NativeMTU, MAD_FORWARD_MAX_MTU);
  LOG_OUT();
}

void
mad_via_channel_init(p_mad_channel_t channel)
{
  p_mad_adapter_t              adapter          = NULL;
  p_mad_via_channel_specific_t chs              = NULL;
  p_tbx_string_t               parameter_string = NULL;
  p_mad_via_cq_t               cq               =    0;

  LOG_IN();
  adapter = channel->adapter;

  cq = mad_via_cq_init(adapter, MAD_VIA_INITIAL_CHANNEL_CQ_SIZE);

  chs = TBX_CALLOC(1, sizeof(mad_via_channel_specific_t));

  chs->input_cq      = cq;
  channel->specific  = chs;
  parameter_string   = tbx_string_init_to_int(channel->id);
  channel->parameter = tbx_string_to_cstring(parameter_string);

  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_via_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_channel_t              channel = NULL;
  p_mad_via_channel_specific_t chs     = NULL;
  p_mad_adapter_t              adapter = NULL;
  p_mad_via_adapter_specific_t as      = NULL;
  p_mad_driver_t               driver  = NULL;
  p_mad_via_driver_specific_t  ds      = NULL;
  mad_via_vi_id_base_t         id_base =   -1;

  LOG_IN();
  channel = (in?:out)->channel;
  chs     = channel->specific;
  adapter = channel->adapter;
  as      = adapter->specific;
  driver  = adapter->driver;
  ds      = driver->specific;

  TBX_CRITICAL_SECTION_ENTER(vi_id_base_computation);
  id_base = ds->next_vi_id_base;
  TBX_CRITICAL_SECTION_LEAVE(vi_id_base_computation);

  if (in)
    {
      p_mad_via_cq_t                     cq =    0;
      p_mad_via_in_connection_specific_t is = NULL;
      p_mad_via_vi_t                     vi = NULL;
      p_tbx_string_t                     s  = NULL;

      cq = chs->input_cq;
      vi = mad_via_vi_init(adapter, rdma_off, NULL, cq, vi_id_idx_in_main);

      is = TBX_CALLOC(1, sizeof(mad_via_in_connection_specific_t));

      is->vi         = vi;
      in->specific   = is;
      in->nb_link    = nb_link;
      is->vi_id_base = id_base;
      s              = tbx_string_init_to_int(id_base);
      in->parameter  = tbx_string_to_cstring(s);

      tbx_string_free(s);
      s              = NULL;

      is->descriptor_set =
	mad_via_descriptor_set_init(in->channel->adapter,
				    MAD_VIA_RECV_MAIN_DESC_NUM,
				    MAD_VIA_RECV_MAIN_DESC_ADDR_NUM,
				    MAD_VIA_RECV_MAIN_DESC_DATA_NUM);
      is->nb_consumed    = 0;
    }

  if (out)
    {
      p_mad_via_out_connection_specific_t os = NULL;
      p_mad_via_vi_t                      vi = NULL;
      p_tbx_string_t                      s  = NULL;

      vi = mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_idx_out_main);

      os = TBX_CALLOC(1, sizeof(mad_via_out_connection_specific_t));

      os->vi          = vi;
      out->specific   = os;
      out->nb_link    =  nb_link;
      os->vi_id_base  = id_base;
      s               = tbx_string_init_to_int(id_base);
      out->parameter  = tbx_string_to_cstring(s);

      tbx_string_free(s);
      s               = NULL;
      os->nb_consumed = 0;
    }
  LOG_OUT();
}

void
mad_via_link_init(p_mad_link_t lnk)
{
  p_mad_connection_t        connection    = NULL;
  p_mad_channel_t           channel       = NULL;
  p_mad_adapter_t           adapter       = NULL;
  p_mad_via_link_specific_t link_specific = NULL;

  LOG_IN();
  connection = lnk->connection;
  channel    = connection->channel;
  adapter    = channel->adapter;

  link_specific = TBX_CALLOC(1, sizeof(mad_via_link_specific_t));

  if (lnk->id == c_link_id)
    {edit
      if (connection->way == mad_incoming_connection)
	{
	  link_specific->vi             =
	    mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_idx_in_credit);
	  link_specific->buffer_set = 
	    mad_via_buffer_set_init(connection->channel->adapter,
				    MAD_VIA_CREDIT_NB_BUFFER,
				    MAD_VIA_CREDIT_BUFFER_SIZE)
	  link_specific->descriptor_set = 
	    mad_via_descriptor_set_init(connection->channel->adapter,
					MAD_VIA_SEND_CREDIT_DESC_NUM,
					MAD_VIA_SEND_CREDIT_DESC_ADDR_NUM,
					MAD_VIA_SEND_CREDIT_DESC_DATA_NUM);
	}
      else if (connection->way == mad_outgoing_connection)
	{
	  link_specific->vi =
	    mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_idx_out_credit);
	  link_specific->buffer_set = 
	    mad_via_buffer_set_init(connection->channel->adapter,
				    MAD_VIA_CREDIT_NB_BUFFER,
				    MAD_VIA_CREDIT_BUFFER_SIZE)
	  link_specific->descriptor_set = 
	    mad_via_descriptor_set_init(connection->channel->adapter,
					MAD_VIA_RECV_CREDIT_DESC_NUM,
					MAD_VIA_RECV_CREDIT_DESC_ADDR_NUM,
					MAD_VIA_RECV_CREDIT_DESC_DATA_NUM);
	}
      else
	FAILURE("unspecified connection way");
    }
  else if (lnk->id == msg_link_id)
    {
      if (connection->way == mad_incoming_connection)
	{
	  link_specific->vi =
	    mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_idx_in_msg);
	}
      else if (connection->way == mad_outgoing_connection)
	{
	  link_specific->vi =
	    mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_idx_out_msg);
	}
      else
	FAILURE("unspecified connection way");
    }
  else if (lnk->id == rdma_link_id)
    {
      if (connection->way == mad_incoming_connection)
	{
#ifdef PM2_DEV
#warning TODO: verify 'rdma' flag
#endif
	  link_specific->vi =
	    mad_via_vi_init(adapter, rdma_write, NULL, NULL, vi_id_idx_in_rdma);
	}
      else if (connection->way == mad_outgoing_connection)
	{
	  link_specific->vi             =
	    mad_via_vi_init(adapter, rdma_write, NULL, NULL, vi_id_idx_out_rdma);
	  link_specific->descriptor_set = 
	    mad_via_descriptor_set_init(connection->channel->adapter,
					MAD_VIA_SEND_RDMA_DESC_NUM,
					MAD_VIA_SEND_RDMA_DESC_ADDR_NUM,
					MAD_VIA_SEND_RDMA_DESC_DATA_NUM);
	}
      else
	FAILURE("unspecified connection way");      
    }
  else
    FAILURE("invalid link id");
  
  lnk->specific    = link_specific;
  lnk->link_mode   = mad_link_mode_buffer_group;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_via_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  // Nothing
  LOG_OUT();
}

/* Point-to-point connection */
void
mad_via_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t ai)
{
  p_mad_channel_t                     c              = NULL;
  p_mad_via_channel_specific_t        cs             = NULL;
  p_mad_adapter_t                     a              = NULL;
  p_mad_driver_t                      drv            = NULL;
  p_ntbx_process_container_t          pc             = NULL;
  p_mad_madeleine_t                   m              = NULL;
  p_mad_session_t                     s              = NULL;
  p_mad_via_adapter_specific_t        as             = NULL;
  p_mad_via_in_connection_specific_t  is             = NULL;
  p_mad_dir_node_t                    rn             = NULL;
  p_mad_dir_adapter_t                 ra             = NULL;
  p_mad_via_vi_t                      vi             = NULL;
  mad_via_vi_id_base_t                local_id_base  =   -1;
  ntbx_process_grank_t                local_g_rank   =   -1;
  mad_via_vi_id_base_t                remote_id_base =   -1;
  ntbx_process_grank_t                remote_g_rank  =   -1;

  LOG_IN();
  c   = in->channel;
  cs  = c->specific;
  pc  = c->pc;
  a   = c->adapter;
  drv = a->driver;
  m   = drv->madeleine;
  s   = m->session;
  as  = a->specific;
  is  = in->specific;
  rn  = ai->dir_node;
  ra  = ai->dir_adapter;

  local_id_base  = is->vi_id_base;
  local_g_rank   = s->process_rank;
  remote_id_base = atoi(ai->connection_parameter);
  remote_g_rank  = ntbx_pc_local_to_global(pc, in->remote_rank);

  {
    VIP_NIC_ATTRIBUTES      *na       = as->nic_attributes;
    VIP_NET_ADDRESS         *local    = NULL;
    VIP_NET_ADDRESS         *remote   = NULL;
    VIP_VI_ATTRIBUTES        rattr    =  {0};
    mad_via_discriminator_t  d        =  {0};
    const size_t             disc_len = sizeof(mad_via_discriminator_t);
    const size_t             addr_len =
      sizeof(VIP_NET_ADDRESS) + na->NicAddressLen + disc_len;

    void accept(void)
      {
	VIP_CONN_HANDLE cnx_h = NULL;

	local                   = TBX_MALLOC(addr_len);
	local->HostAddressLen   = na->NicAddressLen;
	local->DiscriminatorLen = disc_len;
	memcpy(local->HostAddress, na->LocalNicAddress, na->NicAddressLen);

	remote                   = TBX_MALLOC(addr_len);
	memset(remote, 0, addr_len);
	remote->HostAddressLen   = na->NicAddressLen;
	remote->DiscriminatorLen = disc_len;

	d.vi_id_base    = local_id_base;
	d.process_grank = local_g_rank;
	d.vi_id_idx     = vi->id & ~1;
	MAD_VIA_DISCRIMINATOR(local) = d;
	//	mad_via_disp_discriminator("Accept local", d);

	d.vi_id_base    = remote_id_base;
	d.process_grank = remote_g_rank;
	d.vi_id_idx     = vi->id | 1;
	MAD_VIA_DISCRIMINATOR(remote) = d;
	//	mad_via_disp_discriminator("Accept remote", d);

	while (__(VipConnectWait(as->nic_handle, local, VIP_INFINITE,
				 remote, &rattr, &cnx_h))
	       == VIP_TIMEOUT)
	       LOG("*** Mad/VIA is waiting ***");

	_(VipConnectAccept(cnx_h, vi->handle));
      }

    // Main connection
    {
      vi = is->vi;

      {
	p_mad_via_descriptor_set_t  set        = NULL;
	VIP_MEM_HANDLE              mem_handle =    0;

	set        = is->descriptor_set;
	mem_handle = mad_via_descriptor_set_get_handle(set);

	if (mad_via_descriptor_set_desc_avail(set))
	  {
	    do
	      {
		VIP_DESCRIPTOR *desc = NULL;

		desc = mad_via_descriptor_set_get_desc(set);
		desc->CS.SegCount = 0;
		desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
		desc->CS.Length   = 0;
		desc->CS.Status   = 0;
		desc->CS.Reserved = 0;

		_(VipPostRecv(vi->handle, desc, mem_handle));
	      }
	    while (mad_via_descriptor_set_desc_avail(set));
	  }
	else
	  FAILURE("descriptor set is empty");
      }

      accept();
    }

    // Credit link
    {
      p_mad_link_t               l  = NULL;
      p_mad_via_link_specific_t  ls = NULL;

      l  = in->link_array[credit_link_id];
      ls = l->specific;
      vi = ls->vi;
#error descriptors should be posted here
      accept();
    }

    // MSG link
    {
      p_mad_link_t               l  = NULL;
      p_mad_via_link_specific_t  ls = NULL;

      l  = in->link_array[msg_link_id];
      ls = l->specific;
      vi = ls->vi;

      accept();
    }

    // RDMA link
    {
      p_mad_link_t               l  = NULL;
      p_mad_via_link_specific_t  ls = NULL;

      l  = in->link_array[rdma_link_id];
      ls = l->specific;
      vi = ls->vi;

      accept();
    }
  }

  LOG_OUT();
}

void
mad_via_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t ai TBX_UNUSED)
{
  p_mad_channel_t                     c              = NULL;
  p_mad_via_channel_specific_t        cs             = NULL;
  p_mad_adapter_t                     a              = NULL;
  p_mad_driver_t                      drv            = NULL;
  p_ntbx_process_container_t          pc             = NULL;
  p_mad_madeleine_t                   m              = NULL;
  p_mad_session_t                     s              = NULL;
  p_mad_via_adapter_specific_t        as             = NULL;
  p_mad_via_out_connection_specific_t os             = NULL;
  p_mad_dir_node_t                    rn             = NULL;
  p_mad_dir_adapter_t                 ra             = NULL;
  p_mad_via_vi_t                      vi             = NULL;
  mad_via_vi_id_base_t                local_id_base  =   -1;
  ntbx_process_grank_t                local_g_rank   =   -1;
  mad_via_vi_id_base_t                remote_id_base =   -1;
  ntbx_process_grank_t                remote_g_rank  =   -1;

  LOG_IN();
  c   = out->channel;
  cs  = c->specific;
  pc  = c->pc;
  a   = c->adapter;
  drv = a->driver;
  m   = drv->madeleine;
  s   = m->session;
  as  = a->specific;
  os  = out->specific;
  rn  = ai->dir_node;
  ra  = ai->dir_adapter;

  local_id_base  = os->vi_id_base;
  local_g_rank   = s->process_rank;
  remote_id_base = atoi(ai->connection_parameter);
  remote_g_rank  = ntbx_pc_local_to_global(pc, out->remote_rank);

  {
    VIP_NET_ADDRESS         *local    = NULL;
    VIP_NET_ADDRESS         *remote   = NULL;
    VIP_NIC_ATTRIBUTES      *na       = as->nic_attributes;
    VIP_VI_ATTRIBUTES        rattr    =  {0};
    mad_via_discriminator_t  d        =  {0};
    const size_t             disc_len = sizeof(mad_via_discriminator_t);
    const size_t             addr_len =
      sizeof(VIP_NET_ADDRESS) + na->NicAddressLen + disc_len;

    void connect(void)
      {
	local                   = TBX_MALLOC(addr_len);
	local->HostAddressLen   = na->NicAddressLen;
	local->DiscriminatorLen = disc_len;
	memcpy(local->HostAddress, na->LocalNicAddress, na->NicAddressLen);
	
	remote                   = TBX_MALLOC(addr_len);
	memset(remote, 0, addr_len);
	remote->HostAddressLen   = na->NicAddressLen;
	remote->DiscriminatorLen = disc_len;
	_(VipNSGetHostByName(as->nic_handle, rn->name, remote, 0));

	remote->DiscriminatorLen = disc_len;

	d.vi_id_base    = local_id_base;
	d.process_grank = local_g_rank;
	d.vi_id_idx     = vi->id | 1;
	MAD_VIA_DISCRIMINATOR(local) = d;
	//	mad_via_disp_discriminator("Connect local", d);

	d.vi_id_base    = remote_id_base;
	d.process_grank = remote_g_rank;
	d.vi_id_idx     = vi->id & ~1;
	MAD_VIA_DISCRIMINATOR(remote) = d;
	//	mad_via_disp_discriminator("Connect remote", d);

	while (1)
	  {
	    VIP_RETURN status = VIP_SUCCESS;

	    status =
	      VipConnectRequest(vi->handle, local, remote, VIP_INFINITE, &rattr);

	    switch (status)
	      {
	      default:
		_(status);
		FAILURE("nexpected path");

	      case VIP_SUCCESS:
		goto connected;

	      case VIP_NO_MATCH:
		break;
	      }
	  }
	
      connected:
      }

    // Main connection
    {
      VIP_DESCRIPTOR *desc       = NULL;
      VIP_MEM_HANDLE  mem_handle =    0;

      vi = os->vi;

      desc       = vi->input_desc;
      mem_handle = vi->input_mem_handle;

      desc->CS.SegCount = 0;
      desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
      desc->CS.Length   = 0;
      desc->CS.Status   = 0;
      desc->CS.Reserved = 0;

      _(VipPostRecv(vi->handle, desc, mem_handle));
      connect();
    }

    // Credit link
    {
      p_mad_link_t               l          = NULL;
      p_mad_via_link_specific_t  ls         = NULL;
      VIP_DESCRIPTOR            *desc       = NULL;
      VIP_MEM_HANDLE             mem_handle =    0;

      l  = out->link_array[credit_link_id];
      ls = l->specific;
      vi = ls->vi;

      connect();
    }

    // MSG link
    {
      p_mad_link_t               l          = NULL;
      p_mad_via_link_specific_t  ls         = NULL;
      VIP_DESCRIPTOR            *desc       = NULL;
      VIP_MEM_HANDLE             mem_handle =    0;

      l  = out->link_array[msg_link_id];
      ls = l->specific;
      vi = ls->vi;

      desc       = vi->input_desc;
      mem_handle = vi->input_mem_handle;

      desc->CS.SegCount = 0;
      desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
      desc->CS.Length   = 0;
      desc->CS.Status   = 0;
      desc->CS.Reserved = 0;

      _(VipPostRecv(vi->handle, desc, mem_handle));

      connect();
    }

    // RDMA link
    {
      p_mad_link_t               l          = NULL;
      p_mad_via_link_specific_t  ls         = NULL;
      VIP_DESCRIPTOR            *desc       = NULL;
      VIP_MEM_HANDLE             mem_handle =    0;

      l  = out->link_array[rdma_link_id];
      ls = l->specific;
      vi = ls->vi;

      desc       = vi->input_desc;
      mem_handle = vi->input_mem_handle;

      desc->CS.SegCount = 0;
      desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
      desc->CS.Length   = 0;
      desc->CS.Status   = 0;
      desc->CS.Reserved = 0;

      _(VipPostRecv(vi->handle, desc, mem_handle));

      connect();
    }
  }

  LOG_OUT();
}

/* Channel clean-up functions */
void
mad_via_after_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  // Nothing
  LOG_OUT();
}

void
mad_via_before_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  // Nothing
  LOG_OUT();
}

/* Connection clean-up function */
void
mad_via_disconnect(p_mad_connection_t connection)
{
  LOG_IN();
  if (connection->way == mad_incoming_connection)
    {
      p_mad_connection_t                 in = NULL;
      p_mad_via_in_connection_specific_t is = NULL;
      p_mad_via_vi_t                     vi = NULL;

      in = connection;
      is = in->specific;
      
      {
	p_mad_link_t              l  = NULL;
	p_mad_via_link_specific_t ls = NULL;
	
	l  = in->link_array[msg_link_id];
	ls = l->specific;
	vi = ls->vi;
	
	VipDisconnect(vi->handle);
      }

      vi = is->vi;
      VipDisconnect(vi->handle);
    }
  else if (connection->way == mad_outgoing_connection)
    {
      p_mad_connection_t                  out = NULL;
      p_mad_via_out_connection_specific_t os  = NULL;
      p_mad_via_vi_t                      vi = NULL;

      out = connection;
      os  = out->specific;
      
      {
	p_mad_link_t              l  = NULL;
	p_mad_via_link_specific_t ls = NULL;
	
	l  = out->link_array[msg_link_id];
	ls = l->specific;
	vi = ls->vi;
	
	VipDisconnect(vi->handle);
      }

      vi = os->vi;
      VipDisconnect(vi->handle);
    }
  else
    FAILURE("invalid connection direction");
 
  LOG_OUT();
}

void
mad_via_after_close_channel(p_mad_channel_t channel)
{
  LOG_IN();
  // Nothing
  LOG_OUT();
}

/* Deallocation functions */
void
mad_via_link_exit(p_mad_link_t lnk)
{
  p_mad_via_link_specific_t ls = NULL;
  p_mad_via_vi_t            vi = NULL;

  LOG_IN();
  ls = lnk->specific;
  vi = ls->vi;
	
  VipDestroyVi(vi->handle);

  TBX_FREE(ls);
  lnk->specific = NULL;
  LOG_OUT();
}

void
mad_via_connection_exit(p_mad_connection_t in,
			p_mad_connection_t out)
{
  LOG_IN();
  if (in)
    {
      p_mad_via_in_connection_specific_t is = NULL;
      p_mad_via_vi_t                     vi = NULL;

      is  = in->specific;
      vi = is->vi;
      VipDestroyVi(vi->handle);

      TBX_FREE(is);
      in->specific  = NULL;
    }
  
  if (out)
    {
      p_mad_via_out_connection_specific_t os = NULL;
      p_mad_via_vi_t                      vi = NULL;

      os = out->specific;
      vi = os->vi;
      VipDestroyVi(vi->handle);

      TBX_FREE(os);
      out->specific = NULL;
    } 
  LOG_OUT();
}

void
mad_via_channel_exit(p_mad_channel_t channel)
{
  p_mad_via_channel_specific_t channel_specific = NULL;

  LOG_IN();
  channel_specific = channel->specific;
  TBX_FREE(channel_specific);
  channel->specific = NULL;
  LOG_OUT();
}

void
mad_via_adapter_exit(p_mad_adapter_t adapter)
{
  p_mad_via_adapter_specific_t adapter_specific = NULL;

  LOG_IN();
  adapter_specific = adapter->specific;
  TBX_FREE(adapter_specific);
  adapter->specific = NULL;
  LOG_OUT();
}

void
mad_via_driver_exit(p_mad_driver_t driver)
{
  p_mad_via_driver_specific_t driver_specific = NULL;

  LOG_IN();
  driver_specific = driver->specific;
  TBX_FREE(driver_specific);
  driver->specific = NULL;
  LOG_OUT();
}

/* Dynamic paradigm selection */
p_mad_link_t
mad_via_choice(p_mad_connection_t connection,
	       size_t             length,
	       mad_send_mode_t    s_mode,
	       mad_receive_mode_t r_mode)
{
  p_mad_link_t lnk = NULL;

  LOG_IN();
  if (length <= MAD_VIA_MSG_THRES)
    {
      lnk = connection->link_array[credit_link_id];
    }
  else if (length <= MAD_VIA_RDMA_THRES)
    {
      lnk = connection->link_array[msg_link_id];
    }
  else
    {
      lnk = connection->link_array[rdma_link_id];
    }
  
  LOG_OUT();

  return lnk;
}

/* Static buffers management */
p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t lnk)
{
  p_mad_buffer_t buffer = NULL;
    
  LOG_IN();
  if (lnk->id == credit_link_id)
    {
      p_mad_via_link_specific_t ls = NULL;
      p_mad_via_buffer_set_t    bs = NULL;
      
      ls = lnk->specific;
      bs = ls->buffer_set;

      buffer = mad_via_buffer_set_get_buffer(bs);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();

  return NULL;
}

void
mad_via_return_static_buffer(p_mad_link_t   lnk,
			     p_mad_buffer_t buffer)
{
  LOG_IN();
  if (lnk->id == credit_link_id)
    {
      p_mad_via_link_specific_t ls = NULL;
      p_mad_via_buffer_set_t    bs = NULL;

      ls = lnk->specific;
      bs = ls->buffer_set;

      mad_via_buffer_set_return_buffer(bs, buffer);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();
}

/* Message transfer */
void
mad_via_new_message(p_mad_connection_t out)
{
  p_mad_via_out_connection_specific_t os = NULL;
  p_mad_via_vi_t                      vi = NULL;

  LOG_IN();
  os = out->specific;
  vi = os->vi;

  if (os->nb_consumed >= MAD_VIA_RECV_MAIN_DESC_NUM)
    {
      VIP_DESCRIPTOR *desc       = NULL;
      VIP_MEM_HANDLE  mem_handle =    0;

      _(VipRecvWait(vi->handle, VIP_INFINITE, &desc));
      if (desc != vi->input_desc)
	FAILURE("unexpected descriptor");

      os->nb_consumed = 0;

      desc->CS.SegCount = 0;
      desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
      desc->CS.Length   = 0;
      desc->CS.Status   = 0;
      desc->CS.Reserved = 0;
      mem_handle = vi->input_mem_handle;

      _(VipPostRecv(vi->handle, desc, mem_handle));
    }

  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->output_desc;
    mem_handle = vi->output_mem_handle;

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    _(VipPostSend(vi->handle, desc, mem_handle));
    os->need_send_wait = tbx_true;


#ifdef PM2_DEV
#warning temp
#endif
      _(VipSendWait(vi->handle, VIP_INFINITE, &desc));

      if (desc != vi->output_desc)
	FAILURE("unexpected descriptor");

      os->need_send_wait = tbx_false;
  }

  LOG_OUT();
}

void
mad_via_finalize_message(p_mad_connection_t out)
{
  LOG_IN();
  LOG_OUT();
}

p_mad_connection_t
mad_via_receive_message(p_mad_channel_t channel)
{
  p_mad_via_channel_specific_t        cs        =  NULL;
  p_mad_connection_t                  in        =  NULL;
  p_mad_via_in_connection_specific_t  is        =  NULL;

  LOG_IN();
  cs = channel->specific;
  {
    p_mad_via_cq_t     cq        =      NULL;
    p_tbx_darray_t     darray    =      NULL;
    tbx_darray_index_t idx       =        -1;
    VIP_BOOLEAN        is_recv   = VIP_FALSE;
    VIP_VI_HANDLE      vi_handle =         0;

    cq = cs->input_cq;
    __(VipCQWait(cq->handle, VIP_INFINITE, &vi_handle, &is_recv));

    darray = channel->in_connection_darray;

    if ((in = tbx_darray_first_idx(darray, &idx)))
      {
	do
	  {
	    is = in->specific;

	    if (is->vi->handle == vi_handle)
	      goto found;
	  }
	while ((in = tbx_darray_next_idx(darray, &idx)));

	FAILURE("unexpected vi handle");
      }
    else
      FAILURE("empty channel");
  }

 found:

  {
    p_mad_via_vi_t vi = NULL;

    vi = is->vi;

    {
      p_mad_via_descriptor_set_t  set  = NULL;
      VIP_DESCRIPTOR             *desc = NULL;

      set = is->descriptor_set;
      _(VipRecvDone(vi->handle, &desc));
      mad_via_descriptor_set_return_desc(set, desc);
    }
    
    {
      p_mad_via_descriptor_set_t  set        = NULL;
      VIP_DESCRIPTOR             *desc       = NULL;
      VIP_MEM_HANDLE              mem_handle =    0;

      set        = is->descriptor_set;
      mem_handle = mad_via_descriptor_set_get_handle(set);    
      desc       = mad_via_descriptor_set_get_desc(set);

      desc->CS.SegCount = 0;
      desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
      desc->CS.Length   = 0;
      desc->CS.Status   = 0;
      desc->CS.Reserved = 0;

      _(VipPostRecv(vi->handle, desc, mem_handle));
    }
  
    is->nb_consumed++;
    
    if (is->nb_consumed >= MAD_VIA_RECV_MAIN_DESC_NUM)
      {
	VIP_DESCRIPTOR *desc       = NULL;
	VIP_MEM_HANDLE  mem_handle =    0;

	desc       = vi->output_desc;
	mem_handle = vi->output_mem_handle;

	desc->CS.SegCount = 0;
	desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
	desc->CS.Length   = 0;
	desc->CS.Status   = 0;
	desc->CS.Reserved = 0;

	_(VipPostSend(vi->handle, desc, mem_handle));
	_(VipSendWait(vi->handle, VIP_INFINITE, &desc));

	if (desc != vi->output_desc)
	  FAILURE("unexpected descriptor");
      }
  }
  LOG_OUT();

  return in;
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_via_poll_message(p_mad_channel_t channel)
{
  LOG_IN();
  LOG_OUT();
}
#endif /* MAD_MESSAGE_POLLING */

void
mad_via_message_received(p_mad_connection_t channel)
{
  LOG_IN();
  LOG_OUT();
}

/* Credit link */
void
mad_via_credit_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  p_mad_connection_t                  out = NULL;
  p_mad_via_out_connection_specific_t os  = NULL;
  p_mad_via_link_specific_t           ls  = NULL;
  p_mad_via_vi_t                      vi  = NULL;

  LOG_IN();
  out  = lnk->connection;
  os   = out->specific;

  if (os->need_send_wait)
    {
      p_mad_via_vi_t  o_vi = NULL;
      VIP_DESCRIPTOR *desc = NULL;

      o_vi = os->vi;
      _(VipSendWait(o_vi->handle, VIP_INFINITE, &desc));

      if (desc != o_vi->output_desc)
	FAILURE("unexpected descriptor");

      os->need_send_wait = tbx_false;
    }

  ls = lnk->specific;
  vi = ls->vi;

  LOG_OUT();
}

void
mad_via_credit_receive_buffer(p_mad_link_t   lnk,
			      p_mad_buffer_t buffer)
{
  p_mad_connection_t                 in     = NULL;
  p_mad_via_in_connection_specific_t is     = NULL;
  p_mad_via_link_specific_t          ls     = NULL;
  p_mad_via_vi_t                     vi     = NULL;

  LOG_IN();
  in     =  lnk->connection;
  is     =  in->specific;
  ls     =  lnk->specific;
  vi     =  ls->vi;
  LOG_OUT();
}

void
mad_via_credit_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_credit_send_buffer(lnk, buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/* Msg link */
void
mad_via_msg_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  p_mad_connection_t                  out = NULL;
  p_mad_via_out_connection_specific_t os  = NULL;
  p_mad_via_link_specific_t           ls  = NULL;
  p_mad_via_vi_t                      vi  = NULL;

  LOG_IN();
  out  = lnk->connection;
  os   = out->specific;

  if (os->need_send_wait)
    {
      p_mad_via_vi_t  o_vi = NULL;
      VIP_DESCRIPTOR *desc = NULL;

      o_vi = os->vi;
      _(VipSendWait(o_vi->handle, VIP_INFINITE, &desc));

      if (desc != o_vi->output_desc)
	FAILURE("unexpected descriptor");

      os->need_send_wait = tbx_false;
    }

  ls = lnk->specific;
  vi = ls->vi;

  mad_via_register_buffer(lnk->connection->channel->adapter, buffer);

  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    _(VipRecvWait(vi->handle, VIP_INFINITE, &desc));
    if (desc != vi->input_desc)
      FAILURE("unexpected descriptor");

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;
    mem_handle = vi->input_mem_handle;

    _(VipPostRecv(vi->handle, desc, mem_handle));
  }


  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->output_desc;
    mem_handle = vi->output_mem_handle;

    desc->CS.SegCount = 1;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = buffer->bytes_written - buffer->bytes_read;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    desc->DS[0].Local.Data.Address = buffer->buffer + buffer->bytes_read;
    desc->DS[0].Local.Handle       = (VIP_MEM_HANDLE)buffer->specific;
    desc->DS[0].Local.Length       = buffer->bytes_written - buffer->bytes_read;

    _(VipPostSend(vi->handle, desc, mem_handle));
    _(VipSendWait(vi->handle, VIP_INFINITE, &desc));

    if (desc != vi->output_desc)
      FAILURE("unexpected descriptor");

    buffer->bytes_read = buffer->bytes_written;
  }
  mad_via_unregister_buffer(lnk->connection->channel->adapter, buffer);
  LOG_OUT();
}

void
mad_via_msg_receive_buffer(p_mad_link_t   lnk,
			   p_mad_buffer_t buffer)
{
  p_mad_connection_t                 in     = NULL;
  p_mad_via_in_connection_specific_t is     = NULL;
  p_mad_via_link_specific_t          ls     = NULL;
  p_mad_via_vi_t                     vi     = NULL;

  LOG_IN();
  in     =  lnk->connection;
  is     =  in->specific;
  ls     =  lnk->specific;
  vi     =  ls->vi;

  mad_via_register_buffer(lnk->connection->channel->adapter, buffer);
  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->input_desc;
    mem_handle = vi->input_mem_handle;

    desc->CS.SegCount = 1;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = buffer->length - buffer->bytes_written;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    desc->DS[0].Local.Data.Address = buffer->buffer + buffer->bytes_written;
    desc->DS[0].Local.Handle       = (VIP_MEM_HANDLE)buffer->specific;
    desc->DS[0].Local.Length       = buffer->length - buffer->bytes_written;

    _(VipPostRecv(vi->handle, desc, mem_handle));
  }

  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->output_desc;
    mem_handle = vi->output_mem_handle;

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    _(VipPostSend(vi->handle, desc, mem_handle));
    _(VipSendWait(vi->handle, VIP_INFINITE, &desc));

    if (desc != vi->output_desc)
      FAILURE("unexpected descriptor");
  }

  {
    VIP_DESCRIPTOR *desc = NULL;

    _(VipRecvWait(vi->handle, VIP_INFINITE, &desc));
    
    if (desc != vi->input_desc)
      FAILURE("unexpected descriptor");

    buffer->bytes_written = buffer->length;
  }

  
  mad_via_unregister_buffer(lnk->connection->channel->adapter, buffer);
  LOG_OUT();
}

void
mad_via_msg_send_buffer_group(p_mad_link_t         lnk,
			      p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_msg_send_buffer(lnk, buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_via_msg_receive_sub_buffer_group(p_mad_link_t         lnk,
				     p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_msg_receive_buffer(lnk, &buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/* RDMA link */
void
mad_via_rdma_send_buffer(p_mad_link_t   lnk,
			 p_mad_buffer_t buffer)
{
  p_mad_connection_t                   out = NULL;
  p_mad_via_out_connection_specific_t  os  = NULL;
  p_mad_via_link_specific_t            ls  = NULL;
  p_mad_via_vi_t                       vi  = NULL;
  void                                *ptr = NULL;

  LOG_IN();
  out  = lnk->connection;
  os   = out->specific;

  if (os->need_send_wait)
    {
      p_mad_via_vi_t  o_vi = NULL;
      VIP_DESCRIPTOR *desc = NULL;

      o_vi = os->vi;
      _(VipSendWait(o_vi->handle, VIP_INFINITE, &desc));

      if (desc != o_vi->output_desc)
	FAILURE("unexpected descriptor");

      os->need_send_wait = tbx_false;
    }

  ls = lnk->specific;
  vi = ls->vi;

  mad_via_register_buffer(lnk->connection->channel->adapter, buffer);

  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    _(VipRecvWait(vi->handle, VIP_INFINITE, &desc));
    if (desc != vi->input_desc)
      FAILURE("unexpected descriptor");

    ptr = (void *)desc->CS.ImmediateData;

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;
    mem_handle = vi->input_mem_handle;

    _(VipPostRecv(vi->handle, desc, mem_handle));
  }


  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->output_desc;
    mem_handle = vi->output_mem_handle;

    desc->CS.SegCount = 2;
    desc->CS.Control  =
      VIP_CONTROL_OP_RDMAWRITE| VIP_CONTROL_IMMEDIATE;
    desc->CS.Length   = buffer->bytes_written - buffer->bytes_read;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    desc->DS[0].Local.Data.Address = buffer->buffer + buffer->bytes_read;
    desc->DS[0].Local.Handle       = (VIP_MEM_HANDLE)buffer->specific;
    desc->DS[0].Local.Length       = buffer->bytes_written - buffer->bytes_read;

    desc->AS[0].Remote.Data.Address = ptr;
#error remote address syntax

    _(VipPostSend(vi->handle, desc, mem_handle));
    _(VipSendWait(vi->handle, VIP_INFINITE, &desc));

    if (desc != vi->output_desc)
      FAILURE("unexpected descriptor");

    buffer->bytes_read = buffer->bytes_written;
  }
  mad_via_unregister_buffer(lnk->connection->channel->adapter, buffer);
  LOG_OUT();
}

void
mad_via_rdma_receive_buffer(p_mad_link_t   lnk,
			    p_mad_buffer_t buffer)
{
  p_mad_connection_t                 in     = NULL;
  p_mad_via_in_connection_specific_t is     = NULL;
  p_mad_via_link_specific_t          ls     = NULL;
  p_mad_via_vi_t                     vi     = NULL;

  LOG_IN();
  in     =  lnk->connection;
  is     =  in->specific;
  ls     =  lnk->specific;
  vi     =  ls->vi;

  mad_via_register_buffer(lnk->connection->channel->adapter, buffer);
  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->input_desc;
    mem_handle = vi->input_mem_handle;

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    _(VipPostRecv(vi->handle, desc, mem_handle));
  }

  {
    VIP_DESCRIPTOR *desc       = NULL;
    VIP_MEM_HANDLE  mem_handle =    0;

    desc       = vi->output_desc;
    mem_handle = vi->output_mem_handle;

    desc->CS.SegCount = 0;
    desc->CS.Control  = VIP_CONTROL_OP_SENDRECV;
    desc->CS.Length   = 0;
    desc->CS.Status   = 0;
    desc->CS.Reserved = 0;

    _(VipPostSend(vi->handle, desc, mem_handle));
    _(VipSendWait(vi->handle, VIP_INFINITE, &desc));

    if (desc != vi->output_desc)
      FAILURE("unexpected descriptor");
  }

  {
    VIP_DESCRIPTOR *desc = NULL;

    _(VipRecvWait(vi->handle, VIP_INFINITE, &desc));
    
    if (desc != vi->input_desc)
      FAILURE("unexpected descriptor");

    buffer->bytes_written = buffer->length;
  }

  
  mad_via_unregister_buffer(lnk->connection->channel->adapter, buffer);
  LOG_OUT();
}

void
mad_via_rdma_send_buffer_group(p_mad_link_t         lnk,
			       p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_rdma_send_buffer(lnk, buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_via_rdma_receive_sub_buffer_group(p_mad_link_t         lnk,
				     p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  p_mad_buffer_t buffer = NULL;

	  buffer = tbx_get_list_reference_object(&ref);
	  mad_via_rdma_receive_buffer(lnk, &buffer);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/* buffer transfer */
void
mad_via_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  LOG_IN();
  if (lnk->id == credit_link_id)
    {
      mad_via_credit_send_buffer(lnk, buffer);
    }
  else if (lnk->id == msg_link_id)
    {
      mad_via_msg_send_buffer(lnk, buffer);
    }
  else if (lnk->id == rdma_link_id)
    {
      mad_via_rdma_send_buffer(lnk, buffer);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();
}

void
mad_via_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *pbuffer)
{
  LOG_IN();
  if (lnk->id == credit_link_id)
    {
      mad_via_credit_receive_buffer(lnk, pbuffer);
    }
  else if (lnk->id == msg_link_id)
    {
      mad_via_msg_receive_buffer(lnk, *pbuffer);
    }
  else if (lnk->id == rdma_link_id)
    {
      mad_via_rdma_receive_buffer(lnk, *pbuffer);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();
}

/* Buffer group transfer */
void
mad_via_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (lnk->id == credit_link_id)
    {
      mad_via_credit_send_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == msg_link_id)
    {
      mad_via_msg_send_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == rdma_link_id)
    {
      mad_via_rdma_send_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();
}

void
mad_via_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_subgroup TBX_UNUSED,
				 p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  if (lnk->id == msg_link_id)
    {
      mad_via_msg_receive_buffer_group(lnk, buffer_group);
    }
  else if (lnk->id == rdma_link_id)
    {
      mad_via_rdma_receive_buffer_group(lnk, buffer_group);
    }
  else
    FAILURE("invalid link id");
  
  LOG_OUT();
}
