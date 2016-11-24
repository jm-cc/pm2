/*
 * NewMadeleine
 * Copyright (C) 2010-2013 (see AUTHORS file)
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

#ifndef NM_DCFA_H
#define NM_DCFA_H

#include <nm_private.h>
#include <dcfa.h>

/* *** global IB parameters ******************************** */

#define NM_DCFA_PORT         1
#define NM_DCFA_TX_DEPTH     4
#define NM_DCFA_RX_DEPTH     2
#define NM_DCFA_RDMA_DEPTH   4
#define NM_DCFA_MAX_SG_SQ    1
#define NM_DCFA_MAX_SG_RQ    1
#define NM_DCFA_MAX_INLINE   16
#define NM_DCFA_MTU          IBV_MTU_2048

/** timeout to receive connection check after sending ACK and check (in msec.) */
#define NM_DCFA_TIMEOUT_CHECK 20

uint32_t nm_dcfa_checksum(const char*data, nm_len_t len);
uint32_t nm_dcfa_memcpy_and_checksum(void*_dest, const void*_src, nm_len_t len);
int nm_dcfa_checksum_enabled(void);

extern int nm_dcfa_alignment;
extern int nm_dcfa_memalign;

/** list of WRIDs used in the driver. */
enum {
  _NM_DCFA_WRID_NONE = 0,
  NM_DCFA_WRID_ACK,    /**< ACK for bycopy */
  NM_DCFA_WRID_RDMA,   /**< data for bycopy */
  NM_DCFA_WRID_RDV,    /**< rdv for rcache */
  NM_DCFA_WRID_DATA,   /**< data for rcache */
  NM_DCFA_WRID_HEADER, /**< headers for rcache */
  NM_DCFA_WRID_PACKET, /**< packet for adaptrdma */
  NM_DCFA_WRID_RECV,
  NM_DCFA_WRID_SEND,
  NM_DCFA_WRID_READ,
  NM_DCFA_WRID_SYNC,   /**< synchronize connection establishment */
  _NM_DCFA_WRID_MAX
};

/** an RDMA segment. Each method usually use one segment per connection.
 */
struct nm_dcfa_segment 
{
  uint64_t raddr;
  uint32_t rkey;
};

/** the address for a connection (one per track+gate).
 */
struct nm_dcfa_cnx_addr 
{
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  struct nm_dcfa_segment segment;
};


/** Global state of the HCA */
struct nm_dcfa_hca_s
{
  struct ibv_device*ib_dev;   /**< IB device */
  struct ibv_context*context; /**< global IB context */
  struct ibv_pd*pd;           /**< global IB protection domain */
  uint16_t lid;               /**< local IB LID */
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
    int data_rate;
  } ib_caps;
};


/** an IB connection for a given trk/gate pair */
struct nm_dcfa_cnx
{
  struct nm_dcfa_cnx_addr local_addr;
  struct nm_dcfa_cnx_addr remote_addr;
  struct ibv_qp*qp;       /**< QP for this connection */
  struct ibv_cq*of_cq;    /**< CQ for outgoing packets */
  struct ibv_cq*if_cq;    /**< CQ for incoming packets */
  int max_inline;         /**< max size of data for IBV inline RDMA */
  struct
  {
    int total;
    int wrids[_NM_DCFA_WRID_MAX];
  } pending;              /**< count of pending packets */
};


/* ********************************************************* */

struct nm_dcfa_hca_s*nm_dcfa_hca_resolve(int index);

void nm_dcfa_hca_get_profile(int index, struct nm_drv_profile_s*p_profile);

struct nm_dcfa_cnx*nm_dcfa_cnx_new(struct nm_dcfa_hca_s*p_hca);

void nm_dcfa_cnx_sync(struct nm_dcfa_cnx*p_ibverbs_cnx);

void nm_dcfa_cnx_connect(struct nm_dcfa_cnx*p_ibverbs_cnx);


/* ** RDMA toolbox ***************************************** */


/** messages for Work Completion status  */
static const char*const nm_dcfa_status_strings[] =
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

static inline int nm_dcfa_rdma_send(struct nm_dcfa_cnx*p_ibverbs_cnx, int size,
				    const void*__restrict__ ptr,
				    const void*__restrict__ _raddr,
				    const void*__restrict__ _lbase,
				    const struct nm_dcfa_segment*seg,
				    const struct ibv_mr*__restrict__ mr,
				    int wrid)
{
  const uintptr_t remote_offset = (uintptr_t)_raddr - (uintptr_t)_lbase;
  const uintptr_t local_offset  = (uintptr_t)ptr - (uintptr_t)_lbase;
  const uintptr_t _rbase = seg->raddr;
  const uint64_t raddr   = (uint64_t)(_rbase + remote_offset);
  struct ibv_sge list = {
    .mic_addr = (uintptr_t)ptr,
    .addr = (uintptr_t)mr->host_addr + local_offset,
    .length = size,
    .lkey   = mr->lkey
  };
  struct ibv_send_wr wr = {
    .wr_id      = wrid,
    .sg_list[0] = list,
    .num_sge    = 1,
    .opcode     = IBV_WR_RDMA_WRITE,
    .send_flags = (size < p_ibverbs_cnx->max_inline) ? (IBV_SEND_INLINE | IBV_SEND_SIGNALED) : IBV_SEND_SIGNALED,
    .wr.rdma =
    {
      .remote_addr = raddr,
      .rkey        = seg->rkey
    }
  };
  int rc = ibv_post_send(p_ibverbs_cnx->qp, &wr);
  assert(wrid < _NM_DCFA_WRID_MAX);
  p_ibverbs_cnx->pending.wrids[wrid]++;
  p_ibverbs_cnx->pending.total++;
  if(rc) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: post RDMA send failed.\n");
      abort();
    }
  return NM_ESUCCESS;
}

static inline int nm_dcfa_rdma_poll(struct nm_dcfa_cnx*__restrict__ p_ibverbs_cnx)
{
  struct ibv_wc wc;
  int ne = 0, done = 0;
  do {
    ne = ibv_poll_cq(p_ibverbs_cnx->of_cq, 1, &wc);
    if(ne > 0 && wc.status == IBV_WC_SUCCESS)
      {
	assert(wc.wr_id < _NM_DCFA_WRID_MAX);
	p_ibverbs_cnx->pending.wrids[wc.wr_id]--;
	p_ibverbs_cnx->pending.total--;
	done += ne;
      }
    else if(ne > 0)
      {
	fprintf(stderr, "nmad: FATAL- ibverbs: WC send failed- status=%d (%s)\n", 
		wc.status, nm_dcfa_status_strings[wc.status]);
	abort();
      }
    else if(ne < 0)
      {
	fprintf(stderr, "nmad: FATAL- ibverbs: WC polling failed.\n");
	abort();
      }
  } while(ne > 0);
  return (done > 0) ? NM_ESUCCESS : -NM_EAGAIN;
}

static inline void nm_dcfa_send_flush(struct nm_dcfa_cnx*__restrict__ p_ibverbs_cnx, uint64_t wrid)
{
  while(p_ibverbs_cnx->pending.wrids[wrid])
    {
      nm_dcfa_rdma_poll(p_ibverbs_cnx);
    }
}

static inline void nm_dcfa_send_flushn_total(struct nm_dcfa_cnx*__restrict__ p_ibverbs_cnx, int n)
{
  while(p_ibverbs_cnx->pending.total > n) {
    nm_dcfa_rdma_poll(p_ibverbs_cnx);
  }
}

static inline void nm_dcfa_send_flushn(struct nm_dcfa_cnx*__restrict__ p_ibverbs_cnx, int wrid, int n)
{
  while(p_ibverbs_cnx->pending.wrids[wrid] > n)
    {
      nm_dcfa_rdma_poll(p_ibverbs_cnx);
    }
}


#endif /* NM_DCFA_H */

