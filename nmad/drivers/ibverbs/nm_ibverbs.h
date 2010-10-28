/*
 * NewMadeleine
 * Copyright (C) 2010(see AUTHORS file)
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

#ifndef NM_IBVERBS_H
#define NM_IBVERBS_H

#include <nm_private.h>
#include <infiniband/verbs.h>

/** enable checksum in bycopy/adaptrdma methods (*not* rcache) */
#define NM_IBVERBS_CHECKSUM

#define nm_ibverbs_checksum tbx_checksum_block64


/** list of WRIDs used in the driver. */
enum {
  _NM_IBVERBS_WRID_NONE = 0,
  NM_IBVERBS_WRID_ACK,    /**< ACK for bycopy */
  NM_IBVERBS_WRID_RDMA,   /**< data for bycopy */
  NM_IBVERBS_WRID_RDV,    /**< rdv for regrdma */
  NM_IBVERBS_WRID_DATA,   /**< data for regrdma */
  NM_IBVERBS_WRID_HEADER, /**< headers for regrdma */
  NM_IBVERBS_WRID_PACKET, /**< packet for adaptrdma */
  NM_IBVERBS_WRID_RECV,
  NM_IBVERBS_WRID_SEND,
  NM_IBVERBS_WRID_READ,
  _NM_IBVERBS_WRID_MAX
};

enum nm_ibverbs_cnx_kind
  {
    NM_IBVERBS_CNX_NONE      = 0,
    NM_IBVERBS_CNX_BYCOPY    = 0x0001,
    NM_IBVERBS_CNX_REGRDMA   = 0x0002,
    NM_IBVERBS_CNX_ADAPTRDMA = 0x0004,
    NM_IBVERBS_CNX_RCACHE    = 0x0008,
    NM_IBVERBS_CNX_LR2       = 0x0010,
    NM_IBVERBS_CNX_AUTO      = 0x0100
  };

struct nm_ibverbs_segment 
{
  enum nm_ibverbs_cnx_kind kind;
  uint64_t raddr;
  uint32_t rkey;
};

struct nm_ibverbs_cnx_addr 
{
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  struct nm_ibverbs_segment segments[16];
  int n;
};

/** Global state of the HCA */
struct nm_ibverbs_drv 
{
  struct ibv_device*ib_dev;   /**< IB device */
  struct ibv_context*context; /**< global IB context */
  struct ibv_pd*pd;           /**< global IB protection domain */
  uint16_t lid;               /**< local IB LID */
  struct nm_ibverbs_trk*trks_array; /**< tracks of the driver*/
  int nb_trks;                      /**< number of tracks */
  struct
  {
    int max_qp;               /**< maximum number of QP */
    int max_qp_wr;            /**< maximum number of WR per QP */
    int max_cq;               /**< maximum number of CQ */
    int max_cqe;              /**< maximum number of entries per CQ */
    int max_mr;               /**< maximum number of MR */
    uint64_t max_mr_size;     /**< maximum size for a MR */
    uint64_t page_size_cap;   /**< maximum page size for device */
    uint64_t max_msg_size;    /**< maximum message size */
  } ib_caps;
  char*url;                   /**< Infiniband url for this node */
  struct nm_drv_cap caps;     /**< capabilities */
  int server_sock;            /**< socket used for connection */
};

struct nm_ibverbs_cnx;

struct nm_ibverbs_method_iface_s
{
  void (*cnx_create)(void*_status, struct nm_ibverbs_cnx*cnx, struct nm_ibverbs_drv*p_ibverbs_drv);
  void (*addr_pack)(void*_status,  struct nm_ibverbs_cnx_addr*addr);
  void (*addr_unpack)(void*_status, struct nm_ibverbs_cnx_addr*addr);
  void (*send_post)(void*_status, const struct iovec*v, int n);
  int  (*send_poll)(void*_status);
  void (*recv_init)(void*_status, struct iovec*v, int n);
  int  (*poll_one)(void*_status);
  int  (*poll_any)(struct nm_pkt_wrap*p_pw, struct nm_gate**pp_gate);
  int  (*cancel_recv)(void*_status); 
};
PUK_IFACE_TYPE(NewMad_ibverbs_method, struct nm_ibverbs_method_iface_s);


/** an IB connection for a given trk/gate pair */
struct nm_ibverbs_cnx
{
  struct nm_ibverbs_cnx_addr local_addr;
  struct nm_ibverbs_cnx_addr remote_addr;
  struct ibv_qp*qp;       /**< QP for this connection */
  struct ibv_cq*of_cq;    /**< CQ for outgoing packets */
  struct ibv_cq*if_cq;    /**< CQ for incoming packets */
  int max_inline;         /**< max size of data for IBV inline RDMA */
  struct
  {
    int total;
    int wrids[_NM_IBVERBS_WRID_MAX];
  } pending;              /**< count of pending packets */
  struct puk_receptacle_NewMad_ibverbs_method_s method;
  puk_instance_t method_instance;
};



/* ** RDMA toolbox ***************************************** */


/** messages for Work Completion status  */
static const char*const nm_ibverbs_status_strings[] =
  {
    [IBV_WC_SUCCESS]            = "success",
    [IBV_WC_LOC_LEN_ERR]        = "local length error",
    [IBV_WC_LOC_QP_OP_ERR]      = "local QP op error",
    [IBV_WC_LOC_EEC_OP_ERR]     = "local EEC op error",
    [IBV_WC_LOC_PROT_ERR]       = "local protection error",
    [IBV_WC_WR_FLUSH_ERR]       = "write flush error",
    [IBV_WC_MW_BIND_ERR]        = "MW bind error",
    [IBV_WC_BAD_RESP_ERR]       = "bad response error",
    [IBV_WC_LOC_ACCESS_ERR]     = "local access error",
    [IBV_WC_REM_INV_REQ_ERR]    = "remote invalid request error",
    [IBV_WC_REM_ACCESS_ERR]     = "remote access error",
    [IBV_WC_REM_OP_ERR]         = "remote op error",
    [IBV_WC_RETRY_EXC_ERR]      = "retry exceded error",
    [IBV_WC_RNR_RETRY_EXC_ERR]  = "RNR retry exceded error",
    [IBV_WC_LOC_RDD_VIOL_ERR]   = "local RDD violation error",
    [IBV_WC_REM_INV_RD_REQ_ERR] = "remote invalid read request error",
    [IBV_WC_REM_ABORT_ERR]      = "remote abort error",
    [IBV_WC_INV_EECN_ERR]       = "invalid EECN error",
    [IBV_WC_INV_EEC_STATE_ERR]  = "invalid EEC state error",
    [IBV_WC_FATAL_ERR]          = "fatal error",
    [IBV_WC_RESP_TIMEOUT_ERR]   = "response timeout error",
    [IBV_WC_GENERAL_ERR]        = "general error"
  };

static inline int nm_ibverbs_do_rdma(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
				     const void*__restrict__ buf, int size, uint64_t raddr,
				     int opcode, int flags, uint32_t lkey, uint32_t rkey, uint64_t wrid)
{
  struct ibv_sge list = {
    .addr   = (uintptr_t)buf,
    .length = size,
    .lkey   = lkey
  };
  struct ibv_send_wr wr = {
    .wr_id      = wrid,
    .sg_list    = &list,
    .num_sge    = 1,
    .opcode     = opcode,
    .send_flags = flags,
    .next       = NULL,
    .imm_data   = 0,
    .wr.rdma =
    {
      .remote_addr = raddr,
      .rkey        = rkey
    }
  };
  struct ibv_send_wr*bad_wr = NULL;
  int rc = ibv_post_send(p_ibverbs_cnx->qp, &wr, &bad_wr);
  assert(wrid < _NM_IBVERBS_WRID_MAX);
  p_ibverbs_cnx->pending.wrids[wrid]++;
  p_ibverbs_cnx->pending.total++;
  if(rc) {
    fprintf(stderr, "Infiniband: post RDMA write failed (rc=%d).\n", rc);
    return -NM_EUNKNOWN;
  }
  return NM_ESUCCESS;
}

static inline int nm_ibverbs_rdma_send(struct nm_ibverbs_cnx*p_ibverbs_cnx, int size,
				       const void*__restrict__ ptr,
				       const void*__restrict__ _raddr,
				       const void*__restrict__ _lbase,
				       const struct nm_ibverbs_segment*seg,
				       const struct ibv_mr*__restrict__ mr,
				       int wrid)
{
  const uintptr_t _rbase = seg->raddr;
  const uint64_t raddr   = (uint64_t)(((uintptr_t)_raddr - (uintptr_t)_lbase) + _rbase);
  struct ibv_sge list = {
    .addr   = (uintptr_t)ptr,
    .length = size,
    .lkey   = mr->lkey
  };
  struct ibv_send_wr wr = {
    .wr_id      = wrid,
    .sg_list    = &list,
    .num_sge    = 1,
    .opcode     = IBV_WR_RDMA_WRITE,
    .send_flags = (size < p_ibverbs_cnx->max_inline) ? (IBV_SEND_INLINE | IBV_SEND_SIGNALED) : IBV_SEND_SIGNALED,
    .wr.rdma =
    {
      .remote_addr = raddr,
      .rkey        = seg->rkey
    }
  };
  struct ibv_send_wr*bad_wr = NULL;
  int rc = ibv_post_send(p_ibverbs_cnx->qp, &wr, &bad_wr);
  assert(wrid < _NM_IBVERBS_WRID_MAX);
  p_ibverbs_cnx->pending.wrids[wrid]++;
  p_ibverbs_cnx->pending.total++;
  if(rc) {
    fprintf(stderr, "Infiniband: post RDMA send failed.\n");
    return -NM_EUNKNOWN;
  }
  return NM_ESUCCESS;
}

static inline int nm_ibverbs_rdma_poll(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  struct ibv_wc wc;
  int ne = 0, done = 0;
  do {
    ne = ibv_poll_cq(p_ibverbs_cnx->of_cq, 1, &wc);
    if(ne > 0 && wc.status == IBV_WC_SUCCESS)
      {
	assert(wc.wr_id < _NM_IBVERBS_WRID_MAX);
	p_ibverbs_cnx->pending.wrids[wc.wr_id]--;
	p_ibverbs_cnx->pending.total--;
	done += ne;
      }
    else if(ne > 0)
      {
	fprintf(stderr, "Infiniband: WC send failed- status=%d (%s)\n",
		wc.status, nm_ibverbs_status_strings[wc.status]);
	TBX_FAILURE("Infiniband: error while sending.\n");
	return -NM_EUNKNOWN;
      }
    else if(ne < 0)
      {
	fprintf(stderr, "Infiniband: WC polling failed.\n");
	return -NM_EUNKNOWN;
      }
  } while(ne > 0);
  return (done > 0) ? NM_ESUCCESS : -NM_EAGAIN;
}

static inline void nm_ibverbs_send_flush(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx, uint64_t wrid)
{
  while(p_ibverbs_cnx->pending.wrids[wrid])
    {
      nm_ibverbs_rdma_poll(p_ibverbs_cnx);
    }
}

static inline void nm_ibverbs_send_flushn_total(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx, int n)
{
  while(p_ibverbs_cnx->pending.total > n) {
    nm_ibverbs_rdma_poll(p_ibverbs_cnx);
  }
}

static inline void nm_ibverbs_send_flushn(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx, int wrid, int n)
{
  while(p_ibverbs_cnx->pending.wrids[wrid] > n)
    {
      nm_ibverbs_rdma_poll(p_ibverbs_cnx);
    }
}


#endif /* NM_IBVERBS_H */

