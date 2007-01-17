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

/* -*- Mode: C; tab-width: 8; c-basic-offset: 8 -*- */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h>
#include <tbx.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <nm_protected.h>

#include <infiniband/verbs.h>

#define NM_IBVERBS_CNX_MAX    256
#define NM_IBVERBS_TRK_MAX    256

#define NM_IBVERBS_PORT         1
#define NM_IBVERBS_TX_DEPTH     4
#define NM_IBVERBS_RX_DEPTH     1
#define NM_IBVERBS_RDMA_DEPTH   4
#define NM_IBVERBS_MAX_SG_SQ    1
#define NM_IBVERBS_MAX_SG_RQ    1
#define NM_IBVERBS_MAX_INLINE   128
#define NM_IBVERBS_MTU          IBV_MTU_1024
#define NM_IBVERBS_WRID_SEND    1

#define NM_IBVERBS_BUF_SIZE     (16 * 4096 - sizeof(struct nm_ibverbs_header))
#define NM_IBVERBS_RBUF_NUM     16
#define NM_IBVERBS_SBUF_NUM     2
#define NM_IBVERBS_CREDITS_THR  12

#define NM_IBVERBS_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_IBVERBS_STATUS_CONT    0x02  /**< data (continued) */
#define NM_IBVERBS_STATUS_CREDITS 0x04  /**< message contains credits */


/** Global state of the HCA
 */
struct nm_ibverbs_drv {
	struct ibv_context*context; /**< global IB context */
	struct ibv_pd*pd;           /**< global IB protection domain */
	uint16_t lid;               /**< local IB LID */
	int server_sock;            /**< socket used for connection */
	struct {
		int max_inline;         /**< max size of data for IBV inline RDMA (-1 if not detected yet) */
		int max_qp;             /**< maximum number of QP */
		int max_qp_wr;          /**< maximum number of WR per QP */
		int max_cq;             /**< maximum number of CQ */
		int max_cqe;            /**< maximum number of entries per CQ */
		int max_mr;             /**< maximum number of MR */
		uint64_t max_mr_size;   /**< maximum size for a MR */
		uint64_t page_size_cap; /**< maximum page size for device */
		uint64_t max_msg_size;  /**< maximum message size */
	} ib_caps;
};

struct nm_ibverbs_trk {
	int dummy;
};

struct nm_ibverbs_cnx_addr {
	uint16_t lid;
	uint32_t qpn;
	uint32_t psn;
	uint64_t raddr;
	uint32_t rkey;
};

struct nm_ibverbs_header {
	uint32_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
	uint32_t size;           /**< message size */
	uint8_t  ack;            /**< credits acknowledged */
	volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

struct nm_ibverbs_packet {
	char data[NM_IBVERBS_BUF_SIZE];
	struct nm_ibverbs_header header;
} __attribute__((packed));

struct nm_ibverbs_cnx {
	struct nm_ibverbs_cnx_addr local_addr;
	struct nm_ibverbs_cnx_addr remote_addr;
	struct ibv_mr*mr;
	struct ibv_qp*qp;
	struct ibv_cq*of_cq;
	struct ibv_cq*if_cq;

	struct {
		struct nm_ibverbs_packet rbuf[NM_IBVERBS_RBUF_NUM];
		struct nm_ibverbs_packet sbuf[NM_IBVERBS_SBUF_NUM];
		volatile uint16_t rack, sack;
	} buffer;

	struct {
		uint32_t next_out;  /**< next sequence number for outgoing packet */
		uint32_t credits;   /**< remaining credits for sending */
		uint32_t next_in;   /**< cell number of next expected packet */
		uint32_t to_ack;    /**< credits not acked yet by the receiver */
	} window;
};

struct nm_ibverbs_gate {
        int sock;    /**< connected socket for IB address exchange */
	struct nm_ibverbs_cnx cnx_array[NM_IBVERBS_TRK_MAX];
};

struct nm_ibverbs_pkt_wrap {
        int dummy;
};

static int nm_ibverbs_post_send_iov(struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_send_iov(struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_post_recv_iov(struct nm_pkt_wrap*p_pw);
static int nm_ibverbs_poll_recv_iov(struct nm_pkt_wrap*p_pw);


/* ********************************************************* */


static int nm_ibverbs_init(struct nm_drv*p_drv)
{
	int err;
	int rc;

	srand48(getpid() * time(NULL));

	struct nm_ibverbs_drv*p_ibverbs_drv = TBX_MALLOC(sizeof(struct nm_ibverbs_drv));
        if (!p_ibverbs_drv) {
                err = -NM_ENOMEM;
                goto out;
        }
        p_drv->priv = p_ibverbs_drv;

	/* find IB device */
	struct ibv_device*const*dev_list = ibv_get_device_list(NULL);
	if(!dev_list) {
		fprintf(stderr, "Infiniband: no device found.\n");
		abort();
	}
	struct ibv_device*ib_dev = dev_list[0];
	/* open IB context */
	p_ibverbs_drv->context = ibv_open_device(ib_dev);
	if(p_ibverbs_drv->context == NULL) {
		fprintf(stderr, "Infniband: cannot open IB context.\n");
		abort();
	}
	struct ibv_device_attr device_attr;
	rc = ibv_query_device(p_ibverbs_drv->context, &device_attr);
	if(rc != 0) {
		fprintf(stderr, "Infniband: cannot get device capabilities.\n");
		abort();
	}
	/* allocate Protection Domain */
	p_ibverbs_drv->pd = ibv_alloc_pd(p_ibverbs_drv->context);
	if(p_ibverbs_drv->pd == NULL) {
		fprintf(stderr, "Infniband: cannot allocate IB protection domain.\n");
		abort();
	}
	/* detect LID */
	struct ibv_port_attr port_attr;
	rc = ibv_query_port(p_ibverbs_drv->context, NM_IBVERBS_PORT, &port_attr);
	if(rc != 0) {
		fprintf(stderr, "Infiniband: cannot get local port attributes.\n");
		err = -NM_EUNKNOWN;
		goto out;
	}
	p_ibverbs_drv->lid = port_attr.lid;
	fprintf(stderr, "Infiniband: local LID  = 0x%02X\n", p_ibverbs_drv->lid);

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
	char hostname[256];
	rc = gethostname(hostname, 256);
	const struct hostent*he = gethostbyname(hostname);
	if(!he) {
		fprintf(stderr, "Infiniband: cannot get local address\n");
		err = -NM_EUNREACH;
		goto out;
	}
	char s_port[24];
	snprintf(s_port, 24, "%s:%d", inet_ntoa(*(struct in_addr*)he->h_addr), ntohs(addr.sin_port));
        p_drv->url = tbx_strdup(s_port);

	/* IB capabilities */
	p_ibverbs_drv->ib_caps.max_inline    = -1; /* will be detected upon QP creation */
	p_ibverbs_drv->ib_caps.max_qp        = device_attr.max_qp;
	p_ibverbs_drv->ib_caps.max_qp_wr     = device_attr.max_qp_wr;
	p_ibverbs_drv->ib_caps.max_cq        = device_attr.max_cq;
	p_ibverbs_drv->ib_caps.max_cqe       = device_attr.max_cqe;
	p_ibverbs_drv->ib_caps.max_mr        = device_attr.max_mr;
	p_ibverbs_drv->ib_caps.max_mr_size   = device_attr.max_mr_size;
	p_ibverbs_drv->ib_caps.page_size_cap = device_attr.page_size_cap;
	p_ibverbs_drv->ib_caps.max_msg_size  = port_attr.max_msg_sz;

	fprintf(stderr, "Infiniband: capabilities for device '%s'- \n",
		ibv_get_device_name(ib_dev));
	fprintf(stderr, "Infiniband:   max_qp=%d; max_qp_wr=%d; max_cq=%d; max_cqe=%d;\n",
		p_ibverbs_drv->ib_caps.max_qp, p_ibverbs_drv->ib_caps.max_qp_wr,
		p_ibverbs_drv->ib_caps.max_cq, p_ibverbs_drv->ib_caps.max_cqe);
	fprintf(stderr, "Infiniband:   max_mr=%d; max_mr_size=%lu; page_size_cap=%lu; max_msg_size=%lu\n",
		p_ibverbs_drv->ib_caps.max_mr, p_ibverbs_drv->ib_caps.max_mr_size,
		p_ibverbs_drv->ib_caps.page_size_cap, p_ibverbs_drv->ib_caps.max_msg_size);


        /* driver capabilities encoding */
        p_drv->cap.has_trk_rq_dgram			= 1;
        p_drv->cap.has_selective_receive		= 1;
        p_drv->cap.has_concurrent_selective_receive	= 0;

	err = NM_ESUCCESS;

 out:
	return err;
}

static int nm_ibverbs_exit(struct nm_drv *p_drv)
{
        struct nm_ibverbs_drv	*p_ibverbs_drv	= p_drv->priv;
	int err;

	close(p_ibverbs_drv->server_sock);

        TBX_FREE(p_drv->url);
        TBX_FREE(p_ibverbs_drv);

	err = NM_ESUCCESS;

	return err;
}

static int nm_ibverbs_open_trk(struct nm_trk_rq	*p_trk_rq)
{
        struct nm_trk		*p_trk		= NULL;
        struct nm_ibverbs_trk	*p_ibverbs_trk	= NULL;
	int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_ibverbs_trk	= TBX_MALLOC(sizeof (struct nm_ibverbs_trk));
        if (!p_ibverbs_trk) {
                err = -NM_ENOMEM;
                goto out;
        }
        memset(p_ibverbs_trk, 0, sizeof (struct nm_ibverbs_trk));
        p_trk->priv	= p_ibverbs_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_dgram;
        p_trk->cap.iov_type	= nm_trk_iov_send_only;
        p_trk->cap.max_pending_send_request	= 1;
        p_trk->cap.max_pending_recv_request	= 1;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;
        p_trk->cap.max_iovec_request_length	= 0;
        p_trk->cap.max_iovec_size		= 0;

	err = NM_ESUCCESS;

 out:
	return err;
}

static int nm_ibverbs_close_trk(struct nm_trk *p_trk)
{
        struct nm_ibverbs_trk	*p_ibverbs_trk	= NULL;
	int err;

        p_ibverbs_trk	= p_trk->priv;
        TBX_FREE(p_trk->url);
        TBX_FREE(p_ibverbs_trk);

	err = NM_ESUCCESS;

	return err;
}

static int nm_ibverbs_gate_create(struct nm_cnx_rq*p_crq)
{
	int err = NM_ESUCCESS;
        struct nm_ibverbs_gate*p_ibverbs_gate = p_crq->p_gate->p_gate_drv_array[p_crq->p_drv->id]->info;
	if (!p_ibverbs_gate) {
                p_ibverbs_gate = TBX_MALLOC(sizeof (struct nm_ibverbs_gate));
                if (!p_ibverbs_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }
		p_ibverbs_gate->sock = -1;
		memset(p_ibverbs_gate->cnx_array, 0, sizeof(struct nm_ibverbs_cnx) * NM_IBVERBS_TRK_MAX);
		p_crq->p_gate->p_gate_drv_array[p_crq->p_drv->id]->info = p_ibverbs_gate;
        }
	
 out:
	return err;
}

static int nm_ibverbs_cnx_create(struct nm_cnx_rq*p_crq)
{
	const struct nm_drv*p_drv = p_crq->p_drv;
	const struct nm_gate*p_gate = p_crq->p_gate;
	const struct nm_trk*p_trk = p_crq->p_trk;
	struct nm_ibverbs_gate*p_ibverbs_gate = p_gate->p_gate_drv_array[p_drv->id]->info;
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_trk->id];
	struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
	
	int err = NM_ESUCCESS;
	/* register Memory Region */
	p_ibverbs_cnx->mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->buffer, sizeof(p_ibverbs_cnx->buffer),
				       IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if(p_ibverbs_cnx->mr == NULL) {
		fprintf(stderr, "Infiniband: cannot register MR.\n");
		err = -NM_EUNKNOWN;
		goto out;
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
	if(p_ibverbs_drv->ib_caps.max_inline == -1)
		p_ibverbs_drv->ib_caps.max_inline = qp_init_attr.cap.max_inline_data;

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
	/* init local address */
	p_ibverbs_cnx->local_addr = (struct nm_ibverbs_cnx_addr) {
		.lid   = p_ibverbs_drv->lid,
		.qpn   = p_ibverbs_cnx->qp->qp_num,
		.psn   = lrand48() & 0xffffff,
		.raddr = (uintptr_t)&p_ibverbs_cnx->buffer,
		.rkey  = p_ibverbs_cnx->mr->rkey
	};
	/* init state */
	p_ibverbs_cnx->window.next_out = 1;
	p_ibverbs_cnx->window.next_in  = 1;
	p_ibverbs_cnx->window.credits  = NM_IBVERBS_RBUF_NUM;
	p_ibverbs_cnx->window.to_ack   = 0;
	memset(&p_ibverbs_cnx->buffer, 0, sizeof(p_ibverbs_cnx->buffer));
 out:
	return err;
}

static int nm_ibverbs_cnx_connect(struct nm_cnx_rq*p_crq)
{
	const struct nm_drv*p_drv = p_crq->p_drv;
	const struct nm_gate*p_gate = p_crq->p_gate;
	const struct nm_trk*p_trk = p_crq->p_trk;
	struct nm_ibverbs_gate*p_ibverbs_gate = p_gate->p_gate_drv_array[p_drv->id]->info;
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_trk->id];

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

static int nm_ibverbs_connect(struct nm_cnx_rq *p_crq)
{
        struct nm_gate*p_gate = p_crq->p_gate; 
        struct nm_drv *p_drv  = p_crq->p_drv;
        struct nm_trk *p_trk  = p_crq->p_trk;
	int rc = -1;
	int err = nm_ibverbs_gate_create(p_crq);
	if(err)
		goto out;
        struct nm_ibverbs_gate*p_ibverbs_gate = p_gate->p_gate_drv_array[p_drv->id]->info;
	if(p_ibverbs_gate->sock == -1) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		assert(fd > -1);
		p_ibverbs_gate->sock = fd;
		char*sep = strchr(p_crq->remote_drv_url, ':');
		*sep = 0;
		char*peer_addr = p_crq->remote_drv_url;
		int peer_port = atoi(sep + 1);
		struct in_addr peer_inaddr;
		rc = inet_aton(peer_addr, &peer_inaddr);
		if(!rc) {
			fprintf(stderr, "Infiniband: cannot find host %s\n", peer_addr);
			err = -NM_EUNREACH;
			goto out;
		}
		struct sockaddr_in inaddr = {
			.sin_family = AF_INET,
			.sin_port   = htons(peer_port),
			.sin_addr   = peer_inaddr
		};
		rc = connect(fd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
		if(rc) {
			fprintf(stderr, "Infiniband: cannot connect to %s:%d\n", peer_addr, peer_port);
			err = -NM_EUNREACH;
			goto out;
		}
	}
	
	err = nm_ibverbs_cnx_create(p_crq);
	if(err)
		goto out;

	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_trk->id];
	rc = recv(p_ibverbs_gate->sock, &p_ibverbs_cnx->remote_addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));
	rc = send(p_ibverbs_gate->sock, &p_ibverbs_cnx->local_addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));


	err = nm_ibverbs_cnx_connect(p_crq);


 out:
	return err;
}

static int nm_ibverbs_accept(struct nm_cnx_rq *p_crq)
{
        struct nm_gate	     *p_gate	    = p_crq->p_gate;
        struct nm_drv	     *p_drv	    = p_crq->p_drv;
	struct nm_ibverbs_drv*p_ibverbs_drv = p_drv->priv;
        struct nm_trk	     *p_trk	    = p_crq->p_trk;
	int rc = -1;
	int err = nm_ibverbs_gate_create(p_crq);
	if(err)
		goto out;

        struct nm_ibverbs_gate*p_ibverbs_gate = p_gate->p_gate_drv_array[p_drv->id]->info;
	if(p_ibverbs_gate->sock == -1) {
		struct sockaddr_in addr;
		unsigned addr_len = sizeof addr;
		int fd = accept(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
		assert(fd > -1);
		p_ibverbs_gate->sock = fd;
	}

	err = nm_ibverbs_cnx_create(p_crq);
	if(err)
		goto out;
	
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_trk->id];
	rc = send(p_ibverbs_gate->sock, &p_ibverbs_cnx->local_addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));
	rc = recv(p_ibverbs_gate->sock, &p_ibverbs_cnx->remote_addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));

	err = nm_ibverbs_cnx_connect(p_crq);

 out:
	return err;
}

static int nm_ibverbs_disconnect(struct nm_cnx_rq *p_crq)
{
	int err;

	err = NM_ESUCCESS;

	return err;
}

/* ********************************************************* */

/* ** RDMA ************************************************* */

static int nm_ibverbs_rdma_send(const struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
				const struct nm_ibverbs_drv*__restrict__ p_ibverbs_drv,
				int size, const void*__restrict__ buf, const void*__restrict__ _raddr)
{
	const uintptr_t _lbase = (uintptr_t)&p_ibverbs_cnx->buffer;
	const uintptr_t _rbase = (uintptr_t)p_ibverbs_cnx->remote_addr.raddr;
	const uint64_t raddr   = (uint64_t)(((uintptr_t)_raddr - _lbase) + _rbase);
	struct ibv_sge list = {
		.addr   = (uintptr_t)buf,
		.length = size,
		.lkey   = p_ibverbs_cnx->mr->lkey
	};
	struct ibv_send_wr wr = {
		.wr_id      = NM_IBVERBS_WRID_SEND,
		.sg_list    = &list,
		.num_sge    = 1,
		.opcode     = IBV_WR_RDMA_WRITE,
		.send_flags = (size < p_ibverbs_drv->ib_caps.max_inline) ? (IBV_SEND_INLINE | IBV_SEND_SIGNALED) : IBV_SEND_SIGNALED,
		.wr.rdma =
		{
			.remote_addr = raddr,
			.rkey        = p_ibverbs_cnx->remote_addr.rkey
		}
	};
	struct ibv_send_wr*bad_wr = NULL;
	int rc = ibv_post_send(p_ibverbs_cnx->qp, &wr, &bad_wr);
	if(rc) {
		fprintf(stderr, "Infiniband: post RDMA send failed.\n");
		return -NM_EUNKNOWN;
	}
	return NM_ESUCCESS;
}

static int nm_ibverbs_rdma_wait(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx)
{
	struct ibv_wc wc;
	int ne = 0;
	do {
		ne = ibv_poll_cq(p_ibverbs_cnx->of_cq, 1, &wc);
		if(ne < 0) {
			fprintf(stderr, "Infiniband: poll out CQ failed.\n");
			return -NM_EUNKNOWN;
		}
	}
	while(ne == 0);
	if(ne != 1 || wc.status != IBV_WC_SUCCESS) {
		fprintf(stderr, "Infiniband: WC send failed (status=%d)\n", wc.status);
		return -NM_EUNKNOWN;
	}
	return NM_ESUCCESS;
}


static int nm_ibverbs_post_send_iov(struct nm_pkt_wrap*p_pw)
{
	const struct nm_ibverbs_drv*__restrict__ p_ibverbs_drv = p_pw->p_drv->priv;
	struct nm_ibverbs_gate*__restrict__ p_ibverbs_gate = p_pw->p_gdrv->info;
	struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
	int err = NM_ESUCCESS;

	const struct iovec*v = &p_pw->v[p_pw->v_first];
	const int n = p_pw->v_nb;
	int current_packet = 0;
	int pending_packet = 0;
	int i;
	int todo = 0, done = 0, v_done = 0;

	for(i = 0; i <  n; i++) {
		todo += v[i].iov_len;
	}
	i = 0;
	while(done < todo) { 
		current_packet = (current_packet + 1) % NM_IBVERBS_SBUF_NUM;
		struct nm_ibverbs_packet*__restrict__ packet = &p_ibverbs_cnx->buffer.sbuf[current_packet];
		int packet_size = 0;
		const int offset = (todo - done > NM_IBVERBS_BUF_SIZE) ? 0 : (NM_IBVERBS_BUF_SIZE - (todo - done));
		while(packet_size < NM_IBVERBS_BUF_SIZE && i < n) {
			const int fragment_size = ((v->iov_len - v_done) > NM_IBVERBS_BUF_SIZE) ?
				NM_IBVERBS_BUF_SIZE : (v->iov_len - v_done);
			memcpy(&packet->data[offset + packet_size], v->iov_base + v_done, fragment_size);
			packet_size += fragment_size;
			v_done += fragment_size;
			if(v_done >= v->iov_len) {
				i++;
				v_done = 0;
			}
		}
		assert(NM_IBVERBS_BUF_SIZE - offset == packet_size);

		packet->header = (struct nm_ibverbs_header) {
			.offset = offset,
			.size   = todo,
			.ack    = p_ibverbs_cnx->window.to_ack,
			.status = done ? NM_IBVERBS_STATUS_CONT : NM_IBVERBS_STATUS_DATA
		};
		done += packet_size;
		p_ibverbs_cnx->window.to_ack = 0;

		while(p_ibverbs_cnx->window.credits <= 1) {
			/* recover credits */
			const int rack = p_ibverbs_cnx->buffer.rack;
			if(rack) {
				p_ibverbs_cnx->window.credits += rack;
				p_ibverbs_cnx->buffer.rack = 0;
			}
			int j;
			for(j = 0; j < NM_IBVERBS_RBUF_NUM; j++) {
				if(p_ibverbs_cnx->buffer.rbuf[j].header.status) {
					p_ibverbs_cnx->window.credits += p_ibverbs_cnx->buffer.rbuf[j].header.ack;
					p_ibverbs_cnx->buffer.rbuf[j].header.ack = 0;
					p_ibverbs_cnx->buffer.rbuf[j].header.status &= ~NM_IBVERBS_STATUS_CREDITS;
				}
			}
		}
		
		nm_ibverbs_rdma_send(p_ibverbs_cnx, p_ibverbs_drv,
				     sizeof(struct nm_ibverbs_packet) - offset,
				     &packet->data[offset],
				     &p_ibverbs_cnx->buffer.rbuf[p_ibverbs_cnx->window.next_out].data[offset]);
		p_ibverbs_cnx->window.next_out = (p_ibverbs_cnx->window.next_out + 1) % NM_IBVERBS_RBUF_NUM;
		p_ibverbs_cnx->window.credits--;
		pending_packet++;
		if(pending_packet > 1)	{
			nm_ibverbs_rdma_wait(p_ibverbs_cnx);
			pending_packet--;
		}
	}
	while(pending_packet > 0) {
		nm_ibverbs_rdma_wait(p_ibverbs_cnx);
		pending_packet--;
	}
	
	return err;
}

static int nm_ibverbs_poll_send_iov(struct nm_pkt_wrap*p_pw)
{
	int err = NM_ESUCCESS;
	return err;
}

static int nm_ibverbs_poll_one(struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx,
			       const struct nm_ibverbs_drv*__restrict__ p_ibverbs_drv,
			       void*__restrict__ data, int len)
{
	int err = -NM_EUNKNOWN;
	struct nm_ibverbs_packet*__restrict__ packet = &p_ibverbs_cnx->buffer.rbuf[p_ibverbs_cnx->window.next_in];
	if(packet->header.status) {
		assert(packet->header.status & NM_IBVERBS_STATUS_DATA);
		const int msg_size = packet->header.size;
		int done = 0;
		assert(msg_size <= len);
		while(done < msg_size) {
			while(packet->header.status == 0) {
				/* nothing */
			}
			assert((done == 0 && (packet->header.status & NM_IBVERBS_STATUS_DATA)) ||
			       (done != 0 && (packet->header.status & NM_IBVERBS_STATUS_CONT)));
			const int packet_size = NM_IBVERBS_BUF_SIZE - packet->header.offset;
			memcpy(data + done, &packet->data[packet->header.offset], packet_size);
			done += packet_size;
			p_ibverbs_cnx->window.credits += packet->header.ack;
			packet->header.ack = 0;
			packet->header.status = 0;
			p_ibverbs_cnx->window.to_ack++;
			if(p_ibverbs_cnx->window.to_ack >= NM_IBVERBS_CREDITS_THR) {
				p_ibverbs_cnx->buffer.sack = p_ibverbs_cnx->window.to_ack;
				nm_ibverbs_rdma_send(p_ibverbs_cnx, p_ibverbs_drv,
						     sizeof(uint16_t),
						     (void*)&p_ibverbs_cnx->buffer.sack,
						     (void*)&p_ibverbs_cnx->buffer.rack);
				p_ibverbs_cnx->window.to_ack = 0;
				nm_ibverbs_rdma_wait(p_ibverbs_cnx);
			}
			p_ibverbs_cnx->window.next_in = (p_ibverbs_cnx->window.next_in + 1) % NM_IBVERBS_RBUF_NUM;
			packet = &p_ibverbs_cnx->buffer.rbuf[p_ibverbs_cnx->window.next_in];
		}
		err = NM_ESUCCESS;
	} else {
		err = -NM_EAGAIN;
	}
	return err;
}

static int nm_ibverbs_post_recv_iov(struct nm_pkt_wrap*p_pw)
{
	int err = nm_ibverbs_poll_recv_iov(p_pw);
	return err;
}

static int nm_ibverbs_poll_recv_iov(struct nm_pkt_wrap*p_pw)
{
	const struct nm_ibverbs_drv*p_ibverbs_drv = p_pw->p_drv->priv;
	int err;
	void*data = p_pw->v[p_pw->v_first].iov_base;
	const int len = p_pw->v[p_pw->v_first].iov_len;

	assert(p_pw->v_nb == 1);
	
	if(p_pw->p_gate) {
		struct nm_ibverbs_gate*__restrict__ p_ibverbs_gate = p_pw->p_gdrv->info;
		struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
		err = nm_ibverbs_poll_one(p_ibverbs_cnx, p_ibverbs_drv, data, len);
	} else {
		const struct nm_core*p_core = p_pw->p_drv->p_core;
		int i;
		for(i = 0; i < p_core->nb_gates; i++) {
			const struct nm_gate*__restrict__ p_gate = &p_core->gate_array[i];
			if(p_gate) {
				struct nm_ibverbs_gate*__restrict__ p_ibverbs_gate = p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
				struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
				err = nm_ibverbs_poll_one(p_ibverbs_cnx, p_ibverbs_drv, data, len);
				if(err != -NM_EAGAIN) {
					p_pw->p_gate = (struct nm_gate*)p_gate;
					goto out;
				}
			}
		}
		err = -NM_EAGAIN;
	}
 out:
	return err;
}

int nm_ibverbs_load(struct nm_drv_ops *p_ops)
{
        p_ops->init		= nm_ibverbs_init         ;
        p_ops->exit             = nm_ibverbs_exit         ;

        p_ops->open_trk		= nm_ibverbs_open_trk     ;
        p_ops->close_trk	= nm_ibverbs_close_trk    ;

        p_ops->connect		= nm_ibverbs_connect      ;
        p_ops->accept		= nm_ibverbs_accept       ;
        p_ops->disconnect       = nm_ibverbs_disconnect   ;

        p_ops->post_send_iov	= nm_ibverbs_post_send_iov;
        p_ops->post_recv_iov    = nm_ibverbs_post_recv_iov;

        p_ops->poll_send_iov    = nm_ibverbs_poll_send_iov;
        p_ops->poll_recv_iov    = nm_ibverbs_poll_recv_iov;

        return NM_ESUCCESS;
}

