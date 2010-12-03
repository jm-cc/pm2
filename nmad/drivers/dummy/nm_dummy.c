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
#include <pm2_common.h>

#include "nm_public.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_drv_cap.h"
#include "nm_trk.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_trk_rq.h"
#include "nm_cnx_rq.h"
#include "nm_errno.h"
#include "nm_log.h"

struct nm_dummy_drv {
        int dummy;
        char*url;
        struct nm_drv_cap caps;
        int nb_gates;
};

struct nm_dummy_trk {
        int dummy;
};

struct nm_dummy_cnx {
        int dummy;
};

struct nm_dummy {
        int dummy;
};

struct nm_dummy_pkt_wrap {
        int dummy;
};

/* Components ****************************** */
/* In order to make driver available,
   make sur you added the corresponding 
   part in .../nmad/core/src/nm_core.c; 
   you have to call the load function, 
   and to define the corresponding 
   default adapter as XML text.
   Examples are availables in this file. */

/** Dummy NewMad Driver */
static int nm_dummy_query(struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
static int nm_dummy_init(struct nm_drv *p_drv);
static int nm_dummy_exit(struct nm_drv *p_drv);
static int nm_dummy_open_track(struct nm_trk_rq*p_trk_rq);
static int nm_dummy_close_track(struct nm_trk*p_trk);
static int nm_dummy_connect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_dummy_accept(void*_status, struct nm_cnx_rq *p_crq);
static int nm_dummy_disconnect(void*_status, struct nm_cnx_rq *p_crq);
static int nm_dummy_post_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_dummy_post_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_dummy_poll_send_iov(void*_status, struct nm_pkt_wrap *p_pw);
static int nm_dummy_poll_recv_iov(void*_status, struct nm_pkt_wrap *p_pw);
static struct nm_drv_cap*nm_dummy_get_capabilities(struct nm_drv *p_drv);
static const char*nm_dummy_get_driver_url(struct nm_drv *p_drv);

static const struct nm_drv_iface_s nm_dummy_driver =
  {
    .query              = &nm_dummy_query,
    .init               = &nm_dummy_init,

    .open_trk		= &nm_dummy_open_track,
    .close_trk	        = &nm_dummy_close_track,
    
    .connect		= &nm_dummy_connect,
    .accept		= &nm_dummy_accept,
    .disconnect         = &nm_dummy_disconnect,
    
    .post_send_iov	= &nm_dummy_post_send_iov,
    .post_recv_iov      = &nm_dummy_post_recv_iov,
    
    .poll_send_iov      = &nm_dummy_poll_send_iov,
    .poll_recv_iov      = &nm_dummy_poll_recv_iov,

    .get_driver_url     = &nm_dummy_get_driver_url,
    .get_capabilities   = &nm_dummy_get_capabilities

  };

/** 'PadicoAdapter' facet for Dummy driver */
static void*nm_dummy_instanciate(puk_instance_t, puk_context_t);
static void nm_dummy_destroy(void*);

static const struct puk_adapter_driver_s nm_dummy_adapter_driver =
  {
    .instanciate = &nm_dummy_instanciate,
    .destroy     = &nm_dummy_destroy
  };

/** Component declaration */
static int nm_dummy_load(void)
{
  puk_component_declare("NewMad_Driver_dummy",
			puk_component_provides("PadicoAdapter", "adapter", &nm_dummy_adapter_driver),
			puk_component_provides("NewMad_Driver", "driver", &nm_dummy_driver));

  return 0;
}
PADICO_MODULE_BUILTIN(NewMad_Driver_dummy, &nm_dummy_load, NULL, NULL);



/** Return capabilities */
static struct nm_drv_cap*nm_dummy_get_capabilities(struct nm_drv *p_drv)
{
  struct nm_dummy_drv *p_dummy_drv = p_drv->priv;
  return &p_dummy_drv->caps;
}

/** Instanciate functions */
static void*nm_dummy_instanciate(puk_instance_t instance, puk_context_t context){
  struct nm_dummy*status = TBX_MALLOC(sizeof (struct nm_dummy));
  memset(status, 0, sizeof(struct nm_dummy));
  return status;
}

static void nm_dummy_destroy(void*_status){
  TBX_FREE(_status);
}

const static char*nm_dummy_get_driver_url(struct nm_drv *p_drv){
  struct nm_dummy_drv *p_dummy_drv = p_drv->priv;
  return p_dummy_drv->url;
}

/* ***************************************** */


static
int
nm_dummy_query			(struct nm_drv *p_drv,
				 struct nm_driver_query_param *params,
				 int nparam) {
	int err;
	struct nm_dummy_drv*p_dummy_drv = NULL;

	/* private data							*/
	p_dummy_drv	= TBX_MALLOC(sizeof (struct nm_dummy_drv));
	if (!p_dummy_drv) {
		err = -NM_ENOMEM;
		goto out;
	}

        memset(p_dummy_drv, 0, sizeof (struct nm_dummy_drv));

        /* driver capabilities encoding					*/
        p_dummy_drv->caps.has_trk_rq_dgram			= 1;
        p_dummy_drv->caps.has_selective_receive		        = 0;
        p_dummy_drv->caps.has_concurrent_selective_receive	= 0;
#ifdef PM2_NUIOA
	p_dummy_drv->caps.numa_node = PM2_NUIOA_ANY_NODE;
#endif
	p_dummy_drv->caps.latency = INT_MAX;
	p_dummy_drv->caps.bandwidth = 0;
	p_dummy_drv->caps.min_period = 0;

	p_drv->priv = p_dummy_drv;
	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_dummy_init			(struct nm_drv *p_drv) {
        struct nm_dummy_drv *p_dummy_drv = p_drv->priv;
	int err = NM_ESUCCESS;

	/* driver url encoding						*/
	p_dummy_drv->url	= tbx_strdup("-");

	p_dummy_drv->nb_gates++;
	return err;
}

static
int
nm_dummy_exit			(struct nm_drv *p_drv) {
	int err;
        struct nm_dummy_drv *p_dummy_drv = p_drv->priv;

	p_dummy_drv->nb_gates --;
        TBX_FREE(p_dummy_drv->url);
        TBX_FREE(p_dummy_drv);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_open_track		(struct nm_trk_rq	*p_trk_rq) {
        struct nm_trk		*p_trk		= NULL;
        struct nm_dummy_trk	*p_dummy_trk	= NULL;
	int err;

        p_trk	= p_trk_rq->p_trk;

        /* private data							*/
	p_dummy_trk	= TBX_MALLOC(sizeof (struct nm_dummy_trk));
        if (!p_dummy_trk) {
                err = -NM_ENOMEM;
                goto out;
        }
        memset(p_dummy_trk, 0, sizeof (struct nm_dummy_trk));
        p_trk->priv	= p_dummy_trk;

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
nm_dummy_close_track		(struct nm_trk *p_trk) {
        struct nm_dummy_trk	*p_dummy_trk	= NULL;
	int err;

        p_dummy_trk	= p_trk->priv;
        TBX_FREE(p_trk->url);
        TBX_FREE(p_dummy_trk);

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_connect		(void*_status,
				 struct nm_cnx_rq *p_crq) {
        struct nm_dummy	*status	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_dummy_trk	*p_dummy_trk	= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_dummy_trk	= p_trk->priv;

        /* private data				*/
        status	= _status;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_accept			(void*_status,
				 struct nm_cnx_rq *p_crq) {
        struct nm_dummy	*status	= NULL;
        struct nm_gate		*p_gate		= NULL;
        struct nm_drv		*p_drv		= NULL;
        struct nm_trk		*p_trk		= NULL;
        struct nm_dummy_trk	*p_dummy_trk	= NULL;
	int err;

        p_gate		= p_crq->p_gate;
        p_drv		= p_crq->p_drv;
        p_trk		= p_crq->p_trk;
        p_dummy_trk	= p_trk->priv;

        /* private data				*/
        status	= _status;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_disconnect		(void*_status,
				 struct nm_cnx_rq *p_crq) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_post_send_iov		(void*_status,
				 struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_post_recv_iov		(void*_status,
				 struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_poll_send_iov    	(void*_status,
				 struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}

static
int
nm_dummy_poll_recv_iov    	(void*_status,
				 struct nm_pkt_wrap *p_pw) {
	int err;

	err = NM_ESUCCESS;

	return err;
}
