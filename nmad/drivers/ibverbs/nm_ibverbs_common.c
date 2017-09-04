/*
 * NewMadeleine
 * Copyright (C) 2006-2013 (see AUTHORS file)
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


#include "nm_ibverbs.h"
#include <Padico/Module.h>
#include <tbx.h>
#ifdef PM2_TOPOLOGY
#include <tbx_topology.h>
#include <hwloc/openfabrics-verbs.h>
#endif /* PM2_TOPOLOGY */

PADICO_MODULE_DECLARE(NewMad_ibverbs_common);


static struct
{
  int refcount;              /**< number of drivers using ibverbs common */
  puk_hashtable_t hca_table; /**< HCAs, hashed by key(device+port) */
} nm_ibverbs_common = { .refcount = 0, .hca_table = NULL };

static tbx_checksum_t _nm_ibverbs_checksum = NULL;

/** ptr alignment enforced in every packets (through padding) */
int nm_ibverbs_alignment = 64;
/** ptr alignment enforced for buffer allocation */
int nm_ibverbs_memalign  = 4096;

/* ********************************************************* */

static void nm_ibverbs_cnx_qp_create(struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_hca_s*p_hca);
static void nm_ibverbs_cnx_qp_reset(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_init(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_rtr(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_rts(struct nm_ibverbs_cnx*p_ibverbs_cnx);

static int nm_ibverbs_sync_send(const void*sbuf, const void*rbuf, int size, struct nm_ibverbs_cnx*cnx);

/* ********************************************************* */

uint32_t nm_ibverbs_hca_hash(const void*_key)
{
  struct nm_ibverbs_hca_key_s*p_key = _key;
  const uint32_t hash = p_key->port + puk_hash_string(p_key->device);
  return hash;
}

int nm_ibverbs_hca_eq(const void*_key1, const void*_key2)
{
  const struct nm_ibverbs_hca_key_s*key1 = _key1;
  const struct nm_ibverbs_hca_key_s*key2 = _key2;
  return ((strcmp(key1->device, key2->device) == 0) &&
	  (key1->port == key2->port));
}

static void nm_ibverbs_common_init(void)
{
  nm_ibverbs_common.hca_table = puk_hashtable_new(&nm_ibverbs_hca_hash, &nm_ibverbs_hca_eq);
  const char*checksum_env = getenv("NMAD_IBVERBS_CHECKSUM");
  if(_nm_ibverbs_checksum == NULL && checksum_env != NULL)
    {
      tbx_checksum_t checksum = tbx_checksum_get(checksum_env);
      if(checksum == NULL)
	NM_FATAL("# nmad: checksum algorithm *%s* not available.\n", checksum_env);
      _nm_ibverbs_checksum = checksum;
      NM_DISPF("# nmad ibverbs: checksum enabled (%s).\n", checksum->name);
    }
  const char*align_env = getenv("NMAD_IBVERBS_ALIGN");
  if(align_env != NULL)
    {
      nm_ibverbs_alignment = atoi(align_env);
      NM_DISPF("# nmad ibverbs: alignment forced to %d\n", nm_ibverbs_alignment);
    }
  const char*memalign_env = getenv("NMAD_IBVERBS_MEMALIGN");
  if(memalign_env != NULL)
    {
      nm_ibverbs_memalign = atoi(memalign_env);
      NM_DISPF("# nmad ibverbs: memalign forced to %d\n", nm_ibverbs_memalign);
    }
}

struct nm_ibverbs_hca_s*nm_ibverbs_hca_from_context(puk_context_t context)
{
  if(context == NULL)
    NM_FATAL("nmad: ibverbs- no context for component.\n");
  const char*device = puk_context_getattr(context, "ibv_device");
  const char*s_port = puk_context_getattr(context, "ibv_port");
  if(device == NULL || s_port == NULL)
    NM_FATAL("nmad: ibverbs- cannot find ibv_device/ibv_port attributes.\n");
  const int port = (strcmp(s_port, "auto") == 0) ? 1 : atoi(s_port);
  struct nm_ibverbs_hca_s*p_hca = nm_ibverbs_hca_resolve(device, port);
  return p_hca;
}

struct nm_ibverbs_hca_s*nm_ibverbs_hca_resolve(const char*device, int port)
{
  if(nm_ibverbs_common.hca_table == NULL)
    nm_ibverbs_common_init();
  if(port == -1)
    port = 1;
  struct nm_ibverbs_hca_key_s key = { .device = device, .port = port };
  struct nm_ibverbs_hca_s*p_hca = puk_hashtable_lookup(nm_ibverbs_common.hca_table, &key);
  if(p_hca)
    {
      p_hca->refcount++;
      return p_hca;
    }
  const int autodetect = (strcmp(device, "auto") == 0);
  
  p_hca = malloc(sizeof(struct nm_ibverbs_hca_s));

  /* find IB device */
  int dev_amount = -1;
  int dev_auto = 0;
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: no device found.\n");
      abort();
    }
 retry_autodetect:
  if(autodetect)
    {
      p_hca->ib_dev = dev_list[dev_auto];
    }
  else
    {
      struct ibv_device**dev_ptr = dev_list;
      p_hca->ib_dev = NULL;
      while((p_hca->ib_dev == NULL) && (*dev_ptr != NULL))
	{
	  struct ibv_device*ib_dev = *dev_ptr;
	  const char*ib_devname = ibv_get_device_name(ib_dev);
	  if(strcmp(ib_devname, device) == 0)
	    {
	      p_hca->ib_dev = ib_dev;
	      break;
	    }
	  dev_ptr++;
	}
    }
  if(p_hca->ib_dev == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot find required device %s.\n", device);
      abort();
    }
  
  /* open IB context */
  p_hca->context = ibv_open_device(p_hca->ib_dev);
  if(p_hca->context == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot open IB context.\n");
      abort();
    }

  /* get IB context attributes */
  struct ibv_device_attr device_attr;
  int rc = ibv_query_device(p_hca->context, &device_attr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get device capabilities.\n");
      abort();
    }

  /* detect LID */
  struct ibv_port_attr port_attr;
  rc = ibv_query_port(p_hca->context, port, &port_attr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: dev = %s; port = %d; cannot get local port attributes (%s).\n",
	      ibv_get_device_name(p_hca->ib_dev), port, strerror(rc));
      abort();
    }
  if(port_attr.state != IBV_PORT_ACTIVE)
    {
      fprintf(stderr, "# nmad: ibverbs- dev = %s; port = %d; port is down.\n",
	      ibv_get_device_name(p_hca->ib_dev), port);
      if(autodetect)
	{
	  if(port < device_attr.phys_port_cnt)
	    port++;
	  else
	    dev_auto++;
	  if(dev_list[dev_auto] == NULL)
	    {
	      NM_FATAL("ibverbs: cannot find active IB device.\n");
	    }
	  goto retry_autodetect;
	}
      
    }
  p_hca->lid = port_attr.lid;
  if(p_hca->lid == 0)
    {
      fprintf(stderr, "nmad- WARNING- ibverbs: LID is null- subnet manager has probably crashed.\n");
    }

  /* IB capabilities */
  p_hca->ib_caps.max_qp        = device_attr.max_qp;
  p_hca->ib_caps.max_qp_wr     = device_attr.max_qp_wr;
  p_hca->ib_caps.max_cq        = device_attr.max_cq;
  p_hca->ib_caps.max_cqe       = device_attr.max_cqe;
  p_hca->ib_caps.max_mr        = device_attr.max_mr;
  p_hca->ib_caps.max_mr_size   = device_attr.max_mr_size;
  p_hca->ib_caps.page_size_cap = device_attr.page_size_cap;
  p_hca->ib_caps.max_msg_size  = port_attr.max_msg_sz;
  
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
    case 0x01: link_rate = 2;  s_link_rate = "SDR";   break; /* 2.5 Gbps on the wire + 8/10 encoding */
    case 0x02: link_rate = 4;  s_link_rate = "DDR";   break; /* 5 Gbps on the wire + 8/10 encoding  */
    case 0x04: link_rate = 8;  s_link_rate = "QDR";   break; /* 10 Gbps on the wire + 8/10 encoding  */
    case 0x08: link_rate = 10; s_link_rate = "FDR10"; break; /* 64/66 encoding */
    case 0x10: link_rate = 14; s_link_rate = "FDR";   break; /* 64/66 encoding */
    case 0x20: link_rate = 25; s_link_rate = "EDR";   break; /* 64/66 encoding */
    }
  p_hca->ib_caps.data_rate = link_width * link_rate;

  NM_DISPF("# nmad ibverbs: device '%s'- %dx %s (%d Gb/s); LID = 0x%02X\n",
	   ibv_get_device_name(p_hca->ib_dev), link_width, s_link_rate, p_hca->ib_caps.data_rate, p_hca->lid);
#ifdef DEBUG
  NM_DISPF("# nmad ibverbs:   max_qp=%d; max_qp_wr=%d; max_cq=%d; max_cqe=%d;\n",
	   p_hca->ib_caps.max_qp, p_hca->ib_caps.max_qp_wr,
	   p_hca->ib_caps.max_cq, p_hca->ib_caps.max_cqe);
  NM_DISPF("# nmad ibverbs:   max_mr=%d; max_mr_size=%llu; page_size_cap=%llu; max_msg_size=%llu\n",
	   p_hca->ib_caps.max_mr,
	   (unsigned long long) p_hca->ib_caps.max_mr_size,
	   (unsigned long long) p_hca->ib_caps.page_size_cap,
	   (unsigned long long) p_hca->ib_caps.max_msg_size);
#endif

  /* allocate Protection Domain */
  p_hca->pd = ibv_alloc_pd(p_hca->context);
  if(p_hca->pd == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot allocate IB protection domain.\n");
      abort();
    }
  p_hca->refcount = 1;
  p_hca->key.device = strdup(device);
  p_hca->key.port = port;
  puk_hashtable_insert(nm_ibverbs_common.hca_table, &p_hca->key, p_hca);

  ibv_free_device_list(dev_list);
  
  return p_hca;
}

void nm_ibverbs_hca_get_profile(struct nm_ibverbs_hca_s*p_hca, struct nm_drv_profile_s*p_profile)
{
  /* driver profile encoding */
#ifdef PM2_TOPOLOGY
  static hwloc_topology_t __topology = NULL;
  if(__topology == NULL)
    {
      hwloc_topology_init(&__topology);
      hwloc_topology_load(__topology);
    }
  p_profile->cpuset = hwloc_bitmap_alloc();
  int rc = hwloc_ibv_get_device_cpuset(__topology, p_hca->ib_dev, p_profile->cpuset);
  if(rc)
    {
      fprintf(stderr, "# nmad: ibverbs- error while detecting ibv device location.\n");
      hwloc_bitmap_copy(p_profile->cpuset, hwloc_topology_get_complete_cpuset(__topology));
    }
#endif /* PM2_TOPOLOGY */
  p_profile->latency = 1200; /* from sampling */
  p_profile->bandwidth = 1024 * (p_hca->ib_caps.data_rate / 8) * 0.75; /* empirical estimation of software+protocol overhead */
}

void nm_ibverbs_hca_release(struct nm_ibverbs_hca_s*p_hca)
{
  assert(p_hca->refcount > 0);
  p_hca->refcount--;
  if(p_hca->refcount == 0)
    {
      assert(puk_hashtable_probe(nm_ibverbs_common.hca_table, &p_hca->key));
      puk_hashtable_remove(nm_ibverbs_common.hca_table, &p_hca->key);
      ibv_dealloc_pd(p_hca->pd);
      ibv_close_device(p_hca->context);
      free(p_hca);
    }
}


struct nm_ibverbs_cnx*nm_ibverbs_cnx_new(struct nm_ibverbs_hca_s*p_hca)
{
  struct nm_ibverbs_cnx*p_ibverbs_cnx = padico_malloc(sizeof(struct nm_ibverbs_cnx));
  memset(p_ibverbs_cnx, 0, sizeof(struct nm_ibverbs_cnx));
  nm_ibverbs_cnx_qp_create(p_ibverbs_cnx, p_hca);
  p_ibverbs_cnx->local_addr.lid = p_hca->lid;
  p_ibverbs_cnx->local_addr.qpn = p_ibverbs_cnx->qp->qp_num;
  p_ibverbs_cnx->local_addr.psn = lrand48() & 0xffffff;

  return p_ibverbs_cnx;
}

void nm_ibverbs_cnx_connect(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  nm_ibverbs_cnx_qp_init(p_ibverbs_cnx);
  nm_ibverbs_cnx_qp_rtr(p_ibverbs_cnx);
  nm_ibverbs_cnx_qp_rts(p_ibverbs_cnx);
}

void nm_ibverbs_cnx_close(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  ibv_destroy_qp(p_ibverbs_cnx->qp);
  ibv_destroy_cq(p_ibverbs_cnx->if_cq);
  ibv_destroy_cq(p_ibverbs_cnx->of_cq);
  free(p_ibverbs_cnx);
}

void nm_ibverbs_cnx_sync(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  volatile int*buffer = (void*)p_ibverbs_cnx->local_addr.segment.raddr;
  buffer[0] = 0; /* recv buffer */
  buffer[1] = 1; /* send buffer */

  for(;;)
    {
      /* check connection */
      int rc = nm_ibverbs_sync_send((void*)&buffer[1], (void*)&buffer[0], sizeof(int), p_ibverbs_cnx);
      if(rc)
	{
	  fprintf(stderr, "nmad: WARNING- ibverbs: connection failed to come up before RNR timeout; reset QP and try again.\n");
	  /* nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1); */
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
	      /*  nm_ibverbs_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1); */
	    }
	}
      else
	{
	  /* connection ok- escape from loop */
	  break;
	}
    }
}


/* ********************************************************* */
/* ** state transitions for QP finite-state automaton */

/** create QP and both CQs */
static void nm_ibverbs_cnx_qp_create(struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_hca_s*p_hca)
{
  /* init inbound CQ */
  p_ibverbs_cnx->if_cq = ibv_create_cq(p_hca->context, NM_IBVERBS_RX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->if_cq == NULL) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot create in CQ\n");
      abort();
    }
  /* init outbound CQ */
  p_ibverbs_cnx->of_cq = ibv_create_cq(p_hca->context, NM_IBVERBS_TX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->of_cq == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot create out CQ\n");
      abort();
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
  p_ibverbs_cnx->qp = ibv_create_qp(p_hca->pd, &qp_init_attr);
  if(p_ibverbs_cnx->qp == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: couldn't create QP\n");
      abort();
    }
  p_ibverbs_cnx->max_inline = qp_init_attr.cap.max_inline_data;
  p_ibverbs_cnx->p_hca = p_hca;
}

/** modify QP to state RESET */
static void nm_ibverbs_cnx_qp_reset(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  /* modify QP- step: RTS */
  struct ibv_qp_attr attr =
    {
      .qp_state = IBV_QPS_RESET
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr, IBV_QP_STATE);
  if(rc != 0)
    {
      fprintf(stderr,"nmad: FATAL- ibverbs: failed to modify QP to RESET.\n");
      abort();
    }
}

/** modify QP to state INIT */
static void nm_ibverbs_cnx_qp_init(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state        = IBV_QPS_INIT,
      .pkey_index      = 0,
      .port_num        = p_ibverbs_cnx->p_hca->key.port,
      .qp_access_flags = IBV_ACCESS_REMOTE_WRITE
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_PKEY_INDEX         |
			 IBV_QP_PORT               |
			 IBV_QP_ACCESS_FLAGS);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: failed to modify QP to INIT.\n");
      abort();
    }
}

/** modify QP to state RTR */
static void nm_ibverbs_cnx_qp_rtr(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
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
	.port_num         =  p_ibverbs_cnx->p_hca->key.port
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
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: dev = %s; port = %d; failed to modify QP to RTR (%s)\n",
	      ibv_get_device_name(p_ibverbs_cnx->p_hca->ib_dev), p_ibverbs_cnx->p_hca->key.port, strerror(rc));
      abort();
    }
}

/** modify QP to state RTS */
static void nm_ibverbs_cnx_qp_rts(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state      = IBV_QPS_RTS,
      .timeout       = 2, /* 14 */
      .retry_cnt     = 7,  /* 7 */
      .rnr_retry     = 7,  /* 7 = unlimited */
      .sq_psn        = p_ibverbs_cnx->local_addr.psn,
      .max_rd_atomic = NM_IBVERBS_RDMA_DEPTH /* 1 */
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_TIMEOUT            |
			 IBV_QP_RETRY_CNT          |
			 IBV_QP_RNR_RETRY          |
			 IBV_QP_SQ_PSN             |
			 IBV_QP_MAX_QP_RD_ATOMIC);
  if(rc != 0)
    {
      fprintf(stderr,"nmad: FATAL-  ibverbs: failed to modify QP to RTS\n");
      abort();
    }
}


/** RDMA used to synchronize connection establishment
 */
static int nm_ibverbs_sync_send(const void*sbuf, const void*rbuf, int size, struct nm_ibverbs_cnx*cnx)
{
  const int roffset = ((uintptr_t)rbuf) - cnx->local_addr.segment.raddr;
  assert(rbuf >= (void*)cnx->local_addr.segment.raddr);
  struct ibv_sge list = {
    .addr   = (uintptr_t)sbuf,
    .length = size,
    .lkey   = cnx->local_addr.segment.rkey
  };
  struct ibv_send_wr wr = {
    .wr_id      = NM_IBVERBS_WRID_SYNC,
    .sg_list    = &list,
    .num_sge    = 1,
    .opcode     = IBV_WR_RDMA_WRITE,
    .send_flags = IBV_SEND_SIGNALED | IBV_SEND_INLINE,
    .next       = NULL,
    .imm_data   = 0,
    .wr.rdma =
    {
      .remote_addr = cnx->remote_addr.segment.raddr + roffset,
      .rkey        = cnx->remote_addr.segment.rkey
    }
  };
  struct ibv_send_wr*bad_wr = NULL;
  int rc = ibv_post_send(cnx->qp, &wr, &bad_wr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: post sync send failed.\n");
      abort();
    }
  struct ibv_wc wc;
  int ne = 0;
  do
    {
      ne = ibv_poll_cq(cnx->of_cq, 1, &wc);
      if(ne < 0)
	{
	  fprintf(stderr, "nmad: FATAL- ibverbs: poll out CQ failed.\n");
	  abort();
	}
    }
  while(ne == 0);
  if(ne != 1 || wc.status != IBV_WC_SUCCESS)
    {
      fprintf(stderr, "nmad: WARNING- ibverbs: WC send failed (status=%d; %s)\n",
	      wc.status, nm_ibverbs_status_strings[wc.status]);
      return 1;
    }
  return 0;
}



/* ** checksum ********************************************* */


/** checksum algorithm. Set NMAD_IBVERBS_CHECKSUM to non-null to enable checksums.
 */
uint32_t nm_ibverbs_checksum(const char*data, nm_len_t len)
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

uint32_t nm_ibverbs_memcpy_and_checksum(void*_dest, const void*_src, nm_len_t len)
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
  return 1;
}

uint32_t nm_ibverbs_copy_from_and_checksum(void*dest, nm_data_slicer_t*p_slicer, const void*src, nm_len_t offset, nm_len_t len)
{
  assert(!(nm_data_slicer_isnull(p_slicer) && (src == NULL)));
  assert(!(!nm_data_slicer_isnull(p_slicer) && (src != NULL)));
  if(_nm_ibverbs_checksum && !nm_data_slicer_isnull(p_slicer))
    {
      padico_fatal("ibverbs: FATAL- checksums not supported yet with nm_data.\n");
    }
  if(!nm_data_slicer_isnull(p_slicer))
    {
      nm_data_slicer_copy_from(p_slicer, dest, len);
      return 1;
    }
  else
    {
      return nm_ibverbs_memcpy_and_checksum(dest, src + offset, len);
    }
}

uint32_t nm_ibverbs_copy_to_and_checksum(const void*src, nm_data_slicer_t*p_slicer, void*dest, nm_len_t offset, nm_len_t len)
{
  assert(!(nm_data_slicer_isnull(p_slicer) && (dest == NULL)));
  assert(!((!nm_data_slicer_isnull(p_slicer)) && (dest != NULL)));
  if(_nm_ibverbs_checksum && (p_slicer->p_data != NULL))
    {
      padico_fatal("ibverbs: FATAL- checksums not supported yet with nm_data.\n");
    }
  if(!nm_data_slicer_isnull(p_slicer))
    {
      nm_data_slicer_copy_to(p_slicer, src, len);
      return 1;
    }
  else
    {
      return nm_ibverbs_memcpy_and_checksum(dest + offset, src, len);
    }
}
