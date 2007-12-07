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

#include "nm_public.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_trk.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_trk_rq.h"
#include "nm_cnx_rq.h"
#include "nm_errno.h"
#include "nm_log.h"

#ifdef ENABLE_SAMPLING
#include "nm_parser.h"
#endif

/** Initial number of packet wrappers */
#define INITIAL_PW_NUM		16

/** MX packet wrapper allocator */
p_tbx_memory_t mx_pw_mem;

/** MX specific driver data */
struct nm_mx_drv {
	/** Board number */
	uint32_t board_number;
	/** MX endpoint */
	mx_endpoint_t ep;
	/** Endpoint id */
	uint32_t ep_id;
};

struct nm_mx_trk {
	/** Next value to use for match info */
	uint16_t next_peer_id;
	/** Match info to gate id reverse mapping */
	uint8_t *gate_map;
};

/** MX specific connection data */
struct nm_mx_cnx {
	/** Remote endpoint addr */
	mx_endpoint_addr_t r_ep_addr;
	/** Remote endpoint id */
	uint32_t r_ep_id;
	/** MX match info (when sending) */
	uint64_t send_match_info;
	/** MX match info (when receiving) */
	uint64_t recv_match_info;
};

/** MX specific gate data */
struct nm_mx_gate {
	struct nm_mx_cnx cnx_array[255];
	int ref_cnt;
};

/** MX specific packet wrapper data */
struct nm_mx_pkt_wrap {
	mx_endpoint_t *p_ep;
	mx_request_t rq;
#ifdef PROFILE
        int send_bool;
#endif
};

/** MX specific first administrative packet data */
struct nm_mx_adm_pkt_1 {
	uint64_t match_info;
	char drv_url[MX_MAX_HOSTNAME_LEN];
	char trk_url[16];
};

/** MX specific second administrative packet data */
struct nm_mx_adm_pkt_2 {
	uint64_t match_info;
};

/** Endpoint filter */
/* Note: if this value fails, please use the probable date of the
   granting of tenure ("titularisation" in french) to Brice:
   0x01042010. Sorry for the inconvenience. */
#define NM_MX_ENDPOINT_FILTER 0x31051969

/*
 * How the matching is used
 */

/** @name Administrative packets matching mask
 * Bit 63: administrative packet or not?
 */
/*@{*/
#define NM_MX_ADMIN_MATCHING_BITS 1
#define NM_MX_ADMIN_MATCHING_SHIFT 63
/*@}*/

/** @name Peer id matching mask
 * Bits 47-62: peer id
 */
/*@{*/
#define NM_MX_PEER_ID_MATCHING_BITS 16
#define NM_MX_PEER_ID_MATCHING_SHIFT 47
/*@}*/

/** @name Track id matching mask
 * Bits 39-46: track id (internally multiplexed in MX 1.2)
 */
/*@{*/
#define NM_MX_TRACK_ID_MATCHING_BITS 8
#define NM_MX_TRACK_ID_MATCHING_SHIFT 39
/*@}*/

/** @name Actual matching info generation */
/*@{*/
/** Regular messages matching info */
#define NM_MX_MATCH_INFO(peer, track) ( (((uint64_t)peer) << NM_MX_PEER_ID_MATCHING_SHIFT) | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/** Administrative message mask */
#define NM_MX_ADMIN_MATCH_MASK (UINT64_C(1) << NM_MX_ADMIN_MATCHING_SHIFT)
/** Connect matching info, with the administrative bit */
#define NM_MX_CONNECT_MATCH_INFO(track) ( UINT64_C(0xdeadbeef) | NM_MX_ADMIN_MATCH_MASK | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/** Accept matching info, with the administrative bit */
#define NM_MX_ACCEPT_MATCH_INFO(track) ( UINT64_C(0xbeefdead) | NM_MX_ADMIN_MATCH_MASK | (((uint64_t)track) << NM_MX_TRACK_ID_MATCHING_SHIFT) )
/*@}*/

/*
 * Forwarded prototypes
 */

static
int
nm_mx_poll_iov    	(struct nm_pkt_wrap *p_pw);

/*
 * Functions
 */

/** Display the MX return value */
static __tbx_inline__
void
nm_mx_check_return(const char *msg, mx_return_t return_code) {
	if (tbx_unlikely(return_code != MX_SUCCESS)) {
		const char *msg_mx = NULL;

		msg_mx = mx_strerror(return_code);

		DISP("%s failed with code %s = %d/0x%x", msg, msg_mx, return_code, return_code);
	}
}

#ifdef PM2_NUIOA
/** Return the preferred NUMA node of the board */
static
int nm_mx_get_numa_node(uint32_t board_number)
{
#if MX_API >= 0x301
	mx_return_t ret;
	uint32_t numa_node = PM2_NUIOA_ANY_NODE;
	uint32_t line_speed;

	/* get the type of link */
	ret = mx_get_info(NULL, MX_LINE_SPEED, &board_number, sizeof(board_number), &line_speed, sizeof(line_speed));
	if (ret != MX_SUCCESS)
		return PM2_NUIOA_ANY_NODE;

	/* if myrinet 2000, numa effect are negligible, do not return any numa indication */
	if (line_speed == MX_SPEED_2G)
		return PM2_NUIOA_ANY_NODE;

	ret = mx_get_info(NULL, MX_NUMA_NODE, &board_number, sizeof(board_number), &numa_node, sizeof(numa_node));
	if (ret != MX_SUCCESS)
		return PM2_NUIOA_ANY_NODE;
	else
		return numa_node == -1 ? PM2_NUIOA_ANY_NODE : (int) numa_node;
#else /* MX_API < 0x301 */
	return PM2_NUIOA_ANY_NODE;
#endif /* MX_API < 0x301 */
}
#endif /* PM2_NUIOA */

/** Maintain a usage counter for each board to distribute the workload */
static int *board_use_count = NULL;
static int total_use_count;
static uint32_t boards;

static
int
nm_mx_init_boards(void)
{
        mx_return_t	mx_ret	= MX_SUCCESS;
	int err;

	/* find number of boards */
	mx_ret = mx_get_info(NULL, MX_NIC_COUNT, NULL, 0, &boards, sizeof(boards));
	nm_mx_check_return("mx_get_info", mx_ret);

	/* init total usage counters */
	total_use_count = 0;

	/* allocate usage counters */
	board_use_count = TBX_CALLOC(boards, sizeof(*board_use_count));
	if (!board_use_count) {
		err = -NM_ENOMEM;
		goto out;
	}

        err = NM_ESUCCESS;

 out:
        return err;
}

/** Query MX resources */
static
int
nm_mx_query		(struct nm_drv *p_drv,
			 struct nm_driver_query_param *params,
			 int nparam) {
        struct nm_mx_drv	*p_mx_drv	= NULL;
        mx_return_t	mx_ret	= MX_SUCCESS;
	uint32_t board_number;
	int i;
	int err;

	/* private data                                                 */
	p_mx_drv	= TBX_MALLOC(sizeof (struct nm_mx_drv));
	if (!p_mx_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

        memset(p_mx_drv, 0, sizeof (struct nm_mx_drv));
        p_drv->priv	= p_mx_drv;

	/* init MX */
        mx_set_error_handler(MX_ERRORS_RETURN);
        mx_ret	= mx_init();
	if (mx_ret != MX_ALREADY_INITIALIZED)
		/* special return code only used by mx_init() */
		nm_mx_check_return("mx_init", mx_ret);

	if (!board_use_count) {
		err = nm_mx_init_boards();
		if (err < 0)
			goto out;
	}

	board_number = 0xFFFFFFFF;
	for(i=0; i<nparam; i++) {
		switch (params[i].key) {
		case NM_DRIVER_QUERY_BY_INDEX:
			board_number = params[i].value.index;
			break;
		case NM_DRIVER_QUERY_BY_NOTHING:
			break;
		default:
			err = -NM_EINVAL;
			goto out;
		}
	}

	if (board_number == 0xFFFFFFFF) {
		/* find the least used board */
		int min_count = INT_MAX;
		int min_index = -1;
                uint32_t j;
		for(j=0; j<boards; j++) {
			if (board_use_count[j] < min_count) {
				min_index = j;
				min_count = board_use_count[j];
			}
		}
		board_number = min_index;

	} else if (board_number >= boards) {
		/* if a was has been chosen before, check the number */
		err = -NM_EINVAL;
		goto out;
	}

	p_mx_drv->board_number = board_number;

	/* register the board here so that load_init_some may query multiple boards
         * before initializing them */
	board_use_count[p_mx_drv->board_number]++;
	total_use_count++;

        /* driver capabilities encoding					*/
        p_drv->cap.has_trk_rq_dgram			= 1;
        p_drv->cap.has_selective_receive		= 1;
        p_drv->cap.has_concurrent_selective_receive	= 0;
#ifdef PM2_NUIOA
	p_drv->cap.numa_node = nm_mx_get_numa_node(p_mx_drv->board_number);
#endif

        err = NM_ESUCCESS;

 out:
        return err;
}

/** Initialize the MX driver */
static
int
nm_mx_init		(struct nm_drv *p_drv) {
	struct nm_mx_drv	*p_mx_drv	= p_drv->priv;
	mx_endpoint_t		 ep;
	mx_endpoint_addr_t	 ep_addr;
	uint32_t		 ep_id;
        mx_return_t	mx_ret	= MX_SUCCESS;
        uint64_t	nic_id;
        char		hostname[MX_MAX_HOSTNAME_LEN];
	mx_param_t              *ep_params = NULL;
	uint32_t                 ep_params_count = 0;
        int err;

        tbx_malloc_init(&mx_pw_mem,   sizeof(struct nm_mx_pkt_wrap),
                        INITIAL_PW_NUM,   "nmad/mx/pw");

        mx_board_number_to_nic_id(p_mx_drv->board_number, &nic_id);
        mx_nic_id_to_hostname(nic_id, hostname);

#if MX_API >= 0x301
	{
		/* Multiplex internal queues across tracks since we never match
		 * among multiple tracks. This will reduce the length of the queues
		 * and thus avoid long queue traversal caused by unrelated peers.
		 */
		mx_param_t param;
		param.key = MX_PARAM_CONTEXT_ID;
		param.val.context_id.bits = NM_MX_TRACK_ID_MATCHING_BITS;
		param.val.context_id.shift = NM_MX_TRACK_ID_MATCHING_SHIFT;
		ep_params = &param;
		ep_params_count = 1;
	}
#endif

	/* mx endpoint
	 */
	mx_ret = mx_open_endpoint(p_mx_drv->board_number,
				  MX_ANY_ENDPOINT,
				  NM_MX_ENDPOINT_FILTER,
				  ep_params, ep_params_count,
				  &ep);
	nm_mx_check_return("mx_open_endpoint", mx_ret);

	mx_ret = mx_get_endpoint_addr(ep, &ep_addr);
	nm_mx_check_return("mx_get_endpoint_addr", mx_ret);

	mx_ret = mx_decompose_endpoint_addr(ep_addr, &nic_id, &ep_id);
	nm_mx_check_return("mx_decompose_endpoint_addr", mx_ret);

        p_mx_drv->ep	= ep;
        p_mx_drv->ep_id	= ep_id;

        hostname[MX_MAX_HOSTNAME_LEN-1] = '\0';
        p_drv->url	= tbx_strdup(hostname);
        NM_TRACE_STR("p_drv->url", p_drv->url);
	p_drv->name = tbx_strdup("mx");

#ifdef ENABLE_SAMPLING
        nm_parse_sampling(p_drv, "mx");
#endif

        err = NM_ESUCCESS;
        return err;
}

/** Finalize the MX driver */
static
int
nm_mx_exit		(struct nm_drv *p_drv) {
        struct nm_mx_drv	*p_mx_drv	= NULL;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        p_mx_drv = p_drv->priv;

	mx_ret	= mx_close_endpoint(p_mx_drv->ep);
	nm_mx_check_return("mx_close_endpoint", mx_ret);

	board_use_count[p_mx_drv->board_number]--;
	total_use_count--;

	if (!total_use_count) {
	  mx_ret	= mx_finalize();
	  nm_mx_check_return("mx_finalize", mx_ret);

	  tbx_malloc_clean(mx_pw_mem);
          TBX_FREE(board_use_count);
	}

        TBX_FREE(p_mx_drv);
        p_drv->priv = NULL;

        TBX_FREE(p_drv->url);
        p_drv->url = NULL;

        err = NM_ESUCCESS;

        return err;
}

/** Open a MX track */
static
int
nm_mx_open_track	(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_drv	*p_mx_drv	= NULL;
        p_tbx_string_t		 url_string	= NULL;
        int err;

        p_trk	= p_trk_rq->p_trk;
	p_mx_drv        = p_trk->p_drv->priv;

        /* private data							*/
	p_mx_trk	= TBX_MALLOC(sizeof (struct nm_mx_trk));
        memset(p_mx_trk, 0, sizeof (struct nm_mx_trk));
	p_mx_trk->next_peer_id = 1;
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

	url_string	= tbx_string_init_to_int(p_mx_drv->ep_id);
	p_trk->url	= tbx_string_to_cstring(url_string);
	NM_TRACE_STR("p_trk->url", p_trk->url);
	tbx_string_free(url_string);

        err = NM_ESUCCESS;

        return err;
}

/** Close a MX track */
static
int
nm_mx_close_track	(struct nm_trk *p_trk) {
        struct nm_mx_trk	*p_mx_trk	= NULL;

        /* mx endpoint
         */
        p_mx_trk = p_trk->priv;
        TBX_FREE(p_mx_trk->gate_map);
        p_mx_trk->gate_map = NULL;
        TBX_FREE(p_trk->priv);

        TBX_FREE(p_trk->url);

        return NM_ESUCCESS;
}

/** Connect to a new MX peer */
static
int
nm_mx_connect		(struct nm_cnx_rq *p_crq) {
        struct nm_mx_adm_pkt_1   pkt1;
        struct nm_mx_adm_pkt_2   pkt2;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
	struct nm_mx_drv        *p_mx_drv       = NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_cnx	*p_mx_cnx	= NULL;
        char			*drv_url	= NULL;
        char			*trk_url	= NULL;
        uint64_t		 r_nic_id	= 0;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
	p_mx_drv        = p_drv->priv;
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

	if (p_mx_trk->next_peer_id-1) {
                p_mx_trk->gate_map	=
                        TBX_REALLOC(p_mx_trk->gate_map,
                                    p_mx_trk->next_peer_id+1);
        } else {
                p_mx_trk->gate_map	= TBX_MALLOC(2);
        }

        p_mx_trk->gate_map[p_mx_trk->next_peer_id] = p_gate->id;

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
        mx_ret	= mx_connect(p_mx_drv->ep,
                             r_nic_id,
                             p_mx_cnx->r_ep_id,
                             NM_MX_ENDPOINT_FILTER,
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
		pkt1.match_info	= NM_MX_MATCH_INFO(p_trk->id, p_mx_trk->next_peer_id);
		p_mx_cnx->recv_match_info = pkt1.match_info;
		p_mx_trk->next_peer_id++;
		if (p_mx_trk->next_peer_id == (1 << NM_MX_PEER_ID_MATCHING_BITS) - 1)
			DISP("reached maximal number of peers %d", p_mx_trk->next_peer_id);

                NM_TRACEF("connect - pkt1.drv_url: %s",	pkt1.drv_url);
                NM_TRACEF("connect - pkt1.trk_url: %s",	pkt1.trk_url);
                NM_TRACEF("connect - pkt1.match_info (sender should contact us with this MI): %lu",	pkt1.match_info);

                sg.segment_ptr		= &pkt1;
                sg.segment_length	= sizeof(pkt1);
                mx_ret = mx_isend(p_mx_drv->ep, &sg, 1, p_mx_cnx->r_ep_addr,
				  NM_MX_CONNECT_MATCH_INFO(p_trk->id), 0, &rq);
                nm_mx_check_return("mx_isend", mx_ret);
                mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
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
                mx_ret = mx_irecv(p_mx_drv->ep, &sg, 1,
                                  NM_MX_ACCEPT_MATCH_INFO(p_trk->id),
                                  MX_MATCH_MASK_NONE, 0, &rq);
                nm_mx_check_return("mx_irecv", mx_ret);
                mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("recv pkt2 <--");

        p_mx_cnx->send_match_info	= pkt2.match_info;
	NM_TRACEF("connect - pkt2.match_info (we will contact our peer with this MI): %lu",	pkt2.match_info);

        NMAD_EVENT_NEW_TRK(p_gate->id, p_drv->id, p_trk->id);
        err = NM_ESUCCESS;

        return err;
}

/** Accept the connection request from a MX peer */
static
int
nm_mx_accept		(struct nm_cnx_rq *p_crq) {
        struct nm_mx_adm_pkt_1   pkt1;
        struct nm_mx_adm_pkt_2   pkt2;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
	struct nm_mx_drv        *p_mx_drv       = NULL;
        struct nm_mx_gate	*p_mx_gate	= NULL;
        struct nm_mx_trk	*p_mx_trk	= NULL;
        struct nm_mx_cnx	*p_mx_cnx	= NULL;
        uint64_t		 r_nic_id	= 0;
        mx_return_t	mx_ret	= MX_SUCCESS;
        int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
 	p_mx_drv        = p_drv->priv;
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

	if (p_mx_trk->next_peer_id-1) {
                p_mx_trk->gate_map	=
                        TBX_REALLOC(p_mx_trk->gate_map,
                                    p_mx_trk->next_peer_id+1);
        } else {
                p_mx_trk->gate_map	= TBX_MALLOC(2);
        }

        p_mx_trk->gate_map[p_mx_trk->next_peer_id] = p_gate->id;

        NM_TRACEF("recv pkt1 -->");
        {
                mx_request_t	rq;
                mx_status_t 	s;
                mx_segment_t	sg;
                uint32_t	r;

                sg.segment_ptr		= &pkt1;
                sg.segment_length	= sizeof(pkt1);
                mx_ret = mx_irecv(p_mx_drv->ep, &sg, 1,
                                  NM_MX_CONNECT_MATCH_INFO(p_trk->id),
                                  MX_MATCH_MASK_NONE, 0, &rq);
                nm_mx_check_return("mx_irecv", mx_ret);
                mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
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
        mx_ret	= mx_connect(p_mx_drv->ep,
                             r_nic_id,
                             p_mx_cnx->r_ep_id,
                             NM_MX_ENDPOINT_FILTER,
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

		pkt2.match_info	= NM_MX_MATCH_INFO(p_trk->id, p_mx_trk->next_peer_id);
		p_mx_cnx->recv_match_info = pkt2.match_info;
		p_mx_trk->next_peer_id++;
		if (p_mx_trk->next_peer_id == (1 << NM_MX_PEER_ID_MATCHING_BITS) - 1)
			DISP("reached maximal number of peers %d", p_mx_trk->next_peer_id);

		NM_TRACEF("accept - pkt2.match_info (sender should contact us with this MI): %lu",	pkt2.match_info);

                sg.segment_ptr		= &pkt2;
                sg.segment_length	= sizeof(pkt2);
                mx_ret = mx_issend(p_mx_drv->ep, &sg, 1, p_mx_cnx->r_ep_addr,
                                   NM_MX_ACCEPT_MATCH_INFO(p_trk->id), 0, &rq);
                nm_mx_check_return("mx_issend", mx_ret);
                mx_ret = mx_wait(p_mx_drv->ep, &rq, MX_INFINITE, &s, &r);
                nm_mx_check_return("mx_wait", mx_ret);
        }
        NM_TRACEF("send pkt2 <--");

        NMAD_EVENT_NEW_TRK(p_gate->id, p_drv->id, p_trk->id);
        err = NM_ESUCCESS;

        return err;
}

/** Disconnect from a new MX peer */
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
            TBX_FREE(p_mx_gate);
            p_mx_gate = NULL;
            p_gate->p_gate_drv_array[p_drv->id]->info = NULL;
          }
        }

        return err;
}

/** Post a iov send request to MX */
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

	p_mx_pw	= tbx_malloc(mx_pw_mem);
        p_pw->drv_priv	= p_mx_pw;
#ifdef PROFILE
        p_mx_pw->send_bool = 1;
#endif
        p_mx_pw->p_ep	= &(p_mx_drv->ep);

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

                NMAD_EVENT_SND_START(p_pw->p_gate->id, p_pw->p_drv->id, p_pw->p_trk->id, p_pw->length);

                mx_ret	= mx_isend(p_mx_drv->ep,
                                   seg_list,
                                   p_pw->v_nb,
                                   p_mx_cnx->r_ep_addr,
                                   p_mx_cnx->send_match_info,
                                   NULL,
                                   &p_mx_pw->rq);
                nm_mx_check_return("mx_isend", mx_ret);
        }

        err = nm_mx_poll_iov(p_pw);

        return err;
}

/** Post a iov receive request to MX */
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

        if (tbx_likely(p_pw->p_gate)) {
                struct nm_gate		*p_gate		= NULL;
                struct nm_mx_gate	*p_mx_gate	= NULL;
                struct nm_mx_cnx	*p_mx_cnx	= NULL;

                p_gate		= p_pw->p_gate;
                p_pw->gate_priv	= p_gate->p_gate_drv_array[p_drv->id]->info;
                p_mx_gate	= p_pw->gate_priv;
                p_mx_cnx	= p_mx_gate->cnx_array + p_trk->id;
                match_mask	= MX_MATCH_MASK_NONE;
                match_info	= p_mx_cnx->recv_match_info;
        } else {
		/* filter out administrative pkts */
                match_info	= 0;
                match_mask	= NM_MX_ADMIN_MATCH_MASK;
        }

	p_mx_pw	= tbx_malloc(mx_pw_mem);
        p_pw->drv_priv	= p_mx_pw;
#ifdef PROFILE
        p_mx_pw->send_bool = 0;
#endif

        p_mx_pw->p_ep		= &(p_mx_drv->ep);

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

                mx_ret	= mx_irecv(p_mx_drv->ep,
                                   seg_list,
                                   p_pw->v_nb,
                                   match_info,
                                   match_mask,
                                   NULL,
                                   &p_mx_pw->rq);
                nm_mx_check_return("mx_irecv", mx_ret);
        }

        err = nm_mx_poll_iov(p_pw);

        return err;
}

/** Post a iov request completion in MX */
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

        if (tbx_unlikely(!result)) {
                err	= -NM_EAGAIN;
                goto out;
        }

        if (tbx_unlikely(!p_pw->p_gate)) {
                struct nm_mx_trk	*p_mx_trk	= NULL;

                /* gate-less request */
                p_mx_trk	= p_pw->p_trk->priv;
                p_pw->p_gate	= p_pw->p_drv->p_core->gate_array
                        + p_mx_trk->gate_map[status.match_info];
        }

	tbx_free(mx_pw_mem, p_mx_pw);

        if (tbx_unlikely(status.code != MX_SUCCESS)) {
		WARN("MX driver: request completed with non-successful status: %s",
		     mx_strstatus(status.code));
                switch (status.code) {
                case MX_STATUS_PENDING:
                case MX_STATUS_BUFFERED:
                        err	= -NM_EINPROGRESS;
                        break;
                case MX_STATUS_TIMEOUT:
                        err	= -NM_ETIMEDOUT;
                        break;

                case MX_STATUS_TRUNCATED:
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
#ifdef PROFILE
        if (err == NM_ESUCCESS && !p_mx_pw->send_bool) {
                NMAD_EVENT_RCV_END(p_pw->p_gate->id, p_pw->p_drv->id, p_pw->p_trk->id, p_pw->length);
        }
#endif

        return err;
}

static int nm_mx_release_req(struct nm_pkt_wrap *p_pw){
  struct nm_mx_drv	*p_mx_drv	= NULL;
  struct nm_mx_pkt_wrap	*p_mx_pw	= NULL;

  p_mx_drv = p_pw->p_drv->priv;
  p_mx_pw  = p_pw->drv_priv;

  tbx_free(mx_pw_mem, p_mx_pw);

  return NM_ESUCCESS;
}

/** Load MX operations */
int
nm_mx_load(struct nm_drv_ops *p_ops) {
        p_ops->query		= nm_mx_query         ;
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

        p_ops->release_req      = nm_mx_release_req;

        return NM_ESUCCESS;
}

