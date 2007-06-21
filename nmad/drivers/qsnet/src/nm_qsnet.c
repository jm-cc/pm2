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
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>
#include <assert.h>

#include <elan/elan.h>
#include <elan/capability.h>
#include <qsnet/fence.h>

#include <tbx.h>

#include "nm_qsnet_private.h"

#ifdef ENABLE_SAMPLING
#include "nm_parser.h"
#endif

#define INITIAL_PW_NUM		16
#define NM_QSNET_USE_ELAN_DONE

#ifdef  NM_QSNET_USE_ELAN_DONE
#warning using elan_done event notification
#else
#warning using elan_TxWait/elan_RxWait event notification
#endif

struct nm_qsnet_drv {

        /* memory allocator for private pw data
         */
        p_tbx_memory_t qsnet_pw_mem;

        /* main elan entry point
         */
        ELAN_BASE	*base;

        /* process rank from elan point of view
         */
        u_int		 proc;

        /* number of processes in the session
         */
        u_int		 nproc;
};

struct nm_qsnet_trk {

        /* elan tagged port
         */
        ELAN_TPORT		*p;

        /* elan queue used by the tagged port
         */
        ELAN_QUEUE		*q;

        /* process rank from elan point of view
         */
        u_int		 	 proc;

        /* number of processes in the session
         */
        u_int		 	 nproc;

        /* match info to gate id reverse mapping			*/
        uint8_t			*gate_map;

        /* gate_map size
         */
        uint8_t			 gate_map_size;
};

struct nm_qsnet_cnx {

        /* remote rank from elan point of view
         */
        u_int		 remote_proc;
};

struct nm_qsnet_gate {
        struct nm_qsnet_cnx	cnx_array[255];
};

struct nm_qsnet_pkt_wrap {
        ELAN_EVENT	*ev;
};

struct nm_qsnet_adm_pkt {
        char		drv_url[16];
};

#ifdef PROFILE_QSNET
#warning profiling code activated
static tbx_tick_t      t1;
static tbx_tick_t      t2;
static double sample_array[20000];
static int next_sample = 0;
static char   data_array_1[20000];
static int next_a1 = 0;
static char   data_array_2[20000];
static int next_a2 = 0;

#define SAMPLE_BEGIN	TBX_GET_TICK(t1)
#define SAMPLE_END	TBX_GET_TICK(t2);sample_array[next_sample++] = TBX_TIMING_DELAY(t1, t2)
#else /* PROFILE_QSNET */
#define SAMPLE_BEGIN
#define SAMPLE_END
#endif /* PROFILE_QSNET */

/* prototypes
 */
static
int
nm_qsnet_poll_send_iov    	(struct nm_pkt_wrap *p_pw);

static
int
nm_qsnet_poll_recv_iov    	(struct nm_pkt_wrap *p_pw);

/* functions
 */
static
int
nm_qsnet_query		(struct nm_drv *p_drv,
			 struct nm_driver_query_param *params,
			 int nparam) {
	struct nm_qsnet_drv	*p_qsnet_drv	= NULL;
	int err;

	/* private data                                                 */
	p_qsnet_drv	= TBX_MALLOC(sizeof (struct nm_qsnet_drv));
	if (!p_qsnet_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

	memset(p_qsnet_drv, 0, sizeof (struct nm_qsnet_drv));
	p_drv->priv	= p_qsnet_drv;

	/* driver capabilities encoding					*/
	p_drv->cap.has_trk_rq_dgram			= 1;
	p_drv->cap.has_selective_receive		= 1;
	p_drv->cap.has_concurrent_selective_receive	= 0;

	err = NM_ESUCCESS;

 out:
	return err;
}

/* functions
 */
static
int
nm_qsnet_init		(struct nm_drv *p_drv) {
	struct nm_qsnet_drv	*p_qsnet_drv	= p_drv->priv;
        ELAN_BASE		*base		= NULL;
        char			*node_string	= NULL;
        p_tbx_string_t		 url_string	= NULL;
        int err;

        NM_LOG_IN();
        err = -NM_EINVAL;

        node_string = getenv("LIBELAN_ECAP0");
        if (node_string)
                goto capability_generated;

        node_string = getenv("ELAN_NODE_STRING");
        if (!node_string)
                goto out;

        if (elan_generateCapability (node_string) < 0)
                goto out;

 capability_generated:
        err = -NM_ESCFAILD;
        if (!(base = elan_baseInit(0))) {
                perror("elan_baseInit");
                goto out;
        }

        p_qsnet_drv->base	= base;
        p_qsnet_drv->proc	= base->state->vp;
        p_qsnet_drv->nproc	= base->state->nvp;

        NM_TRACE_VAL("proc", p_qsnet_drv->proc);
        NM_TRACE_VAL("nproc", p_qsnet_drv->nproc);

        tbx_malloc_init(&(p_qsnet_drv->qsnet_pw_mem),   sizeof(struct nm_qsnet_pkt_wrap),
                        INITIAL_PW_NUM,   "nmad/qsnet/pw");

        url_string	= tbx_string_init_to_int(p_qsnet_drv->proc);
        p_drv->url	= tbx_string_to_cstring(url_string);
        NM_TRACE_STR("p_drv->url", p_drv->url);
        tbx_string_free(url_string);

#ifdef ENABLE_SAMPLING
        nm_parse_sampling(p_drv, "qsnet");
#endif

        err = NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

static
int
nm_qsnet_exit		(struct nm_drv *p_drv) {
        int err;

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_qsnet_open_track	(struct nm_trk_rq	*p_trk_rq) {
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_qsnet_drv	*p_qsnet_drv	= NULL;
        struct nm_qsnet_trk	*p_qsnet_trk	= NULL;
        ELAN_BASE		*base		= NULL;
        ELAN_TPORT		*p		= NULL;
        ELAN_QUEUE		*q		= NULL;
        int err;

        NM_LOG_IN();
        p_trk		= p_trk_rq->p_trk;
        p_drv		= p_trk->p_drv;
        p_qsnet_drv	= p_drv->priv;
        base		= p_qsnet_drv->base;

        /* private data							*/
	p_qsnet_trk	= TBX_MALLOC(sizeof (struct nm_qsnet_trk));
        memset(p_qsnet_trk, 0, sizeof (struct nm_qsnet_trk));
        p_trk->priv	= p_qsnet_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_dgram;
        p_trk->cap.iov_type	= nm_trk_iov_none;
        p_trk->cap.max_pending_send_request	= 2;
        p_trk->cap.max_pending_recv_request	= 2;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;

        err	= -NM_ESCFAILD;
        if ((q = elan_gallocQueue(base, base->allGroup)) == NULL) {
                perror("elan_gallocQueue");
                goto out;
        }

        if (!(p = elan_tportInit(base->state,
                                 q,
                                 base->tport_nslots,
                                 base->tport_smallmsg, base->tport_bigmsg,
                                 base->tport_stripemsg,
                                 base->waitType, base->retryCount,
                                 &base->shm_key, base->shm_fifodepth,
                                 base->shm_fragsize, 0))) {
                perror("elan_tportInit");
                goto out;
        }


        p_qsnet_trk->p			= p;
        p_qsnet_trk->q			= q;
        p_qsnet_trk->gate_map_size	= 0;
        p_qsnet_trk->proc		= p_qsnet_drv->proc;
        p_qsnet_trk->nproc		= p_qsnet_drv->nproc;

        p_trk->url	= tbx_strdup("-");

        err = NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

static
int
nm_qsnet_close_track	(struct nm_trk *p_trk) {
        int err;

        TBX_FREE(p_trk->priv);

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_qsnet_connect		(struct nm_cnx_rq *p_crq) {
        struct nm_qsnet_adm_pkt	 pkt;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_qsnet_gate	*p_qsnet_gate	= NULL;
        struct nm_qsnet_trk	*p_qsnet_trk	= NULL;
        struct nm_qsnet_cnx	*p_qsnet_cnx	= NULL;
        ELAN_TPORT		*p		= NULL;
        u_int			 local_proc	= 0;
        u_int			 remote_proc	= 0;
        char			*drv_url	= NULL;
        int err;

        NM_LOG_IN();
        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        p_qsnet_trk	= p_trk->priv;
        p		= p_qsnet_trk->p;
        local_proc	= p_qsnet_trk->proc;

        /* allocate priv gate structure
         */
        p_qsnet_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_qsnet_gate) {
                p_qsnet_gate	= TBX_MALLOC(sizeof (struct nm_qsnet_gate));
                memset(p_qsnet_gate, 0, sizeof(struct nm_qsnet_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_qsnet_gate;
        }

        p_qsnet_cnx	= p_qsnet_gate->cnx_array + p_trk->id;
        drv_url		= p_crq->remote_drv_url;

        /* process remote process URL
         */
        NM_TRACEF("connect - drv_url: %s", drv_url);

        {
                char *ptr = NULL;

                remote_proc = strtol(drv_url, &ptr, 0);
                p_qsnet_cnx->remote_proc = remote_proc;
        }

        /* manage gate reverse map storage
         */
        if (remote_proc >= p_qsnet_trk->gate_map_size) {
                if (p_qsnet_trk->gate_map_size) {
                        p_qsnet_trk->gate_map	=
                                TBX_REALLOC(p_qsnet_trk->gate_map,
                                            remote_proc+1);
                } else {
                        p_qsnet_trk->gate_map	=
                                TBX_MALLOC(remote_proc+1);
                }
        }

        p_qsnet_trk->gate_map[remote_proc]	= p_gate->id;
        NM_TRACEF("new gate mapping: gate %d = remote_proc %d",
                  p_gate->id, remote_proc);


        /* send our own URL to remote process
         */
        NM_TRACEF("send pkt -->");
        {
                ELAN_EVENT	*ev	= NULL;

                strcpy(pkt.drv_url, p_drv->url);
                NM_TRACEF("connect - pkt.drv_url: %s",		pkt.drv_url);

                /* we use tag 1 for administrative pkts and tag 2 for anything else
                 */
                NM_TRACEF("remote = %d, local = %d", remote_proc, local_proc);
                ev = elan_tportTxStart(p, 0, remote_proc, local_proc,
                                       1, &pkt, sizeof(pkt));
                elan_tportTxWait(ev);
        }
        NM_TRACEF("send pkt <--");

        err = NM_ESUCCESS;
        NM_LOG_OUT();

        return err;
}

static
int
nm_qsnet_accept		(struct nm_cnx_rq *p_crq) {
        struct nm_qsnet_adm_pkt	 pkt;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_qsnet_gate	*p_qsnet_gate	= NULL;
        struct nm_qsnet_trk	*p_qsnet_trk	= NULL;
        struct nm_qsnet_cnx	*p_qsnet_cnx	= NULL;
        ELAN_TPORT		*p		= NULL;
        int			 remote_proc	= 0;
        int err;

        NM_LOG_IN();
        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        p_qsnet_trk	= p_trk->priv;
        p		= p_qsnet_trk->p;

        /* allocate priv gate structure
         */
        p_qsnet_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_qsnet_gate) {
                p_qsnet_gate	= TBX_MALLOC(sizeof (struct nm_qsnet_gate));
                memset(p_qsnet_gate, 0, sizeof(struct nm_qsnet_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_qsnet_gate;
        }
        p_qsnet_cnx	= p_qsnet_gate->cnx_array + p_trk->id;


        NM_TRACEF("recv pkt -->");
        {
                ELAN_EVENT	*ev	= NULL;
                int tag;
                size_t size;

                ev	= elan_tportRxStart(p, 0, 0, 0, -1, 1, &pkt, sizeof(pkt));
                elan_tportRxWait(ev, &remote_proc, &tag, &size);
                NM_TRACEF("remote_proc = %d, tag = %d, size = %d",
                          remote_proc, tag, size);

        }
        NM_TRACEF("recv pkt <--");

        NM_TRACEF("accept - pkt.drv_url: %s",	pkt.drv_url);
        p_qsnet_cnx->remote_proc = remote_proc;

        /* manage gate reverse map storage
         */
        if (remote_proc >= p_qsnet_trk->gate_map_size) {
                if (p_qsnet_trk->gate_map_size) {
                        p_qsnet_trk->gate_map	=
                                TBX_REALLOC(p_qsnet_trk->gate_map,
                                            remote_proc+1);
                } else {
                        p_qsnet_trk->gate_map	=
                                TBX_MALLOC(remote_proc+1);
                }
        }

        p_qsnet_trk->gate_map[remote_proc]	= p_gate->id;
        NM_TRACEF("new gate mapping: gate %d = remote_proc %d",
                  p_gate->id, remote_proc);

        err = NM_ESUCCESS;
        NM_LOG_OUT();

        return err;
}

static
int
nm_qsnet_disconnect	(struct nm_cnx_rq *p_crq) {
        int err;


        /* nothing
         */
        err = NM_ESUCCESS;

        return err;
}

static
int
nm_qsnet_post_send_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_qsnet_drv	*p_qsnet_drv	= NULL;
        struct nm_qsnet_trk	*p_qsnet_trk	= NULL;
        struct nm_qsnet_gate	*p_qsnet_gate	= NULL;
        struct nm_qsnet_cnx	*p_qsnet_cnx	= NULL;
        struct nm_qsnet_pkt_wrap	*p_qsnet_pw	= NULL;
        struct iovec		*p_iov		= NULL;
        int err;

        err = -NM_EINVAL;
        assert(p_pw->v_nb == 1);
        if (p_pw->v_nb != 1)
                goto out;

        p_gate		= p_pw->p_gate;
        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_qsnet_drv	= p_drv->priv;
        p_qsnet_trk	= p_trk->priv;
        p_pw->gate_priv	= p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;

        p_qsnet_gate	= p_pw->gate_priv;
        p_qsnet_cnx	= p_qsnet_gate->cnx_array + p_trk->id;

        p_qsnet_pw	= tbx_malloc(p_qsnet_drv->qsnet_pw_mem);
        p_pw->drv_priv	= p_qsnet_pw;

        p_iov		= p_pw->v + p_pw->v_first;
        p_qsnet_pw->ev	=
                elan_tportTxStart(p_qsnet_trk->p, 0, p_qsnet_cnx->remote_proc,
                                  p_qsnet_drv->proc,
                                  2, p_iov->iov_base, p_iov->iov_len);
        SAMPLE_BEGIN;

#if 1
        err = nm_qsnet_poll_send_iov(p_pw);
#else
        err = -NM_EAGAIN;
#endif

 out:
        return err;
}

static
int
nm_qsnet_post_recv_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_drv			*p_drv		= NULL;
        struct nm_trk			*p_trk		= NULL;
        struct nm_qsnet_drv		*p_qsnet_drv	= NULL;
        struct nm_qsnet_trk		*p_qsnet_trk	= NULL;
        struct nm_qsnet_pkt_wrap	*p_qsnet_pw	= NULL;
        struct iovec			*p_iov		= NULL;
        u_int				 remote_proc	= 0;
        u_int				 remote_proc_mask	= 0;
        int err;

        err = -NM_EINVAL;
        assert(p_pw->v_nb == 1);
        if (p_pw->v_nb != 1)
                goto out;

        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_qsnet_drv	= p_drv->priv;
        p_qsnet_trk	= p_trk->priv;

        if (p_pw->p_gate) {
                struct nm_gate		*p_gate		= NULL;
                struct nm_qsnet_gate	*p_qsnet_gate	= NULL;
                struct nm_qsnet_cnx	*p_qsnet_cnx	= NULL;

                p_gate		= p_pw->p_gate;
                p_pw->gate_priv	= p_gate->p_gate_drv_array[p_drv->id]->info;
                p_qsnet_gate	= p_pw->gate_priv;
                p_qsnet_cnx	= p_qsnet_gate->cnx_array + p_trk->id;
                remote_proc	= p_qsnet_cnx->remote_proc;
                remote_proc_mask	= (u_int)-1;
        }

        p_qsnet_pw	= tbx_malloc(p_qsnet_drv->qsnet_pw_mem);
        p_pw->drv_priv	= p_qsnet_pw;

        p_iov	= p_pw->v + p_pw->v_first;
        p_qsnet_pw->ev	=
                elan_tportRxStart(p_qsnet_trk->p, 0, remote_proc_mask, remote_proc,
                                  -1, 2, p_iov->iov_base, p_iov->iov_len);

#if 1
        err = nm_qsnet_poll_recv_iov(p_pw);
#else
        err = -NM_EAGAIN;
#endif

#ifdef PROFILE_QSNET
#warning profiling code activated
        data_array_1[next_a1++] = (char)(err == NM_ESUCCESS);
        data_array_2[next_a2++] = (char)p_trk->id;
#endif /* PROFILE_QSNET */

 out:
        return err;
}

static
int
nm_qsnet_poll_send_iov    	(struct nm_pkt_wrap *p_pw) {
        struct nm_qsnet_drv		*p_qsnet_drv	= NULL;
        struct nm_qsnet_pkt_wrap	*p_qsnet_pw	= NULL;
        int boolean;
        int err;

        p_qsnet_pw	= p_pw->drv_priv;
#ifdef NM_QSNET_USE_ELAN_DONE
        boolean		= elan_done(p_qsnet_pw->ev, 0);
#else
        boolean		= elan_tportTxDone(p_qsnet_pw->ev);
#endif

        if (!boolean) {
                err	= -NM_EAGAIN;
                goto out;
        }

        elan_tportTxWait(p_qsnet_pw->ev);

        p_qsnet_drv	= p_pw->p_drv->priv;
        tbx_free(p_qsnet_drv->qsnet_pw_mem, p_qsnet_pw);
        err = NM_ESUCCESS;

out:
        return err;
}

static
int
nm_qsnet_poll_recv_iov    	(struct nm_pkt_wrap *p_pw) {
        struct nm_qsnet_drv		*p_qsnet_drv	= NULL;
        struct nm_qsnet_pkt_wrap	*p_qsnet_pw	= NULL;
        int				 remote_proc	= 0;
        int boolean;
        int err;

        p_qsnet_pw	= p_pw->drv_priv;
#ifdef NM_QSNET_USE_ELAN_DONE
        boolean		= elan_done(p_qsnet_pw->ev, 0);
#else
        boolean		= elan_tportRxDone(p_qsnet_pw->ev);
#endif
        if (!boolean) {
                err	= -NM_EAGAIN;
                goto out;
        }

        SAMPLE_END;
        elan_tportRxWait(p_qsnet_pw->ev, &remote_proc, NULL, NULL);

        p_qsnet_drv	= p_pw->p_drv->priv;
        if (!p_pw->p_gate) {
                struct nm_qsnet_trk	*p_qsnet_trk	= NULL;

                /* gate-less request */
                p_qsnet_trk	= p_pw->p_trk->priv;
                p_pw->p_gate	= p_pw->p_drv->p_core->gate_array
                        + p_qsnet_trk->gate_map[remote_proc];

                NM_TRACE_VAL("new pkt from qsnet proc", remote_proc);
                NM_TRACE_PTR("gate ptr", p_pw->p_gate);
        }

        tbx_free(p_qsnet_drv->qsnet_pw_mem, p_qsnet_pw);
        err = NM_ESUCCESS;

out:
        return err;
}

int
nm_qsnet_load(struct nm_drv_ops *p_ops) {
        p_ops->query		= nm_qsnet_query         ;
        p_ops->init		= nm_qsnet_init         ;
        p_ops->exit             = nm_qsnet_exit         ;

        p_ops->open_trk		= nm_qsnet_open_track   ;
        p_ops->close_trk	= nm_qsnet_close_track  ;

        p_ops->connect		= nm_qsnet_connect      ;
        p_ops->accept		= nm_qsnet_accept       ;
        p_ops->disconnect       = nm_qsnet_disconnect   ;

        p_ops->post_send_iov	= nm_qsnet_post_send_iov;
        p_ops->post_recv_iov    = nm_qsnet_post_recv_iov;

        p_ops->poll_send_iov    = nm_qsnet_poll_send_iov;
        p_ops->poll_recv_iov    = nm_qsnet_poll_recv_iov;

        return NM_ESUCCESS;
}

#ifdef PROFILE_QSNET
void
disp_stats() {
        int i;
        double _sum = 0;
        double _min = 0;
        double _max = 0;

        if (!next_sample)
                return;

        for (i = 0; i < next_sample; i++) {
                double s = sample_array[i];

                if (i) {
                        if (s > _max)
                                _max = s;
                        if (s < _min)
                                _min = s;
                }else {
                        _max = _min = s;
                }

                _sum += s;
#if 0
                NM_DISPF("%lf\t\t d1 = %d\t d2 = %d", s, (int)data_array_1[i], (int)data_array_2[i]);
#endif
        }

        NM_DISPF("nb_samples = %d", next_sample);
        NM_DISPF("min = %lf", _min);
        NM_DISPF("max = %lf", _max);
        NM_DISPF("avg = %lf", _sum / next_sample);
}
#endif /* PROFILE_QSNET */
