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


#include "nm_dcfa.h"

#include <Padico/Module.h>
#ifdef PM2_TOPOLOGY
#include <tbx_topology.h>
#include <hwloc/openfabrics-verbs.h>
#endif /* PM2_TOPOLOGY */

PADICO_MODULE_DECLARE(NewMad_dcfa_common);

#define NM_DCFA_HCA_MAX 16
static struct
{
  int init_done;
  struct nm_dcfa_hca_s*hca_table[NM_DCFA_HCA_MAX]; /**< HCAs, hashed by index (as int) */
} nm_dcfa_common = { .init_done = 0, .hca_table = { NULL } };

static tbx_checksum_t _nm_dcfa_checksum = NULL;

/** ptr alignment enforced in every packets (through padding) */
int nm_dcfa_alignment = 64;
/** ptr alignment enforced for buffer allocation */
int nm_dcfa_memalign  = 4096;

/* ********************************************************* */

static void nm_dcfa_cnx_qp_create(struct nm_dcfa_cnx*p_ibverbs_cnx, struct nm_dcfa_hca_s*p_hca);
static void nm_dcfa_cnx_qp_reset(struct nm_dcfa_cnx*p_ibverbs_cnx);
static void nm_dcfa_cnx_qp_init(struct nm_dcfa_cnx*p_ibverbs_cnx);
static void nm_dcfa_cnx_qp_rtr(struct nm_dcfa_cnx*p_ibverbs_cnx);
static void nm_dcfa_cnx_qp_rts(struct nm_dcfa_cnx*p_ibverbs_cnx);

static int nm_dcfa_sync_send(const void*sbuf, const void*rbuf, int size, struct nm_dcfa_cnx*cnx);

/* ********************************************************* */

static void nm_dcfa_common_init(void)
{
  nm_dcfa_common.init_done = 1;
  const char*checksum_env = getenv("NMAD_IBVERBS_CHECKSUM");
  if(_nm_dcfa_checksum == NULL && checksum_env != NULL)
    {
      tbx_checksum_t checksum = tbx_checksum_get(checksum_env);
      if(checksum == NULL)
	NM_FATAL("# nmad: checksum algorithm *%s* not available.\n", checksum_env);
      _nm_dcfa_checksum = checksum;
      NM_DISPF("# nmad dcfa: checksum enabled (%s).\n", checksum->name);
    }
  const char*align_env = getenv("NMAD_IBVERBS_ALIGN");
  if(align_env != NULL)
    {
      nm_dcfa_alignment = atoi(align_env);
      NM_DISPF("# nmad dcfa: alignment forced to %d\n", nm_dcfa_alignment);
    }
  const char*memalign_env = getenv("NMAD_IBVERBS_MEMALIGN");
  if(memalign_env != NULL)
    {
      nm_dcfa_memalign = atoi(memalign_env);
      NM_DISPF("# nmad dcfa: memalign forced to %d\n", nm_dcfa_memalign);
    }
}

struct nm_dcfa_hca_s*nm_dcfa_hca_resolve(int index)
{
  if(index < 0)
    index = 0;
  if(index >= NM_DCFA_HCA_MAX)
    {
      fprintf(stderr, "# nmad: FATAL- dcfa: more than %d HCA in table.\n", NM_DCFA_HCA_MAX);
      abort();
    }
  if(!nm_dcfa_common.init_done)
    {
      nm_dcfa_common_init();
    }
  struct nm_dcfa_hca_s*p_hca = nm_dcfa_common.hca_table[index];
  if(p_hca)
    return p_hca;
  p_hca = TBX_MALLOC(sizeof(struct nm_dcfa_hca_s));

  /* find IB device */
  int dev_number = index;
  int dev_amount = 1;
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) 
    {
      fprintf(stderr, "nmad: FATAL- dcfa: no device found.\n");
      abort();
    }
  if (dev_number >= dev_amount)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: device #%d/%d not available.\n", dev_number, dev_amount);
      abort();
    }
  p_hca->ib_dev = dev_list[dev_number];
  
  /* open IB context */
  p_hca->context = ibv_open_device(p_hca->ib_dev);
  if(p_hca->context == NULL)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot open IB context.\n");
      abort();
    }

  ibv_free_device_list(dev_list);
  
  /* get IB context attributes */
  /*  struct ibv_device_attr device_attr; 
  int rc = ibv_query_device(p_hca->context, NULL);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot get device capabilities.\n");
      abort();
    }
  */
  /* detect LID */
  /* struct ibv_port_attr port_attr;
  rc = ibv_query_port(p_hca->context, NM_DCFA_PORT, NULL);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot get local port attributes.\n");
      abort();
    }
  */
  p_hca->lid = p_hca->context->lid; /* port_attr.lid; */
  if(p_hca->lid == 0)
    {
      fprintf(stderr, "nmad- WARNING- dcfa: LID is null- subnet manager has probably crashed.\n");
    }

  /* IB capabilities */
  /*
  p_hca->ib_caps.max_qp        = device_attr.max_qp;
  p_hca->ib_caps.max_qp_wr     = device_attr.max_qp_wr;
  p_hca->ib_caps.max_cq        = device_attr.max_cq;
  p_hca->ib_caps.max_cqe       = device_attr.max_cqe;
  p_hca->ib_caps.max_mr        = device_attr.max_mr;
  p_hca->ib_caps.max_mr_size   = device_attr.max_mr_size;
  p_hca->ib_caps.page_size_cap = device_attr.page_size_cap;
  p_hca->ib_caps.max_msg_size  = port_attr.max_msg_sz;
  */
  int link_width = -1;
  /*
  switch(port_attr.active_width)
    {
    case 1: link_width = 1;  break;
    case 2: link_width = 4;  break;
    case 4: link_width = 8;  break;
    case 8: link_width = 12; break;
    }
  */
  int link_rate = -1;
  const char*s_link_rate = "unknown";
  /*
  switch(port_attr.active_speed)
    {
    case 0x01: link_rate = 2;  s_link_rate = "SDR"; break;
    case 0x02: link_rate = 4;  s_link_rate = "DDR"; break;
    case 0x04: link_rate = 8;  s_link_rate = "QDR"; break;
    case 0x08: link_rate = 14; s_link_rate = "FDR"; break;
    case 0x10: link_rate = 25; s_link_rate = "EDR"; break;
    }
  */
  p_hca->ib_caps.data_rate = link_width * link_rate;

  NM_DISPF("# nmad dcfa: device '%s'- %dx %s (%d Gb/s); LID = 0x%02X\n",
	   "dcfa" /*ibv_get_device_name(p_hca->ib_dev)*/, link_width, s_link_rate, p_hca->ib_caps.data_rate, p_hca->lid);
#ifdef DEBUG
  /*
  NM_DISPF("# nmad dcfa:   max_qp=%d; max_qp_wr=%d; max_cq=%d; max_cqe=%d;\n",
	   p_hca->ib_caps.max_qp, p_hca->ib_caps.max_qp_wr,
	   p_hca->ib_caps.max_cq, p_hca->ib_caps.max_cqe);
  NM_DISPF("# nmad dcfa:   max_mr=%d; max_mr_size=%llu; page_size_cap=%llu; max_msg_size=%llu\n",
	   p_hca->ib_caps.max_mr,
	   (unsigned long long) p_hca->ib_caps.max_mr_size,
	   (unsigned long long) p_hca->ib_caps.page_size_cap,
	   (unsigned long long) p_hca->ib_caps.max_msg_size);
  */
#endif

  /* allocate Protection Domain */
  p_hca->pd = ibv_alloc_pd(p_hca->context);
  if(p_hca->pd == NULL)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot allocate IB protection domain.\n");
      abort();
    }
  nm_dcfa_common.hca_table[index] = p_hca;
  return p_hca;
}


void nm_dcfa_hca_get_profile(int index, struct nm_drv_profile_s*p_profile)
{
  struct nm_dcfa_hca_s*p_hca = nm_dcfa_hca_resolve(index);
  /* driver profile encoding */
#ifdef PM2_TOPOLOGY
  int rc = hwloc_ibv_get_device_cpuset(topology, p_hca->ib_dev, p_profile->cpuset);
  if(rc)
    {
      fprintf(stderr, "# nmad: ibverbs- error while detecting ibv device location.\n");
      hwloc_bitmap_copy(p_profile->cpuset, hwloc_topology_get_complete_cpuset(topology));
    }
#endif /* PM2_TOPOLOGY */
  p_profile->latency = 1350; /* from sampling */
  p_profile->bandwidth = 1024 * (p_hca->ib_caps.data_rate / 8) * 0.75; /* empirical estimation of software+protocol overhead */
}

struct nm_dcfa_cnx*nm_dcfa_cnx_new(struct nm_dcfa_hca_s*p_hca)
{
  struct nm_dcfa_cnx*p_ibverbs_cnx = padico_malloc(sizeof(struct nm_dcfa_cnx));
  memset(p_ibverbs_cnx, 0, sizeof(struct nm_dcfa_cnx));
  nm_dcfa_cnx_qp_create(p_ibverbs_cnx, p_hca);
  p_ibverbs_cnx->local_addr.lid = p_hca->lid;
  p_ibverbs_cnx->local_addr.qpn = p_ibverbs_cnx->qp->qp_num;
  p_ibverbs_cnx->local_addr.psn = lrand48() & 0xffffff;

  return p_ibverbs_cnx;
}

void nm_dcfa_cnx_connect(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  nm_dcfa_cnx_qp_init(p_ibverbs_cnx);
  nm_dcfa_cnx_qp_rtr(p_ibverbs_cnx);
  nm_dcfa_cnx_qp_rts(p_ibverbs_cnx);
}

#if 0
void nm_dcfa_cnx_sync(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  volatile int*buffer = (void*)p_ibverbs_cnx->local_addr.segment.raddr;
  buffer[0] = 0; /* recv buffer */
  buffer[1] = 1; /* send buffer */

  for(;;)
    {
      /* check connection */
      int rc = nm_dcfa_sync_send((void*)&buffer[1], (void*)&buffer[0], sizeof(int), p_ibverbs_cnx);
      if(rc)
	{
	  fprintf(stderr, "nmad: WARNING- dcfa: connection failed to come up before RNR timeout; reset QP and try again.\n");
	  /* nm_dcfa_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1); */
	  nm_dcfa_cnx_qp_reset(p_ibverbs_cnx);
	  nm_dcfa_cnx_qp_init(p_ibverbs_cnx);
	  nm_dcfa_cnx_qp_rtr(p_ibverbs_cnx);
	  nm_dcfa_cnx_qp_rts(p_ibverbs_cnx);
	}
      else if(buffer[0] == 0)
	{
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);
	  do
	    {
	      TBX_GET_TICK(t2);
	    }
	  while(buffer[0] == 0 && TBX_TIMING_DELAY(t1, t2) < NM_DCFA_TIMEOUT_CHECK);
	  if(buffer[0] == 0)
	    {
	      /*  nm_dcfa_connect_send(p_ibverbs_drv, remote_url, trk_id, &p_ibverbs_cnx->local_addr, 1); */
	    }
	}
      else
	{
	  /* connection ok- escape from loop */
	  break;
	}
    }
}
#endif /* 0 */


/* ********************************************************* */
/* ** state transitions for QP finite-state automaton */

/** create QP and both CQs */
static void nm_dcfa_cnx_qp_create(struct nm_dcfa_cnx*p_ibverbs_cnx, struct nm_dcfa_hca_s*p_hca)
{
  /* init inbound CQ */
  p_ibverbs_cnx->if_cq = ibv_create_cq(p_hca->context, NM_DCFA_RX_DEPTH);
  if(p_ibverbs_cnx->if_cq == NULL) 
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot create in CQ\n");
      abort();
    }
  /* init outbound CQ */
  p_ibverbs_cnx->of_cq = ibv_create_cq(p_hca->context, NM_DCFA_TX_DEPTH);
  if(p_ibverbs_cnx->of_cq == NULL)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: cannot create out CQ\n");
      abort();
    }
  /* create QP */
  struct ibv_qp_init_attr qp_init_attr = {
    .send_cq = p_ibverbs_cnx->of_cq,
    .recv_cq = p_ibverbs_cnx->if_cq,
    .cap     = {
      .max_send_wr     = NM_DCFA_TX_DEPTH,
      .max_recv_wr     = NM_DCFA_RX_DEPTH,
      .max_send_sge    = NM_DCFA_MAX_SG_SQ,
      .max_recv_sge    = NM_DCFA_MAX_SG_RQ,
      .max_inline_data = NM_DCFA_MAX_INLINE
    },
    .qp_type = IBV_QPT_RC
  };
  p_ibverbs_cnx->qp = ibv_create_qp(p_hca->pd, &qp_init_attr);
  if(p_ibverbs_cnx->qp == NULL)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: couldn't create QP\n");
      abort();
    }
  p_ibverbs_cnx->max_inline = qp_init_attr.cap.max_inline_data;
}

/** modify QP to state RESET */
static void nm_dcfa_cnx_qp_reset(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  /* modify QP- step: RTS */
  struct ibv_qp_attr attr =
    {
      .qp_state = IBV_QPS_RESET
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr, IBV_QP_STATE);
  if(rc != 0)
    {
      fprintf(stderr,"nmad: FATAL- dcfa: failed to modify QP to RESET.\n");
      abort();
    }
}

/** modify QP to state INIT */
static void nm_dcfa_cnx_qp_init(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state        = IBV_QPS_INIT,
      .pkey_index      = 0,
      .port_num        = NM_DCFA_PORT,
      .qp_access_flags = IBV_ACCESS_REMOTE_WRITE
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_PKEY_INDEX         |
			 IBV_QP_PORT               |
			 IBV_QP_ACCESS_FLAGS);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- dcfa: failed to modify QP to INIT.\n");
      abort();
    }
}

/** modify QP to state RTR */
static void nm_dcfa_cnx_qp_rtr(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state           = IBV_QPS_RTR,
      .path_mtu           = NM_DCFA_MTU,
      .dest_qp_num        = p_ibverbs_cnx->remote_addr.qpn,
      .rq_psn             = p_ibverbs_cnx->remote_addr.psn,
      .max_dest_rd_atomic = NM_DCFA_RDMA_DEPTH,
      .min_rnr_timer      = 12, /* 12 */
      .ah_attr            = {
	.is_global        = 0,
	.dlid             = p_ibverbs_cnx->remote_addr.lid,
	.sl               = 0,
	.src_path_bits    = 0,
	.port_num         = NM_DCFA_PORT
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
      fprintf(stderr, "nmad: FATAL- dcfa: failed to modify QP to RTR\n");
      abort();
    }
}

/** modify QP to state RTS */
static void nm_dcfa_cnx_qp_rts(struct nm_dcfa_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state      = IBV_QPS_RTS,
      .timeout       = 2, /* 14 */
      .retry_cnt     = 7,  /* 7 */
      .rnr_retry     = 7,  /* 7 = unlimited */
      .sq_psn        = p_ibverbs_cnx->local_addr.psn,
      .max_rd_atomic = NM_DCFA_RDMA_DEPTH /* 1 */
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
      fprintf(stderr,"nmad: FATAL-  dcfa: failed to modify QP to RTS\n");
      abort();
    }
}


/** RDMA used to synchronize connection establishment
 */
/*
static int nm_dcfa_sync_send(const void*sbuf, const void*rbuf, int size, struct nm_dcfa_cnx*cnx)
{
  const int roffset = ((uintptr_t)rbuf) - cnx->local_addr.segment.raddr;
  assert(rbuf >= (void*)cnx->local_addr.segment.raddr);
  struct ibv_sge list = {
    .addr   = (uintptr_t)sbuf,
    .length = size,
    .lkey   = cnx->local_addr.segment.rkey
  };
  struct ibv_send_wr wr = {
    .wr_id      = NM_DCFA_WRID_SYNC,
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
      fprintf(stderr, "nmad: FATAL- dcfa: post sync send failed.\n");
      abort();
    }
  struct ibv_wc wc;
  int ne = 0;
  do
    {
      ne = ibv_poll_cq(cnx->of_cq, 1, &wc);
      if(ne < 0)
	{
	  fprintf(stderr, "nmad: FATAL- dcfa: poll out CQ failed.\n");
	  abort();
	}
    }
  while(ne == 0);
  if(ne != 1 || wc.status != IBV_WC_SUCCESS)
    {
      fprintf(stderr, "nmad: WARNING- dcfa: WC send failed (status=%d; %s)\n",
	      wc.status, nm_dcfa_status_strings[wc.status]);
      return 1;
    }
  return 0;
}
*/


/* ** checksum ********************************************* */


/** checksum algorithm. Set NMAD_IBVERBS_CHECKSUM to non-null to enable checksums.
 */
uint32_t nm_dcfa_checksum(const char*data, nm_len_t len)
{
  if(_nm_dcfa_checksum)
    return (*_nm_dcfa_checksum->func)(data, len);
  else
    return 0;
}

int nm_dcfa_checksum_enabled(void)
{
  return _nm_dcfa_checksum != NULL;
}

uint32_t nm_dcfa_memcpy_and_checksum(void*_dest, const void*_src, nm_len_t len)
{
  if(_nm_dcfa_checksum)
    {
      if(_nm_dcfa_checksum->checksum_and_copy)
	{
	  return (*_nm_dcfa_checksum->checksum_and_copy)(_dest, _src, len);
	}
      else
	{
	  memcpy(_dest, _src, len);
	  return nm_dcfa_checksum(_dest, len);
	}
    }
  else
    {
      memcpy(_dest, _src, len);
    }
  return 0;
}

