/*
 * NewMadeleine
 * Copyright (C) 2011-2013 (see AUTHORS file)
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

#include "nm_ibverbs.h"

#include <Padico/Module.h>

PADICO_MODULE_HOOK(NewMad_ibverbs);


static struct
{
  puk_hashtable_t hca_table; /**< HCAs, hashed by index (as int) */
} nm_ibverbs_common = { .hca_table = NULL };


static void nm_ibverbs_cnx_qp_create(struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_hca_s*p_hca);
static void nm_ibverbs_cnx_qp_reset(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_init(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_rtr(struct nm_ibverbs_cnx*p_ibverbs_cnx);
static void nm_ibverbs_cnx_qp_rts(struct nm_ibverbs_cnx*p_ibverbs_cnx);

static int nm_ibverbs_sync_send(const void*sbuf, const void*rbuf, int size, struct nm_ibverbs_cnx*cnx);

/* ********************************************************* */

struct nm_ibverbs_hca_s*nm_ibverbs_hca_resolve(int index)
{
  if(nm_ibverbs_common.hca_table == NULL)
    nm_ibverbs_common.hca_table = puk_hashtable_new_int();
  struct nm_ibverbs_hca_s*p_hca = puk_hashtable_lookup(nm_ibverbs_common.hca_table, (void*)(uintptr_t)(index + 1));
  if(p_hca)
    return p_hca;

  p_hca = TBX_MALLOC(sizeof(struct nm_ibverbs_hca_s));

  /* find IB device */
  int dev_number = index;
  int dev_amount;
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: no device found.\n");
      abort();
    }
  if (dev_number >= dev_amount)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: device #%d/%d not available.\n", dev_number, dev_amount);
      abort();
    }
  p_hca->ib_dev = dev_list[dev_number];
  
  /* open IB context */
  p_hca->context = ibv_open_device(p_hca->ib_dev);
  if(p_hca->context == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot open IB context.\n");
      abort();
    }

  ibv_free_device_list(dev_list);
  
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
  rc = ibv_query_port(p_hca->context, NM_IBVERBS_PORT, &port_attr);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get local port attributes.\n");
      abort();
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
    case 0x01: link_rate = 2;  s_link_rate = "SDR"; break;
    case 0x02: link_rate = 4;  s_link_rate = "DDR"; break;
    case 0x04: link_rate = 8;  s_link_rate = "QDR"; break;
    case 0x08: link_rate = 14; s_link_rate = "FDR"; break;
    case 0x10: link_rate = 25; s_link_rate = "EDR"; break;
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

  puk_hashtable_insert(nm_ibverbs_common.hca_table, (void*)(uintptr_t)(index + 1), p_hca);
  return p_hca;
}

#ifdef PM2_NUIOA

#define NM_IBVERBS_NUIOA_SYSFILE_LENGTH 16

int nm_ibverbs_hca_get_numa_node(struct nm_ibverbs_hca_s*p_hca)
{
  struct ibv_device*ib_dev = p_hca->ib_dev;
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
      .port_num        = NM_IBVERBS_PORT,
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
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: failed to modify QP to RTR\n");
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
