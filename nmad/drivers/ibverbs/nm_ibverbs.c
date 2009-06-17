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

#include <nm_private.h>

#include <infiniband/verbs.h>

#ifdef PUK_ABI
#include <Padico/Puk-ABI.h>
#endif


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

enum nm_ibverbs_trk_kind
  {
    NM_IBVERBS_TRK_NONE      = 0,
    NM_IBVERBS_TRK_BYCOPY    = 0x01,
    NM_IBVERBS_TRK_REGRDMA   = 0x02,
    NM_IBVERBS_TRK_ADAPTRDMA = 0x04,
    NM_IBVERBS_TRK_RCACHE    = 0x08,
    NM_IBVERBS_TRK_AUTO      = 0x10
  };

struct nm_ibverbs_trk 
{
  enum nm_ibverbs_trk_kind kind;
};

struct nm_ibverbs_segment 
{
  enum nm_ibverbs_trk_kind kind;
  uint64_t raddr;
  uint32_t rkey;
};

struct nm_ibverbs_cnx_addr 
{
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  struct nm_ibverbs_segment segments[16];
};


/* *** method: 'bycopy' ************************************ */

#define NM_IBVERBS_BYCOPY_BUFSIZE     (4096 - sizeof(struct nm_ibverbs_bycopy_header))
#define NM_IBVERBS_BYCOPY_RBUF_NUM    32
#define NM_IBVERBS_BYCOPY_SBUF_NUM    2
#define NM_IBVERBS_BYCOPY_CREDITS_THR 17

#define NM_IBVERBS_BYCOPY_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_BYCOPY_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_IBVERBS_BYCOPY_STATUS_LAST    0x02  /**< last data block */
#define NM_IBVERBS_BYCOPY_STATUS_CREDITS 0x04  /**< message contains credits */

struct nm_ibverbs_bycopy_header 
{
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bycopy' method */
struct nm_ibverbs_bycopy_packet 
{
  char data[NM_IBVERBS_BYCOPY_BUFSIZE];
  struct nm_ibverbs_bycopy_header header;
} __attribute__((packed));

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bycopy
{
  struct {
    struct nm_ibverbs_bycopy_packet sbuf[NM_IBVERBS_BYCOPY_SBUF_NUM];
    struct nm_ibverbs_bycopy_packet rbuf[NM_IBVERBS_BYCOPY_RBUF_NUM];
    volatile uint16_t rack, sack; /**< field for credits acknowledgement */
  } buffer;
  
  struct {
    uint32_t next_out;  /**< next sequence number for outgoing packet */
    uint32_t credits;   /**< remaining credits for sending */
    uint32_t next_in;   /**< cell number of next expected packet */
    uint32_t to_ack;    /**< credits not acked yet by the receiver */
  } window;
  
  struct {
    int done;           /**< size of data received so far */
    int msg_size;       /**< size of whole message */
    void*buf_posted;    /**< buffer posted for receive */
    int size_posted;    /**< length of above buffer */
  } recv;
  
  struct {
    const struct iovec*v;
    int n;              /**< size of above iovec */
    int current_packet; /**< current buffer for sending */
    int msg_size;       /**< total size of message to send */
    int done;           /**< total amount of data sent */
    int v_done;         /**< size of current iovec segment already sent */
    int v_current;      /**< current iovec segment beeing sent */
  } send;
  
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct ibv_mr*mr;           /**< global MR (used for 'buffer') */
  
};


/* *** method: 'regrdma' *********************************** */

static const int nm_ibverbs_regrdma_frag_base    = 256 * 1024;
static const int nm_ibverbs_regrdma_frag_inc     = 512 * 1024;
static const int nm_ibverbs_regrdma_frag_overrun = 64  * 1024;
static const int nm_ibverbs_regrdma_frag_max     = 4096 * 1024;

struct nm_ibverbs_regrdma_cell
{
  struct ibv_mr*mr;
  int packet_size;
  int offset;
};

/** Header sent to ask for RDMA data (regrdma rdv) */
struct nm_ibverbs_regrdma_rdvhdr 
{
  uint64_t raddr;
  uint32_t rkey;
  volatile int busy;
};

/** Header sent to signal presence of regrdma data */
struct nm_ibverbs_regrdma_sighdr 
{
  int k;
  volatile int busy;
};

/** Connection state for tracks sending with on-the-fly memory registration */
struct nm_ibverbs_regrdma 
{
  struct ibv_mr*mr;              /**< global MR (used for 'headers') */
  struct nm_ibverbs_segment seg; /**< remote segment */
  
  struct {
    char*message;
    int todo;
    int max_cell;
    struct nm_ibverbs_regrdma_cell*cells;
    int k;
    int pending_reg;
    int frag_size;
  } recv;
  
  struct {
    const char*message;
    int size;
    int todo;
    int max_cell;
    struct nm_ibverbs_regrdma_cell*cells;
    int k;
    int pending_reg;
    int pending_packet;
    int frag_size;
  } send;
  
  struct {
    struct nm_ibverbs_regrdma_rdvhdr shdr, rhdr[2];
    struct nm_ibverbs_regrdma_sighdr ssig, rsig[2];
  } headers;
};


/* *** method: 'adaptrdma' ********************************* */

static const int nm_ibverbs_adaptrdma_block_granularity = 2048;
static const int nm_ibverbs_adaptrdma_step_base         = 16  * 1024;
static const int nm_ibverbs_adaptrdma_step_overrun      = 16  * 1024;
static const int nm_ibverbs_adaptrdma_reg_threshold     = 384 * 1024;
static const int nm_ibverbs_adaptrdma_reg_remaining     = 120 * 1024;

#define NM_IBVERBS_ADAPTRDMA_BUFSIZE (3 * 1024 * 1024)
#define NM_IBVERBS_ADAPTRDMA_HDRSIZE (sizeof(struct nm_ibverbs_adaptrdma_header))

#define NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR      0x01
#define NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE    0x02
#define NM_IBVERBS_ADAPTRDMA_FLAG_REGISTER_BEG 0x04
#define NM_IBVERBS_ADAPTRDMA_FLAG_REGISTER_END 0x08

/** threshold to switch from adaptrdma to regrdma */
#define NM_IBVERBS_AUTO_THRESHOLD (3 * 1024 * 1024 - NM_IBVERBS_ADAPTRDMA_HDRSIZE - 8)

/** on the wire header of method 'adaptrdma' */
struct nm_ibverbs_adaptrdma_header
{
  int packet_size;
  volatile int offset;
  volatile int busy;
};

/** Connection state for tracks sending with adaptive RDMA super-pipeline */
struct nm_ibverbs_adaptrdma 
{
  
  struct ibv_mr*mr;
  struct nm_ibverbs_segment seg; /**< remote segment */
  
  struct
  {
    char sbuf[NM_IBVERBS_ADAPTRDMA_BUFSIZE];
    char rbuf[NM_IBVERBS_ADAPTRDMA_BUFSIZE];
  } buffer;

  struct
  {
    const char*message;
    int todo;
    int done;
    void*rbuf;
    void*sbuf;
    int size_guard;
  } send;

  struct
  {
    char*message;
    int size;
    void*rbuf;
    int done;
    int block_size;
  } recv;
};

/* *** method: 'rcache' ********************************* */


/** Header sent by the receiver to ask for RDMA data (rcache rdv) */
struct nm_ibverbs_rcache_rdvhdr 
{
  uint64_t raddr;
  uint32_t rkey;
  volatile int busy;
};

/** Header sent to signal presence of rcache data */
struct nm_ibverbs_rcache_sighdr 
{
  volatile int busy;
};

/** Connection state for 'rcache' tracks */
struct nm_ibverbs_rcache
{
  struct ibv_mr*mr;              /**< global MR (used for headers) */
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct
  {
    char*message;
    int size;
    struct ibv_mr*mr;
  } recv;
  struct
  {
    const char*message;
    int size;
    struct ibv_mr*mr;
  } send;
  struct
  {
    struct nm_ibverbs_rcache_rdvhdr shdr, rhdr;
    struct nm_ibverbs_rcache_sighdr ssig, rsig;
  } headers;
};



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
  struct
  {
    struct nm_ibverbs_bycopy    bycopy;
    struct nm_ibverbs_regrdma   regrdma;
    struct nm_ibverbs_adaptrdma adaptrdma;
    struct nm_ibverbs_rcache    rcache;
  };
};

struct nm_ibverbs
{
  struct nm_ibverbs_cnx*cnx_array;
  int nb_cnx;
  int sock;    /**< connected socket for IB address exchange */
};

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
static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_recv_iov(void*_status, struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_cancel_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static const char* nm_ibverbs_get_driver_url(struct nm_drv *p_drv);
static struct nm_drv_cap*nm_ibverbs_get_capabilities(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_ibverbs_driver =
  {
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

    .cancel_recv_iov    = &nm_ibverbs_cancel_recv_iov,

    .get_driver_url     = &nm_ibverbs_get_driver_url,
    .get_track_url      = NULL,
    .get_capabilities   = &nm_ibverbs_get_capabilities
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
  puk_component_declare("NewMad_Driver_ibverbs",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_ibverbs_driver));
  return 0;
}
PADICO_MODULE_BUILTIN(NewMad_Driver_ibverbs, &nm_ibverbs_load, NULL, NULL);


/** Instanciate functions */
static void* nm_ibverbs_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs*status = TBX_MALLOC(sizeof (struct nm_ibverbs));

  status->sock = -1;
  status->cnx_array = NULL;

  return status;
}

static void nm_ibverbs_destroy(void*_status)
{
  struct nm_ibverbs*status = _status;
  if(status->cnx_array)
    {
      TBX_FREE(status->cnx_array);
    }
  TBX_FREE(_status);
}

const static char*nm_ibverbs_get_driver_url(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  return p_ibverbs_drv->url;
}

/** Return capabilities */
static struct nm_drv_cap*nm_ibverbs_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  return &p_ibverbs_drv->caps;
}

/* ** helpers ********************************************** */

static inline struct nm_ibverbs_cnx*nm_ibverbs_get_cnx(void*_status, nm_trk_id_t trk_id)
{
  struct nm_ibverbs*status = _status;
  struct nm_ibverbs_cnx*p_ibverbs_cnx = &status->cnx_array[trk_id];
  return p_ibverbs_cnx;
}

static inline int nm_ibverbs_get_kind(struct nm_drv*p_drv, int trk_id)
{
  struct nm_ibverbs_drv*__restrict__ p_ibverbs_drv = p_drv->priv;
  int kind = p_ibverbs_drv->trks_array[trk_id].kind;
  return kind;
}

static void nm_ibverbs_addr_send(const void*_status,
				 const struct nm_ibverbs_cnx_addr*addr)
{
  const struct nm_ibverbs*status = _status;
  int rc = send(status->sock, addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
  if(rc != sizeof(struct nm_ibverbs_cnx_addr)) {
    fprintf(stderr, "Infiniband: cannot send address to peer.\n");
    abort();
  }
}

static void nm_ibverbs_addr_recv(void*_status,
				 struct nm_ibverbs_cnx_addr*addr)
{
  const struct nm_ibverbs*status = _status;
  int rc = recv(status->sock, addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
  if(rc != sizeof(struct nm_ibverbs_cnx_addr)) {
    fprintf(stderr, "Infiniband: cannot get address from peer.\n");
    abort();
  }
}

static void nm_ibverbs_addr_pack(struct nm_ibverbs_cnx*p_ibverbs_cnx, int kind)
{
  int n = 0;
  
  if(kind & NM_IBVERBS_TRK_BYCOPY) {
    p_ibverbs_cnx->local_addr.segments[n].kind  = NM_IBVERBS_TRK_BYCOPY;
    p_ibverbs_cnx->local_addr.segments[n].raddr = (uintptr_t)&p_ibverbs_cnx->bycopy.buffer;
    p_ibverbs_cnx->local_addr.segments[n].rkey  = p_ibverbs_cnx->bycopy.mr->rkey;
    n++;
  }
  if(kind & NM_IBVERBS_TRK_REGRDMA) {
    p_ibverbs_cnx->local_addr.segments[n].kind  = NM_IBVERBS_TRK_REGRDMA;
    p_ibverbs_cnx->local_addr.segments[n].raddr = (uintptr_t)&p_ibverbs_cnx->regrdma.headers;
    p_ibverbs_cnx->local_addr.segments[n].rkey  = p_ibverbs_cnx->regrdma.mr->rkey;
    n++;
  }
  if(kind & NM_IBVERBS_TRK_ADAPTRDMA) {
    p_ibverbs_cnx->local_addr.segments[n].kind  = NM_IBVERBS_TRK_ADAPTRDMA;
    p_ibverbs_cnx->local_addr.segments[n].raddr = (uintptr_t)&p_ibverbs_cnx->adaptrdma.buffer;
    p_ibverbs_cnx->local_addr.segments[n].rkey  = p_ibverbs_cnx->adaptrdma.mr->rkey;
    n++;
  }
  if(kind & NM_IBVERBS_TRK_RCACHE) {
    p_ibverbs_cnx->local_addr.segments[n].kind  = NM_IBVERBS_TRK_RCACHE;
    p_ibverbs_cnx->local_addr.segments[n].raddr = (uintptr_t)&p_ibverbs_cnx->rcache.headers;
    p_ibverbs_cnx->local_addr.segments[n].rkey  = p_ibverbs_cnx->rcache.mr->rkey;
    n++;
  }
}

static void nm_ibverbs_addr_unpack(const struct nm_ibverbs_cnx_addr*addr,
				   struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  int i;
  for(i = 0; addr->segments[i].raddr; i++)
    {
      switch(addr->segments[i].kind)
	{
	case NM_IBVERBS_TRK_BYCOPY:
	  p_ibverbs_cnx->bycopy.seg = addr->segments[i];
	  break;
	case NM_IBVERBS_TRK_REGRDMA:
	  p_ibverbs_cnx->regrdma.seg = addr->segments[i];
	  break;
	case NM_IBVERBS_TRK_ADAPTRDMA:
	  p_ibverbs_cnx->adaptrdma.seg = addr->segments[i];
	  break;
	case NM_IBVERBS_TRK_RCACHE:
	  p_ibverbs_cnx->rcache.seg = addr->segments[i];
	  break;
	default:
	  fprintf(stderr, "Infiniband: got unknown address kind.\n");
	  abort();
	  break;
	}
    }
}


#ifdef PM2_NUIOA
/* ******************** numa node ***************************** */

#define NM_IBVERBS_NUIOA_SYSFILE_LENGTH 16

static int nm_ibverbs_get_numa_node(struct ibv_device* ib_dev)
{
#ifdef LINUX_SYS
  int i, nb_nodes;
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
  
  nb_nodes = numa_max_node();
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
  struct nm_ibverbs_drv*p_ibverbs_drv = NULL;
  
  /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_bycopy_packet) % 1024 == 0);
  assert(NM_IBVERBS_BYCOPY_CREDITS_THR > NM_IBVERBS_BYCOPY_RBUF_NUM / 2);
  
  p_ibverbs_drv = TBX_MALLOC(sizeof(struct nm_ibverbs_drv));
  
  if (!p_ibverbs_drv) {
    err = -NM_ENOMEM;
    goto out;
  }
  
  /* find IB device */
  struct ibv_device**dev_list = ibv_get_device_list(&dev_amount);
  if(!dev_list) {
    fprintf(stderr, "Infiniband: no device found.\n");
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
    fprintf(stderr, "Infiniband: cannot open IB context.\n");
    err = -NM_ESCFAILD;
    goto out;
  }

  ibv_free_device_list(dev_list);
  
  /* driver capabilities encoding */
  p_ibverbs_drv->caps.has_trk_rq_dgram			= 1;
  p_ibverbs_drv->caps.has_trk_rq_rdv		        = 1;
  p_ibverbs_drv->caps.has_selective_receive		= 1;
  p_ibverbs_drv->caps.has_concurrent_selective_receive	= 0;
  p_ibverbs_drv->caps.rdv_threshold                       = 256 * 1024;
#ifdef PM2_NUIOA
  p_ibverbs_drv->caps.numa_node = nm_ibverbs_get_numa_node(p_ibverbs_drv->ib_dev);
  p_ibverbs_drv->caps.latency = 170; /* from sr_ping */
  p_ibverbs_drv->caps.bandwidth = 1400; /* from sr_ping, use 200 * link width instead? */
#endif
  
  p_drv->priv = p_ibverbs_drv;
  err = NM_ESUCCESS;
  
 out:
  return err;
}

#ifdef NM_IBVERBS_RCACHE
static struct ibv_pd*global_pd; /**< global IB protection domain */
static void*nm_ibverbs_mem_reg(const void*ptr, size_t len)
{
  struct ibv_mr*mr = ibv_reg_mr(global_pd, (void*)ptr, len, 
				IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  return mr;
}
static void nm_ibverbs_mem_unreg(const void*ptr, void*key)
{
  struct ibv_mr*mr = key;
  ibv_dereg_mr(mr);
}
#endif /* BM_IBVERBS_RCACHE */

static int nm_ibverbs_init(struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks)
{
  int err;
  int rc;
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  
  srand48(getpid() * time(NULL));
  
  /* get IB context attributes */
  struct ibv_device_attr device_attr;
  rc = ibv_query_device(p_ibverbs_drv->context, &device_attr);
  if(rc != 0) {
    fprintf(stderr, "Infiniband: cannot get device capabilities.\n");
    abort();
  }
  /* allocate Protection Domain */
  p_ibverbs_drv->pd = ibv_alloc_pd(p_ibverbs_drv->context);
  if(p_ibverbs_drv->pd == NULL) {
    fprintf(stderr, "Infiniband: cannot allocate IB protection domain.\n");
    abort();
  }
#ifdef NM_IBVERBS_RCACHE
  global_pd = p_ibverbs_drv->pd;
  puk_mem_set_handlers(&nm_ibverbs_mem_reg, &nm_ibverbs_mem_unreg);
#endif /* NM_IBVERBS_RCACHE */

  /* detect LID */
  struct ibv_port_attr port_attr;
  rc = ibv_query_port(p_ibverbs_drv->context, NM_IBVERBS_PORT, &port_attr);
  if(rc != 0) {
    fprintf(stderr, "Infiniband: cannot get local port attributes.\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  p_ibverbs_drv->lid = port_attr.lid;
  fprintf(stderr, "# Infiniband: local LID = 0x%02X\n", p_ibverbs_drv->lid);
  if(p_ibverbs_drv->lid == 0)
    {
      fprintf(stderr, "Infiniband: WARNING: LID is null- subnet manager has probably crashed.\n");
    }
  
  /* open helper socket */
  p_ibverbs_drv->server_sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(p_ibverbs_drv->server_sock > -1);
  struct sockaddr_in addr;
  unsigned addr_len = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = INADDR_ANY;
  rc = bind(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, addr_len);
  if(rc) {
    fprintf(stderr, "Infiniband: bind error (%s)\n", strerror(errno));
    abort();
  }
  rc = getsockname(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
  listen(p_ibverbs_drv->server_sock, 255);
	
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
      fprintf(stderr, "Infiniband: cannot get local address\n");
      err = -NM_EUNREACH;
      goto out;
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
  
  fprintf(stderr, "# Infiniband: capabilities for device '%s'- \n",
	  ibv_get_device_name(p_ibverbs_drv->ib_dev));
  fprintf(stderr, "# Infiniband:   max_qp=%d; max_qp_wr=%d; max_cq=%d; max_cqe=%d;\n",
	  p_ibverbs_drv->ib_caps.max_qp, p_ibverbs_drv->ib_caps.max_qp_wr,
	  p_ibverbs_drv->ib_caps.max_cq, p_ibverbs_drv->ib_caps.max_cqe);
  fprintf(stderr, "# Infiniband:   max_mr=%d; max_mr_size=%llu; page_size_cap=%llu; max_msg_size=%llu\n",
	  p_ibverbs_drv->ib_caps.max_mr,
	  (unsigned long long) p_ibverbs_drv->ib_caps.max_mr_size,
	  (unsigned long long) p_ibverbs_drv->ib_caps.page_size_cap,
	  (unsigned long long) p_ibverbs_drv->ib_caps.max_msg_size);
  
  fprintf(stderr, "# Infiniband:   active_width=%d; active_speed=%d\n",
	  (int)port_attr.active_width, (int)port_attr.active_speed);

#ifdef SAMPLING
  nm_parse_sampling(p_drv, "ibverbs");
#endif

  /* open tracks */
  p_ibverbs_drv->nb_trks = nb_trks;
  p_ibverbs_drv->trks_array = TBX_MALLOC(nb_trks * sizeof(struct nm_ibverbs_trk));
  nm_trk_id_t i;
  for(i = 0; i < nb_trks; i++)
    {
      /* auto-detect track kind */ 
      if(trk_caps[i].rq_type == nm_trk_rq_rdv)
	{
#ifdef NM_IBVERBS_RCACHE
	  p_ibverbs_drv->trks_array[i].kind = NM_IBVERBS_TRK_RCACHE;
#else
	  p_ibverbs_drv->trks_array[i].kind = NM_IBVERBS_TRK_REGRDMA | NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_AUTO; /* XXX */
#endif
	}
      else
	{
	  p_ibverbs_drv->trks_array[i].kind = NM_IBVERBS_TRK_BYCOPY;
	}
      /* fill capabilities */
      switch(p_ibverbs_drv->trks_array[i].kind) 
	{
	case NM_IBVERBS_TRK_REGRDMA | NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_AUTO:
	case NM_IBVERBS_TRK_REGRDMA:
	case NM_IBVERBS_TRK_ADAPTRDMA:
	case NM_IBVERBS_TRK_RCACHE:
	  trk_caps[i].rq_type  = nm_trk_rq_rdv;
	  trk_caps[i].iov_type = nm_trk_iov_none;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 1;
	  break;
	case NM_IBVERBS_TRK_BYCOPY:
	  trk_caps[i].rq_type  = nm_trk_rq_dgram;
	  trk_caps[i].iov_type = nm_trk_iov_send_only;
	  trk_caps[i].max_pending_send_request	= 1;
	  trk_caps[i].max_pending_recv_request	= 1;
	  trk_caps[i].max_single_request_length	= SSIZE_MAX;
	  trk_caps[i].max_iovec_request_length	= 0;
	  trk_caps[i].max_iovec_size		= 0;
	  break;
	default:
	  fprintf(stderr, "Infiniband: cannot handle method 0x%x in %s\n", p_ibverbs_drv->trks_array[i].kind, __FUNCTION__);
	  abort();
	  break;
	}
    }
  
  err = NM_ESUCCESS;
  
 out:
  return err;
}

static int nm_ibverbs_close(struct nm_drv *p_drv)
{
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  close(p_ibverbs_drv->server_sock);
  TBX_FREE(p_ibverbs_drv->trks_array);
  TBX_FREE(p_ibverbs_drv->url);
  TBX_FREE(p_ibverbs_drv);
  return NM_ESUCCESS;
}


static int nm_ibverbs_gate_create(void*_status,
				  struct nm_cnx_rq*p_crq)
{
  int err = NM_ESUCCESS;
  struct nm_ibverbs*status = _status;
  if(!status->cnx_array)
    {
      status->nb_cnx    = p_crq->p_drv->nb_tracks;
      status->cnx_array = TBX_MALLOC(sizeof(struct nm_ibverbs_cnx) * status->nb_cnx);
      if(!status->cnx_array){
	err = -NM_ENOMEM;
	goto out;
      }
      memset(status->cnx_array, 0, sizeof(struct nm_ibverbs_cnx) * status->nb_cnx);
    }
  
 out:
  return err;
}

static int nm_ibverbs_cnx_create(void*_status, struct nm_cnx_rq*p_crq)
{
  struct nm_drv *p_drv = p_crq->p_drv;
  int err = nm_ibverbs_gate_create(_status, p_crq);	
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
  int kind = p_ibverbs_drv->trks_array[p_crq->trk_id].kind;
  
  if(kind & NM_IBVERBS_TRK_BYCOPY)
    {
      /* init state */
      p_ibverbs_cnx->bycopy.window.next_out    = 1;
      p_ibverbs_cnx->bycopy.window.next_in     = 1;
      p_ibverbs_cnx->bycopy.window.credits     = NM_IBVERBS_BYCOPY_RBUF_NUM;
      p_ibverbs_cnx->bycopy.window.to_ack      = 0;
      memset(&p_ibverbs_cnx->bycopy.buffer, 0, sizeof(p_ibverbs_cnx->bycopy.buffer));
      /* register Memory Region */
      p_ibverbs_cnx->bycopy.mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->bycopy.buffer,
					    sizeof(p_ibverbs_cnx->bycopy.buffer),
					    IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      if(p_ibverbs_cnx->bycopy.mr == NULL) {
	fprintf(stderr, "Infiniband: cannot register MR.\n");
	err = -NM_EUNKNOWN;
	goto out;
      }
    }
  if(kind & NM_IBVERBS_TRK_REGRDMA)
    {
      /* init state */
      memset(&p_ibverbs_cnx->regrdma.headers, 0, sizeof(p_ibverbs_cnx->regrdma.headers));
      /* register Memory Region */
      p_ibverbs_cnx->regrdma.mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->regrdma.headers,
					     sizeof(p_ibverbs_cnx->regrdma.headers),
					     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      if(p_ibverbs_cnx->regrdma.mr == NULL) {
	fprintf(stderr, "Infiniband: cannot register MR.\n");
	err = -NM_EUNKNOWN;
	goto out;
      }
    }
  if(kind & NM_IBVERBS_TRK_ADAPTRDMA)
    {
      /* init state */
      memset(&p_ibverbs_cnx->adaptrdma.buffer, 0, sizeof(p_ibverbs_cnx->adaptrdma.buffer));
      /* register Memory Region */
      p_ibverbs_cnx->adaptrdma.mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->adaptrdma.buffer,
					       sizeof(p_ibverbs_cnx->adaptrdma.buffer),
					       IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      if(p_ibverbs_cnx->adaptrdma.mr == NULL) {
	fprintf(stderr, "Infiniband: cannot register MR.\n");
	err = -NM_EUNKNOWN;
	goto out;
      }
    }
    if(kind & NM_IBVERBS_TRK_RCACHE)
    {
      /* init state */
      memset(&p_ibverbs_cnx->rcache.headers, 0, sizeof(p_ibverbs_cnx->rcache.headers));
      /* register Memory Region */
      p_ibverbs_cnx->rcache.mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->rcache.headers,
					    sizeof(p_ibverbs_cnx->rcache.headers),
					    IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      if(p_ibverbs_cnx->rcache.mr == NULL) {
	fprintf(stderr, "Infiniband: cannot register MR.\n");
	err = -NM_EUNKNOWN;
	goto out;
      }
    }

  /* init incoming CQ */
  p_ibverbs_cnx->if_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_RX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->if_cq == NULL) {
    fprintf(stderr, "Infiniband: cannot create in CQ\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  /* init outgoing CQ */
  p_ibverbs_cnx->of_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_TX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->of_cq == NULL) {
    fprintf(stderr, "Infiniband: cannot create out CQ\n");
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
    fprintf(stderr, "Infiniband: couldn't create QP\n");
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
    fprintf(stderr, "Infiniband: failed to modify QP to INIT\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  
  p_ibverbs_cnx->local_addr.lid   = p_ibverbs_drv->lid;
  p_ibverbs_cnx->local_addr.qpn   = p_ibverbs_cnx->qp->qp_num;
  p_ibverbs_cnx->local_addr.psn   = lrand48() & 0xffffff;
  nm_ibverbs_addr_pack(p_ibverbs_cnx, kind);
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
    fprintf(stderr, "Infiniband: failed to modify QP to RTR\n");
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
    fprintf(stderr,"Infiniband: failed to modify QP to RTS\n");
    err = -NM_EUNKNOWN;
    goto out;
  }
  
 out:
  return err;
}

static int nm_ibverbs_connect(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_ibverbs*status = _status;
  
  int err = nm_ibverbs_gate_create(_status, p_crq);
  if(err)
    {
      err = -NM_ENOMEM;
      goto out;
    }
  if(status->sock == -1) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
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
    int rc = connect(fd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
    if(rc) {
      fprintf(stderr, "Infiniband: cannot connect to %s:%d\n", inet_ntoa(inaddr.sin_addr), peer_port);
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
  nm_ibverbs_addr_unpack(&p_ibverbs_cnx->remote_addr, p_ibverbs_cnx);
  err = nm_ibverbs_cnx_connect(_status, p_crq);
  
 out:
  return err;
}

static int nm_ibverbs_accept(void*_status, struct nm_cnx_rq *p_crq)
{
  struct nm_ibverbs*status = _status;
  struct nm_ibverbs_drv*p_ibverbs_drv = p_crq->p_drv->priv;
  
  int err =  nm_ibverbs_gate_create(_status, p_crq);
  if(err)
    {
      err = -NM_ENOMEM;
      goto out;
    }
  if(status->sock == -1) {
    struct sockaddr_in addr;
    unsigned addr_len = sizeof addr;
    int fd = accept(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
    assert(fd > -1);
    status->sock = fd;
  }
  
  err = nm_ibverbs_cnx_create(_status, p_crq);
  if(err)
    goto out;
  
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_crq->trk_id);
  nm_ibverbs_addr_send(_status, &p_ibverbs_cnx->local_addr);
  nm_ibverbs_addr_recv(_status, &p_ibverbs_cnx->remote_addr);
  nm_ibverbs_addr_unpack(&p_ibverbs_cnx->remote_addr, p_ibverbs_cnx);
  err = nm_ibverbs_cnx_connect(_status, p_crq);
  
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

/* ** RDMA toolbox ***************************************** */

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

static int nm_ibverbs_rdma_send(struct nm_ibverbs_cnx*p_ibverbs_cnx, int size,
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
  while(p_ibverbs_cnx->pending.wrids[wrid] > n) {
    nm_ibverbs_rdma_poll(p_ibverbs_cnx);
  }
}

/* ********************************************************* */

/* ** bycopy I/O ******************************************* */

static inline void nm_ibverbs_bycopy_send_post(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
					       const struct iovec*v, int n)
{
  p_ibverbs_cnx->bycopy.send.v              = v;
  p_ibverbs_cnx->bycopy.send.n              = n;
  p_ibverbs_cnx->bycopy.send.current_packet = 0;
  p_ibverbs_cnx->bycopy.send.msg_size       = 0;
  p_ibverbs_cnx->bycopy.send.done           = 0;
  p_ibverbs_cnx->bycopy.send.v_done         = 0;
  p_ibverbs_cnx->bycopy.send.v_current      = 0;
  int i;
  for(i = 0; i < p_ibverbs_cnx->bycopy.send.n; i++)
    {
      p_ibverbs_cnx->bycopy.send.msg_size += p_ibverbs_cnx->bycopy.send.v[i].iov_len;
    }
}

static inline int nm_ibverbs_bycopy_send_poll(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct nm_ibverbs_bycopy*__restrict__ bycopy = &p_ibverbs_cnx->bycopy;
  while(bycopy->send.done < bycopy->send.msg_size)
    { 
      /* 1- check credits */
      const int rack = bycopy->buffer.rack;
      if(rack) {
	bycopy->window.credits += rack;
	bycopy->buffer.rack = 0;
      }
      if(bycopy->window.credits <= 1)
	{
	  int j;
	  for(j = 0; j < NM_IBVERBS_BYCOPY_RBUF_NUM; j++)
	    {
	      if(bycopy->buffer.rbuf[j].header.status)
		{
		  bycopy->window.credits += bycopy->buffer.rbuf[j].header.ack;
		  bycopy->buffer.rbuf[j].header.ack = 0;
		  bycopy->buffer.rbuf[j].header.status &= ~NM_IBVERBS_BYCOPY_STATUS_CREDITS;
		}
	    }
	}
      if(bycopy->window.credits <= 1) 
	{
	  goto wouldblock;
	}
      
      /* 2- check window availability */
      nm_ibverbs_rdma_poll(p_ibverbs_cnx);
      if(p_ibverbs_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] >= NM_IBVERBS_BYCOPY_SBUF_NUM)
	{
	  goto wouldblock;
	}
      
      /* 3- prepare and send packet */
      struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.sbuf[bycopy->send.current_packet];
      const int remaining = bycopy->send.msg_size - bycopy->send.done;
      const int offset = (remaining > NM_IBVERBS_BYCOPY_BUFSIZE) ? 0 : (NM_IBVERBS_BYCOPY_BUFSIZE - remaining);
      int available   = NM_IBVERBS_BYCOPY_BUFSIZE - offset;
      int packet_size = 0;
      while((packet_size < available) &&
	    (bycopy->send.v_current < bycopy->send.n))
	{
	  const int v_remaining = bycopy->send.v[bycopy->send.v_current].iov_len - bycopy->send.v_done;
	  const int fragment_size = (v_remaining > available) ? available : v_remaining;
	  memcpy(&packet->data[offset + packet_size],
		 bycopy->send.v[bycopy->send.v_current].iov_base + bycopy->send.v_done, fragment_size);
	  packet_size += fragment_size;
	  available   -= fragment_size;
	  bycopy->send.v_done += fragment_size;
	  if(bycopy->send.v_done >= bycopy->send.v[bycopy->send.v_current].iov_len) 
	    {
	      bycopy->send.v_current++;
	      bycopy->send.v_done = 0;
	    }
	}
      assert(NM_IBVERBS_BYCOPY_BUFSIZE - offset == packet_size);
      
      packet->header.offset = offset;
      packet->header.ack    = bycopy->window.to_ack;
      packet->header.status = NM_IBVERBS_BYCOPY_STATUS_DATA;
      bycopy->window.to_ack = 0;
      bycopy->send.done += packet_size;
      if(bycopy->send.done >= bycopy->send.msg_size)
	packet->header.status |= NM_IBVERBS_BYCOPY_STATUS_LAST;
      
      nm_ibverbs_rdma_send(p_ibverbs_cnx,
			   sizeof(struct nm_ibverbs_bycopy_packet) - offset,
			   &packet->data[offset],
			   &bycopy->buffer.rbuf[bycopy->window.next_out].data[offset],
			   &bycopy->buffer,
			   &bycopy->seg,
			   bycopy->mr,
			   NM_IBVERBS_WRID_RDMA);
      bycopy->send.current_packet = (bycopy->send.current_packet + 1) % NM_IBVERBS_BYCOPY_SBUF_NUM;
      bycopy->window.next_out     = (bycopy->window.next_out     + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      bycopy->window.credits--;
    }
  nm_ibverbs_rdma_poll(p_ibverbs_cnx);
  if(p_ibverbs_cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}


static inline void nm_ibverbs_bycopy_recv_init(struct nm_pkt_wrap*p_pw, struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  p_ibverbs_cnx->bycopy.recv.done        = 0;
  p_ibverbs_cnx->bycopy.recv.msg_size    = -1;
  p_ibverbs_cnx->bycopy.recv.buf_posted  = p_pw->v[p_pw->v_first].iov_base;
  p_ibverbs_cnx->bycopy.recv.size_posted = p_pw->v[p_pw->v_first].iov_len;
  p_pw->drv_priv = p_ibverbs_cnx;
}


static inline int nm_ibverbs_bycopy_poll_one(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  int err = -NM_EUNKNOWN;
  int complete = 0;
  struct nm_ibverbs_bycopy*__restrict__ bycopy = &p_ibverbs_cnx->bycopy;
  struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.rbuf[p_ibverbs_cnx->bycopy.window.next_in];
  while( ( (bycopy->recv.msg_size == -1) || (!complete) ) &&
	 (packet->header.status != 0) ) 
    {
      if(bycopy->recv.msg_size == -1) 
	{
	  assert((packet->header.status & NM_IBVERBS_BYCOPY_STATUS_DATA) != 0);
	  bycopy->recv.msg_size = 0;
	}
      complete = (packet->header.status & NM_IBVERBS_BYCOPY_STATUS_LAST);
      assert((bycopy->recv.done == 0 && (packet->header.status & NM_IBVERBS_BYCOPY_STATUS_DATA)));
      const int offset = packet->header.offset;
      const int packet_size = NM_IBVERBS_BYCOPY_BUFSIZE - offset;
      memcpy(bycopy->recv.buf_posted + bycopy->recv.done, &packet->data[offset], packet_size);
      bycopy->recv.msg_size += packet_size;
      bycopy->recv.done += packet_size;
      bycopy->window.credits += packet->header.ack;
      packet->header.ack = 0;
      packet->header.status = 0;
      bycopy->window.to_ack++;
      if(bycopy->window.to_ack > NM_IBVERBS_BYCOPY_CREDITS_THR) 
	{
	  bycopy->buffer.sack = bycopy->window.to_ack;
	  nm_ibverbs_rdma_send(p_ibverbs_cnx,
			       sizeof(uint16_t),
			       (void*)&bycopy->buffer.sack,
			       (void*)&bycopy->buffer.rack,
			       &bycopy->buffer,
			       &bycopy->seg,
			       bycopy->mr,
			       NM_IBVERBS_WRID_ACK);
	  bycopy->window.to_ack = 0;
	}
      nm_ibverbs_rdma_poll(p_ibverbs_cnx);
      bycopy->window.next_in = (bycopy->window.next_in + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
    }
  nm_ibverbs_rdma_poll(p_ibverbs_cnx);
  if((bycopy->recv.msg_size > 0) && complete)
    {
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_ACK);
      err = NM_ESUCCESS;
    }
  else 
    {
      err = -NM_EAGAIN;
    }
  return err;
}


/* ********************************************************* */

/* ** regrdma I/O ****************************************** */

static inline int nm_ibverbs_regrdma_count(size_t size)
{
  return 1 /* first block */
    + (size - nm_ibverbs_regrdma_frag_base) / nm_ibverbs_regrdma_frag_inc /* blocks */
    + 2 /* rounding */
    + 1; /* guard to detect overrun */
}

static inline size_t nm_ibverbs_regrdma_step_next(size_t frag_size)
{
  const size_t s = (frag_size <= nm_ibverbs_regrdma_frag_max) ?
    (nm_ibverbs_regrdma_frag_inc + frag_size) :
    (nm_ibverbs_regrdma_frag_max);
  return s;
}

static inline void nm_ibverbs_regrdma_send_init(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx, const char*message, int size)
{
  p_ibverbs_cnx->regrdma.send.message        = message;
  p_ibverbs_cnx->regrdma.send.size           = size;
  p_ibverbs_cnx->regrdma.send.todo           = size;
  p_ibverbs_cnx->regrdma.send.k              = -1;
  p_ibverbs_cnx->regrdma.send.pending_reg    = 0;
  p_ibverbs_cnx->regrdma.send.pending_packet = 0;
  p_ibverbs_cnx->regrdma.send.frag_size      = nm_ibverbs_regrdma_frag_base;
  p_ibverbs_cnx->regrdma.send.max_cell       = nm_ibverbs_regrdma_count(size);
  p_ibverbs_cnx->regrdma.send.cells = TBX_MALLOC(sizeof(struct nm_ibverbs_regrdma_cell) * p_ibverbs_cnx->regrdma.send.max_cell);
  memset(p_ibverbs_cnx->regrdma.send.cells, 0, sizeof(struct nm_ibverbs_regrdma_cell) *   p_ibverbs_cnx->regrdma.send.max_cell);
}

static inline int nm_ibverbs_regrdma_send_done(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  return (p_ibverbs_cnx->regrdma.send.todo == 0) && (p_ibverbs_cnx->regrdma.send.pending_reg == 0);
}

static inline int nm_ibverbs_regrdma_send_wouldblock(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int k = p_ibverbs_cnx->regrdma.send.k;
  return (k >= 0 &&
	  p_ibverbs_cnx->regrdma.send.cells[p_ibverbs_cnx->regrdma.send.k].mr && 
	  !p_ibverbs_cnx->regrdma.headers.rhdr[k & 0x01].busy);
}

static inline void nm_ibverbs_regrdma_send_step_send(struct nm_ibverbs_cnx*__restrict__ const p_ibverbs_cnx)
{
  const int prev = p_ibverbs_cnx->regrdma.send.k - 1;
  if(p_ibverbs_cnx->regrdma.send.k >= 0 && p_ibverbs_cnx->regrdma.send.cells[p_ibverbs_cnx->regrdma.send.k].mr)
    {
      /* ** send current packet */
      const int k = p_ibverbs_cnx->regrdma.send.k;
      struct nm_ibverbs_regrdma_rdvhdr*const h = &p_ibverbs_cnx->regrdma.headers.rhdr[k & 0x01];
      assert(h->busy);
      const uint64_t raddr = h->raddr;
      const uint32_t rkey  = h->rkey;
      h->raddr = 0;
      h->rkey  = 0;
      h->busy  = 0;
      nm_ibverbs_send_flushn_total(p_ibverbs_cnx, 2); /* TODO */
      nm_ibverbs_do_rdma(p_ibverbs_cnx, 
			 p_ibverbs_cnx->regrdma.send.message + p_ibverbs_cnx->regrdma.send.cells[k].offset,
			 p_ibverbs_cnx->regrdma.send.cells[k].packet_size, raddr, IBV_WR_RDMA_WRITE,
			 (p_ibverbs_cnx->regrdma.send.cells[k].packet_size > p_ibverbs_cnx->max_inline ?
			  IBV_SEND_SIGNALED : (IBV_SEND_SIGNALED | IBV_SEND_INLINE)),
			 p_ibverbs_cnx->regrdma.send.cells[k].mr->lkey, rkey, NM_IBVERBS_WRID_DATA);
      p_ibverbs_cnx->regrdma.send.pending_packet++;
      assert(p_ibverbs_cnx->regrdma.send.pending_packet <= 2);
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_HEADER);
      p_ibverbs_cnx->regrdma.headers.ssig.k    = k;
      p_ibverbs_cnx->regrdma.headers.ssig.busy = 1;
      nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(struct nm_ibverbs_regrdma_sighdr),
			   &p_ibverbs_cnx->regrdma.headers.ssig,
			   &p_ibverbs_cnx->regrdma.headers.rsig[k & 0x01],
			   &p_ibverbs_cnx->regrdma.headers,
			   &p_ibverbs_cnx->regrdma.seg,
			   p_ibverbs_cnx->regrdma.mr,
			   NM_IBVERBS_WRID_HEADER);
    }
  if(prev >= 0 && p_ibverbs_cnx->regrdma.send.cells[prev].mr) 
    {
      p_ibverbs_cnx->regrdma.send.pending_packet--;
      p_ibverbs_cnx->regrdma.send.pending_reg--;
      nm_ibverbs_send_flushn(p_ibverbs_cnx, NM_IBVERBS_WRID_DATA, p_ibverbs_cnx->regrdma.send.pending_packet);
      ibv_dereg_mr(p_ibverbs_cnx->regrdma.send.cells[prev].mr);
      p_ibverbs_cnx->regrdma.send.cells[prev].mr = NULL;
    }
}

static inline void nm_ibverbs_regrdma_send_step_reg(struct nm_ibverbs_cnx*__restrict__ const p_ibverbs_cnx)
{
  if(p_ibverbs_cnx->regrdma.send.todo > 0) 
    {
      const int next = p_ibverbs_cnx->regrdma.send.k + 1;
      /* ** prepare next */
      p_ibverbs_cnx->regrdma.send.cells[next].packet_size = 
	p_ibverbs_cnx->regrdma.send.todo > (p_ibverbs_cnx->regrdma.send.frag_size + nm_ibverbs_regrdma_frag_overrun) ?
	p_ibverbs_cnx->regrdma.send.frag_size : p_ibverbs_cnx->regrdma.send.todo;
      p_ibverbs_cnx->regrdma.send.cells[next].offset = p_ibverbs_cnx->regrdma.send.size - p_ibverbs_cnx->regrdma.send.todo;
      p_ibverbs_cnx->regrdma.send.cells[next].mr     =
	ibv_reg_mr(p_ibverbs_cnx->qp->pd, (void*)(p_ibverbs_cnx->regrdma.send.message + p_ibverbs_cnx->regrdma.send.cells[next].offset),
		   p_ibverbs_cnx->regrdma.send.cells[next].packet_size, IBV_ACCESS_LOCAL_WRITE);
      p_ibverbs_cnx->regrdma.send.pending_reg++;
      assert(p_ibverbs_cnx->regrdma.send.pending_reg <= 2);
      p_ibverbs_cnx->regrdma.send.todo -= p_ibverbs_cnx->regrdma.send.cells[next].packet_size;
      p_ibverbs_cnx->regrdma.send.frag_size = nm_ibverbs_regrdma_step_next(p_ibverbs_cnx->regrdma.send.frag_size);
    }
}

static void nm_ibverbs_regrdma_send_post(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
					 const struct iovec*v, int n)
{
  const char*message = v[0].iov_base;
  const int size = v[0].iov_len;
  assert(n == 1);
  nm_ibverbs_regrdma_send_init(p_ibverbs_cnx, message, size);
}

static int nm_ibverbs_regrdma_send_poll(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  if(!nm_ibverbs_regrdma_send_done(p_ibverbs_cnx) &&
     !nm_ibverbs_regrdma_send_wouldblock(p_ibverbs_cnx))
    {
      nm_ibverbs_regrdma_send_step_send(p_ibverbs_cnx);
      nm_ibverbs_regrdma_send_step_reg(p_ibverbs_cnx);
      p_ibverbs_cnx->regrdma.send.k++;
      if(p_ibverbs_cnx->regrdma.send.k >= p_ibverbs_cnx->regrdma.send.max_cell)
	{
	  TBX_FAILUREF("Infiniband: bad regrdma cells count while sending (k = %d; max = %d).\n",
		       p_ibverbs_cnx->regrdma.send.k, p_ibverbs_cnx->regrdma.send.max_cell);
	}
    }
  if(nm_ibverbs_regrdma_send_done(p_ibverbs_cnx))
    {
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_HEADER);
      TBX_FREE(p_ibverbs_cnx->regrdma.send.cells);
      p_ibverbs_cnx->regrdma.send.cells = NULL;
      p_ibverbs_cnx->regrdma.send.max_cell = 0;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}


static inline void nm_ibverbs_regrdma_recv_init(struct nm_pkt_wrap*p_pw, struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int size = p_pw->v[p_pw->v_first].iov_len;
  p_ibverbs_cnx->regrdma.recv.message     = p_pw->v[p_pw->v_first].iov_base;
  p_ibverbs_cnx->regrdma.recv.todo        = size;
  p_ibverbs_cnx->regrdma.recv.max_cell    = nm_ibverbs_regrdma_count(size);
  p_ibverbs_cnx->regrdma.recv.cells       = TBX_MALLOC(sizeof(struct nm_ibverbs_regrdma_cell) * p_ibverbs_cnx->regrdma.recv.max_cell);
  memset(p_ibverbs_cnx->regrdma.recv.cells, 0, sizeof(struct nm_ibverbs_regrdma_cell) * p_ibverbs_cnx->regrdma.recv.max_cell);
  p_ibverbs_cnx->regrdma.recv.k           = 0;
  p_ibverbs_cnx->regrdma.recv.pending_reg = 0;
  p_ibverbs_cnx->regrdma.recv.frag_size   = nm_ibverbs_regrdma_frag_base;
  p_pw->drv_priv = p_ibverbs_cnx;
}

static inline int nm_ibverbs_regrdma_recv_isdone(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  return (p_ibverbs_cnx->regrdma.recv.todo <= 0 && p_ibverbs_cnx->regrdma.recv.pending_reg == 0);
}
static inline void nm_ibverbs_regrdma_recv_step_reg(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int k = p_ibverbs_cnx->regrdma.recv.k;
  if(p_ibverbs_cnx->regrdma.recv.todo > 0 &&
     p_ibverbs_cnx->regrdma.recv.cells[k].mr == NULL)
    {
      const int packet_size = p_ibverbs_cnx->regrdma.recv.todo > (p_ibverbs_cnx->regrdma.recv.frag_size + nm_ibverbs_regrdma_frag_overrun) ?
	p_ibverbs_cnx->regrdma.recv.frag_size : p_ibverbs_cnx->regrdma.recv.todo;
      p_ibverbs_cnx->regrdma.recv.cells[k].mr = ibv_reg_mr(p_ibverbs_cnx->qp->pd, 
							   p_ibverbs_cnx->regrdma.recv.message,
							   packet_size,
							   IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
      p_ibverbs_cnx->regrdma.recv.pending_reg++;
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_RDV);
      p_ibverbs_cnx->regrdma.headers.shdr.raddr = (uintptr_t)p_ibverbs_cnx->regrdma.recv.message;
      p_ibverbs_cnx->regrdma.headers.shdr.rkey  = p_ibverbs_cnx->regrdma.recv.cells[k].mr->rkey;
      p_ibverbs_cnx->regrdma.headers.shdr.busy  = 1;
      nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(struct nm_ibverbs_regrdma_rdvhdr),
			   &p_ibverbs_cnx->regrdma.headers.shdr, 
			   &p_ibverbs_cnx->regrdma.headers.rhdr[k & 0x01],
			   &p_ibverbs_cnx->regrdma.headers,
			   &p_ibverbs_cnx->regrdma.seg,
			   p_ibverbs_cnx->regrdma.mr,
			   NM_IBVERBS_WRID_RDV);
      p_ibverbs_cnx->regrdma.recv.todo     -= packet_size;
      p_ibverbs_cnx->regrdma.recv.message  += packet_size;
      p_ibverbs_cnx->regrdma.recv.frag_size = nm_ibverbs_regrdma_step_next(p_ibverbs_cnx->regrdma.recv.frag_size);
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_RDV);
    }
}
static inline void nm_ibverbs_regrdma_recv_step_dereg(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int prev = p_ibverbs_cnx->regrdma.recv.k - 1;
  struct nm_ibverbs_regrdma_sighdr*rsig = &p_ibverbs_cnx->regrdma.headers.rsig[prev & 0x01];
  if( (prev >= 0) &&
      (p_ibverbs_cnx->regrdma.recv.cells[prev].mr) &&
      (rsig->busy) )
    {
      assert(rsig->k == prev);
      rsig->busy = 0;
      rsig->k    = 0xffff;
      ibv_dereg_mr(p_ibverbs_cnx->regrdma.recv.cells[prev].mr);
      p_ibverbs_cnx->regrdma.recv.pending_reg--;
      p_ibverbs_cnx->regrdma.recv.cells[prev].mr = NULL;
    }
  if(prev < 0 ||
     p_ibverbs_cnx->regrdma.recv.cells[prev].mr == NULL)
    {
      p_ibverbs_cnx->regrdma.recv.k++;
      if(p_ibverbs_cnx->regrdma.recv.k >= p_ibverbs_cnx->regrdma.recv.max_cell)
	{
	  TBX_FAILUREF("Infiniband: bad regrdma cells count while receiving (k=%d; max=%d).\n",
		       p_ibverbs_cnx->regrdma.recv.k, p_ibverbs_cnx->regrdma.recv.max_cell);
	}
    }
}

static int nm_ibverbs_regrdma_poll_one(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  if(!nm_ibverbs_regrdma_recv_isdone(p_ibverbs_cnx))
    {
      nm_ibverbs_regrdma_recv_step_reg(p_ibverbs_cnx);
      nm_ibverbs_regrdma_recv_step_dereg(p_ibverbs_cnx);
    }
  if(nm_ibverbs_regrdma_recv_isdone(p_ibverbs_cnx)) 
    {
      TBX_FREE(p_ibverbs_cnx->regrdma.recv.cells);
      p_ibverbs_cnx->regrdma.recv.cells = NULL;
      p_ibverbs_cnx->regrdma.recv.max_cell = 0;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
}

/* ********************************************************* */

/* ** adaptrdma I/O **************************************** */

static inline int nm_ibverbs_adaptrdma_block_size(int done)
{
  if(done < 16 * 1024)
    return 2048;
  else if(done < 128 * 1024)
    return  4096;
  else if(done < 512 * 1024)
    return  8192;
  else if(done < 1024 * 1024)
    return  16384;
  else
    return 16384;
}

/** calculates the position of the packet header, given the beginning of the packet and its size
 */
static inline struct nm_ibverbs_adaptrdma_header*nm_ibverbs_adaptrdma_get_header(void*buf, int packet_size)
{
  return buf + packet_size - sizeof(struct nm_ibverbs_adaptrdma_header);
}


static void nm_ibverbs_adaptrdma_send_post(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
					   const struct iovec*v, int n)
{
  assert(n == 1);
  p_ibverbs_cnx->adaptrdma.send.message = v[0].iov_base;
  p_ibverbs_cnx->adaptrdma.send.todo    = v[0].iov_len;
  p_ibverbs_cnx->adaptrdma.send.done    = 0;
  p_ibverbs_cnx->adaptrdma.send.rbuf    = p_ibverbs_cnx->adaptrdma.buffer.rbuf;
  p_ibverbs_cnx->adaptrdma.send.sbuf    = p_ibverbs_cnx->adaptrdma.buffer.sbuf;
  p_ibverbs_cnx->adaptrdma.send.size_guard = nm_ibverbs_adaptrdma_step_base;
}

static int nm_ibverbs_adaptrdma_send_poll(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{

  int packet_size = 0;
  const int block_size = nm_ibverbs_adaptrdma_block_size(p_ibverbs_cnx->adaptrdma.send.done);
  const int available  = block_size - NM_IBVERBS_ADAPTRDMA_HDRSIZE;
  while((p_ibverbs_cnx->adaptrdma.send.todo > 0) && 
	((p_ibverbs_cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) ||
	 (packet_size < p_ibverbs_cnx->adaptrdma.send.size_guard) ||
	 (p_ibverbs_cnx->adaptrdma.send.todo <= nm_ibverbs_adaptrdma_step_overrun)))
    {
      const int frag_size = (p_ibverbs_cnx->adaptrdma.send.todo > available) ? available : p_ibverbs_cnx->adaptrdma.send.todo;
      memcpy(p_ibverbs_cnx->adaptrdma.send.sbuf + packet_size, 
	     &p_ibverbs_cnx->adaptrdma.send.message[p_ibverbs_cnx->adaptrdma.send.done], frag_size);
      struct nm_ibverbs_adaptrdma_header*const h = 
	nm_ibverbs_adaptrdma_get_header(p_ibverbs_cnx->adaptrdma.send.sbuf, packet_size + block_size);
      h->busy   = NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR;
      h->offset = frag_size;
      p_ibverbs_cnx->adaptrdma.send.todo -= frag_size;
      p_ibverbs_cnx->adaptrdma.send.done += frag_size;
      packet_size += block_size;
      if((p_ibverbs_cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) &&
	 (p_ibverbs_cnx->adaptrdma.send.done > nm_ibverbs_adaptrdma_step_base))
	{
	  nm_ibverbs_rdma_poll(p_ibverbs_cnx);
	}
    }
  struct nm_ibverbs_adaptrdma_header*const h_last =
    nm_ibverbs_adaptrdma_get_header(p_ibverbs_cnx->adaptrdma.send.sbuf, packet_size);
  h_last->busy = NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE;
  nm_ibverbs_rdma_send(p_ibverbs_cnx, packet_size, p_ibverbs_cnx->adaptrdma.send.sbuf, p_ibverbs_cnx->adaptrdma.send.rbuf, 
		       &p_ibverbs_cnx->adaptrdma.buffer,
		       &p_ibverbs_cnx->adaptrdma.seg,
		       p_ibverbs_cnx->adaptrdma.mr,
		       NM_IBVERBS_WRID_PACKET);
  p_ibverbs_cnx->adaptrdma.send.size_guard *= (p_ibverbs_cnx->adaptrdma.send.size_guard <= 8192) ? 4 : 2;
  nm_ibverbs_rdma_poll(p_ibverbs_cnx);
  p_ibverbs_cnx->adaptrdma.send.sbuf += packet_size;
  p_ibverbs_cnx->adaptrdma.send.rbuf += packet_size;
  if(p_ibverbs_cnx->adaptrdma.send.todo <= 0)
    {
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_PACKET);
      return NM_ESUCCESS;
    }
  else
    return -NM_EAGAIN;

}

static inline void nm_ibverbs_adaptrdma_recv_init(struct nm_pkt_wrap*p_pw, struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  p_ibverbs_cnx->adaptrdma.recv.done       = 0;
  p_ibverbs_cnx->adaptrdma.recv.block_size = nm_ibverbs_adaptrdma_block_size(0);
  p_ibverbs_cnx->adaptrdma.recv.message    = p_pw->v[p_pw->v_first].iov_base;
  p_ibverbs_cnx->adaptrdma.recv.size       = p_pw->v[p_pw->v_first].iov_len;
  p_ibverbs_cnx->adaptrdma.recv.rbuf       = p_ibverbs_cnx->adaptrdma.buffer.rbuf;
  p_pw->drv_priv = p_ibverbs_cnx;
}


static inline int nm_ibverbs_adaptrdma_poll_one(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  do
    {
      struct nm_ibverbs_adaptrdma_header*h = nm_ibverbs_adaptrdma_get_header(p_ibverbs_cnx->adaptrdma.recv.rbuf,
									     p_ibverbs_cnx->adaptrdma.recv.block_size);
      if(!h->busy)
	goto wouldblock;
      const int frag_size = h->offset;
      const int flag = h->busy;
      memcpy(&p_ibverbs_cnx->adaptrdma.recv.message[p_ibverbs_cnx->adaptrdma.recv.done],
	     p_ibverbs_cnx->adaptrdma.recv.rbuf, frag_size);
      /* clear blocks */
      void*_rbuf = p_ibverbs_cnx->adaptrdma.recv.rbuf;
      p_ibverbs_cnx->adaptrdma.recv.done += frag_size;
      p_ibverbs_cnx->adaptrdma.recv.rbuf += p_ibverbs_cnx->adaptrdma.recv.block_size;
      while(_rbuf < p_ibverbs_cnx->adaptrdma.recv.rbuf)
	{
	  h = nm_ibverbs_adaptrdma_get_header(_rbuf, nm_ibverbs_adaptrdma_block_granularity);
	  h->offset = 0;
	  h->busy = 0;
	  _rbuf += nm_ibverbs_adaptrdma_block_granularity;
	}
      switch(flag)
	{
	case NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR:
	  break;
	  
	case NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE:
	  p_ibverbs_cnx->adaptrdma.recv.block_size = nm_ibverbs_adaptrdma_block_size(p_ibverbs_cnx->adaptrdma.recv.done);
	  break;
	  
	default:
	  fprintf(stderr, "Infiniband: unexpected flag 0x%x in adaptrdma_recv()\n", flag);
	  abort();
	  break;
	}
    }
  while(p_ibverbs_cnx->adaptrdma.recv.done < p_ibverbs_cnx->adaptrdma.recv.size);
  
  p_ibverbs_cnx->adaptrdma.recv.message = NULL;
  return NM_ESUCCESS;
  
 wouldblock:
  return -NM_EAGAIN;
}


/* ********************************************************* */

/* ** reg cache I/O **************************************** */


static inline void nm_ibverbs_rcache_send_post(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
					       const struct iovec*v, int n)
{
#ifdef NM_IBVERBS_RCACHE
  char*message = v[0].iov_base;
  const int size = v[0].iov_len;
  assert(n == 1);
  p_ibverbs_cnx->rcache.send.message = message;
  p_ibverbs_cnx->rcache.send.size = size;
  p_ibverbs_cnx->rcache.send.mr = puk_mem_reg(message, size);
#endif
}

static inline int nm_ibverbs_rcache_send_poll(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
#ifdef NM_IBVERBS_RCACHE
  struct nm_ibverbs_rcache_rdvhdr*const h = &p_ibverbs_cnx->rcache.headers.rhdr;
  if(h->busy)
    {
      const uint64_t raddr = h->raddr;
      const uint32_t rkey  = h->rkey;
      h->raddr = 0;
      h->rkey  = 0;
      h->busy  = 0;
      nm_ibverbs_do_rdma(p_ibverbs_cnx, 
			 p_ibverbs_cnx->rcache.send.message,
			 p_ibverbs_cnx->rcache.send.size, raddr, IBV_WR_RDMA_WRITE,
			 (p_ibverbs_cnx->rcache.send.size > p_ibverbs_cnx->max_inline ?
			  IBV_SEND_SIGNALED : (IBV_SEND_SIGNALED | IBV_SEND_INLINE)),
			 p_ibverbs_cnx->rcache.send.mr->lkey, rkey, NM_IBVERBS_WRID_DATA);
      p_ibverbs_cnx->rcache.headers.ssig.busy = 1;
      nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(struct nm_ibverbs_rcache_sighdr),
			   &p_ibverbs_cnx->rcache.headers.ssig,
			   &p_ibverbs_cnx->rcache.headers.rsig,
			   &p_ibverbs_cnx->rcache.headers,
			   &p_ibverbs_cnx->rcache.seg,
			   p_ibverbs_cnx->rcache.mr,
			   NM_IBVERBS_WRID_HEADER);
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_HEADER);  
      puk_mem_unreg(p_ibverbs_cnx->rcache.send.message);
      p_ibverbs_cnx->rcache.send.message = NULL;
      p_ibverbs_cnx->rcache.send.size    = -1;
      p_ibverbs_cnx->rcache.send.mr = NULL;
      nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_DATA);  
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
#else
  return -NM_ENOTIMPL;
#endif
}

static inline void nm_ibverbs_rcache_recv_init(struct nm_pkt_wrap*p_pw, struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
#ifdef NM_IBVERBS_RCACHE
  p_ibverbs_cnx->rcache.headers.rsig.busy = 0;
  p_ibverbs_cnx->rcache.recv.message = p_pw->v[p_pw->v_first].iov_base;
  p_ibverbs_cnx->rcache.recv.size = p_pw->v[p_pw->v_first].iov_len;
  p_ibverbs_cnx->rcache.recv.mr = puk_mem_reg(p_ibverbs_cnx->rcache.recv.message, 
					      p_ibverbs_cnx->rcache.recv.size);
  struct nm_ibverbs_rcache_rdvhdr*const h = &p_ibverbs_cnx->rcache.headers.shdr;
  h->raddr =  (uintptr_t)p_ibverbs_cnx->rcache.recv.message;
  h->rkey  = p_ibverbs_cnx->rcache.recv.mr->rkey;
  h->busy  = 1;
  nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(struct nm_ibverbs_rcache_rdvhdr),
		       &p_ibverbs_cnx->rcache.headers.shdr, 
		       &p_ibverbs_cnx->rcache.headers.rhdr,
		       &p_ibverbs_cnx->rcache.headers,
		       &p_ibverbs_cnx->rcache.seg,
		       p_ibverbs_cnx->rcache.mr,
		       NM_IBVERBS_WRID_RDV);
  nm_ibverbs_send_flush(p_ibverbs_cnx, NM_IBVERBS_WRID_RDV);
  p_pw->drv_priv = p_ibverbs_cnx;
#endif
}

static inline int nm_ibverbs_rcache_poll_one(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
#ifdef NM_IBVERBS_RCACHE
  struct nm_ibverbs_rcache_sighdr*rsig = &p_ibverbs_cnx->rcache.headers.rsig;
  if(rsig->busy)
    {
      puk_mem_unreg(p_ibverbs_cnx->rcache.recv.message);
      p_ibverbs_cnx->rcache.recv.message = NULL;
      p_ibverbs_cnx->rcache.recv.size = -1;
      p_ibverbs_cnx->rcache.recv.mr = NULL;
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }
#else
  return -NM_ENOTIMPL;
#endif
}

/* ********************************************************* */

/* ** driver I/O functions ********************************* */

static int nm_ibverbs_post_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
  int kind = nm_ibverbs_get_kind(p_pw->p_drv, p_pw->trk_id);
  int err = NM_ESUCCESS;
  p_pw->drv_priv = p_ibverbs_cnx;
  switch(kind)
    {
    case NM_IBVERBS_TRK_BYCOPY:
      nm_ibverbs_bycopy_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
      break;
    case NM_IBVERBS_TRK_AUTO |  NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_REGRDMA:
    case NM_IBVERBS_TRK_AUTO:
      if(p_pw->length > NM_IBVERBS_AUTO_THRESHOLD) 
	{
	  nm_ibverbs_regrdma_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
	}
      else
	{
	  nm_ibverbs_adaptrdma_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
	}
      break;
    case NM_IBVERBS_TRK_REGRDMA:
      nm_ibverbs_regrdma_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
      break;
    case NM_IBVERBS_TRK_ADAPTRDMA:
      nm_ibverbs_adaptrdma_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
      break;
    case NM_IBVERBS_TRK_RCACHE:
      nm_ibverbs_rcache_send_post(p_ibverbs_cnx, &p_pw->v[p_pw->v_first], p_pw->v_nb);
      break;
    default:
      fprintf(stderr, "Infiniband: cannot handle method 0x%x in %s\n", kind, __FUNCTION__);
      abort();
      break;
    }
  err = nm_ibverbs_poll_send_iov(_status, p_pw);
  return err;
}

static int nm_ibverbs_poll_send_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int kind = nm_ibverbs_get_kind(p_pw->p_drv, p_pw->trk_id);
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = p_pw->drv_priv;
  int err = -NM_EUNKNOWN;
  switch(kind)
    {
    case NM_IBVERBS_TRK_BYCOPY:
      err = nm_ibverbs_bycopy_send_poll(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_AUTO |  NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_REGRDMA:
    case NM_IBVERBS_TRK_AUTO:
      if(p_pw->length > NM_IBVERBS_AUTO_THRESHOLD)
	{
	  err = nm_ibverbs_regrdma_send_poll(p_ibverbs_cnx);
	}
      else
	{
	  err = nm_ibverbs_adaptrdma_send_poll(p_ibverbs_cnx);
	}
      break;
    case NM_IBVERBS_TRK_REGRDMA:
      err = nm_ibverbs_regrdma_send_poll(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_ADAPTRDMA:
      err = nm_ibverbs_adaptrdma_send_poll(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_RCACHE:
      err = nm_ibverbs_rcache_send_poll(p_ibverbs_cnx);
      break;
    default:
      fprintf(stderr, "Infiniband: cannot handle method 0x%x in %s\n", kind, __FUNCTION__);
      abort();
      break;
    }
  return err;
}

static inline void nm_ibverbs_recv_init(struct nm_pkt_wrap*__restrict__ p_pw,
					struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int kind = nm_ibverbs_get_kind(p_pw->p_drv, p_pw->trk_id);
  switch(kind)
    {
    case NM_IBVERBS_TRK_BYCOPY:
      nm_ibverbs_bycopy_recv_init(p_pw, p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_AUTO |  NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_REGRDMA:
    case NM_IBVERBS_TRK_AUTO:
      if(p_pw->length > NM_IBVERBS_AUTO_THRESHOLD)
	{
	  nm_ibverbs_regrdma_recv_init(p_pw, p_ibverbs_cnx);
	}
      else 
	{
	  nm_ibverbs_adaptrdma_recv_init(p_pw, p_ibverbs_cnx);
	}
      break;
    case NM_IBVERBS_TRK_REGRDMA:
      nm_ibverbs_regrdma_recv_init(p_pw, p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_ADAPTRDMA:
      nm_ibverbs_adaptrdma_recv_init(p_pw, p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_RCACHE:
      nm_ibverbs_rcache_recv_init(p_pw, p_ibverbs_cnx);
      break;
    default:
      fprintf(stderr, "Infiniband: cannot handle method 0x%x in %s\n", kind, __FUNCTION__);
      abort();
      break;
    }
}

static inline int nm_ibverbs_poll_one(struct nm_pkt_wrap*__restrict__ p_pw,
				      struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
  const int kind = nm_ibverbs_get_kind(p_pw->p_drv, p_pw->trk_id);
  switch(kind) 
    {
    case NM_IBVERBS_TRK_BYCOPY:
      return nm_ibverbs_bycopy_poll_one(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_AUTO |  NM_IBVERBS_TRK_ADAPTRDMA | NM_IBVERBS_TRK_REGRDMA:
    case NM_IBVERBS_TRK_AUTO:
      if(p_pw->length > NM_IBVERBS_AUTO_THRESHOLD)
	{
	  assert(p_pw->p_gate != NULL);
	  return nm_ibverbs_regrdma_poll_one(p_ibverbs_cnx);
	}
      else
	{
	  assert(p_pw->p_gate != NULL);
	  return nm_ibverbs_adaptrdma_poll_one(p_ibverbs_cnx);
	}
      break;
    case NM_IBVERBS_TRK_REGRDMA:
      assert(p_pw->p_gate != NULL);
      return nm_ibverbs_regrdma_poll_one(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_ADAPTRDMA:
      assert(p_pw->p_gate != NULL);
      return nm_ibverbs_adaptrdma_poll_one(p_ibverbs_cnx);
      break;
    case NM_IBVERBS_TRK_RCACHE:
      assert(p_pw->p_gate != NULL);
      return nm_ibverbs_rcache_poll_one(p_ibverbs_cnx);
      break;
    default:
      fprintf(stderr, "Infiniband: cannot handle method 0x%x in %s\n", kind, __FUNCTION__);
      abort();
      break;
    }
  return -NM_EUNKNOWN;
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
      const struct nm_core*p_core = p_pw->p_drv->p_core;
      int i;
      for(i = 0; i < p_core->nb_gates; i++)
	{
	  const struct nm_gate*__restrict__ p_gate = &p_core->gate_array[i];
	  if(p_gate) {
	    struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
	    if(p_ibverbs_cnx->bycopy.buffer.rbuf[p_ibverbs_cnx->bycopy.window.next_in].header.status)
	      {
		p_pw->p_gate = (struct nm_gate*)p_gate;
		nm_ibverbs_recv_init(p_pw, p_ibverbs_cnx);
		err = nm_ibverbs_poll_one(p_pw, p_ibverbs_cnx);
		goto out;
	      }
	  }
	}
    }
 out:
  return err;
}

static int nm_ibverbs_post_recv_iov(void*_status, struct nm_pkt_wrap*__restrict__ p_pw)
{
  int err = NM_ESUCCESS;
  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
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
  const int kind = nm_ibverbs_get_kind(p_pw->p_drv, p_pw->trk_id);

  if(kind == NM_IBVERBS_TRK_BYCOPY)
    {
      struct nm_ibverbs_cnx* p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
      if(p_ibverbs_cnx->bycopy.recv.done == 0)
	{
	  p_ibverbs_cnx->bycopy.recv.buf_posted = NULL;
	  p_ibverbs_cnx->bycopy.recv.size_posted = 0;
	  p_pw->drv_priv = NULL;
	  err = NM_ESUCCESS;
	}
    }
  return err;
}
