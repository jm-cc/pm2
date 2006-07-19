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

#include <sisci_api.h>

#include "nm_sisci_private.h"

#define NO_FLAGS	0
#define NO_CALLBACK	0
#define NO_ARG		0

struct nm_sisci_cnx_seg {
        uint32_t		flags;
        uint32_t		padding;
        uint32_t		node_id;
        uint32_t		seg_id;
};

struct nm_sisci_seg {
        uint8_t	dummy[65536];
};

struct nm_sisci_drv {
        sci_desc_t		sci_dev;
        sci_local_segment_t	sci_l_cnx_seg;
        unsigned int		l_node_id;
        int			next_seg_id;
};

struct nm_sisci_trk {
        int sisci;
};

struct nm_sisci_cnx {
        int sisci;
};

struct nm_sisci_segment {
        sci_local_segment_t	 sci_l_seg;
        sci_map_t		 sci_l_map;
        struct nm_sisci_seg	*l_seg;

        sci_remote_segment_t	 sci_r_seg;
        sci_map_t	 	 sci_r_map;
        sci_sequence_t		 sci_r_seq;
        volatile struct nm_sisci_seg	*r_seg;
};

struct nm_sisci_gate {
        struct nm_sisci_segment	seg_array[2];
};

struct nm_sisci_pkt_wrap {
        int sisci;
};

static
int
nm_sisci_init			(struct nm_drv *p_drv) {
        struct nm_sisci_drv	*p_sisci_drv	= NULL;
        sci_desc_t		 sci_dev;
        unsigned int		 l_node_id;
        sci_query_adapter_t	 query;
        sci_local_segment_t	 sci_l_seg;
        sci_error_t		 sci_err;
	int err;

#define _CHK_ \
	do {						\
        	if (sci_err	!= SCI_ERR_OK) {	\
        	        err = -NM_ESCFAILD;     	\
        	        goto out;               	\
        	}					\
	} while (0)

        /* SiSCI runtime initialization
         */
        SCIInitialize(NO_FLAGS, &sci_err);
        _CHK_;

        /* SCI device opening
         */
        SCIOpen(&sci_dev, NO_FLAGS, &sci_err);
        _CHK_;

        /* Node id of the local adapter 0
         */
        query.subcommand	= SCI_Q_ADAPTER_NODEID;
        query.localAdapterNo	= 0;
        query.data		= &l_node_id;

        SCIQuery(SCI_Q_ADAPTER, &query, NO_FLAGS, &sci_err);
        _CHK_;

        /* Connection segment setup
         */
        SCICreateSegment(sci_dev, &sci_l_seg, 0, sizeof(struct nm_sisci_cnx_seg),
                         NO_CALLBACK, NO_ARG, NO_FLAGS, &sci_err);
        _CHK_;
        SCIPrepareSegment(sci_l_seg, 0, NO_FLAGS, &sci_err);
        _CHK_;

#undef _CHK_

        /* private data							*/
	p_sisci_drv	= TBX_MALLOC(sizeof (struct nm_sisci_drv));
        if (!p_sisci_drv) {
                err = -NM_ENOMEM;
                goto out;
        }

        memset(p_sisci_drv, 0, sizeof (struct nm_sisci_drv));
        p_sisci_drv->sci_dev		= sci_dev;
        p_sisci_drv->sci_l_cnx_seg	= sci_l_seg;
        p_sisci_drv->l_node_id		= l_node_id;
        p_sisci_drv->next_seg_id	= 1;

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
        sci_error_t		 sci_err;
	int err;

        p_sisci_drv	= p_drv->priv;

#define _CHK_ \
	do {						\
        	if (sci_err	!= SCI_ERR_OK) {	\
        	        err = -NM_ESCFAILD;     	\
        	        goto out;               	\
        	}					\
	} while (0)

        SCIRemoveSegment(p_sisci_drv->sci_l_cnx_seg, NO_FLAGS, &sci_err);
        _CHK_;
        SCIClose(p_sisci_drv->sci_dev, NO_FLAGS, &sci_err);
        _CHK_;

#undef _CHK_

        TBX_FREE(p_drv->url);
        TBX_FREE(p_sisci_drv);
        SCITerminate();

	err = NM_ESUCCESS;

 out:
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
        struct nm_sisci_drv	*p_sisci_drv	= NULL;
        struct nm_sisci_trk	*p_sisci_trk	= NULL;

        sci_desc_t		 sci_dev;

        unsigned int		 l_node_id;
        sci_local_segment_t	 sci_l_cnx_seg;
        sci_map_t		 sci_l_cnx_map;
        struct nm_sisci_cnx_seg *l_cnx_seg;

        unsigned int		 r_node_id;
        unsigned int		 r_seg_id;
        sci_remote_segment_t	 sci_r_cnx_seg;
        sci_map_t	 	 sci_r_cnx_map;
        sci_sequence_t		 sci_r_cnx_seq;
        volatile struct nm_sisci_cnx_seg *r_cnx_seg;

        sci_error_t		 sci_err;
        int i;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_sisci_drv	= p_drv->priv;
        p_sisci_trk	= p_trk->priv;

        sci_dev		= p_sisci_drv->sci_dev;
        sci_l_cnx_seg	= p_sisci_drv->sci_l_cnx_seg;
        l_node_id	= p_sisci_drv->l_node_id;
        r_node_id	= strtoul(p_crq->remote_drv_url, (char **)NULL, 10);

        NM_TRACE_VAL("sisci connect remote node id", (int)r_node_id);

        /* private data
         */
        p_sisci_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_sisci_gate) {
                p_sisci_gate	= TBX_MALLOC(sizeof (struct nm_sisci_gate));
                if (!p_sisci_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_sisci_gate, 0, sizeof(struct nm_sisci_gate));

                /* update gate private data
                 */
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_sisci_gate;
        }

#define _CHK_ \
	do {						\
        	if (sci_err	!= SCI_ERR_OK) {	\
        	        err = -NM_ESCFAILD;     	\
        	        goto out;               	\
        	}					\
	} while (0)

        /* map and clear local segment
         */
        l_cnx_seg	=
                SCIMapLocalSegment(sci_l_cnx_seg, &sci_l_cnx_map,
                                   0, sizeof(struct nm_sisci_cnx_seg), NULL,
                                   NO_FLAGS, &sci_err);
        _CHK_;
        memset(l_cnx_seg, 0, sizeof(struct nm_sisci_cnx_seg));

        /* unlock local segment
         */
        SCISetSegmentAvailable(sci_l_cnx_seg, 0, NO_FLAGS, &sci_err);
        _CHK_;

        /* connect to the remote segment
         */
        SCIConnectSegment(sci_dev, &sci_r_cnx_seg, r_node_id, 0,
                          0, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT,
                          NO_FLAGS, &sci_err);
        _CHK_;
        r_cnx_seg	=
                SCIMapRemoteSegment(sci_r_cnx_seg, &sci_r_cnx_map,
                                    0, sizeof(struct nm_sisci_cnx_seg), NULL,
                                    NO_FLAGS, &sci_err);
        _CHK_;
        SCICreateMapSequence(sci_r_cnx_map, &sci_r_cnx_seq, NO_FLAGS, &sci_err);
        _CHK_;


        /* ... exchange data   ... */
        r_cnx_seg->node_id	= l_node_id;
        r_cnx_seg->seg_id	= p_sisci_drv->next_seg_id;
        SCIStoreBarrier(sci_r_cnx_seq, NO_FLAGS);

        r_cnx_seg->flags	= 1;
        SCIStoreBarrier(sci_r_cnx_seq, NO_FLAGS);

        /* ... read ... */
        while (!(volatile int)l_cnx_seg->flags)
                ;

        r_seg_id	= l_cnx_seg->seg_id;
        /* --- end of exchange --- */


        /* setup data segments */
        for (i = 0; i < 2; i++) {
                sci_local_segment_t	 sci_l_seg;
                sci_map_t		 sci_l_map;
                struct nm_sisci_seg 	*l_seg;

                sci_remote_segment_t	 sci_r_seg;
                sci_map_t	 	 sci_r_map;
                sci_sequence_t		 sci_r_seq;
                volatile struct nm_sisci_seg *r_seg;

                /* create local */
                SCICreateSegment(sci_dev, &sci_l_seg, 0,
                                 sizeof(struct nm_sisci_cnx_seg),
                                 NO_CALLBACK, NO_ARG, NO_FLAGS, &sci_err);
                _CHK_;
                /* prepare local */
                SCIPrepareSegment(sci_l_seg, 0, NO_FLAGS, &sci_err);
                _CHK_;

                /* map local */
                l_seg	=
                SCIMapLocalSegment(sci_l_seg, &sci_l_map,
                                   0, sizeof(struct nm_sisci_seg), NULL,
                                   NO_FLAGS, &sci_err);
                _CHK_;
                /* clear local */
                memset(l_seg, 0, sizeof(struct nm_sisci_seg));

                /* unlock local */
                SCISetSegmentAvailable(sci_l_seg, 0, NO_FLAGS, &sci_err);
                _CHK_;


                /* connect remote */
                SCIConnectSegment(sci_dev, &sci_r_seg, r_node_id, r_seg_id+i,
                                  0, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT,
                                  NO_FLAGS, &sci_err);
                _CHK_;

                /* map remote */
                r_seg	=
                SCIMapRemoteSegment(sci_r_seg, &sci_r_map,
                                    0, sizeof(struct nm_sisci_seg), NULL,
                                    NO_FLAGS, &sci_err);
                _CHK_;

                /* sequence remote */
                SCICreateMapSequence(sci_r_map, &sci_r_seq, NO_FLAGS, &sci_err);
                _CHK_;


                /* fill gate struct */
                p_sisci_gate->seg_array[i].sci_l_seg	= sci_l_seg;
                p_sisci_gate->seg_array[i].sci_l_map	= sci_l_map;
                p_sisci_gate->seg_array[i].l_seg    	= l_seg    ;

                p_sisci_gate->seg_array[i].sci_r_seg	= sci_r_seg;
                p_sisci_gate->seg_array[i].sci_r_map	= sci_r_map;
                p_sisci_gate->seg_array[i].sci_r_seq	= sci_r_seq;
                p_sisci_gate->seg_array[i].r_seg    	= r_seg    ;
        }


        /* unmap segments
         */
        SCIUnmapSegment(sci_r_cnx_map, NO_FLAGS, &sci_err);
        _CHK_;
        SCIUnmapSegment(sci_l_cnx_map, NO_FLAGS, &sci_err);
        _CHK_;

        /* disconnect remote
         */
        SCIRemoveSequence(sci_r_cnx_seq, NO_FLAGS, &sci_err);
        _CHK_;
        SCIDisconnectSegment(sci_r_cnx_seg, NO_FLAGS, &sci_err);
        _CHK_;

        /* lock cnx segment
         */
        SCISetSegmentUnavailable(p_sisci_drv->sci_l_cnx_seg, 0, NO_FLAGS,
                                 &sci_err);
        _CHK_;

#undef _CHK_

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
        struct nm_sisci_drv	*p_sisci_drv	= NULL;
        struct nm_sisci_trk	*p_sisci_trk	= NULL;

        sci_desc_t		 sci_dev;

        unsigned int		 l_node_id;
        sci_local_segment_t	 sci_l_cnx_seg;
        sci_map_t		 sci_l_cnx_map;
        struct nm_sisci_cnx_seg *l_cnx_seg;

        unsigned int		 r_node_id;
        unsigned int		 r_seg_id;
        sci_remote_segment_t	 sci_r_cnx_seg;
        sci_map_t	 	 sci_r_cnx_map;
        sci_sequence_t		 sci_r_cnx_seq;
        volatile struct nm_sisci_cnx_seg *r_cnx_seg;

        sci_error_t		 sci_err;
        int i;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_sisci_trk	= p_trk->priv;

        sci_dev		= p_sisci_drv->sci_dev;
        sci_l_cnx_seg	= p_sisci_drv->sci_l_cnx_seg;
        l_node_id	= p_sisci_drv->l_node_id;

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

#define _CHK_ \
	do {						\
        	if (sci_err	!= SCI_ERR_OK) {	\
        	        err = -NM_ESCFAILD;     	\
        	        goto out;               	\
        	}					\
	} while (0)

        /* map and clear local segment
         */
        l_cnx_seg	=
                SCIMapLocalSegment(sci_l_cnx_seg, &sci_l_cnx_map,
                                   0, sizeof(struct nm_sisci_cnx_seg), NULL,
                                   NO_FLAGS, &sci_err);
        _CHK_;
        memset(l_cnx_seg, 0, sizeof(struct nm_sisci_cnx_seg));

        /* unlock local segment
         */
        SCISetSegmentAvailable(sci_l_cnx_seg, 0, NO_FLAGS, &sci_err);
        _CHK_;

        /* ... read ... */
        while (!(volatile int)l_cnx_seg->flags)
                ;

        r_node_id	= l_cnx_seg->node_id;
        r_seg_id	= l_cnx_seg->seg_id;

        NM_TRACE_VAL("sisci accept remote node id", (int)r_node_id);

        /* connect to the remote segment
         */
        SCIConnectSegment(sci_dev, &sci_r_cnx_seg, r_node_id, 0,
                          0, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT,
                          NO_FLAGS, &sci_err);
        _CHK_;
        r_cnx_seg	=
                SCIMapRemoteSegment(sci_r_cnx_seg, &sci_r_cnx_map,
                                    0, sizeof(struct nm_sisci_cnx_seg), NULL,
                                    NO_FLAGS, &sci_err);
        _CHK_;
        SCICreateMapSequence(sci_r_cnx_map, &sci_r_cnx_seq, NO_FLAGS, &sci_err);
        _CHK_;

        /* ... write ... */
        r_cnx_seg->node_id	= l_node_id;
        r_cnx_seg->seg_id	= p_sisci_drv->next_seg_id;
        SCIStoreBarrier(sci_r_cnx_seq, NO_FLAGS);

        r_cnx_seg->flags	= 1;
        SCIStoreBarrier(sci_r_cnx_seq, NO_FLAGS);
        /* --- end of exchange --- */


        /* setup data segments */
        for (i = 0; i < 2; i++) {
                sci_local_segment_t	 sci_l_seg;
                sci_map_t		 sci_l_map;
                struct nm_sisci_seg 	*l_seg;

                sci_remote_segment_t	 sci_r_seg;
                sci_map_t	 	 sci_r_map;
                sci_sequence_t		 sci_r_seq;
                volatile struct nm_sisci_seg *r_seg;

                /* create local */
                SCICreateSegment(sci_dev, &sci_l_seg, 0,
                                 sizeof(struct nm_sisci_cnx_seg),
                                 NO_CALLBACK, NO_ARG, NO_FLAGS, &sci_err);
                _CHK_;
                /* prepare local */
                SCIPrepareSegment(sci_l_seg, 0, NO_FLAGS, &sci_err);
                _CHK_;

                /* map local */
                l_seg	=
                SCIMapLocalSegment(sci_l_seg, &sci_l_map,
                                   0, sizeof(struct nm_sisci_seg), NULL,
                                   NO_FLAGS, &sci_err);
                _CHK_;
                /* clear local */
                memset(l_seg, 0, sizeof(struct nm_sisci_seg));

                /* unlock local */
                SCISetSegmentAvailable(sci_l_seg, 0, NO_FLAGS, &sci_err);
                _CHK_;


                /* connect remote */
                SCIConnectSegment(sci_dev, &sci_r_seg, r_node_id, r_seg_id+i,
                                  0, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT,
                                  NO_FLAGS, &sci_err);
                _CHK_;

                /* map remote */
                r_seg	=
                SCIMapRemoteSegment(sci_r_seg, &sci_r_map,
                                    0, sizeof(struct nm_sisci_seg), NULL,
                                    NO_FLAGS, &sci_err);
                _CHK_;

                /* sequence remote */
                SCICreateMapSequence(sci_r_map, &sci_r_seq, NO_FLAGS, &sci_err);
                _CHK_;


                /* fill gate struct */
                p_sisci_gate->seg_array[i].sci_l_seg	= sci_l_seg;
                p_sisci_gate->seg_array[i].sci_l_map	= sci_l_map;
                p_sisci_gate->seg_array[i].l_seg    	= l_seg    ;

                p_sisci_gate->seg_array[i].sci_r_seg	= sci_r_seg;
                p_sisci_gate->seg_array[i].sci_r_map	= sci_r_map;
                p_sisci_gate->seg_array[i].sci_r_seq	= sci_r_seq;
                p_sisci_gate->seg_array[i].r_seg    	= r_seg    ;
        }


        /* unmap segments
         */
        SCIUnmapSegment(sci_r_cnx_map, NO_FLAGS, &sci_err);
        _CHK_;
        SCIUnmapSegment(sci_l_cnx_map, NO_FLAGS, &sci_err);
        _CHK_;

        /* disconnect remote
         */
        SCIRemoveSequence(sci_r_cnx_seq, NO_FLAGS, &sci_err);
        _CHK_;
        SCIDisconnectSegment(sci_r_cnx_seg, NO_FLAGS, &sci_err);
        _CHK_;

        /* lock cnx segment
         */
        SCISetSegmentUnavailable(p_sisci_drv->sci_l_cnx_seg, 0, NO_FLAGS,
                                 &sci_err);
        _CHK_;

#undef _CHK_

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

