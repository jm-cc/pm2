/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

static int nm_ibverbs_load(void);

PADICO_MODULE_BUILTIN(NewMad_Driver_ibverbs, &nm_ibverbs_load, NULL, NULL);



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


/* *** global IB parameters ******************************** */

#define NM_IBVERBS_PORT         1
#define NM_IBVERBS_TX_DEPTH     4
#define NM_IBVERBS_RX_DEPTH     2
#define NM_IBVERBS_RDMA_DEPTH   4
#define NM_IBVERBS_MAX_SG_SQ    1
#define NM_IBVERBS_MAX_SG_RQ    1
#define NM_IBVERBS_MAX_INLINE   128
#define NM_IBVERBS_MTU          IBV_MTU_1024


struct nm_ibverbs_trk 
{
  puk_component_t method;
  const struct nm_ibverbs_method_iface_s*method_iface;
};

struct nm_ibverbs
{
  struct nm_ibverbs_cnx cnx_array[2];
  int sock;    /**< connected socket for IB address exchange */
};

tbx_checksum_func_t _nm_ibverbs_checksum = NULL;

/* ********************************************************* */

/* ** component declaration */

static int nm_ibverbs_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
static int nm_ibverbs_close(struct nm_drv*p_drv);
static int nm_ibverbs_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_ibverbs_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_ibverbs_disconnect(void*_status, struct nm_cnx_rq *p_crq);
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
    .accept		= &nm_ibverbs_accept,
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

static int nm_ibverbs_load(void)
{
  if(getenv("NMAD_IBVERBS_CHECKSUM"))
    {
      _nm_ibverbs_checksum = tbx_checksum_block64;
      NM_DISPF("# nmad ibverbs: checksum enabled.\n");
    }
  else
    {
      _nm_ibverbs_checksum = NULL;
    }

  puk_component_declare("NewMad_Driver_ibverbs",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_ibverbs_driver));
  return 0;
}


/** Instanciate functions */
static void* nm_ibverbs_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs*status = TBX_MALLOC(sizeof (struct nm_ibverbs));
  status->sock = -1;
  memset(status->cnx_array, 0, sizeof(struct nm_ibverbs_cnx) * 2);
  return status;
}

static void nm_ibverbs_destroy(void*_status)
{
  struct nm_ibverbs*status = _status;
  TBX_FREE(_status);
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

static void nm_ibverbs_addr_send(const void*_status,
				 const struct nm_ibverbs_cnx_addr*addr)
{
  const struct nm_ibverbs*status = _status;
  int rc = NM_SYS(send)(status->sock, addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
  if(rc != sizeof(struct nm_ibverbs_cnx_addr)) {
    fprintf(stderr, "nmad ibverbs: cannot send address to peer.\n");
    abort();
  }
}

static void nm_ibverbs_addr_recv(void*_status,
				 struct nm_ibverbs_cnx_addr*addr)
{
  const struct nm_ibverbs*status = _status;
  int rc = -1;
 retry_recv:
  rc = NM_SYS(recv)(status->sock, addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
  int err = errno;
  if(rc == -1 && err == EINTR)
    goto retry_recv;
  if(rc != sizeof(struct nm_ibverbs_cnx_addr)) 
    {
      fprintf(stderr, "nmad ibverbs: cannot get address from peer (%s).\n", strerror(err));
      abort();
    }
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
  
  /* find IB device */
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) {
    fprintf(stderr, "nmad ibverbs: no device found.\n");
    err = -NM_ENOTFOUND;
    goto out;
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
  if(p_ibverbs_drv->context == NULL) {
    fprintf(stderr, "nmad ibverbs: cannot open IB context.\n");
    err = -NM_ESCFAILD;
    goto out;
  }

  ibv_free_device_list(dev_list);
  
  /* get IB context attributes */
  struct ibv_device_attr device_attr;
  int rc = ibv_query_device(p_ibverbs_drv->context, &device_attr);
  if(rc != 0) {
    fprintf(stderr, "nmad ibverbs: cannot get device capabilities.\n");
    abort();
  }

  /* detect LID */
  struct ibv_port_attr port_attr;
  rc = ibv_query_port(p_ibverbs_drv->context, NM_IBVERBS_PORT, &port_attr);
  if(rc != 0) {
    fprintf(stderr, "nmad ibverbs: cannot get local port attributes.\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  p_ibverbs_drv->lid = port_attr.lid;
  if(p_ibverbs_drv->lid == 0)
    {
      fprintf(stderr, "nmad ibverbs: WARNING: LID is null- subnet manager has probably crashed.\n");
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
  int err;
  int rc;
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  
  srand48(getpid() * time(NULL));
  
  /* allocate Protection Domain */
  p_ibverbs_drv->pd = ibv_alloc_pd(p_ibverbs_drv->context);
  if(p_ibverbs_drv->pd == NULL) {
    fprintf(stderr, "nmad ibverbs: cannot allocate IB protection domain.\n");
    abort();
  }
  
  /* open helper socket */
  p_ibverbs_drv->server_sock = NM_SYS(socket)(AF_INET, SOCK_STREAM, 0);
  assert(p_ibverbs_drv->server_sock > -1);
  struct sockaddr_in addr;
  unsigned addr_len = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = INADDR_ANY;
  rc = NM_SYS(bind)(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, addr_len);
  if(rc) {
    fprintf(stderr, "nmad ibverbs: bind error (%s)\n", strerror(errno));
    abort();
  }
  rc = NM_SYS(getsockname)(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
  NM_SYS(listen)(p_ibverbs_drv->server_sock, 255);
	
  /* driver url encoding */

  struct ifaddrs*ifa_list = NULL;
  rc = getifaddrs(&ifa_list);
  if(rc == 0)
    {
      struct ifaddrs*i;
      for(i = ifa_list; i != NULL; i = i->ifa_next)
	{
	  if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET)
	    {
	      struct sockaddr_in*inaddr = (struct sockaddr_in*)i->ifa_addr;
	      if(!(i->ifa_flags & IFF_LOOPBACK))
		{
		  char s_url[16];
		  snprintf(s_url, 16, "%08x%04x", htonl(inaddr->sin_addr.s_addr), addr.sin_port);
		  p_ibverbs_drv->url = tbx_strdup(s_url);
		  break;
		}
	    }
	}
    }
  else
    {
      fprintf(stderr, "nmad ibverbs: cannot get local address\n");
      err = -NM_EUNREACH;
      goto out;
     }

  /* open tracks */
  p_ibverbs_drv->nb_trks = nb_trks;
  p_ibverbs_drv->trks_array = TBX_MALLOC(nb_trks * sizeof(struct nm_ibverbs_trk));
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
#ifdef NM_IBVERBS_RCACHE
		  ib_method = puk_adapter_resolve(ib_rcache);
#else
		  ib_method = puk_adapter_resolve(ib_lr2);
#endif
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
  
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_ibverbs_close(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  NM_SYS(close)(p_ibverbs_drv->server_sock);
  TBX_FREE(p_ibverbs_drv->trks_array);
  TBX_FREE(p_ibverbs_drv->url);
  TBX_FREE(p_ibverbs_drv);
  return NM_ESUCCESS;
}


static int nm_ibverbs_cnx_create(void*_status, struct nm_cnx_rq*p_crq)
{
  struct nm_drv *p_drv = p_crq->p_drv;
  int err = NM_ESUCCESS;
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  
  p_ibverbs_cnx->method_instance = puk_adapter_instanciate(p_ibverbs_drv->trks_array[p_crq->trk_id].method);
  puk_instance_indirect_NewMad_ibverbs_method(p_ibverbs_cnx->method_instance, NULL, &p_ibverbs_cnx->method);
  
  (*p_ibverbs_cnx->method.driver->cnx_create)(p_ibverbs_cnx->method._status, p_ibverbs_cnx, p_ibverbs_drv);

  /* init incoming CQ */
  p_ibverbs_cnx->if_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_RX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->if_cq == NULL) {
    fprintf(stderr, "nmad ibverbs: cannot create in CQ\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  /* init outgoing CQ */
  p_ibverbs_cnx->of_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_TX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->of_cq == NULL) {
    fprintf(stderr, "nmad ibverbs: cannot create out CQ\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  /* create QP */
  struct ibv_qp_init_attr qp_init_attr = {
    .send_cq = p_ibverbs_cnx->of_cq,
    .recv_cq = p_ibverbs_cnx->if_cq,
    .cap     = {
      .max_send_wr     = NM_IBVERBS_TX_DEPTH,
      .max_recv_wr     = NM_IBVERBS_RX_DEPTH,
      .max_send_sge    = NM_IBVERBS_MAX_SG_SQ,
      .max_recv_sge    = NM_IBVERBS_MAX_SG_RQ,
      .max_inline_data = NM_IBVERBS_MAX_INLINE
    },
    .qp_type = IBV_QPT_RC
  };
  p_ibverbs_cnx->qp = ibv_create_qp(p_ibverbs_drv->pd, &qp_init_attr);
  if(p_ibverbs_cnx->qp == NULL) {
    fprintf(stderr, "nmad ibverbs: couldn't create QP\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  p_ibverbs_cnx->max_inline = qp_init_attr.cap.max_inline_data;
  
  /* modifiy QP- step: INIT */
  struct ibv_qp_attr qp_attr = {
    .qp_state        = IBV_QPS_INIT,
    .pkey_index      = 0,
    .port_num        = NM_IBVERBS_PORT,
    .qp_access_flags = IBV_ACCESS_REMOTE_WRITE
  };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &qp_attr,
			 IBV_QP_STATE              |
			 IBV_QP_PKEY_INDEX         |
			 IBV_QP_PORT               |
			 IBV_QP_ACCESS_FLAGS);
  if(rc != 0) {
    fprintf(stderr, "nmad ibverbs: failed to modify QP to INIT\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  
  p_ibverbs_cnx->local_addr.lid = p_ibverbs_drv->lid;
  p_ibverbs_cnx->local_addr.qpn = p_ibverbs_cnx->qp->qp_num;
  p_ibverbs_cnx->local_addr.psn = lrand48() & 0xffffff;
  p_ibverbs_cnx->local_addr.n   = 0;
  (*p_ibverbs_cnx->method.driver->addr_pack)(p_ibverbs_cnx->method._status, &p_ibverbs_cnx->local_addr);

 out:
  return err;
}

static int nm_ibverbs_cnx_connect(void*_status, struct nm_cnx_rq*p_crq)
{
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  
  int err = NM_ESUCCESS;
  
  /* modify QP- step: RTR */
  struct ibv_qp_attr attr = {
    .qp_state           = IBV_QPS_RTR,
    .path_mtu           = NM_IBVERBS_MTU,
    .dest_qp_num        = p_ibverbs_cnx->remote_addr.qpn,
    .rq_psn             = p_ibverbs_cnx->remote_addr.psn,
    .max_dest_rd_atomic = NM_IBVERBS_RDMA_DEPTH,
    .min_rnr_timer      = 12, /* 12 */
    .ah_attr            = {
      .is_global        = 0,
      .dlid             = p_ibverbs_cnx->remote_addr.lid,
      .sl               = 0,
      .src_path_bits    = 0,
      .port_num         = NM_IBVERBS_PORT
    }
  };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_AV                 |
			 IBV_QP_PATH_MTU           |
			 IBV_QP_DEST_QPN           |
			 IBV_QP_RQ_PSN             |
			 IBV_QP_MAX_DEST_RD_ATOMIC |
			 IBV_QP_MIN_RNR_TIMER);
  if(rc != 0) {
    fprintf(stderr, "nmad ibverbs: failed to modify QP to RTR\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  /* modify QP- step: RTS */
  attr.qp_state      = IBV_QPS_RTS;
  attr.timeout       = 14; /* 14 */
  attr.retry_cnt     = 7;  /* 7 */
  attr.rnr_retry     = 7;  /* 7 = infinity */
  attr.sq_psn        = p_ibverbs_cnx->local_addr.psn;
  attr.max_rd_atomic = NM_IBVERBS_RDMA_DEPTH; /* 1 */
  rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
		     IBV_QP_STATE              |
		     IBV_QP_TIMEOUT            |
		     IBV_QP_RETRY_CNT          |
		     IBV_QP_RNR_RETRY          |
		     IBV_QP_SQ_PSN             |
		     IBV_QP_MAX_QP_RD_ATOMIC);
  if(rc != 0) {
    fprintf(stderr,"nmad ibverbs: failed to modify QP to RTS\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  
 out:
  return err;
}

static int nm_ibverbs_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_ibverbs*status = _status;
  
  int err = NM_ESUCCESS;
  if(status->sock == -1)
    {
      int fd = NM_SYS(socket)(AF_INET, SOCK_STREAM, 0);
      assert(fd > -1);
      status->sock = fd;
      assert(strlen(p_crq->remote_drv_url) == 12);
      in_addr_t peer_addr;
      int peer_port;
      sscanf(p_crq->remote_drv_url, "%08x%04x", &peer_addr, &peer_port);
      struct sockaddr_in inaddr = {
	.sin_family = AF_INET,
	.sin_port   = peer_port,
	.sin_addr   = (struct in_addr){ .s_addr = ntohl(peer_addr) }
      };
      int rc = -1;
    retry_connect:
      rc = NM_SYS(connect)(fd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
      if(rc)
	{
	  int error = errno;
	  if(error == EINTR)
	    goto retry_connect;
	  fprintf(stderr, "nmad ibverbs: cannot connect to %s:%d (%s)\n",
		  inet_ntoa(inaddr.sin_addr), peer_port, strerror(error));
	  err = -NM_EUNREACH;
	  goto out;
	}
    }
  
  err = nm_ibverbs_cnx_create(_status, p_crq);
  if(err)
    goto out;
  
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  nm_ibverbs_addr_recv(_status, &p_ibverbs_cnx->remote_addr);
  nm_ibverbs_addr_send(_status, &p_ibverbs_cnx->local_addr);
  (*p_ibverbs_cnx->method.driver->addr_unpack)(p_ibverbs_cnx->method._status, &p_ibverbs_cnx->remote_addr);
  err = nm_ibverbs_cnx_connect(_status, p_crq);
  
  if(p_crq->trk_id >= 1)
    {
      NM_SYS(close)(status->sock);
      status->sock = -1;
    }

 out:
  return err;
}

static int nm_ibverbs_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_ibverbs*status = _status;
  struct nm_ibverbs_drv*p_ibverbs_drv = p_crq->p_drv->priv;
  int err = NM_ESUCCESS;
  if(status->sock == -1)
    {
      struct sockaddr_in addr;
      unsigned addr_len = sizeof addr;
      int fd = -1;
    retry_accept:
      fd = NM_SYS(accept)(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
      int error = errno;
      if(fd < 0)
	{
	  if(error == EINTR)
	    goto retry_accept;
	  fprintf(stderr, "nmad ibverbs: error while accepting connection (%s)\n", strerror(error));
	  abort();
	}
      status->sock = fd;
    }
  err = nm_ibverbs_cnx_create(_status, p_crq);
  if(err)
    goto out;
  
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  nm_ibverbs_addr_send(_status, &p_ibverbs_cnx->local_addr);
  nm_ibverbs_addr_recv(_status, &p_ibverbs_cnx->remote_addr);
  (*p_ibverbs_cnx->method.driver->addr_unpack)(p_ibverbs_cnx->method._status, &p_ibverbs_cnx->remote_addr);
  err = nm_ibverbs_cnx_connect(_status, p_crq);
  
  if(p_crq->trk_id >= 1)
    {
      NM_SYS(close)(status->sock);
      status->sock = -1;
    }

 out:
  return err;
}

static int nm_ibverbs_disconnect(void*_status, struct nm_cnx_rq *p_crq)
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
  p_pw->drv_priv = p_ibverbs_cnx;
  (*p_ibverbs_cnx->method.driver->send_post)(p_ibverbs_cnx->method._status, &p_pw->v[0], p_pw->v_nb);
  int err = nm_ibverbs_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = p_pw->drv_priv;
  int err = (*p_ibverbs_cnx->method.driver->send_poll)(p_ibverbs_cnx->method._status);
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
