
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
#define MAD_VIA_INITIAL_CHANNEL_CQ_SIZE 16

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

typedef enum e_mad_via_vi_id
  {
    vi_id_in_main  = 0,
    vi_id_out_main = 1,
  }
mad_via_vi_id_t, *p_mad_via_vi_id_t;

/*
 * Structures
 * ----------------------------------------------------------------------------
 */
typedef struct s_mad_via_discriminator
{
  unsigned char node_id;
  unsigned char adapter_id;
  unsigned char channel_id;
  unsigned char vi_id;
} mad_via_discriminator_t, *p_mad_via_discriminator_t;

typedef struct s_mad_via_buffer_specific
{
  VIP_MEM_HANDLE mem_handle;
} mad_via_buffer_specific_t, *p_mad_via_buffer_specific_t;

typedef struct s_mad_via_driver_specific
{
  int dummy;
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

typedef struct s_mad_via_channel_specific
{
  p_mad_via_cq_t input_cq;
} mad_via_channel_specific_t, *p_mad_via_channel_specific_t;

typedef struct s_mad_via_vi
{
  VIP_VI_HANDLE      handle;
  VIP_VI_ATTRIBUTES *attributes;
  VIP_DESCRIPTOR    *input_desc;
  VIP_MEM_HANDLE     input_mem_handle;
  VIP_DESCRIPTOR    *output_desc;
  VIP_MEM_HANDLE     output_mem_handle;
  mad_via_vi_id_t    id;
} mad_via_vi_t, *p_mad_via_vi_t;

typedef struct s_mad_via_in_connection_specific
{
  p_mad_via_vi_t vi;
} mad_via_in_connection_specific_t, *p_mad_via_in_connection_specific_t;

typedef struct s_mad_via_out_connection_specific
{
  p_mad_via_vi_t vi;
} mad_via_out_connection_specific_t, *p_mad_via_out_connection_specific_t;

typedef struct s_mad_via_link_specific
{
  int dummy;
} mad_via_link_specific_t, *p_mad_via_link_specific_t;

/*
 * Static variables
 * ----------------------------------------------------------------------------
 */
p_tbx_memory_t


/*
 * Static functions
 * ----------------------------------------------------------------------------
 */
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
    case VIP_ERROR_RESOURCE:
      s = "VIA: error - resource";
      break;
    case VIP_INVALID_PARAMETER:
      s = "VIA: invalid parameter";
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

static void
mad_via_disp_nic_attributes(VIP_NIC_ATTRIBUTES *na)
{
  LOG("Name: %s",               na->Name);
  LOG("Hardware version: %lu",  na->HardwareVersion);
  LOG("Provider version: %lu",  na->ProviderVersion);
  LOG("Nic address length: %u", na->NicAddressLen);
  LOG("Nic address: %x",        (int) na->LocalNicAddress);
  if (na->ThreadSafe)
    {
      LOG("Thread safe: yes");
    }
  else
    {
      LOG("Thread safe: no");
    }
  LOG("Max discriminator length: %d",     (int) na->MaxDiscriminatorLen);
  LOG("Max register byte: %lu",           na->MaxRegisterBytes);
  LOG("Max register regions: %lu",        na->MaxRegisterRegions);
  LOG("Max register block byte: %lu",     na->MaxRegisterBlockBytes);
  LOG("Max VI: %lu",                      na->MaxVI);
  LOG("Max descriptors per queue: %lu",   na->MaxDescriptorsPerQueue);
  LOG("Max segments per descriptor: %lu", na->MaxSegmentsPerDesc);
  LOG("Max CQ: %lu",                      na->MaxCQ);
  LOG("Max CQ entries: %lu",              na->MaxCQEntries);
  LOG("Max transfer size: %lu",           na->MaxTransferSize);
  LOG("Native MTU: %lu",                  na->NativeMTU);
  LOG("Max Ptags: %lu",                   na->MaxPtags);

  switch (na->ReliabilityLevelSupport)
    {
    case VIP_SERVICE_UNRELIABLE:
      LOG("Reliability level support : unreliable");
      break;
    case VIP_SERVICE_RELIABLE_DELIVERY:
      LOG("Reliability level support : reliable delivery");
      break;
    case VIP_SERVICE_RELIABLE_RECEPTION:
      LOG("Reliability level support : reliable reception");
      break;
    default:
      LOG("Reliability level support : unknown !!!");
      break;
    }

  if (na->RDMAReadSupport)
    {
      LOG("RDMA read support: yes");
    }
  else
    {
      LOG("RDMA read support: no");
    }
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
		mad_via_vi_id_t        id)
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

    _(VipRegisterMem(as->nic_handle, vi->input_desc, size,
		     as->memory_attributes, &vi->input_mem_handle));
    _(VipRegisterMem(as->nic_handle, vi->input_desc, size,
		     as->memory_attributes, &vi->output_mem_handle));
  }
  LOG_OUT();

  return vi;
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
mad_via_driver_init(p_mad_driver_t driver)
{
  p_mad_via_driver_specific_t driver_specific = NULL;

  LOG_IN();
  TRACE("Initializing VIA driver");
  driver_specific = TBX_CALLOC(1, sizeof(mad_via_driver_specific_t));
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
  chs->input_cq      = cq;
  channel->specific  = chs;
  parameter_string   = tbx_string_init_to_int(channel->id);
  channel->parameter = tbx_string_to_cstring(parameter_string);
  tbx_string_free(parameter_string);
  parameter_string   = NULL;
  LOG_OUT();
}

void
mad_via_before_open_channel(p_mad_channel_t channel)
{
  LOG_IN();
  // Nothing
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

  LOG_IN();
  channel = (in?:out)->channel;
  chs     = channel->specific;
  adapter = channel->adapter;
  as      = adapter->specific;

  if (in)
    {
      p_mad_via_cq_t                     cq =    0;
      p_mad_via_in_connection_specific_t is = NULL;
      p_mad_via_vi_t                     vi = NULL;
      p_tbx_string_t                     s  = NULL;

      cq = chs->input_cq;
      vi = mad_via_vi_init(adapter, rdma_off, cq, NULL, vi_id_in_main);

      is->vi       = vi;
      in->specific = is;
      in->nb_link  =  1;

      s             = tbx_string_init_to_int(in->remote_rank);
      in->parameter = tbx_string_to_cstring(s);
      tbx_string_free(s);
      s             = NULL;
    }

  if (out)
    {
      p_mad_via_out_connection_specific_t os = NULL;
      p_mad_via_vi_t                      vi = NULL;
      p_tbx_string_t                      s  = NULL;

      vi = mad_via_vi_init(adapter, rdma_off, NULL, NULL, vi_id_out_main);

      os->vi        = vi;
      out->specific = os;
      out->nb_link  =  1;

      s              = tbx_string_init_to_int(out->remote_rank);
      out->parameter = tbx_string_to_cstring(s);
      tbx_string_free(s);
      s              = NULL;
    }
  LOG_OUT();
}

void
mad_via_link_init(p_mad_link_t lnk)
{
  p_mad_via_link_specific_t link_specific = NULL;

  LOG_IN();
  link_specific    = TBX_CALLOC(1, sizeof(mad_via_link_specific_t));
  lnk->specific    = link_specific;
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

/* Point-to-point connection */
void
mad_via_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t ai)
{
  p_mad_channel_t                    c   = NULL;
  p_mad_via_channel_specific_t       cs  = NULL;
  p_mad_adapter_t                    a   = NULL;
  p_mad_driver_t                     drv = NULL;
  p_mad_madeleine_t                  m   = NULL;
  p_mad_session_t                    s   = NULL;
  p_mad_directory_t                  dir = NULL;
  p_mad_via_adapter_specific_t       as  = NULL;
  p_mad_via_in_connection_specific_t is  = NULL;
  p_mad_dir_node_t                   rn  = NULL;
  p_mad_dir_adapter_t                ra  = NULL;
  p_mad_via_vi_t                     vi  = NULL;

  LOG_IN();
  c   = in->channel;
  cs  = c->specific;
  a   = c->adapter;
  as  = a->specific;
  drv = a->driver;
  m   = drv->madeleine;
  s   = m->session;
  dir = m->dir;
  is  = in->specific;
  rn  = ai->dir_node;
  ra  = ai->dir_adapter;
  vi  = is->vi;

  DISP_STR("accept: remote channel parameter",    ai->channel_parameter);
  DISP_STR("accept: remote connection parameter", ai->connection_parameter);

  {
    VIP_NIC_ATTRIBUTES      *na       = as->nic_attributes;
    VIP_NET_ADDRESS         *local    = NULL;
    VIP_NET_ADDRESS         *remote   = NULL;
    VIP_CONN_HANDLE          cnx_h    = NULL;
    VIP_VI_ATTRIBUTES        rattr    =  {0};
    mad_via_discriminator_t  d        =  {0};
    const size_t             disc_len = sizeof(mad_via_discriminator_t);
    const size_t             addr_len =
      sizeof(VIP_NET_ADDRESS) + na->NicAddressLen + disc_len;

    local                   = TBX_MALLOC(addr_len);
    local->HostAddressLen   = na->NicAddressLen;
    local->DiscriminatorLen = disc_len;
    memcpy(local->HostAddress, na->LocalNicAddress, na->NicAddressLen);

    remote                   = TBX_MALLOC(addr_len);
    memset(remote, 0, addr_len);
    remote->DiscriminatorLen = disc_len;

    {
      p_ntbx_process_t     process = NULL;
      p_mad_dir_node_t     node    = NULL;
      ntbx_process_grank_t rank    =   -1;

      rank      = s->process_rank;
      process   = tbx_darray_get(dir->process_darray, rank);
      node      = tbx_htable_get(process->ref, "node");
      d.node_id = node->id;
    }

    d.adapter_id                 = a ->id;
    d.channel_id                 = c ->id;
    d.vi_id                      = vi->id;
    MAD_VIA_DISCRIMINATOR(local) = d;

    _(VipConnectWait(as->nic_handle, local, VIP_INFINITE, remote, &rattr, &cnx_h));
  }
  LOG_OUT();
}

void
mad_via_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t ai TBX_UNUSED)
{
  p_mad_channel_t                     c   = NULL;
  p_mad_via_channel_specific_t        cs  = NULL;
  p_mad_adapter_t                     a   = NULL;
  p_mad_driver_t                      drv = NULL;
  p_mad_madeleine_t                   m   = NULL;
  p_mad_session_t                     s   = NULL;
  p_mad_directory_t                   dir = NULL;
  p_mad_via_adapter_specific_t        as  = NULL;
  p_mad_via_out_connection_specific_t os  = NULL;
  p_mad_dir_node_t                    rn  = NULL;
  p_mad_dir_adapter_t                 ra  = NULL;
  p_mad_via_vi_t                      vi  = NULL;

  LOG_IN();
  c   = out->channel;
  cs  = c->specific;
  a   = c->adapter;
  drv = a->driver;
  m   = drv->madeleine;
  s   = m->session;
  dir = m->dir;
  as  = a->specific;
  os  = out->specific;
  rn  = ai->dir_node;
  ra  = ai->dir_adapter;
  vi  = os->vi;

  DISP_STR("connect: remote channel parameter",    ai->channel_parameter);
  DISP_STR("connect: remote connection parameter", ai->connection_parameter);

  {
    VIP_NET_ADDRESS         *local    = NULL;
    VIP_NET_ADDRESS         *remote   = NULL;
    VIP_NIC_ATTRIBUTES      *na       = as->nic_attributes;
    VIP_VI_ATTRIBUTES        rattr    =  {0};
    mad_via_discriminator_t  d        =  {0};
    const size_t             disc_len = sizeof(mad_via_discriminator_t);
    const size_t             addr_len =
      sizeof(VIP_NET_ADDRESS) + na->NicAddressLen + disc_len;

    local                   = TBX_MALLOC(addr_len);
    local->HostAddressLen   = na->NicAddressLen;
    local->DiscriminatorLen = disc_len;
    memcpy(local->HostAddress, na->LocalNicAddress, na->NicAddressLen);

    remote                   = TBX_MALLOC(addr_len);
    memset(remote, 0, addr_len);
    remote->DiscriminatorLen = disc_len;

    {
      p_ntbx_process_t     process = NULL;
      p_mad_dir_node_t     node    = NULL;
      ntbx_process_grank_t rank    =   -1;

      rank      = s->process_rank;
      process   = tbx_darray_get(dir->process_darray, rank);
      node      = tbx_htable_get(process->ref, "node");
      d.node_id = node->id;
    }

    d.adapter_id                 = a ->id;
    d.channel_id                 = c ->id;
    d.vi_id                      = vi->id;
    MAD_VIA_DISCRIMINATOR(local) = d;

    _(VipNSGetHostByName(as->nic_handle, rn->name, remote, 0));

    /* NOTE: Poster un descripteur */

    _(VipConnectRequest(vi->handle, local, remote, VIP_INFINITE, &rattr));
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
  p_mad_via_link_specific_t lnk_specific = NULL;

  LOG_IN();
  lnk_specific = lnk->specific;
  TBX_FREE(lnk_specific);
  lnk->specific = NULL;
  LOG_OUT();
}

void
mad_via_connection_exit(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_via_in_connection_specific_t in_specific  = NULL;
  p_mad_via_out_connection_specific_t out_specific = NULL;

  LOG_IN();
  in_specific  = in->specific;
  out_specific = out->specific;
  TBX_FREE(in_specific);
  TBX_FREE(out_specific);
  in->specific  = NULL;
  out->specific = NULL;
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
  lnk = connection->link_array[0];
  LOG_OUT();

  return lnk;
}

/* Static buffers management */
p_mad_buffer_t
mad_via_get_static_buffer(p_mad_link_t lnk)
{
  LOG_IN();
  LOG_OUT();

  return NULL;
}

void
mad_via_return_static_buffer(p_mad_link_t   lnk,
			     p_mad_buffer_t buffer)
{
  LOG_IN();
  LOG_OUT();
}

/* Message transfer */
void
mad_via_new_message(p_mad_connection_t out)
{
  LOG_IN();
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
  p_mad_connection_t in = NULL;

  LOG_IN();
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

/* Buffer transfer */
void
mad_via_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  LOG_IN();
  LOG_OUT();
}

void
mad_via_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *buffer)
{
  LOG_IN();
  LOG_OUT();
}

/* Buffer group transfer */
void
mad_via_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  LOG_OUT();
}

void
mad_via_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_subgroup TBX_UNUSED,
				 p_mad_buffer_group_t buffer_group)
{
  LOG_IN();
  LOG_OUT();
}
