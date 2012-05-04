/*
 * NewMadeleine
 * Copyright (C) 2006-2011 (see AUTHORS file)
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

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>

#include "nm_ibverbs.h"

#include <Padico/Module.h>
#include <tbx.h>


/* *********************************************************
 * Rationale of the newmad/ibverbs driver
 *
 * This is a multi-method driver. Four methods are available:
 *   -- bycopy: data is copied in a pre-allocated, pre-registered
 *      memory region, then sent through RDMA into a pre-registered
 *      memory region of the peer node.
 *   -- adaptrdma: data is copied in a pre-allocated, pre-registered
 *      memory region, with an adaptive super-pipeline. Memory blocks are 
 *      copied as long as the previous RDMA send doesn't complete.
 *      A guard check ensures that block size progression is at least
 *      2-base exponential to prevent artifacts to kill performance.
 *   -- regrdma: memory is registered on the fly on both sides, using 
 *      a pipeline with variable block size. For each block, the
 *      receiver sends an ACK with RDMA info (raddr, rkey), then 
 *      zero-copy RDMA is performed.
 *   -- rcache: memory is registered on the fly on both sides, 
 *      sequencially, using puk_mem_* functions from Puk-ABI that
 *      manage a cache.
 *
 * Method is chosen as follows:
 *   -- tracks for small messages always uses 'bycopy'
 *   -- tracks with rendez-vous use 'adaptrdma' for smaller messages 
 *      and 'regrdma' for larger messages. Usually, the threshold 
 *      is 224kB (from empirical results about registration/send overlap)
 *      on MT23108, and 2MB on ConnectX.
 *   -- tracks with rendez-vous use 'rcache' when ib_rcache is activated
 *      in the flavor.
 */



/** status for an instance. */
struct nm_ibverbs
{
  struct nm_ibverbs_cnx cnx_array[2];
};

static tbx_checksum_t _nm_ibverbs_checksum = NULL;

/* ********************************************************* */

/* ** component declaration */

static int nm_ibverbs_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_ibverbs_close(struct nm_drv*p_drv);
static int nm_ibverbs_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url);
static int nm_ibverbs_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id);
static int nm_ibverbs_post_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_send_prefetch(void*_status,  struct nm_pkt_wrap *p_pw);
static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static const char* nm_ibverbs_get_driver_url(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_ibverbs_driver =
  {
    .name               = "ibverbs",

    .query              = &nm_ibverbs_query,
    .init               = &nm_ibverbs_init,
    .close              = &nm_ibverbs_close,

    .connect		= &nm_ibverbs_connect,
    .disconnect         = &nm_ibverbs_disconnect,
    
    .post_send_iov	= &nm_ibverbs_post_send_iov,
    .post_recv_iov      = &nm_ibverbs_post_recv_iov,
    
    .poll_send_iov      = &nm_ibverbs_poll_send_iov,
    .poll_recv_iov      = &nm_ibverbs_poll_recv_iov,
    /* TODO: add poll_any callbacks  */
    .poll_send_any_iov  = NULL,
    .poll_recv_any_iov  = NULL,

    .prefetch_send      = &nm_ibverbs_send_prefetch,

    .cancel_recv_iov    = &nm_ibverbs_cancel_recv_iov,

    .get_driver_url     = &nm_ibverbs_get_driver_url,

    .capabilities.min_period    = 0,
    .capabilities.rdv_threshold = 32 * 1024
  };

/** 'PadicoAdapter' facet for Ibverbs driver */
static void*nm_ibverbs_instanciate(puk_instance_t, puk_context_t);
static void nm_ibverbs_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_adapter_driver =
  {
    .instanciate = &nm_ibverbs_instanciate,
    .destroy     = &nm_ibverbs_destroy
  };


PADICO_MODULE_COMPONENT(NewMad_Driver_ibverbs,
  puk_component_declare("NewMad_Driver_ibverbs",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_ibverbs_driver)) );


/** Instanciate functions */
static void* nm_ibverbs_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs*status = TBX_MALLOC(sizeof(struct nm_ibverbs));
  memset(status->cnx_array, 0, sizeof(struct nm_ibverbs_cnx) * 2);
  const char*checksum_env = getenv("NMAD_IBVERBS_CHECKSUM");
  if(_nm_ibverbs_checksum == NULL && checksum_env != NULL)
    {
      tbx_checksum_t checksum = tbx_checksum_get(checksum_env);
      if(checksum == NULL)
	TBX_FAILUREF("# nmad: checksum algorithm *%s* not available.\n", checksum_env);
      _nm_ibverbs_checksum = checksum;
      NM_DISPF("# nmad ibverbs: checksum enabled (%s).\n", checksum->name);
    }
  return status;
}

static void nm_ibverbs_destroy(void*_status)
{
  struct nm_ibverbs*status = _status;
  TBX_FREE(status);
}

const static char*nm_ibverbs_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  return p_ibverbs_drv->url;
}


/* ** helpers ********************************************** */

static inline struct nm_ibverbs_cnx*nm_ibverbs_get_cnx(void*_status, nm_trk_id_t trk_id)
{
  struct nm_ibverbs*status = _status;
  struct nm_ibverbs_cnx*p_ibverbs_cnx = &status->cnx_array[trk_id];
  return p_ibverbs_cnx;
}

/* ** checksum ********************************************* */


/** checksum algorithm. Set NMAD_IBVERBS_CHECKSUM to non-null to enable checksums.
 */
uint32_t nm_ibverbs_checksum(const char*data, uint32_t len)
{
  if(_nm_ibverbs_checksum)
    return (*_nm_ibverbs_checksum->func)(data, len);
  else
    return 0;
}

int nm_ibverbs_checksum_enabled(void)
{
  return _nm_ibverbs_checksum != NULL;
}

uint32_t nm_ibverbs_memcpy_and_checksum(void*_dest, const void*_src, uint32_t len)
{
  if(_nm_ibverbs_checksum)
    {
      if(_nm_ibverbs_checksum->checksum_and_copy)
	{
	  return (*_nm_ibverbs_checksum->checksum_and_copy)(_dest, _src, len);
	}
      else
	{
	  memcpy(_dest, _src, len);
	  return nm_ibverbs_checksum(_dest, len);
	}
    }
  else
    {
      memcpy(_dest, _src, len);
    }
  return 0;
}


#ifdef PM2_NUIOA
/* ******************** numa node ***************************** */

#define NM_IBVERBS_NUIOA_SYSFILE_LENGTH 16

static int nm_ibverbs_get_numa_node(struct ibv_device* ib_dev)
{
#ifdef LINUX_SYS
  FILE *sysfile = NULL;
  char file[128] = "";
  char line[NM_IBVERBS_NUIOA_SYSFILE_LENGTH]="";
  
  /* try to read /sys/class/infiniband/%s/device/numa_node (>= 2.6.22) */
  
  sprintf(file, "/sys/class/infiniband/%s/device/numa_node", ibv_get_device_name(ib_dev));
  sysfile = fopen(file, "r");
  if (sysfile) {
    int node;
    fgets(line, NM_IBVERBS_NUIOA_SYSFILE_LENGTH, sysfile);
    fclose(sysfile);
    node = strtoul(line, NULL, 0);
    return node >= 0 ? node : PM2_NUIOA_ANY_NODE;
  }
  
  /* or revert to libnuma-parsing /sys/class/infiniband/%s/device/local_cpus */
  
  if (numa_available() < 0) {
    return PM2_NUIOA_ANY_NODE;
  }

#if 0 /* FIXME when libnuma will have an API to do bitmask -> integer or char* */
  
  sprintf(file, "/sys/class/infiniband/%s/device/local_cpus", ibv_get_device_name(ib_dev));
  sysfile = fopen(file, "r");
  if (sysfile == NULL) {
    return PM2_NUIOA_ANY_NODE;
  }
  
  fgets(line, NM_IBVERBS_NUIOA_SYSFILE_LENGTH, sysfile);
  fclose(sysfile);
  
  int nb_nodes = numa_max_node();
  int i;
  for(i = 0; i <= nb_nodes; i++)
    {
      unsigned long mask;
      numa_node_to_cpus(i, &mask, sizeof(unsigned long));
      if (strtoul(line, NULL, 16) == mask)
	return i;
    }
#endif /* 0 */
#endif /* LINUX_SYS */
  
  return PM2_NUIOA_ANY_NODE;
}
#endif /* PM2_NUIOA */

/* ** init & connection ************************************ */

static int nm_ibverbs_query(struct nm_drv *p_drv,
                            struct nm_driver_query_param *params,
			    int nparam)
{
  int err;
  int dev_amount;
  int dev_number;
  int i;
  struct nm_ibverbs_drv*p_ibverbs_drv = TBX_MALLOC(sizeof(struct nm_ibverbs_drv));
  
  if (!p_ibverbs_drv) {
    err = -NM_ENOMEM;
    goto out;
  }
  p_ibverbs_drv->connector = NULL;
  p_ibverbs_drv->url = NULL;
  
  /* find IB device */
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: no device found.\n");
      abort();
    }
  
  dev_number = 0;
  for(i = 0; i < nparam; i++)
    {
      switch (params[i].key)
	{
	case NM_DRIVER_QUERY_BY_INDEX:
	  dev_number = params[i].value.index;
	  break;
	case NM_DRIVER_QUERY_BY_NOTHING:
	  break;
	default:
	  err = -NM_EINVAL;
	  goto out;
	}
    }
  if (dev_number >= dev_amount) {
    err = -NM_EINVAL;
    goto out;
  }
  p_ibverbs_drv->ib_dev = dev_list[dev_number];
  
  /* open IB context */
  p_ibverbs_drv->context = ibv_open_device(p_ibverbs_drv->ib_dev);
  if(p_ibverbs_drv->context == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot open IB context.\n");
      abort();
    }

  ibv_free_device_list(dev_list);
  
  /* get IB context attributes */
  struct ibv_device_attr device_attr;
  int rc = ibv_query_device(p_ibverbs_drv->context, &device_attr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get device capabilities.\n");
      abort();
    }

  /* detect LID */
  struct ibv_port_attr port_attr;
  rc = ibv_query_port(p_ibverbs_drv->context, NM_IBVERBS_PORT, &port_attr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get local port attributes.\n");
      abort();
    }
  p_ibverbs_drv->lid = port_attr.lid;
  if(p_ibverbs_drv->lid == 0)
    {
      fprintf(stderr, "nmad- WARNING- ibverbs: LID is null- subnet manager has probably crashed.\n");
    }

  /* IB capabilities */
  p_ibverbs_drv->ib_caps.max_qp        = device_attr.max_qp;
  p_ibverbs_drv->ib_caps.max_qp_wr     = device_attr.max_qp_wr;
  p_ibverbs_drv->ib_caps.max_cq        = device_attr.max_cq;
  p_ibverbs_drv->ib_caps.max_cqe       = device_attr.max_cqe;
  p_ibverbs_drv->ib_caps.max_mr        = device_attr.max_mr;
  p_ibverbs_drv->ib_caps.max_mr_size   = device_attr.max_mr_size;
  p_ibverbs_drv->ib_caps.page_size_cap = device_attr.page_size_cap;
  p_ibverbs_drv->ib_caps.max_msg_size  = port_attr.max_msg_sz;
  
  int link_width = -1;
  switch(port_attr.active_width)
    {
    case 1: link_width = 1;  break;
    case 2: link_width = 4;  break;
    case 4: link_width = 8;  break;
    case 8: link_width = 12; break;
    }
  int link_rate = -1;
  const char*s_link_rate = "unknown";
  switch(port_attr.active_speed)
    {
    case 0x01: link_rate = 2;  s_link_rate = "SDR"; break;
    case 0x02: link_rate = 4;  s_link_rate = "DDR"; break;
    case 0x04: link_rate = 8;  s_link_rate = "QDR"; break;
    case 0x08: link_rate = 14; s_link_rate = "FDR"; break;
    case 0x10: link_rate = 25; s_link_rate = "EDR"; break;
    }
  const int data_rate = link_width * link_rate;

  NM_DISPF("# nmad ibverbs: device '%s'- %dx %s (%d Gb/s); LID = 0x%02X\n",
	   ibv_get_device_name(p_ibverbs_drv->ib_dev), link_width, s_link_rate, data_rate, p_ibverbs_drv->lid);
#ifdef DEBUG
  NM_DISPF("# nmad ibverbs:   max_qp=%d; max_qp_wr=%d; max_cq=%d; max_cqe=%d;\n",
	   p_ibverbs_drv->ib_caps.max_qp, p_ibverbs_drv->ib_caps.max_qp_wr,
	   p_ibverbs_drv->ib_caps.max_cq, p_ibverbs_drv->ib_caps.max_cqe);
  NM_DISPF("# nmad ibverbs:   max_mr=%d; max_mr_size=%llu; page_size_cap=%llu; max_msg_size=%llu\n",
	   p_ibverbs_drv->ib_caps.max_mr,
	   (unsigned long long) p_ibverbs_drv->ib_caps.max_mr_size,
	   (unsigned long long) p_ibverbs_drv->ib_caps.page_size_cap,
	   (unsigned long long) p_ibverbs_drv->ib_caps.max_msg_size);
#endif

  /* driver profile encoding */
#ifdef PM2_NUIOA
  p_drv->profile.numa_node = nm_ibverbs_get_numa_node(p_ibverbs_drv->ib_dev);
#endif
  p_drv->profile.latency = 1350; /* from sampling */
  p_drv->profile.bandwidth = 1024 * (data_rate / 8) * 0.75; /* empirical estimation of software+protocol overhead */

  p_drv->priv = p_ibverbs_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  
  srand48(getpid() * time(NULL));
  
  /* allocate Protection Domain */
  p_ibverbs_drv->pd = ibv_alloc_pd(p_ibverbs_drv->context);
  if(p_ibverbs_drv->pd == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot allocate IB protection domain.\n");
      abort();
    }
  
  /* open helper socket */
  nm_ibverbs_connect_create(p_ibverbs_drv);

  /* open tracks */
  nm_trk_id_t i;
  for(i = 0; i < nb_trks; i++)
    {
      if(trk_caps[i].rq_type == nm_trk_rq_rdv)
	{
	  static const char const ib_rcache[] = "NewMad_ibverbs_rcache";
	  static const char const ib_lr2[] = "NewMad_ibverbs_lr2";
	  static puk_component_t ib_method = NULL;
	  if(ib_method == NULL)
	    {
	      if(getenv("NMAD_IBVERBS_RCACHE") != NULL)
		{
		  ib_method = puk_adapter_resolve(ib_rcache);
		  NM_DISPF("# nmad ibverbs: rcache forced by environment.\n");
		}
	      else
		{
		  ib_method = puk_adapter_resolve(ib_lr2);
		}
	    }
	  p_ibverbs_drv->trks_array[i].method = ib_method;
	  trk_caps[i].rq_type  = nm_trk_rq_rdv;
	  trk_caps[i].iov_type = nm_trk_iov_none;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 1;
	}
      else
	{
	  p_ibverbs_drv->trks_array[i].method = puk_adapter_resolve("NewMad_ibverbs_bycopy");
	  trk_caps[i].rq_type  = nm_trk_rq_dgram;
	  trk_caps[i].iov_type = nm_trk_iov_send_only;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 0;
	}
      p_ibverbs_drv->trks_array[i].method_iface =
	puk_component_get_driver_NewMad_ibverbs_method(p_ibverbs_drv->trks_array[i].method, NULL);
    }
  
  return NM_ESUCCESS;
}

static int nm_ibverbs_close(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  TBX_FREE(p_ibverbs_drv->url);
  TBX_FREE(p_ibverbs_drv);
  return NM_ESUCCESS;
}


static int nm_ibverbs_connect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, trk_id);
  p_ibverbs_cnx->method_instance = puk_adapter_instanciate(p_ibverbs_drv->trks_array[trk_id].method);
  puk_instance_indirect_NewMad_ibverbs_method(p_ibverbs_cnx->method_instance, NULL, &p_ibverbs_cnx->method);
  /* connection create */
  nm_ibverbs_cnx_qp_create(p_ibverbs_cnx, p_ibverbs_drv);
  (*p_ibverbs_cnx->method.driver->cnx_create)(p_ibverbs_cnx->method._status, p_ibverbs_cnx, p_ibverbs_drv);
  p_ibverbs_cnx->local_addr.lid = p_ibverbs_drv->lid;
  p_ibverbs_cnx->local_addr.qpn = p_ibverbs_cnx->qp->qp_num;
  p_ibverbs_cnx->local_addr.psn = lrand48() & 0xffffff;
  p_ibverbs_cnx->local_addr.n   = 0;
  /* exchange addresses */
  (*p_ibverbs_cnx->method.driver->addr_pack)(p_ibverbs_cnx->method._status, &p_ibverbs_cnx->local_addr);
  int rc = -1;
  do
    {
      nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 0);
      rc = nm_ibverbs_connect_recv(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->remote_addr);
    }
  while(rc != 0);
  (*p_ibverbs_cnx->method.driver->addr_unpack)(p_ibverbs_cnx->method._status, &p_ibverbs_cnx->remote_addr);
  /* connect */
  volatile int*buffer = (void*)p_ibverbs_cnx->local_addr.segments[0].raddr;
  buffer[0] = 0; /* recv buffer */
  buffer[1] = 1; /* send buffer */
  nm_ibverbs_cnx_qp_init(p_ibverbs_cnx);
  nm_ibverbs_cnx_qp_rtr(p_ibverbs_cnx);
  nm_ibverbs_cnx_qp_rts(p_ibverbs_cnx);
  /* exchange ack */
  do
    {
      nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1);
      rc = nm_ibverbs_connect_wait_ack(p_ibverbs_drv, remote_url, trk_id);
    }
  while(rc != 0);

  for(;;)
    {
      /* check connection */
      nm_ibverbs_sync_send_post((void*)&buffer[1], sizeof(int), p_ibverbs_cnx);
      rc = nm_ibverbs_sync_send_wait(p_ibverbs_cnx);
      if(rc)
	{
	  fprintf(stderr, "nmad: WARNING- ibverbs: connection failed to come up before RNR timeout; reset QP and try again.\n");
	  nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1);
	  nm_ibverbs_cnx_qp_reset(p_ibverbs_cnx);
	  nm_ibverbs_cnx_qp_init(p_ibverbs_cnx);
	  nm_ibverbs_cnx_qp_rtr(p_ibverbs_cnx);
	  nm_ibverbs_cnx_qp_rts(p_ibverbs_cnx);
	}
      else if(buffer[0] == 0)
	{
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);
	  do
	    {
	      TBX_GET_TICK(t2);
	    }
	  while(buffer[0] == 0 && TBX_TIMING_DELAY(t1, t2) < NM_IBVERBS_TIMEOUT_CHECK);
	  if(buffer[0] == 0)
	    {
	      nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1);
	    }
	}
      else
	{
	  /* connection ok- escape from loop */
	  break;
	}
    }

  return NM_ESUCCESS;
}

static int nm_ibverbs_disconnect(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id)
{
  int err;
  
  /* TODO */
  
  err = NM_ESUCCESS;
  
  return err;
}

/* ********************************************************* */

/* ** driver I/O functions ********************************* */

static int nm_ibverbs_post_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
  assert(p_pw->drv_priv == NULL);
  p_pw->drv_priv = p_ibverbs_cnx;
  (*p_ibverbs_cnx->method.driver->send_post)(p_ibverbs_cnx->method._status, &p_pw->v[0], p_pw->v_nb);
  int err = nm_ibverbs_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = p_pw->drv_priv;
  int err = (*p_ibverbs_cnx->method.driver->send_poll)(p_ibverbs_cnx->method._status);
  if(err == NM_ESUCCESS)
    p_pw->drv_priv = NULL;
  return err;
}

static int nm_ibverbs_send_prefetch(void*_status,  struct nm_pkt_wrap *p_pw)
{
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, NM_TRK_LARGE);
  if(p_ibverbs_cnx->method.driver->send_prefetch)
    {
      (*p_ibverbs_cnx->method.driver->send_prefetch)(p_ibverbs_cnx->method._status,
						     p_pw->v[0].iov_base, p_pw->v[0].iov_len);
    }
  return NM_ESUCCESS;
}

static inline void nm_ibverbs_recv_init(struct nm_pkt_wrap*__restrict__ p_pw,
					struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  (*p_ibverbs_cnx->method.driver->recv_init)(p_ibverbs_cnx->method._status, 
					     &p_pw->v[0], p_pw->v_nb);
}

static inline int nm_ibverbs_poll_one(struct nm_pkt_wrap*__restrict__ p_pw,
				      struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  int err = (*p_ibverbs_cnx->method.driver->poll_one)(p_ibverbs_cnx->method._status);
  return err;
}

static int nm_ibverbs_poll_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = -NM_EAGAIN;
  if(p_pw->drv_priv)
    {
      err = nm_ibverbs_poll_one(p_pw, p_pw->drv_priv);
    }
  else
    {
#warning TODO- poll_any
      TBX_FAILURE("poll_any not implemented yet.");
#if 0
      struct nm_ibverbs_drv*p_ibverbs_drv = p_pw->p_drv->drv_priv;
      if(p_ibverbs_drv->trks_array[p_pw->trk_id].method_iface->poll_any)
	{
	  struct nm_gate*p_gate = NULL;
	  p_ibverbs_drv->trks_array[p_pw->trk_id].method_iface->poll_any(p_pw, &p_gate);
	  if(pp_gate)
	    {
	      struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
	      nm_ibverbs_recv_init(p_pw, p_ibverbs_cnx);
	      err = nm_ibverbs_poll_one(p_pw, p_ibverbs_cnx);
	      goto out;
	    }
	}
    out:
#endif
    }
  return err;
}

static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = NM_ESUCCESS;
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
  p_pw->drv_priv  = p_ibverbs_cnx;
  if(p_pw->p_gate)
    {
      nm_ibverbs_recv_init(p_pw, p_ibverbs_cnx);
    }
  err = nm_ibverbs_poll_recv_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw)
{
  int err = -NM_ENOTIMPL;
  struct nm_ibverbs_cnx* p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
  if(p_ibverbs_cnx->method.driver->cancel_recv)
    {
      err = (*p_ibverbs_cnx->method.driver->cancel_recv)(p_ibverbs_cnx->method._status);
      p_pw->drv_priv = NULL;
    }
  return err;
}
