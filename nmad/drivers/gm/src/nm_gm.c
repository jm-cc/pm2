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
#include <assert.h>

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

#include <gm.h>

#if 1
#  define __error__(s, ...) fprintf(stderr, "%s:%d: *error* "s"\n", __FUNCTION__, __LINE__ , ## __VA_ARGS__)
#else
#  define __error__(s, ...)
#endif

#define __gmerror__(x) (nm_gm_error((x), __LINE__))

struct nm_gm_drv {
        int gm;
};

/* TODO: setup full gate id reverse mapping struct
   - reverse mapping is currently performed only on node_id and therefore
   does not support multiple processes per node, it should also include port_id
   mapping to allow distinguishing packets coming from the same node but from
   different processes
 */
#warning TODO


struct nm_gm_request {
        volatile int		 lock;
        gm_status_t		 status;
};

struct nm_gm_trk {
	struct gm_port		*p_gm_port;
	int			 port_id;
	unsigned int		 node_id;
        unsigned char		 unique_node_id[6];
        unsigned int		 max_node_id;
        uint8_t			*gate_map;
};

struct nm_gm_cnx {
	int			 port_id;
	unsigned int		 node_id;
        unsigned char		 unique_node_id[6];
};

struct nm_gm_gate {
        struct nm_gm_cnx	 cnx_array[255];
};

struct nm_gm_pkt_wrap {
	struct gm_port		*p_gm_port;
        struct iovec		*p_iov;
        struct nm_gm_request	 rq;
};

static
const int nm_gm_pub_port_array[] = {
         2,
         4,
         5,
         6,
         7,
        -1
};

static
const int nm_gm_nb_ports = 5;

/* GM error message display fonction. */
static
void
nm_gm_error(gm_status_t gm_status, int line)
{
	char *msg = NULL;

	switch (gm_status) {
	case GM_SUCCESS:
		break;

        case GM_FAILURE:
                msg = "GM failure";
                break;

        case GM_INPUT_BUFFER_TOO_SMALL:
                msg = "GM input buffer too small";
                break;

        case GM_OUTPUT_BUFFER_TOO_SMALL:
                msg = "GM output buffer too small";
                break;

        case GM_TRY_AGAIN:
                msg = "GM try again";
                break;

        case GM_BUSY:
                msg = "GM busy";
                break;

        case GM_MEMORY_FAULT:
                msg = "GM memory fault";
                break;

        case GM_INTERRUPTED:
                msg = "GM interrupted";
                break;

        case GM_INVALID_PARAMETER:
                msg = "GM invalid parameter";
                break;

        case GM_OUT_OF_MEMORY:
                msg = "GM out of memory";
                break;

        case GM_INVALID_COMMAND:
                msg = "GM invalid command";
                break;

        case GM_PERMISSION_DENIED:
                msg = "GM permission denied";
                break;

        case GM_INTERNAL_ERROR:
                msg = "GM internal error";
                break;

        case GM_UNATTACHED:
                msg = "GM unattached";
                break;

        case GM_UNSUPPORTED_DEVICE:
                msg = "GM unsupported device";
                break;

        case GM_SEND_TIMED_OUT:
		msg = "GM send timed out";
                break;

        case GM_SEND_REJECTED:
		msg = "GM send rejected";
                break;

        case GM_SEND_TARGET_PORT_CLOSED:
		msg = "GM send target port closed";
                break;

        case GM_SEND_TARGET_NODE_UNREACHABLE:
		msg = "GM send target node unreachable";
                break;

        case GM_SEND_DROPPED:
		msg = "GM send dropped";
                break;

        case GM_SEND_PORT_CLOSED:
		msg = "GM send port closed";
                break;

        case GM_NODE_ID_NOT_YET_SET:
                msg = "GM id not yet set";
                break;

        case GM_STILL_SHUTTING_DOWN:
                msg = "GM still shutting down";
                break;

        case GM_CLONE_BUSY:
                msg = "GM clone busy";
                break;

        case GM_NO_SUCH_DEVICE:
                msg = "GM no such device";
                break;

        case GM_ABORTED:
                msg = "GM aborted";
                break;

#if defined (GM_API_VERSION_1_5) && GM_API_VERSION >= GM_API_VERSION_1_5
        case GM_INCOMPATIBLE_LIB_AND_DRIVER:
                msg = "GM incompatible lib and driver";
                break;

        case GM_UNTRANSLATED_SYSTEM_ERROR:
                msg = "GM untranslated system error";
                break;

        case GM_ACCESS_DENIED:
                msg = "GM access denied";
                break;
#endif

	default:
		msg = "unknown GM error";
		break;
	}

	if (msg) {
		DISP("%d:%s\n", line, msg);
                gm_perror ("gm_message", gm_status);
	}
}

static
void
nm_gm_callback(struct gm_port *p_gm_port,
               void           *ptr,
               gm_status_t     gms) {
        struct nm_gm_request *p_rq   = NULL;

	if (gms != GM_SUCCESS) {
	  __gmerror__(gms);
	  TBX_FAILURE("send request failed");
	}

        p_rq		= ptr;
        p_rq->status	= gms;
        p_rq->lock		= 1;

        NM_TRACEF("<nm_gm_callback>");
}

static
void
nm_gm_extract_info(char			 *trk_url,
                   struct nm_gm_trk	 *p_gm_trk,
                   struct nm_gm_cnx	 *p_gm_cnx) {
	gm_status_t                    gms = GM_SUCCESS;
        char		*str	= NULL;
        char		*ptr1	= NULL;
        char		*ptr2	= NULL;
        char		*ptr3	= NULL;
        char		*pa	= NULL;
        char		*pb	= NULL;
        unsigned int	 ui	= 0;
        int		 dev_id;

        str = tbx_strdup(trk_url);

        ui = strtoul(str, &ptr1, 0);
        if (str == ptr1) {
                __error__("could not extract the remote device id");
                goto error;
        }
        dev_id = ui;

        ui = strtoul(ptr1, &ptr2, 0);
        if (ptr1 == ptr2) {
                __error__("could not extract the remote port id");
                goto error;
        }
        p_gm_cnx->port_id = ui;

        ui = strtoul(ptr2, &ptr3, 0);
        if (ptr2 == ptr3) {
                __error__("could not extract the remote node id");
                goto error;
        }
        p_gm_cnx->node_id = ui;

        NM_TRACE_VAL("remote port_id", p_gm_cnx->port_id);
        NM_TRACE_VAL("remote node_id", p_gm_cnx->node_id);

#if defined (GM_API_VERSION_2_0) && GM_API_VERSION >= GM_API_VERSION_2_0
        {
                int i = 0;

                pa = ptr3;
                pb = ptr3;

                for (i = 0; i < 6; i++) {
                        unsigned int val = 0;
                        val = strtoul(pa, &pb, 0);
                        if (pa == pb) {
                                __error__("could not extract the remote unique node id");
                                goto error;
                        }

                        pa = pb;
                        p_gm_cnx->unique_node_id[i] = (char)val;
                }

                for (i = 0; i < 6; i++) {
                        NM_TRACEF("unique id[%d] = %3d", i, p_gm_cnx->unique_node_id[i]);
                }
        }

        gms = gm_unique_id_to_node_id(p_gm_trk->p_gm_port,
                                      p_gm_cnx->unique_node_id,
                                      &(p_gm_cnx->node_id));
        if (gms != GM_SUCCESS) {
                __error__("gm_unique_id_to_node_id failed");
                __gmerror__(gms);
                goto error;
        }
#endif

        NM_TRACE_VAL("remote node_id (GM v2.0)", p_gm_cnx->node_id);

        TBX_FREE(str);
        str = NULL;

        return;

 error:
        TBX_FAILURE("mad_gm_extract_info failed");
}

static
int
nm_gm_query			(struct nm_drv *p_drv,
				 struct nm_driver_query_param *params,
				 int nparam) {
	struct nm_gm_drv	*p_gm_drv	= NULL;
	int err;

	/* private data							*/
	p_gm_drv	= TBX_MALLOC(sizeof (struct nm_gm_drv));
	if (!p_gm_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

	memset(p_gm_drv, 0, sizeof (struct nm_gm_drv));
	p_drv->priv	= p_gm_drv;

	/* driver capabilities encoding					*/
	p_drv->cap.has_trk_rq_dgram			= 1;
	p_drv->cap.has_selective_receive		= 0;
	p_drv->cap.has_concurrent_selective_receive	= 0;
#ifdef PM2_NUIOA
	p_drv->cap.numa_node = PM2_NUIOA_ANY_NODE;
#endif

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_gm_init			(struct nm_drv *p_drv) {
        struct nm_gm_drv	*p_gm_drv	= p_drv->priv;
        gm_status_t		 gms		= GM_SUCCESS;
	int err;

        /* GM								*/
        gms = gm_init();
        if (gms) {
                __error__("gm_init failed");
                __gmerror__(gms);
                goto error;
        }

	/* driver url encoding						*/
        p_drv->url	= tbx_strdup("-");
	p_drv->name 	= tbx_strdup("gm");

        NM_TRACE_STR("drv_url", p_drv->url);

	err = NM_ESUCCESS;
	return err;

 error:
        TBX_FAILURE("nm_gm_init failed");
}

static
int
nm_gm_exit			(struct nm_drv *p_drv) {
        struct nm_gm_drv	*p_gm_drv	= NULL;
	int err;

        p_gm_drv	= p_drv->priv;

        TBX_FREE(p_drv->url);
        TBX_FREE(p_gm_drv);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_gm_open_trk		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_trk	*p_gm_trk	= NULL;
	struct gm_port		*p_gm_port	= NULL;
	int			 port_id	= 0;
	unsigned int		 node_id	= 0;
        int			 n		= 0;
        gm_status_t		 gms		= GM_SUCCESS;
        p_tbx_string_t		 url_string	= NULL;
        const int		 device_id	= 0;
	int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_gm_trk	= TBX_MALLOC(sizeof (struct nm_gm_trk));
        if (!p_gm_trk) {
                err = -NM_ENOMEM;
                goto out;
        }
        memset(p_gm_trk, 0, sizeof (struct nm_gm_trk));
        p_trk->priv	= p_gm_trk;

        /* track capabilities encoding					*/
        p_trk->cap.rq_type	= nm_trk_rq_dgram;
        p_trk->cap.iov_type	= nm_trk_iov_none;
        p_trk->cap.max_pending_send_request	= 1;
        p_trk->cap.max_pending_recv_request	= 1;
        p_trk->cap.max_single_request_length	= SSIZE_MAX;
        p_trk->cap.max_iovec_request_length	= 0;
        p_trk->cap.max_iovec_size		= 0;

        /* GM								*/
        while ((port_id = nm_gm_pub_port_array[n]) != -1) {
                gms = gm_open(&p_gm_port, device_id, port_id,
                              "nm_gm", GM_API_VERSION_1_1);
                if (gms == GM_SUCCESS) {
                        goto found;
                }

                if (gms != GM_BUSY) {
                        __error__("gm_open failed");
                        __gmerror__(gms);
                        goto error;
                }
                n++;
	}

        __error__("no more GM ports");
        goto error;


 found:
	gms = gm_get_node_id(p_gm_port, &node_id);
	if (gms != GM_SUCCESS) {
                __error__("gm_get_node_id failed");
		__gmerror__(gms);
                goto error;
	}

        p_gm_trk->p_gm_port	= p_gm_port;
        p_gm_trk->port_id	= port_id;
        p_gm_trk->node_id	= node_id;

#if defined (GM_API_VERSION_2_0) && GM_API_VERSION >= GM_API_VERSION_2_0
	gms = gm_node_id_to_unique_id(p_gm_port, node_id, p_gm_trk->unique_node_id);

	if (gms != GM_SUCCESS) {
                __error__("gm_node_id_to_unique_id failed");
		__gmerror__(gms);
                goto error;
	}
#endif

        NM_TRACE_VAL("local port_id", port_id);
        NM_TRACE_VAL("local node_id", node_id);

        url_string    = tbx_string_init_to_uint(device_id);
        tbx_string_append_char(url_string, ' ');
        tbx_string_append_uint(url_string,  port_id);
        tbx_string_append_char(url_string, ' ');
        tbx_string_append_uint(url_string,  node_id);

        {
                int i = 0;
                for (i = 0; i < 6; i++) {
                        tbx_string_append_char(url_string, ' ');
                        tbx_string_append_uint(url_string,  (unsigned int)p_gm_trk->unique_node_id[i]);
                        //
                }
        }

        p_trk->url	= tbx_string_to_cstring(url_string);
        tbx_string_free(url_string);

        NM_TRACE_STR("trk_url", p_trk->url);

	err = NM_ESUCCESS;

 out:
	return err;

 error:
        TBX_FAILURE("nm_gm_open_trk failed");
}

static
int
nm_gm_close_trk		(struct nm_trk *p_trk) {
        struct nm_gm_trk	*p_gm_trk	= NULL;
	int err;

        p_gm_trk	= p_trk->priv;
        TBX_FREE(p_trk->url);
        TBX_FREE(p_gm_trk);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_gm_connect		(struct nm_cnx_rq *p_crq) {
        struct nm_gm_gate	*p_gm_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_trk	*p_gm_trk	= NULL;
        struct nm_gm_cnx	*p_gm_cnx	= NULL;
        struct gm_port		*p_gm_port	= NULL;
        struct nm_gm_request	 rq;
        void			*pkt		= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_gm_trk	= p_trk->priv;

        /* private data				*/
        p_gm_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_gm_gate) {
                p_gm_gate	= TBX_MALLOC(sizeof (struct nm_gm_gate));
                if (!p_gm_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_gm_gate, 0, sizeof(struct nm_gm_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_gm_gate;
        }

        p_gm_cnx	= p_gm_gate->cnx_array + p_trk->id;
        nm_gm_extract_info(p_crq->remote_trk_url, p_gm_trk, p_gm_cnx);

        if (p_gm_trk->max_node_id) {
                if (p_gm_cnx->node_id > p_gm_trk->max_node_id) {
                        p_gm_trk->max_node_id	= p_gm_cnx->node_id;
                        p_gm_trk->gate_map	=
                                TBX_REALLOC(p_gm_trk->gate_map,
                                            p_gm_trk->max_node_id+1);
                }
        } else {
                p_gm_trk->max_node_id	= p_gm_cnx->node_id;
                p_gm_trk->gate_map	= TBX_MALLOC(p_gm_trk->max_node_id+1);
        }

        p_gm_trk->gate_map[p_gm_cnx->node_id]	= p_gate->id;
        NM_TRACEF("new gate map entry: %lu --> %lu",
             (unsigned long)p_gm_cnx->node_id,
             (unsigned long)p_gate->id);

        p_gm_port	= p_gm_trk->p_gm_port;

        pkt = gm_dma_malloc(p_gm_port, 64);
        if (!pkt) {
                __error__("could not allocate a dma buffer");
                goto error;
        }

        rq.lock = 0;
        strcpy(pkt, p_trk->url);
        gm_send_with_callback(p_gm_port,
                              pkt,
                              gm_min_size_for_length(64),
                              64,
                              GM_HIGH_PRIORITY,
                              p_gm_cnx->node_id,
                              p_gm_cnx->port_id,
                              nm_gm_callback,
                              &rq);

        do {
                gm_recv_event_t *p_event   = NULL;

                p_event = gm_blocking_receive(p_gm_port);
                gm_unknown(p_gm_port, p_event);
        } while (!rq.lock);

        gm_dma_free(p_gm_port, pkt);

	err = NM_ESUCCESS;

 out:
	return err;

 error:
        TBX_FAILURE("nm_gm_connect failed");
}

static
int
nm_gm_accept			(struct nm_cnx_rq *p_crq) {
        struct nm_gm_gate	*p_gm_gate	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_trk	*p_gm_trk	= NULL;
        struct nm_gm_cnx	*p_gm_cnx	= NULL;
        struct gm_port		*p_gm_port	= NULL;
        void			*pkt		= NULL;
        gm_recv_event_t		*p_event	= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_gm_trk	= p_trk->priv;

        /* private data				*/
        p_gm_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
        if (!p_gm_gate) {
                p_gm_gate	= TBX_MALLOC(sizeof (struct nm_gm_gate));
                if (!p_gm_gate) {
                        err = -NM_ENOMEM;
                        goto out;
                }

                memset(p_gm_gate, 0, sizeof(struct nm_gm_gate));

                /* update gate private data		*/
                p_gate->p_gate_drv_array[p_drv->id]->info	= p_gm_gate;
        }
        p_gm_cnx	= p_gm_gate->cnx_array + p_trk->id;

        p_gm_port	= p_gm_trk->p_gm_port;

        pkt = gm_dma_malloc(p_gm_port, 64);
        if (!pkt) {
                __error__("could not allocate a dma buffer");
                goto error;
        }

        gm_provide_receive_buffer_with_tag(p_gm_port,
                                           pkt,
                                           gm_min_size_for_length(64),
                                           GM_HIGH_PRIORITY, 1);

        for (;;) {
                p_event = gm_blocking_receive(p_gm_port);

                switch (gm_ntohc(p_event->recv.type)) {

                case GM_FAST_HIGH_PEER_RECV_EVENT:
                case GM_FAST_HIGH_RECV_EVENT:
                        {
                                char	*_msg	= NULL;

                                _msg	= gm_ntohp(p_event->recv.message);
                                nm_gm_extract_info(_msg, p_gm_trk, p_gm_cnx);

                        }
                        goto received;

                case GM_HIGH_PEER_RECV_EVENT:
                case GM_HIGH_RECV_EVENT:
                        {
                                char	*_pkt	= NULL;

                                _pkt	= gm_ntohp(p_event->recv.buffer);
                                nm_gm_extract_info(_pkt, p_gm_trk, p_gm_cnx);
                        }
                        goto received;

                default:
                        gm_unknown(p_gm_port, p_event);
                        break;
                }
        }

 received:
        gm_dma_free(p_gm_port, pkt);

        if (p_gm_trk->max_node_id) {
                if (p_gm_cnx->node_id > p_gm_trk->max_node_id) {
                        p_gm_trk->max_node_id	= p_gm_cnx->node_id;
                        p_gm_trk->gate_map	=
                                TBX_REALLOC(p_gm_trk->gate_map,
                                            p_gm_trk->max_node_id+1);
                }
        } else {
                p_gm_trk->max_node_id	= p_gm_cnx->node_id;
                p_gm_trk->gate_map	= TBX_MALLOC(p_gm_trk->max_node_id+1);
        }

        p_gm_trk->gate_map[p_gm_cnx->node_id]	= p_gate->id;
        NM_TRACEF("new gate map entry: %lu --> %lu",
             (unsigned long)p_gm_cnx->node_id,
             (unsigned long)p_gate->id);

	err = NM_ESUCCESS;

 out:
	return err;

 error:
        TBX_FAILURE("nm_gm_accept failed");
}

static
int
nm_gm_disconnect		(struct nm_cnx_rq *p_crq) {
        gm_status_t		 gms		= GM_SUCCESS;
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_gm_post_send_iov		(struct nm_pkt_wrap *p_pw) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_gate	*p_gm_gate	= NULL;
        struct nm_gm_trk	*p_gm_trk	= NULL;
        struct nm_gm_cnx	*p_gm_cnx	= NULL;
        struct nm_gm_pkt_wrap	*p_gm_pw	= NULL;
	struct gm_port		*p_gm_port	= NULL;
        struct iovec		*p_iov		= NULL;
        gm_status_t		 gms		= GM_SUCCESS;
	int err;

        p_gate		= p_pw->p_gate;
        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_gm_trk	= p_trk->priv;
        p_pw->gate_priv	= p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
        p_gm_gate	= p_pw->gate_priv;
        p_gm_cnx	= p_gm_gate->cnx_array + p_trk->id;

        p_gm_pw		= TBX_MALLOC(sizeof(struct nm_gm_pkt_wrap));
        p_pw->drv_priv	= p_gm_pw;

        p_gm_pw->p_gm_port	= p_gm_trk->p_gm_port;
        p_gm_port		= p_gm_pw->p_gm_port;

        p_gm_pw->p_iov	= p_pw->v + p_pw->v_first;
        p_iov		= p_gm_pw->p_iov;

        p_gm_pw->rq.lock	= 0;

        /* register memory block
         */
        gms = gm_register_memory(p_gm_port, p_iov->iov_base, p_iov->iov_len);
        if (gms) {
                __error__("memory registration failed");
                __gmerror__(gms);
                goto error;
        }

        gm_send_with_callback(p_gm_port,
                              p_iov->iov_base,
                              gm_min_size_for_length(2*1024*1024),
                              p_iov->iov_len,
                              GM_LOW_PRIORITY,
                              p_gm_cnx->node_id,
                              p_gm_cnx->port_id,
                              nm_gm_callback,
                              &p_gm_pw->rq);


	err = -NM_EAGAIN;

	return err;

 error:
        TBX_FAILURE("nm_gm_post_send_iov");
}

static
int
nm_gm_post_recv_iov		(struct nm_pkt_wrap *p_pw) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_trk	*p_gm_trk	= NULL;
        struct nm_gm_pkt_wrap	*p_gm_pw	= NULL;
	struct gm_port		*p_gm_port	= NULL;
        struct iovec		*p_iov		= NULL;
         gm_status_t		 gms		= GM_SUCCESS;
	int err;

        p_gate		= p_pw->p_gate;
        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;

        p_gm_trk	= p_trk->priv;

        if (p_gate) {
                p_pw->gate_priv	= p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id]->info;
        }

        p_gm_pw		= TBX_MALLOC(sizeof(struct nm_gm_pkt_wrap));
        p_pw->drv_priv	= p_gm_pw;

        p_gm_pw->p_gm_port	= p_gm_trk->p_gm_port;
        p_gm_port		= p_gm_pw->p_gm_port;

        p_gm_pw->p_iov	= p_pw->v + p_pw->v_first;
        p_iov		= p_gm_pw->p_iov;

        /* register memory block
         */
        gms = gm_register_memory(p_gm_port, p_iov->iov_base, p_iov->iov_len);
        if (gms) {
                __error__("memory registration failed");
                __gmerror__(gms);
                goto error;
        }

        gm_provide_receive_buffer_with_tag(p_gm_port,
                                           p_iov->iov_base,
                                           gm_min_size_for_length(2*1024*1024),
                                           GM_LOW_PRIORITY, 1);

	err = -NM_EAGAIN;

	return err;

 error:
        TBX_FAILURE("nm_gm_post_recv_iov");
}

static
int
nm_gm_poll_send_iov    	(struct nm_pkt_wrap *p_pw) {
        struct nm_gm_pkt_wrap	*p_gm_pw	= NULL;
        struct gm_port		*p_gm_port	= NULL;
        gm_status_t		 gms		= GM_SUCCESS;
	int err;

        p_gm_pw		= p_pw->drv_priv;
        p_gm_port	= p_gm_pw->p_gm_port;

        if (!p_gm_pw->rq.lock) {
                gm_recv_event_t *p_event   = NULL;
                p_event = gm_receive(p_gm_port);
                gm_unknown(p_gm_port, p_event);

                if (!p_gm_pw->rq.lock) {
                        err = -NM_EAGAIN;
                        goto out;
                }
        }

        gms = gm_deregister_memory(p_gm_port,
                                   p_gm_pw->p_iov->iov_base,
                                   p_gm_pw->p_iov->iov_len);
        if (gms) {
                __error__("memory deregistration failed");
                __gmerror__(gms);
                goto error;
        }

        TBX_FREE(p_gm_pw);
	err = NM_ESUCCESS;

 out:
	return err;

 error:
        TBX_FAILURE("nm_gm_poll_send_iov");
}

static
int
nm_gm_poll_recv_iov    	(struct nm_pkt_wrap *p_pw) {
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_gm_pkt_wrap	*p_gm_pw	= NULL;
        struct gm_port		*p_gm_port	= NULL;
        short int		 remote_node_id = 0;
        gm_recv_event_t		*p_event	= NULL;
        gm_status_t		 gms		= GM_SUCCESS;
	int err;

        p_drv		= p_pw->p_drv;
        p_trk		= p_pw->p_trk;
        p_gm_pw		= p_pw->drv_priv;
        p_gm_port	= p_gm_pw->p_gm_port;

        p_event = gm_receive(p_gm_port);

        switch (gm_ntohc(p_event->recv.type)) {

        case GM_PEER_RECV_EVENT:
        case GM_RECV_EVENT:
                {
                        char	*_pkt	= NULL;

                        _pkt	= gm_ntohp(p_event->recv.buffer);
                        assert(_pkt == p_gm_pw->p_iov->iov_base);
                        remote_node_id =
                                gm_ntohs(p_event->recv.sender_node_id);
                        NM_TRACEF("remote_node_id: %lu", (unsigned long)remote_node_id);
                }
                goto received;

        default:
                gm_unknown(p_gm_port, p_event);
                break;
        }

        err = -NM_EAGAIN;
        goto out;

 received:
        gms = gm_deregister_memory(p_gm_port,
                                   p_gm_pw->p_iov->iov_base,
                                   p_gm_pw->p_iov->iov_len);
        if (gms) {
                __error__("memory deregistration failed");
                __gmerror__(gms);
                goto error;
        }

        if (p_pw->p_gate) {
                struct nm_gate		*p_gate		= NULL;
                struct nm_gm_gate	*p_gm_gate	= NULL;

                p_gate		= p_pw->p_gate;
                p_gm_gate	= p_gate->p_gate_drv_array[p_drv->id]->info;
                assert(remote_node_id
                       == p_gm_gate->cnx_array[p_trk->id].node_id);
        } else {
                struct nm_gm_trk	*p_gm_trk	= NULL;

                /* gate-less request */
                p_gm_trk	= p_pw->p_trk->priv;
                p_pw->p_gate	= p_pw->p_drv->p_core->gate_array
                        + p_gm_trk->gate_map[remote_node_id];
        }

        TBX_FREE(p_gm_pw);
	err = NM_ESUCCESS;

 out:
	return err;

 error:
        TBX_FAILURE("nm_gm_poll_recv_iov");
}


int
nm_gm_load(struct nm_drv_ops *p_ops) {
        p_ops->query		= nm_gm_query        ;
        p_ops->init             = nm_gm_init         ;
        p_ops->exit             = nm_gm_exit         ;

        p_ops->open_trk		= nm_gm_open_trk     ;
        p_ops->close_trk	= nm_gm_close_trk    ;

        p_ops->connect		= nm_gm_connect      ;
        p_ops->accept		= nm_gm_accept       ;
        p_ops->disconnect       = nm_gm_disconnect   ;

        p_ops->post_send_iov	= nm_gm_post_send_iov;
        p_ops->post_recv_iov    = nm_gm_post_recv_iov;

        p_ops->poll_send_iov    = nm_gm_poll_send_iov;
        p_ops->poll_recv_iov    = nm_gm_poll_recv_iov;

        return NM_ESUCCESS;
}

