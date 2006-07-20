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

#define DATA_SEG_SIZE		65536
#define FLAG_AREA_SIZE		   64
#define FLAGS_AREA_SIZE		 4096
#define DATA_AREA_SIZE		(DATA_SEG_SIZE - FLAGS_AREA_SIZE)

#define INITIAL_PW_NUM		16

struct nm_sisci_cnx_seg {
        uint32_t		flags;
        uint32_t		padding;
        uint32_t		node_id;
        uint32_t		seg_id;
};

/* Data segment */
struct nm_sisci_seg {

        /* union enforcing segment length */
        union {

                /* area actually used */
                struct {

                        /* union enforcing control area size */
                        union {

                                /* control area */
                                struct {
                                        /* read flag */
                                        union {
                                                volatile int r;
                                                uint8_t	__r_dummy[FLAG_AREA_SIZE];
                                        };

                                        /* write flag */
                                        union {
                                                volatile int w;
                                                uint8_t __w_dummy[FLAG_AREA_SIZE];
                                        };
                                };

                                uint8_t __f_dummy[FLAGS_AREA_SIZE];
                        };

                        /* data area */
                        uint8_t d[DATA_AREA_SIZE];
                };

                uint8_t	__d_dummy[DATA_SEG_SIZE];
        };
};

struct nm_sisci_drv {
        sci_desc_t		sci_dev;
        sci_local_segment_t	sci_l_cnx_seg;
        unsigned int		l_node_id;
        int			next_seg_id;
        p_tbx_memory_t 		sci_pw_mem;
};

struct nm_sisci_trk {
        uint8_t			  next_cnx_id;
        uint8_t			  last_active_id;

        /* gate id reverse mapping */
        struct nm_gate		**gate_map;
        struct nm_sisci_cnx	**cnx_map;
};

struct nm_sisci_segment {
        sci_local_segment_t	 sci_l_seg;
        sci_map_t		 sci_l_map;
        struct nm_sisci_seg	*l_seg;

        sci_remote_segment_t	 sci_r_seg;
        sci_map_t	 	 sci_r_map;
        sci_sequence_t		 sci_r_seq;
        volatile struct nm_sisci_seg	*r_seg;
        volatile int		 write_flag_flushed;
};

struct nm_sisci_cnx {
        struct nm_sisci_segment	seg_array[2];
};

struct nm_sisci_gate {
        struct nm_sisci_cnx	cnx_array[255];
};

struct nm_sisci_pkt_wrap {
        struct nm_iovec_iter	 vi;
        int			 next_seg;
        struct nm_sisci_cnx	*p_sisci_cnx;
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

        tbx_malloc_init(&(p_sisci_drv->sci_pw_mem),
                        sizeof(struct nm_sisci_pkt_wrap),
                        INITIAL_PW_NUM,   "nmad/sisci/pw");

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
nm_sisci_cnx_setup		(struct nm_sisci_cnx	*p_sisci_cnx,
                                 sci_desc_t		 sci_dev,
                                 unsigned int		 r_node_id,
                                 unsigned int		 r_seg_id) {
        sci_error_t	sci_err;
        int i;
        int err;

#define _CHK_ \
	do {						\
        	if (sci_err	!= SCI_ERR_OK) {	\
        	        err = -NM_ESCFAILD;     	\
        	        goto out;               	\
        	}					\
	} while (0)

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
                p_sisci_cnx->seg_array[i].sci_l_seg	= sci_l_seg;
                p_sisci_cnx->seg_array[i].sci_l_map	= sci_l_map;
                p_sisci_cnx->seg_array[i].l_seg    	= l_seg    ;

                p_sisci_cnx->seg_array[i].sci_r_seg	= sci_r_seg;
                p_sisci_cnx->seg_array[i].sci_r_map	= sci_r_map;
                p_sisci_cnx->seg_array[i].sci_r_seq	= sci_r_seq;
                p_sisci_cnx->seg_array[i].r_seg    	= r_seg    ;

                /* Allow remote process to write
                 */
                r_seg->w	= 1;
                SCIStoreBarrier(sci_r_seq, NO_FLAGS);
                p_sisci_cnx->seg_array[i].write_flag_flushed	= 1;
        }

#undef _CHK_


	err = NM_ESUCCESS;

 out:
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
        struct nm_sisci_cnx	*p_sisci_cnx	= NULL;

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

        if (p_sisci_trk->next_cnx_id) {
                p_sisci_trk->gate_map	=
                        TBX_REALLOC(p_sisci_trk->gate_map,
                                    sizeof(struct nm_gate *)
                                    * (p_sisci_trk->next_cnx_id+1));
                p_sisci_trk->cnx_map	=
                        TBX_REALLOC(p_sisci_trk->gate_map,
                                    sizeof(struct nm_sisci_cnx *)
                                    * (p_sisci_trk->next_cnx_id+1));
        } else {
                p_sisci_trk->gate_map	= TBX_MALLOC(sizeof(struct nm_gate *));
                p_sisci_trk->cnx_map	=
                        TBX_MALLOC(sizeof(struct nm_sisci_cnx *));
        }

        p_sisci_trk->gate_map[p_sisci_trk->next_cnx_id]	= p_gate;
        p_sisci_trk->cnx_map[p_sisci_trk->next_cnx_id]	=
                p_sisci_gate->cnx_array + p_trk->id;
        p_sisci_trk->next_cnx_id++;

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


        p_sisci_cnx	= p_sisci_gate->cnx_array + p_trk->id;
        err = nm_sisci_cnx_setup(p_sisci_cnx, sci_dev, r_node_id, r_seg_id);
        if (err != NM_ESUCCESS)
                goto out;

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

        p_sisci_drv->next_seg_id = p_sisci_drv->next_seg_id + 2;

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
        struct nm_sisci_cnx	*p_sisci_cnx	= NULL;

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

        if (p_sisci_trk->next_cnx_id) {
                p_sisci_trk->gate_map	=
                        TBX_REALLOC(p_sisci_trk->gate_map,
                                    sizeof(struct nm_gate *)
                                    * (p_sisci_trk->next_cnx_id+1));
                p_sisci_trk->cnx_map	=
                        TBX_REALLOC(p_sisci_trk->gate_map,
                                    sizeof(struct nm_sisci_cnx *)
                                    * (p_sisci_trk->next_cnx_id+1));
        } else {
                p_sisci_trk->gate_map	= TBX_MALLOC(sizeof(struct nm_gate *));
                p_sisci_trk->cnx_map	=
                        TBX_MALLOC(sizeof(struct nm_sisci_cnx *));
        }

        p_sisci_trk->gate_map[p_sisci_trk->next_cnx_id]	= p_gate;
        p_sisci_trk->cnx_map[p_sisci_trk->next_cnx_id]	=
                p_sisci_gate->cnx_array + p_trk->id;
        p_sisci_trk->next_cnx_id++;

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


        p_sisci_cnx	= p_sisci_gate->cnx_array + p_trk->id;
        err = nm_sisci_cnx_setup(p_sisci_cnx, sci_dev, r_node_id, r_seg_id);
        if (err != NM_ESUCCESS)
                goto out;


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

        p_sisci_drv->next_seg_id = p_sisci_drv->next_seg_id + 2;

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_sisci_disconnect		(struct nm_cnx_rq *p_crq) {
	int err;

        /* TODO: add disconnect code
         */
#warning TODO
	err = NM_ESUCCESS;

	return err;
}

static
int
nm_sisci_send_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_gate			*p_gate		= NULL;
        struct nm_drv			*p_drv		= NULL;
        struct nm_trk			*p_trk		= NULL;

        struct nm_sisci_gate		*p_sisci_gate	= NULL;
        struct nm_sisci_drv		*p_sisci_drv	= NULL;
        struct nm_sisci_trk		*p_sisci_trk	= NULL;

        struct nm_sisci_cnx		*p_sisci_cnx	= NULL;
        struct nm_sisci_pkt_wrap	*p_sisci_pw	= NULL;
        struct nm_sisci_segment		*p_sisci_seg	= NULL;

        struct nm_iovec_iter		*p_vi		= NULL;
        struct iovec			*p_cur		= NULL;

        uint8_t *dst;
        size_t   seg_avail;

        int i;
	int err;

        p_gate		= p_pw->p_gate;
        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_sisci_drv	= p_drv->priv;
        p_sisci_trk	= p_trk->priv;


        if (!p_pw->gate_priv) {
                /* first call */

                p_sisci_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
                p_pw->gate_priv	= p_sisci_gate;

                p_sisci_pw	= tbx_malloc(p_sisci_drv->sci_pw_mem);

                /* we always start sending new pkts on segment 0, at least for now
                 */
                p_sisci_pw->next_seg	= 0;

                /* current entry num is first entry num
                 */
                p_sisci_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_sisci_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_sisci_pw->vi.v_cur_size	= p_pw->v_nb;

                p_pw->drv_priv	= p_sisci_pw;

                p_sisci_cnx		= p_sisci_gate->cnx_array + p_trk->id;
                p_sisci_pw->p_sisci_cnx	= p_sisci_cnx;
        } else {
                p_sisci_gate	= p_pw->gate_priv;
                p_sisci_pw	= p_pw->drv_priv;
                p_sisci_cnx	= p_sisci_pw->p_sisci_cnx;
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_sisci_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_sisci_pw->vi.v_cur;

        NM_TRACE_VAL("sisci outgoing cur size",	p_vi->v_cur_size);
        NM_TRACE_PTR("sisci outgoing cur base",	p_cur->iov_base);
        NM_TRACE_VAL("sisci outgoing cur len",	p_cur->iov_len);

        i = p_sisci_pw->next_seg;

 next_seg:
        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out;
        }

        p_sisci_seg	= p_sisci_cnx->seg_array + i;

        /* wait for authorization to write
         */
        if (!p_sisci_seg->l_seg->w) {
                p_sisci_pw->next_seg	= i;
                err = -NM_EAGAIN;
                goto out;
        }

        /* -=- send data -=-
         */
        seg_avail	= DATA_AREA_SIZE;
        dst		= (uint8_t*)p_sisci_seg->r_seg->d;

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, seg_avail);

                /* copy iov_base...iov_base+len-1 */

                /* TODO: use SCIMemCpy instead of memcpy for sending
                 */
#warning TODO
                memcpy(dst, p_cur->iov_base, len);

                p_cur->iov_base	+= len;
                p_cur->iov_len	-= len;

                /* still something to send for this vector entry?
                 */
                if (p_cur->iov_len)
                        break;

                /* restore vector entry
                 */
                *p_cur = p_vi->cur_copy;

                /* increment cursors
                 */
                p_vi->v_cur++;
                p_vi->v_cur_size--;

                if (!p_vi->v_cur_size) {

                        /* flush data
                         */
                        SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);

                        /* reset write flag */
                        p_sisci_seg->l_seg->w	= 0;

                        /* set read flag */
                        p_sisci_seg->r_seg->r	= 1;

                        /* the reverse write flag is implicitely flushed here */
                        p_sisci_seg->write_flag_flushed	= 1;

                        /* flush flags
                         */
                        SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);

                        tbx_free(p_sisci_drv->sci_pw_mem, p_sisci_pw);
                        err	= NM_ESUCCESS;

                        goto out;
                }

                p_cur++;
                p_vi->cur_copy	 = *p_cur;

                seg_avail	-= len;
                dst		+= len;
        } while (seg_avail);

        /* flush data
         */
        SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);

        /* reset write flag */
        p_sisci_seg->l_seg->w	= 0;

        /* set read flag */
        p_sisci_seg->r_seg->r	= 1;

        /* the reverse write flag is implicitely flushed here */
        p_sisci_seg->write_flag_flushed	= 1;

        /* flush flags
         */
        SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);

        i = !i;
        goto next_seg;

 out:
	return err;
}

static
int
nm_sisci_recv_iov		(struct nm_pkt_wrap *p_pw) {
        struct nm_gate			*p_gate		= NULL;
        struct nm_drv			*p_drv		= NULL;
        struct nm_trk			*p_trk		= NULL;

        struct nm_sisci_gate		*p_sisci_gate	= NULL;
        struct nm_sisci_drv		*p_sisci_drv	= NULL;
        struct nm_sisci_trk		*p_sisci_trk	= NULL;

        struct nm_sisci_cnx		*p_sisci_cnx	= NULL;
        struct nm_sisci_pkt_wrap	*p_sisci_pw	= NULL;
        struct nm_sisci_segment		*p_sisci_seg	= NULL;

        struct nm_iovec_iter		*p_vi		= NULL;
        struct iovec			*p_cur		= NULL;

        uint8_t *src;
        size_t   seg_avail;

        int i;
	int err;

        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_sisci_drv	= p_drv->priv;
        p_sisci_trk	= p_trk->priv;

        if (!p_pw->p_gate) {
                /* permissive request, no gate selected yet
                   --> try to find an active one
                 */

                uint8_t j; /* loop counter	*/
                uint8_t k; /* cnx index		*/

                /* which cnx should we poll first?
                 */
                if (p_sisci_trk->last_active_id < p_sisci_trk->next_cnx_id) {
                        k = p_sisci_trk->last_active_id;
                } else {
                        k = 0;
                        p_sisci_trk->last_active_id	= 0;
                }

                /* loop on each cnx
                 */
                for (j = 0; j < p_sisci_trk->next_cnx_id; j++) {
                        p_sisci_cnx	= p_sisci_trk->cnx_map[k];
                        p_sisci_seg	= p_sisci_cnx->seg_array + 0;

                        /* if read flag is set, the gate is active*/
                        if (p_sisci_seg->l_seg->r)
                                goto active_gate_found;

                        /* if the write flag was not flushed, do it now */
                        if (!p_sisci_seg->write_flag_flushed) {
                                SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);
                                p_sisci_seg->write_flag_flushed	= 1;
                        }

                        /* if read flag is set, the gate is active*/
                        if (p_sisci_seg->l_seg->r)
                                goto active_gate_found;

                        k = (k+1) % p_sisci_trk->next_cnx_id;
                }

                err = -NM_EAGAIN;
                goto out;

        active_gate_found:
                p_pw->p_gate	= p_sisci_trk->gate_map[k];

                /* next initialization steps are done in common with the
                   selective request initialization code below */
        }

        if (!p_pw->gate_priv) {
                /* first call */

                p_gate		= p_pw->p_gate;
                p_sisci_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
                p_pw->gate_priv	= p_sisci_gate;

                p_sisci_pw	= tbx_malloc(p_sisci_drv->sci_pw_mem);

                /* we always start sending new pkts on segment 0, at least for now
                 */
                p_sisci_pw->next_seg	= 0;

                /* current entry num is first entry num
                 */
                p_sisci_pw->vi.v_cur	= p_pw->v_first;

                /* save a copy of current entry
                 */
                p_sisci_pw->vi.cur_copy	= p_pw->v[p_pw->v_first];

                /* current vector size is full vector size
                 */
                p_sisci_pw->vi.v_cur_size	= p_pw->v_nb;

                p_pw->drv_priv	= p_sisci_pw;

                p_sisci_cnx		= p_sisci_gate->cnx_array + p_trk->id;
                p_sisci_pw->p_sisci_cnx	= p_sisci_cnx;
        } else {
                p_gate		= p_pw->p_gate;
                p_sisci_gate	= p_pw->gate_priv;
                p_sisci_pw	= p_pw->drv_priv;
                p_sisci_cnx	= p_sisci_pw->p_sisci_cnx;
        }

        /* get a pointer to the iterator
         */
        p_vi	= &(p_sisci_pw->vi);

        /* get a pointer to the current entry
         */
        p_cur	= p_pw->v + p_sisci_pw->vi.v_cur;

        NM_TRACE_VAL("sisci outgoing cur size",	p_vi->v_cur_size);
        NM_TRACE_PTR("sisci outgoing cur base",	p_cur->iov_base);
        NM_TRACE_VAL("sisci outgoing cur len",	p_cur->iov_len);

        i = p_sisci_pw->next_seg;

 next_seg:
        if (!p_vi->v_cur_size) {
                err	= NM_ESUCCESS;

                goto out;
        }

        p_sisci_seg	= p_sisci_cnx->seg_array + i;

        /* wait for authorization to read
           note: for permissive requests, the first test of the read flag
           is uselessly done twice */
        if (!p_sisci_seg->l_seg->r) {
                if (!p_sisci_seg->write_flag_flushed) {
                        SCIStoreBarrier(p_sisci_seg->sci_r_seq, NO_FLAGS);
                        p_sisci_seg->write_flag_flushed	= 1;
                }

                if (!p_sisci_seg->l_seg->r) {
                        p_sisci_pw->next_seg	= i;
                        err = -NM_EAGAIN;
                        goto out;
                }
        }

        /* -=- read data -=-
         */
        seg_avail	= DATA_AREA_SIZE;
        src		= (uint8_t*)p_sisci_seg->r_seg->d;

        do {
                uint64_t len	= tbx_min(p_cur->iov_len, seg_avail);

                /* copy iov_base...iov_base+len-1 */

                memcpy(p_cur->iov_base, src, len);

                p_cur->iov_base	+= len;
                p_cur->iov_len	-= len;

                /* still something to send for this vector entry?
                 */
                if (p_cur->iov_len)
                        break;

                /* restore vector entry
                 */
                *p_cur = p_vi->cur_copy;

                /* increment cursors
                 */
                p_vi->v_cur++;
                p_vi->v_cur_size--;

                if (!p_vi->v_cur_size) {

                        /* reset read flag */
                        p_sisci_seg->l_seg->r	= 0;

                        /* set write flag */
                        p_sisci_seg->r_seg->w	= 1;

                        /* do not flush flags
                         */
                        p_sisci_seg->write_flag_flushed	= 0;

                        tbx_free(p_sisci_drv->sci_pw_mem, p_sisci_pw);
                        err	= NM_ESUCCESS;

                        goto out;
                }

                p_cur++;
                p_vi->cur_copy	 = *p_cur;

                seg_avail	-= len;
                src		+= len;
        } while (seg_avail);

        /* reset read flag */
        p_sisci_seg->l_seg->r	= 0;

        /* set write flag */
        p_sisci_seg->r_seg->w	= 1;

        /* do not flush flags
         */
        p_sisci_seg->write_flag_flushed	= 0;

        i = !i;
        goto next_seg;

 out:
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

        p_ops->post_send_iov	= nm_sisci_send_iov     ;
        p_ops->post_recv_iov    = nm_sisci_recv_iov	;

        p_ops->poll_send_iov    = nm_sisci_send_iov     ;
        p_ops->poll_recv_iov    = nm_sisci_recv_iov	;

        return NM_ESUCCESS;
}

