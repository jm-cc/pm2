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

#include <nm_protected.h>

#include <infiniband/verbs.h>

#define NM_IBVERBS_CNX_MAX    256
#define NM_IBVERBS_TRK_MAX    256

#define NM_IBVERBS_PORT       1
#define NM_IBVERBS_TX_DEPTH   4
#define NM_IBVERBS_RX_DEPTH   2
#define NM_IBVERBS_RDMA_DEPTH 4
#define NM_IBVERBS_MAX_SG_SQ  2
#define NM_IBVERBS_MAX_SG_RQ  2
#define NM_IBVERBS_MAX_INLINE 128
#define NM_IBVERBS_WRID_SEND  1

/** Global state of the HCA
 */
struct nm_ibverbs_drv {
	struct ibv_context*context; /**< global IB context */
	struct ibv_pd*pd;           /**< global IB protection domain */
	uint16_t lid;               /**< local IB LID */
	int server_sock;            /**< socket used for connection */
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

struct nm_ibverbs_cnx {
	struct nm_ibverbs_cnx_addr local_addr;
	struct nm_ibverbs_cnx_addr remote_addr;
	struct ibv_mr*mr;
	struct ibv_qp*qp;
	struct ibv_cq*of_cq;
	struct ibv_cq*if_cq;
	struct {
		struct nm_ibverbs_buffer {
			char data[4096];
			volatile int busy;
			volatile int ack;
		} sbuf, rbuf;
	} buffer;
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
	fprintf(stderr, "Infiniband: found IB device '%s'\n", ibv_get_device_name(ib_dev));
	/* open IB context */
	p_ibverbs_drv->context = ibv_open_device(ib_dev);
	if(p_ibverbs_drv->context == NULL) {
		fprintf(stderr, "Infniband: cannot get IB context.\n");
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
	int rc = ibv_query_port(p_ibverbs_drv->context, NM_IBVERBS_PORT, &port_attr);
	if(rc != 0) {
		fprintf(stderr, "Infiniband: couldn't get local LID.\n");
		err = -NM_EUNKNOWN;
		goto out;
	}
	p_ibverbs_drv->lid = port_attr.lid;
	fprintf(stderr, "Infiniband: local LID  = 0x%02X\n", p_ibverbs_drv->lid);

        /* driver url encoding						*/
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
	char s_port[8]; /* port is 16 bits */
	snprintf(s_port, 8, "%d", ntohs(addr.sin_port));
        p_drv->url = tbx_strdup(s_port);

        /* driver capabilities encoding					*/
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
        p_trk->cap.iov_type	= nm_trk_iov_none;
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

static int nm_ibverbs_gate_create(struct nm_gate*p_gate,
				  struct nm_drv*p_drv,
				  struct nm_ibverbs_gate**pp_ibverbs_gate)
{
	int err = NM_ESUCCESS;

	if (!*pp_ibverbs_gate) {
                *pp_ibverbs_gate = TBX_MALLOC(sizeof (struct nm_ibverbs_gate));
                if (!*pp_ibverbs_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }
		(*pp_ibverbs_gate)->sock = -1;
		memset((*pp_ibverbs_gate)->cnx_array, 0, sizeof(struct nm_ibverbs_cnx) * NM_IBVERBS_TRK_MAX);
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
	fprintf(stderr, "Infiniband: create connection- trk=%d; gate=%d\n", p_trk->id, p_gate->id);
	/* register Memory Region */
	p_ibverbs_cnx->mr = ibv_reg_mr(p_ibverbs_drv->pd, &p_ibverbs_cnx->buffer,
				       2 * sizeof(struct nm_ibverbs_buffer),
				       IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if(p_ibverbs_cnx->mr == NULL) {
		fprintf(stderr, "Infniband: couldn't register MR\n");
		err = -NM_EUNKNOWN;
		goto out;
	}
	/* init incoming CQ */
	p_ibverbs_cnx->if_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_RX_DEPTH, NULL, NULL, 0);
	if(p_ibverbs_cnx->if_cq == NULL) {
		fprintf(stderr, "Infiniband: couldn't create in CQ\n");
		err = -NM_EUNKNOWN;
		goto out;
	}
	/* init outgoing CQ */
	p_ibverbs_cnx->of_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_TX_DEPTH, NULL, NULL, 0);
	if(p_ibverbs_cnx->of_cq == NULL) {
		fprintf(stderr, "Infiniband: couldn't create out CQ\n");
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
	if(p_ibverbs_drv->pd == NULL) {
		fprintf(stderr, "Infiniband: couldn't create QP\n");
		err = -NM_EUNKNOWN;
		goto out;
	}
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
  
	fprintf(stderr, "Infiniband: connect\n");
	/* modify QP- step: RTR */
	struct ibv_qp_attr attr = {
		.qp_state           = IBV_QPS_RTR,
		.path_mtu           = IBV_MTU_1024,
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
        struct nm_gate	       *p_gate	        = p_crq->p_gate; 
        struct nm_drv	       *p_drv	        = p_crq->p_drv;
        struct nm_trk	       *p_trk	        = p_crq->p_trk;
        struct nm_ibverbs_trk  *p_ibverbs_trk	= p_trk->priv;
        struct nm_ibverbs_gate**pp_ibverbs_gate = (struct nm_ibverbs_gate**)&p_gate->p_gate_drv_array[p_drv->id]->info;
	int rc = -1;
	int err = nm_ibverbs_gate_create(p_gate, p_drv, pp_ibverbs_gate);
	if(err)
		goto out;
	if((*pp_ibverbs_gate)->sock == -1) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		assert(fd > -1);
		(*pp_ibverbs_gate)->sock = fd;
		const char*peer_name = p_crq->remote_host_url;
		const int peer_port  = atoi(p_crq->remote_drv_url);
		const struct hostent*he = gethostbyname(peer_name);
		if(!he) {
			fprintf(stderr, "Infiniband: cannot find host %s\n", peer_name);
			err = -NM_EUNREACH;
			goto out;
		}
		struct sockaddr_in inaddr = {
			.sin_family = AF_INET,
			.sin_port   = htons(peer_port),
			.sin_addr   = *(struct in_addr*)he->h_addr
		};
		rc = connect(fd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
		if(rc) {
			fprintf(stderr, "Infiniband: cannot connect to %s:%d\n", peer_name, peer_port);
			err = -NM_EUNREACH;
			goto out;
		}
	}
	
	err = nm_ibverbs_cnx_create(p_crq);
	if(err)
		goto out;

	struct nm_ibverbs_cnx*p_ibverbs_cnx = &(*pp_ibverbs_gate)->cnx_array[p_trk->id];
	rc = recv((*pp_ibverbs_gate)->sock, &p_ibverbs_cnx->remote_addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));
	rc = send((*pp_ibverbs_gate)->sock, &p_ibverbs_cnx->local_addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));


	err = nm_ibverbs_cnx_connect(p_crq);


 out:
	return err;
}

static int nm_ibverbs_accept(struct nm_cnx_rq *p_crq)
{
        struct nm_gate		*p_gate		= p_crq->p_gate;
        struct nm_drv		*p_drv		= p_crq->p_drv;
	struct nm_ibverbs_drv   *p_ibverbs_drv  = p_drv->priv;
        struct nm_trk		*p_trk		= p_crq->p_trk;
        struct nm_ibverbs_trk	*p_ibverbs_trk	= p_trk->priv;
        struct nm_ibverbs_gate**pp_ibverbs_gate = (struct nm_ibverbs_gate**)&p_gate->p_gate_drv_array[p_drv->id]->info;
	int rc = -1;
	int err = nm_ibverbs_gate_create(p_gate, p_drv, pp_ibverbs_gate);
	if(err)
		goto out;

	if((*pp_ibverbs_gate)->sock == -1) {
		struct sockaddr_in addr;
		unsigned addr_len = sizeof addr;
		int fd = accept(p_ibverbs_drv->server_sock, (struct sockaddr*)&addr, &addr_len);
		assert(fd > -1);
		(*pp_ibverbs_gate)->sock = fd;
	}

	err = nm_ibverbs_cnx_create(p_crq);
	if(err)
		goto out;
	
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &(*pp_ibverbs_gate)->cnx_array[p_trk->id];
	rc = send((*pp_ibverbs_gate)->sock, &p_ibverbs_cnx->local_addr, sizeof(struct nm_ibverbs_cnx_addr), 0);
	assert(rc == sizeof(struct nm_ibverbs_cnx_addr));
	rc = recv((*pp_ibverbs_gate)->sock, &p_ibverbs_cnx->remote_addr, sizeof(struct nm_ibverbs_cnx_addr), MSG_WAITALL);
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

static int nm_ibverbs_rdma_send(struct nm_ibverbs_cnx*p_ibverbs_cnx, int size, const void*buf, const void*_raddr)
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
		.send_flags = IBV_SEND_SIGNALED,
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

static int nm_ibverbs_rdma_wait(struct nm_ibverbs_cnx*p_ibverbs_cnx)
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
	struct nm_ibverbs_gate*p_ibverbs_gate = p_pw->p_gdrv->info;
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
	const void*data = p_pw->v[p_pw->v_first].iov_base;
	const int len   = p_pw->v[p_pw->v_first].iov_len;
	int err = NM_ESUCCESS;
	
	assert(p_pw->v_nb == 1);

	fprintf(stderr, "Infiniband: trk #%d; send len=%d\n", p_pw->p_trk->id, len);

	memcpy(p_ibverbs_cnx->buffer.sbuf.data, data, len);
	p_ibverbs_cnx->buffer.sbuf.busy = 1;
	p_ibverbs_cnx->buffer.rbuf.ack  = 0;

	nm_ibverbs_rdma_send(p_ibverbs_cnx, len,
			     &p_ibverbs_cnx->buffer.sbuf.data[0],
			     &p_ibverbs_cnx->buffer.rbuf.data[0]);
	nm_ibverbs_rdma_wait(p_ibverbs_cnx);

	nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(int),
			     (void*)&p_ibverbs_cnx->buffer.sbuf.busy,
			     (void*)&p_ibverbs_cnx->buffer.rbuf.busy);
	nm_ibverbs_rdma_wait(p_ibverbs_cnx);

	return err;
}

static int nm_ibverbs_poll_send_iov(struct nm_pkt_wrap*p_pw)
{
	struct nm_ibverbs_gate*p_ibverbs_gate = p_pw->p_gdrv->info;
	struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
	int err;
	if(p_ibverbs_cnx->buffer.rbuf.ack) {
		p_ibverbs_cnx->buffer.sbuf.busy = 0;
		p_ibverbs_cnx->buffer.rbuf.ack  = 0;
		err =  NM_ESUCCESS;
		fprintf(stderr, "Infiniband: send done.\n");
	} else {
		err = -NM_EAGAIN;
	}
	return err;
}

static int nm_ibverbs_poll_one(struct nm_ibverbs_cnx*p_ibverbs_cnx, void*data, int len)
{
	int err = -NM_EUNKNOWN;
	if(p_ibverbs_cnx->buffer.rbuf.busy) {
		memcpy(data, &p_ibverbs_cnx->buffer.rbuf.data[0], len);
		p_ibverbs_cnx->buffer.rbuf.busy = 0;
		p_ibverbs_cnx->buffer.sbuf.ack  = 1;
		nm_ibverbs_rdma_send(p_ibverbs_cnx, sizeof(int),
				     (void*)&p_ibverbs_cnx->buffer.sbuf.ack,
				     (void*)&p_ibverbs_cnx->buffer.rbuf.ack);
		nm_ibverbs_rdma_wait(p_ibverbs_cnx);
		err = NM_ESUCCESS;
	} else {
		err = -NM_EAGAIN;
	}
	return err;
}

static int nm_ibverbs_post_recv_iov(struct nm_pkt_wrap*p_pw)
{
	const int len = p_pw->v[0].iov_len;
	fprintf(stderr, "Infiniband: trk #%d; post receive- len=%d\n", p_pw->p_trk->id, len);
	int err = nm_ibverbs_poll_recv_iov(p_pw);
	return err;
}

static int nm_ibverbs_poll_recv_iov(struct nm_pkt_wrap*p_pw)
{
	int err;
	void*data = p_pw->v[p_pw->v_first].iov_base;
	const int len = p_pw->v[p_pw->v_first].iov_len;

	assert(p_pw->v_nb == 1);

	if(p_pw->p_gate) {
		struct nm_ibverbs_gate*p_ibverbs_gate = p_pw->p_gdrv->info;
		struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
		err = nm_ibverbs_poll_one(p_ibverbs_cnx, data, len);
	} else {
		const struct nm_core*p_core = p_pw->p_drv->p_core;
		int i;
		for(i = 0; i < p_core->nb_gates; i++) {
			const struct nm_gate*p_gate = &p_core->gate_array[i];
			if(p_gate) {
				struct nm_ibverbs_gate*p_ibverbs_gate = p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
				struct nm_ibverbs_cnx*p_ibverbs_cnx = &p_ibverbs_gate->cnx_array[p_pw->p_trk->id];
				err = nm_ibverbs_poll_one(p_ibverbs_cnx, data, len);
				if(err != -NM_EAGAIN) {
					p_pw->p_gate = p_gate;
					goto out;
				}
			}
		}
		err = -NM_EAGAIN;
	}
 out:
	if(err == NM_ESUCCESS) {
		fprintf(stderr, "Infiniband: trk #%d; received- len=%d\n", p_pw->p_trk->id, len);
	}
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

