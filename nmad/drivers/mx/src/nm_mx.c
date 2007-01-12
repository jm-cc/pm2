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
#include <myriexpress.h>

#include <tbx.h>

#include "nm_mx_private.h"

#define INITIAL_PW_NUM		16

struct nm_mx_drv {
        p_tbx_memory_t mx_pw_mem;
};

struct nm_mx_trk {

        /* MX endpoing
         */
        mx_endpoint_t		ep;

        /* endpoint id
         */
        uint32_t		ep_id;

        /* next value to use for match info
         */
        uint64_t 		next_match_info;

        /* match info to gate id reverse mapping			*/
        uint8_t		*gate_map;
};

struct nm_mx_cnx {

        /* remote endpoint addr
         */
        mx_endpoint_addr_t	r_ep_addr;

        /* remote endpoint id
         */
        uint32_t		r_ep_id;

        /* MX match info (when sending)
         */
        uint64_t 		send_match_info;

        /* MX match info (when receiving)
         */
        uint64_t 		recv_match_info;
};

struct nm_mx_gate {
        struct nm_mx_cnx	cnx_array[255];
  	int			ref_cnt;
};

struct nm_mx_pkt_wrap {
        mx_endpoint_t		*p_ep;
        mx_request_t		 rq;
};

struct nm_mx_adm_pkt_1 {
        uint64_t 	match_info;
        char		drv_url[MX_MAX_HOSTNAME_LEN];
        char		trk_url[16];
};

struct nm_mx_adm_pkt_2 {
        uint64_t 	match_info;
};

#ifdef XPAULETTE

static
int
nm_mx_do_poll(xpaul_server_t	        server,
               xpaul_op_t		_op,
               xpaul_req_t		req,
               int			nb_ev,
	      int			option) ;

	struct xpaul_server nm_mx_ev_server = XPAUL_SERVER_INIT(nm_mx_ev_server, "NMad/MX I/O");
	tbx_bool_t nm_mx_ev_server_started = tbx_false;
#endif /* XPAULETTE */
/* prototypes
 */
static
int
nm_mx_poll_iov    	(struct nm_pkt_wrap *p_pw);

/* functions
 */
static
void
nm_mx_print_status(mx_status_t s) {
        const char *msg = NULL;

        msg = mx_strstatus(s.code);
        DISP("mx status: %s", msg);
}

static
void
nm_mx_check_return(char *msg, mx_return_t return_code) {
        if (return_code != MX_SUCCESS) {
                const char *msg_mx = NULL;

                msg_mx = mx_strerror(return_code);

                DISP("%s failed with code %s = %d/0x%x", msg, msg_mx, return_code, return_code);
        }
}

static
int
nm_mx_init		(struct nm_drv *p_drv) {
        struct nm_mx_drv	*p_mx_drv	= NULL;
        mx_return_t	mx_ret	= MX_SUCCESS;
        uint64_t	nic_id;
        char		hostname[MX_MAX_HOSTNAME_LEN];
        int err;

	p_mx_drv	= TBX_MALLOC(sizeof (struct nm_mx_drv));
        memset(p_mx_drv, 0, sizeof (struct nm_mx_drv));
        p_drv->priv	= p_mx_drv;

        tbx_malloc_init(&(p_mx_drv->mx_pw_mem),   sizeof(struct nm_mx_pkt_wrap),
                        INITIAL_PW_NUM,   "nmad/mx/pw");

        mx_set_error_handler(MX_ERRORS_RETURN);
        mx_ret	= mx_init();
        nm_mx_check_return("mx_init", mx_ret);

        mx_board_number_to_nic_id(0, &nic_id);
        mx_nic_id_to_hostname(nic_id, hostname);

        hostname[MX_MAX_HOSTNAME_LEN-1] = '\0';
        p_drv->url	= tbx_strdup(hostname);
        NM_TRACE_STR("p_drv->url", p_drv->url);

        /* driver capabilities encoding					*/
        p_drv->cap.has_trk_rq_dgram			= 1;
        p_drv->cap.has_selective_receive		= 1;
        p_drv->cap.has_concurrent_selective_receive	= 0;

        err = NM_ESUCCESS;
#ifdef XPAULETTE

	nm_mx_ev_server.stopable=1;

	xpaul_server_set_poll_settings(&nm_mx_ev_server,
				       XPAUL_POLL_AT_TIMER_SIG
				       | XPAUL_POLL_AT_IDLE | XPAUL_POLL_AT_YIELD, 1, -1);

	xpaul_server_add_callback(&nm_mx_ev_server,
				  XPAUL_FUNCTYPE_POLL_POLLONE,
				  (xpaul_pcallback_t) {
				  .func = &nm_mx_do_poll,.speed =
				  XPAUL_CALLBACK_FAST});

	if (!nm_mx_ev_server_started) {
                xpaul_server_start(&nm_mx_ev_server);
                nm_mx_ev_server_started = tbx_true;
        }
#endif /* XPAULETTE */
        return err;
}

static
int
nm_mx_exit		(struct nm_drv *p_drv) {
        struct nm_mx_drv	*p_mx_drv	= NULL;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        mx_ret	= mx_finalize();
        nm_mx_check_return("mx_finalize", mx_ret);

        p_mx_drv = p_drv->priv;
        TBX_FREE(p_mx_drv);
        p_drv->priv = NULL;

        TBX_FREE(p_drv->url);
        p_drv->url = NULL;

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_mx_open_track	(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        uint32_t                 ep_id     	= 1;
        const uint32_t		 ep_key		= 0xFFFFFFFF;
        mx_endpoint_t		 ep;
        mx_return_t		 mx_ret		= MX_SUCCESS;
        p_tbx_string_t		 url_string	= NULL;
        int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_mx_trk	= TBX_MALLOC(sizeof (struct nm_mx_trk));
        memset(p_mx_trk, 0, sizeof (struct nm_mx_trk));
	p_mx_trk->next_match_info	= 1;
        p_trk->priv	= p_mx_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_dgram;
        p_trk->cap.iov_type	= nm_trk_iov_both_sym;
        p_trk->cap.max_pending_send_request	= 1;
        p_trk->cap.max_pending_recv_request	= 1;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;
        p_trk->cap.max_iovec_request_length	= 32768;
#ifdef IOV_MAX
        p_trk->cap.max_iovec_size		= IOV_MAX;
#else /* IOV_MAX */
        p_trk->cap.max_iovec_size		= sysconf(_SC_IOV_MAX);
#endif /* IOV_MAX */


        /* mx endpoint
         */
        do {
                mx_ret	= mx_open_endpoint(0,
                                           ep_id,
                                           ep_key,
                                           NULL,
                                           0,
                                           &ep);
        } while (mx_ret == MX_BUSY && ep_id++);
        nm_mx_check_return("mx_open_endpoint", mx_ret);

        p_mx_trk->ep	= ep;
        p_mx_trk->ep_id	= ep_id;

        url_string	= tbx_string_init_to_int(ep_id);
        p_trk->url	= tbx_string_to_cstring(url_string);
        NM_TRACE_STR("p_trk->url", p_trk->url);
        tbx_string_free(url_string);

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_mx_close_track	(struct nm_trk *p_trk) {
        mx_return_t		 mx_ret		= MX_SUCCESS;
        struct nm_mx_trk	*p_mx_trk	= NULL;

        /* mx endpoint
         */
        p_mx_trk = p_trk->priv;
        mx_ret	= mx_close_endpoint(p_mx_trk->ep);
        nm_mx_check_return("mx_close_endpoint", mx_ret);
        TBX_FREE(p_trk->priv);

        TBX_FREE(p_trk->url);

        return NM_ESUCCESS;
}

static
int
nm_mx_connect		(struct nm_cnx_rq *p_crq) {
        struct nm_mx_adm_pkt_1   pkt1;
        struct nm_mx_adm_pkt_2   pkt2;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_cnx	*p_mx_cnx	= NULL;
        char			*drv_url	= NULL;
        char			*trk_url	= NULL;
        uint64_t		 r_nic_id	= 0;
        const uint32_t		 ep_key		= 0xFFFFFFFF;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        p_mx_trk	= p_trk->priv;
        p_mx_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_mx_gate) {
                p_mx_gate	= TBX_MALLOC(sizeof (struct nm_mx_gate));
                memset(p_mx_gate, 0, sizeof(struct nm_mx_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_mx_gate;
        } else {
          p_mx_gate->ref_cnt++;
        }

        p_mx_cnx	= p_mx_gate->cnx_array + p_trk->id;

        drv_url		= p_crq->remote_drv_url;
        trk_url		= p_crq->remote_trk_url;

        if (p_mx_trk->next_match_info-1) {
                p_mx_trk->gate_map	=
                        TBX_REALLOC(p_mx_trk->gate_map,
                                    p_mx_trk->next_match_info+1);
        } else {
                p_mx_trk->gate_map	= TBX_MALLOC(2);
        }

        p_mx_trk->gate_map[p_mx_trk->next_match_info]		= p_gate->id;

        /* we use the remote_drv_url instead of the remote_host_url
           since we want the hostname "as MX sees it"
         */
        NM_TRACEF("connect - drv_url: %s", drv_url);
        NM_TRACEF("connect - trk_url: %s", trk_url);
        mx_ret	= mx_hostname_to_nic_id(drv_url, &r_nic_id);
        nm_mx_check_return("(connect) mx_hostname_to_nic_id", mx_ret);

        {
                char *ptr = NULL;

                p_mx_cnx->r_ep_id = strtol(trk_url, &ptr, 0);
        }

        NM_TRACEF("mx_connect -->");
        mx_ret	= mx_connect(p_mx_trk->ep,
                             r_nic_id,
                             p_mx_cnx->r_ep_id,
                             ep_key,
                             MX_INFINITE,
                             &p_mx_cnx->r_ep_addr);
        NM_TRACEF("mx_connect <--");
        nm_mx_check_return("mx_connect", mx_ret);

        NM_TRACEF("send pkt1 -->");
        {
                mx_request_t	rq;
                mx_status_t 	s;
                mx_segment_t	sg;
                uint32_t	r;

                strcpy(pkt1.drv_url, p_drv->url);
                strcpy(pkt1.trk_url, p_trk->url);
		p_mx_cnx->recv_match_info	= p_mx_trk->next_match_info;
                pkt1.match_info	= p_mx_trk->next_match_info++;
                NM_TRACEF("connect - pkt1.drv_url: %s",	pkt1.drv_url);
                NM_TRACEF("connect - pkt1.trk_url: %s",	pkt1.trk_url);
                NM_TRACEF("connect - pkt1.match_info (sender should contact us with this MI): %lu",	pkt1.match_info);

                sg.segment_ptr		= &pkt1;
                sg.segment_length	= sizeof(pkt1);
                mx_ret = mx_isend(p_mx_trk->ep, &sg, 1, p_mx_cnx->r_ep_addr,
                                   UINT64_C(0xdeadbeefdeadbeef), 0, &rq);
                nm_mx_check_return("mx_isend", mx_ret);
                mx_ret = mx_wait(p_mx_trk->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("send pkt1 <--");

        NM_TRACEF("recv pkt2 -->");
        {
                mx_request_t	rq;
                mx_status_t 	s;
                mx_segment_t	sg;
                uint32_t	r;

                sg.segment_ptr		= &pkt2;
                sg.segment_length	= sizeof(pkt2);
                mx_ret = mx_irecv(p_mx_trk->ep, &sg, 1,
                                  UINT64_C(0xbeefdeadbeefdead),
                                  UINT64_C(0xffffffffffffffff), 0, &rq);
                nm_mx_check_return("mx_irecv", mx_ret);
                mx_ret = mx_wait(p_mx_trk->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("recv pkt2 <--");

        p_mx_cnx->send_match_info	= pkt2.match_info;
	NM_TRACEF("connect - pkt2.match_info (we will contact our peer with this MI): %lu",	pkt2.match_info);

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_mx_accept		(struct nm_cnx_rq *p_crq) {
        struct nm_mx_adm_pkt_1   pkt1;
        struct nm_mx_adm_pkt_2   pkt2;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_cnx	*p_mx_cnx	= NULL;
        uint64_t		 r_nic_id	= 0;
        const uint32_t		 ep_key		= 0xFFFFFFFF;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        p_mx_trk	= p_trk->priv;
        p_mx_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_mx_gate) {
                p_mx_gate	= TBX_MALLOC(sizeof (struct nm_mx_gate));
                memset(p_mx_gate, 0, sizeof(struct nm_mx_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_mx_gate;
        } else {
          p_mx_gate->ref_cnt++;
        }

        p_mx_cnx	= p_mx_gate->cnx_array + p_trk->id;

        if (p_mx_trk->next_match_info-1) {
                p_mx_trk->gate_map	=
                        TBX_REALLOC(p_mx_trk->gate_map,
                                    p_mx_trk->next_match_info+1);
        } else {
                p_mx_trk->gate_map	= TBX_MALLOC(2);
        }

        p_mx_trk->gate_map[p_mx_trk->next_match_info]		= p_gate->id;

        NM_TRACEF("recv pkt1 -->");
        {
                mx_request_t	rq;
                mx_status_t 	s;
                mx_segment_t	sg;
                uint32_t	r;

                sg.segment_ptr		= &pkt1;
                sg.segment_length	= sizeof(pkt1);
                mx_ret = mx_irecv(p_mx_trk->ep, &sg, 1,
                                  UINT64_C(0xdeadbeefdeadbeef),
                                  UINT64_C(0xffffffffffffffff), 0, &rq);
                nm_mx_check_return("mx_irecv", mx_ret);
                mx_ret = mx_wait(p_mx_trk->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("recv pkt1 <--");

        NM_TRACEF("accept - pkt1.drv_url: %s",	pkt1.drv_url);
        NM_TRACEF("accept - pkt1.trk_url: %s",	pkt1.trk_url);
        mx_ret	= mx_hostname_to_nic_id(pkt1.drv_url, &r_nic_id);
        nm_mx_check_return("(accept) mx_hostname_to_nic_id", mx_ret);

        {
                char *ptr = NULL;

                p_mx_cnx->r_ep_id = strtol(pkt1.trk_url, &ptr, 0);
        }

        p_mx_cnx->send_match_info	= pkt1.match_info;
	NM_TRACEF("accept - pkt1.match_info (we will contact our peer with this MI): %lu",	pkt1.match_info);

        NM_TRACEF("mx_connect -->");
        mx_ret	= mx_connect(p_mx_trk->ep,
                             r_nic_id,
                             p_mx_cnx->r_ep_id,
                             ep_key,
                             MX_INFINITE,
                             &p_mx_cnx->r_ep_addr);
        NM_TRACEF("mx_connect <--");
        nm_mx_check_return("mx_connect", mx_ret);

        NM_TRACEF("send pkt2 -->");
        {
                mx_request_t	rq;
                mx_status_t 	s;
                mx_segment_t	sg;
                uint32_t	r;

		p_mx_cnx->recv_match_info	= p_mx_trk->next_match_info;
                pkt2.match_info	= p_mx_trk->next_match_info++;
		NM_TRACEF("accept - pkt2.match_info (sender should contact us with this MI): %lu",	pkt2.match_info);

                sg.segment_ptr		= &pkt2;
                sg.segment_length	= sizeof(pkt2);
                mx_ret = mx_issend(p_mx_trk->ep, &sg, 1, p_mx_cnx->r_ep_addr,
                                   UINT64_C(0xbeefdeadbeefdead), 0, &rq);
                nm_mx_check_return("mx_issend", mx_ret);
                mx_ret = mx_wait(p_mx_trk->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("send pkt2 <--");

        err = NM_ESUCCESS;

        return err;
}

static
int
nm_mx_disconnect	(struct nm_cnx_rq *p_crq) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        int err = NM_ESUCCESS;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;

        p_mx_trk	= p_trk->priv;
        p_mx_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;

        if (p_mx_gate) {
          p_mx_gate->ref_cnt--;

          if (!p_mx_gate->ref_cnt) {
            TBX_FREE(p_mx_trk->gate_map);
            p_mx_trk->gate_map = NULL;
            TBX_FREE(p_mx_gate);
            p_mx_gate = NULL;
            p_gate->p_gate_drv_array[p_drv->id]->info = NULL;
          }
        }

        return err;
}

static
int
nm_mx_post_send_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_mx_drv	*p_mx_drv	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_cnx	*p_mx_cnx	= NULL;
        struct nm_mx_pkt_wrap	*p_mx_pw	= NULL;
        mx_return_t		 mx_ret		= MX_SUCCESS;
        int err;

        p_gate		= p_pw->p_gate;
        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_mx_drv	= p_drv->priv;
        p_mx_trk	= p_trk->priv;
        p_pw->gate_priv	= p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
        p_mx_gate	= p_pw->gate_priv;
        p_mx_cnx	= p_mx_gate->cnx_array + p_trk->id;

        p_mx_pw	= tbx_malloc(p_mx_drv->mx_pw_mem);
        p_pw->drv_priv	= p_mx_pw;

        p_mx_pw->p_ep	= &(p_mx_trk->ep);

        {
                mx_segment_t	 seg_list[p_pw->v_nb];
                mx_segment_t	*p_dst	= NULL;
                struct iovec	*p_src	= NULL;
                uint32_t	 i;

                p_src	= p_pw->v + p_pw->v_first;
                p_dst	= seg_list;

                for (i = 0; i < p_pw->v_nb; i++) {
                        p_dst->segment_ptr	= p_src->iov_base;
                        p_dst->segment_length	= p_src->iov_len;
                        p_src++;
                        p_dst++;
                }

                mx_ret	= mx_isend(p_mx_trk->ep,
                                   seg_list,
                                   p_pw->v_nb,
                                   p_mx_cnx->r_ep_addr,
                                   p_mx_cnx->send_match_info,
                                   NULL,
                                   &p_mx_pw->rq);
                nm_mx_check_return("mx_isend", mx_ret);
        }
#ifdef XPAULETTE
	p_pw->err=0;
	xpaul_req_init(&p_pw->inst);
	p_pw->inst.state|=XPAUL_STATE_ONE_SHOT;
	xpaul_req_submit(&nm_mx_ev_server, &p_pw->inst);
	err=p_pw->err;
#else
        err = nm_mx_poll_iov(p_pw);
#endif /* XPAULETTE */

        return err;
}

static
int
nm_mx_post_recv_iov	(struct nm_pkt_wrap *p_pw) {
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_drv	*p_mx_drv	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_pkt_wrap	*p_mx_pw	= NULL;
	uint64_t		 match_mask	= 0;
	uint64_t		 match_info	= 0;
        mx_return_t		 mx_ret		= MX_SUCCESS;
        int err;

        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_mx_drv	= p_drv->priv;
        p_mx_trk	= p_trk->priv;

        if (p_pw->p_gate) {
                struct nm_gate		*p_gate		= NULL;
                struct nm_mx_gate	*p_mx_gate	= NULL;
                struct nm_mx_cnx	*p_mx_cnx	= NULL;

                p_gate		= p_pw->p_gate;
                p_pw->gate_priv	= p_gate->p_gate_drv_array[p_drv->id]->info;
                p_mx_gate	= p_pw->gate_priv;
                p_mx_cnx	= p_mx_gate->cnx_array + p_trk->id;
                match_mask	= UINT64_C(0xffffffffffffffff);
                match_info	= p_mx_cnx->recv_match_info;
        } else {
                match_info	= 0;

                /* the first 'f' filters out administrative pkts */
                match_mask	= UINT64_C(0xf000000000000000);
        }

        p_mx_pw	= tbx_malloc(p_mx_drv->mx_pw_mem);
        p_pw->drv_priv	= p_mx_pw;

        p_mx_pw->p_ep		= &(p_mx_trk->ep);

        {
                mx_segment_t	 seg_list[p_pw->v_nb];
                mx_segment_t	*p_dst	= NULL;
                struct iovec	*p_src	= NULL;
                uint32_t	 i;

                p_src	= p_pw->v + p_pw->v_first;
                p_dst	= seg_list;

                for (i = 0; i < p_pw->v_nb; i++) {
                        p_dst->segment_ptr	= p_src->iov_base;
                        p_dst->segment_length	= p_src->iov_len;
                        p_src++;
                        p_dst++;
                }

                mx_ret	= mx_irecv(p_mx_trk->ep,
                                   seg_list,
                                   p_pw->v_nb,
                                   match_info,
                                   match_mask,
                                   NULL,
                                   &p_mx_pw->rq);
                nm_mx_check_return("mx_irecv", mx_ret);
        }
#ifdef XPAULETTE
	p_pw->err=0;
	xpaul_req_init(&p_pw->inst);
	p_pw->inst.state|=XPAUL_STATE_ONE_SHOT;
	xpaul_req_submit(&nm_mx_ev_server, &p_pw->inst);
	err=p_pw->err;
#else
        err = nm_mx_poll_iov(p_pw);
#endif /* XPAULETTE */

        return err;
}

#ifdef XPAULETTE

static
int
nm_mx_do_poll(xpaul_server_t	        server,
               xpaul_op_t		_op,
               xpaul_req_t		req,
               int			nb_ev,
               int			option) {
	struct nm_pkt_wrap      *p_ev;

        LOG_IN();
	p_ev = struct_up(req,  struct nm_pkt_wrap, inst);

	p_ev->err=nm_mx_poll_iov(p_ev);

	if(p_ev->err==NM_ESUCCESS)
		xpaul_req_success(&(p_ev->inst));
        LOG_OUT();
        return 0;
}
static
int
nm_mx_wait_iov    	(struct nm_pkt_wrap *p_pw) {
        LOG_IN();
	struct xpaul_wait wait;
	xpaul_req_wait(&p_pw->inst, &wait, 0);
	return p_pw->err;
}

#endif /* XPAULETTE */


static
int
nm_mx_poll_iov    	(struct nm_pkt_wrap *p_pw) {
        struct nm_mx_drv	*p_mx_drv	= NULL;
        struct nm_mx_pkt_wrap	*p_mx_pw	= NULL;
        mx_return_t	mx_ret	= MX_SUCCESS;
        mx_status_t	status;
        uint32_t	result;
        int err;
        p_mx_drv	= p_pw->p_drv->priv;
        p_mx_pw		= p_pw->drv_priv;

        mx_ret	= mx_test(*(p_mx_pw->p_ep), &p_mx_pw->rq, &status, &result);
        nm_mx_check_return("mx_test", mx_ret);

        if (!result) {
                err	= -NM_EAGAIN;
                goto out;
        }

        if (!p_pw->p_gate) {
                struct nm_mx_trk	*p_mx_trk	= NULL;

                /* gate-less request */
                p_mx_trk	= p_pw->p_trk->priv;
                p_pw->p_gate	= p_pw->p_drv->p_core->gate_array
                        + p_mx_trk->gate_map[status.match_info];
        }

        tbx_free(p_mx_drv->mx_pw_mem, p_mx_pw);

        if (status.code != MX_SUCCESS) {
                switch (status.code) {
                case MX_STATUS_PENDING:
                case MX_STATUS_BUFFERED:
                        err	= -NM_EINPROGRESS;
                        break;
                case MX_STATUS_TIMEOUT:
                        err	= -NM_ETIMEDOUT;
                        break;

                case MX_STATUS_TRUNCATED:
                        /* TODO: report truncated status
                         */
#warning TODO
                        err	=  NM_ESUCCESS;
                        break;

                case MX_STATUS_CANCELLED:
                        err	= -NM_ECANCELED;
                        break;

                case MX_STATUS_ENDPOINT_CLOSED:
                        err	= -NM_ECLOSED;
                        break;

                case MX_STATUS_ENDPOINT_UNREACHABLE:
                        err	= -NM_EUNREACH;
                        break;

                case MX_STATUS_REJECTED:
                case MX_STATUS_ENDPOINT_UNKNOWN:
                case MX_STATUS_BAD_SESSION:
                case MX_STATUS_BAD_KEY:
                case MX_STATUS_BAD_ENDPOINT:
                case MX_STATUS_BAD_RDMAWIN:
                        err	= -NM_EINVAL;
                        break;

                case MX_STATUS_ABORTED:
                        err	= -NM_EABORTED;
                        break;

                default:
                        err	= -NM_EUNKNOWN;
                        break;
                }

                goto out;
        }

        err = NM_ESUCCESS;

out:
        return err;
}

int
nm_mx_load(struct nm_drv_ops *p_ops) {
        p_ops->init		= nm_mx_init         ;
        p_ops->exit             = nm_mx_exit         ;

        p_ops->open_trk		= nm_mx_open_track   ;
        p_ops->close_trk	= nm_mx_close_track  ;

        p_ops->connect		= nm_mx_connect      ;
        p_ops->accept		= nm_mx_accept       ;
        p_ops->disconnect       = nm_mx_disconnect   ;

        p_ops->post_send_iov	= nm_mx_post_send_iov;
        p_ops->post_recv_iov    = nm_mx_post_recv_iov;

        p_ops->poll_send_iov    = nm_mx_poll_iov     ;
        p_ops->poll_recv_iov    = nm_mx_poll_iov     ;

#ifdef XPAULETTE
	p_ops->wait_iov         = nm_mx_wait_iov     ;
#endif /* XPAULETTE */

        return NM_ESUCCESS;
}

