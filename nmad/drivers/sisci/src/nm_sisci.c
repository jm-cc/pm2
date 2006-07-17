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


#include <stdint.h>
#include <limits.h>
#include <sys/uio.h>
#include <tbx.h>

#include "nm_sisci_private.h"

struct nm_sisci_drv {
        int sisci;
};

struct nm_sisci_trk {
        int sisci;
};

struct nm_sisci_cnx {
        int sisci;
};

struct nm_sisci_gate {
        int sisci;
};

struct nm_sisci_pkt_wrap {
        int sisci;
};

static
int
nm_sisci_init			(struct nm_drv *p_drv) {
        struct nm_sisci_drv	*p_sisci_drv	= NULL;
	int err;

        /* private data							*/
	p_sisci_drv	= TBX_MALLOC(sizeof (struct nm_sisci_drv));
        if (!p_sisci_drv) {
                err = -NM_ENOMEM;
                goto out;
        }

        memset(p_sisci_drv, 0, sizeof (struct nm_sisci_drv));
        p_drv->priv	= p_sisci_drv;

        /* driver url encoding						*/
        p_drv->url	= tbx_strdup("-");

        /* driver capabilities encoding					*/
        p_drv->cap.has_trk_rq_dgram			= 1;
        p_drv->cap.has_selective_receive		= 0;
        p_drv->cap.has_concurrent_selective_receive	= 0;

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_sisci_exit			(struct nm_drv *p_drv) {
        struct nm_sisci_drv	*p_sisci_drv	= NULL;
	int err;

        p_sisci_drv	= p_drv->priv;

        TBX_FREE(p_drv->url);
        TBX_FREE(p_sisci_drv);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_open_trk		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_sisci_trk	*p_sisci_trk	= NULL;
	int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_sisci_trk	= TBX_MALLOC(sizeof (struct nm_sisci_trk));
        if (!p_sisci_trk) {
                err = -NM_ENOMEM;
                goto out;
        }
        memset(p_sisci_trk, 0, sizeof (struct nm_sisci_trk));
        p_trk->priv	= p_sisci_trk;

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

static
int
nm_sisci_close_trk		(struct nm_trk *p_trk) {
        struct nm_sisci_trk	*p_sisci_trk	= NULL;
	int err;

        p_sisci_trk	= p_trk->priv;
        TBX_FREE(p_trk->url);
        TBX_FREE(p_sisci_trk);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_connect		(struct nm_cnx_rq *p_crq) {
        struct nm_sisci_gate	*p_sisci_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_sisci_trk	*p_sisci_trk	= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_sisci_trk	= p_trk->priv;

        /* private data				*/
        p_sisci_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_sisci_gate) {
                p_sisci_gate	= TBX_MALLOC(sizeof (struct nm_sisci_gate));
                if (!p_sisci_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_sisci_gate, 0, sizeof(struct nm_sisci_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_sisci_gate;
        }

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_sisci_accept			(struct nm_cnx_rq *p_crq) {
        struct nm_sisci_gate	*p_sisci_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_sisci_trk	*p_sisci_trk	= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_sisci_trk	= p_trk->priv;

        /* private data				*/
        p_sisci_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_sisci_gate) {
                p_sisci_gate	= TBX_MALLOC(sizeof (struct nm_sisci_gate));
                if (!p_sisci_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_sisci_gate, 0, sizeof(struct nm_sisci_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_sisci_gate;
        }

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_sisci_disconnect		(struct nm_cnx_rq *p_crq) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_post_send_iov		(struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_post_recv_iov		(struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_poll_send_iov    	(struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_poll_recv_iov    	(struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

int
nm_sisci_load(struct nm_drv_ops *p_ops) {
        p_ops->init		= nm_sisci_init         ;
        p_ops->exit             = nm_sisci_exit         ;

        p_ops->open_trk		= nm_sisci_open_trk     ;
        p_ops->close_trk	= nm_sisci_close_trk    ;

        p_ops->connect		= nm_sisci_connect      ;
        p_ops->accept		= nm_sisci_accept       ;
        p_ops->disconnect       = nm_sisci_disconnect   ;

        p_ops->post_send_iov	= nm_sisci_post_send_iov;
        p_ops->post_recv_iov    = nm_sisci_post_recv_iov;

        p_ops->poll_send_iov    = nm_sisci_poll_send_iov;
        p_ops->poll_recv_iov    = nm_sisci_poll_recv_iov;

        return NM_ESUCCESS;
}

