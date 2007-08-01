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
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>
#include "nm_so_basic_private.h"
#include "nm_so_pkt_wrap.h"

/* Macros
 */
#define INITIAL_BRQ_NUM		4

/* fast allocator structs */
static p_tbx_memory_t nm_so_basic_rq_mem	= NULL;

int
nm_so_basic_isend(struct nm_proto		 *p_proto,
                  gate_id_t			  gate_id,
                  uint8_t			  tag_id,
                  void			 *ptr,
                  uint64_t			  len,
                  struct nm_so_basic_rq	**pp_so_rq) {

    struct nm_core		*p_core	= NULL;
    struct nm_gate              *p_gate = NULL;
    struct nm_so_basic_rq	*p_so_rq	= NULL;
    struct nm_so_pkt_wrap	*p_so_pw	= NULL;
    struct nm_pkt_wrap	*p_pw	= NULL;
    int err;

    if (tag_id > 126) {
        err = -NM_EINVAL;
        goto out;
    }

    p_core	= p_proto->p_core;
    p_gate	= p_core->gate_array + gate_id;
    if (!p_gate){
        err	= -NM_EINVAL;
        goto out;
    }

    // construction de so_wrap
    err = nm_so_pw_alloc(p_core, p_gate, 128+tag_id, 0,&p_so_pw);
    err = nm_so_pw_add_buf(p_core, p_so_pw, ptr, len);



    // construction de la requete du protocole
    p_so_rq	= tbx_malloc(nm_so_basic_rq_mem);
    if (!p_so_rq) {
        err = -NM_ENOMEM;
        goto out;
    }
    p_so_rq->p_so_pw		= p_so_pw;
    p_so_rq->lock		=    1;
    // stockage de la requete du protocole
    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    p_pw->proto_priv	= p_so_rq;


    err	= __nm_core_post_send(p_core, p_pw);
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_core_post_send returned err = %d", err);
        goto out;
    }

    *pp_so_rq			= p_so_rq;

    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_basic_irecv(struct nm_proto		 *p_proto,
                  gate_id_t			  gate_id,
                  uint8_t			  tag_id,
                  void			         *ptr,
                  uint64_t			  len,
                  struct nm_so_basic_rq	**pp_so_rq) {

    struct nm_core		*p_core	= NULL;
    struct nm_gate              *p_gate = NULL;
    struct nm_so_basic_rq	*p_so_rq	= NULL;
    struct nm_so_pkt_wrap	*p_so_pw	= NULL;
    struct nm_pkt_wrap	*p_pw	= NULL;
    int err;

    if (tag_id > 126) {
        err = -NM_EINVAL;
        goto out;
    }

    p_core	= p_proto->p_core;
    p_gate	= p_core->gate_array + gate_id;
    if (!p_gate){
        err	= -NM_EINVAL;
        goto out;
    }


    // construction de so_wrap
    err = nm_so_pw_alloc(p_core, p_gate, 128+tag_id, 0,&p_so_pw);
    err = nm_so_pw_add_buf(p_core, p_so_pw, ptr, len);


    // construction de la requete du protocole
    p_so_rq	= tbx_malloc(nm_so_basic_rq_mem);
    if (!p_so_rq) {
        err = -NM_ENOMEM;
        goto out;
    }
    p_so_rq->p_so_pw		= p_so_pw;
    p_so_rq->lock		=    1;
    // stockage de la requete du protocole
    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    p_pw->proto_priv	= p_so_rq;

    err	= __nm_core_post_recv(p_core, p_pw);
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_core_post_recv returned err = %d", err);
        goto out;
    }

    *pp_so_rq			= p_so_rq;

    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_basic_irecv_any(struct nm_proto	 *p_proto,
                      uint8_t		  tag_id,
                      void			 *ptr,
                      uint64_t		  len,
                      struct nm_so_basic_rq	**pp_so_rq) {
    TBX_FAILURE("nm_so_basic_irecv_any pas implémenté");
    return 0;

    //struct nm_core		*p_core	= NULL;
    //struct nm_basic_rq	*p_rq	= NULL;
    //struct nm_pkt_wrap	*p_pw	= NULL;
    //int err;
    //
    //if (tag_id > 126) {
    //    err = -NM_EINVAL;
    //    goto out;
    //}
    //
    //p_core	= p_proto->p_core;
    //err	= __nm_core_wrap_buffer(p_core,
    //                            -1, /* gateless request */
    //                            128 + tag_id,
    //                            0,
    //                            ptr,
    //                            len,
    //                            &p_pw);
    //if (err != NM_ESUCCESS) {
    //    NM_DISPF("nm_core_wrap_buf returned err = %d", err);
    //    goto out;
    //}
    //
    //p_rq	= tbx_malloc(nm_basic_rq_mem);
    //if (!p_rq) {
    //    err = -NM_ENOMEM;
    //    goto out;
    //}
    //
    //p_rq->p_pw		= p_pw;
    //p_rq->lock		=    1;
    //
    //p_pw->proto_priv	= p_rq;
    //*pp_rq			= p_rq;
    //
    //err	= __nm_core_post_recv(p_core, p_pw);
    //if (err != NM_ESUCCESS) {
    //    NM_DISPF("nm_core_post_recv returned err = %d", err);
    //    goto out;
    //}
    //
    //err = NM_ESUCCESS;
    //
    //out:
    //return err;
     }

/* TODO: add a means for the application to retrieve the actual gate
   id for completed gateless requests
*/
#warning TODO

int
nm_so_basic_wait(struct nm_so_basic_rq	 *p_so_rq) {
    struct nm_pkt_wrap *p_pw = NULL;
    struct nm_core *p_core = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_rq->p_so_pw, &p_pw);
    p_core = p_pw->p_proto->p_core;

    while (p_so_rq->lock) {
        err = nm_schedule(p_core);
        if (err != NM_ESUCCESS) {
            NM_DISPF("nm_schedule returned err = %d", err);
            goto out;
        }
    }

    tbx_free(nm_so_basic_rq_mem, p_so_rq);
    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_basic_test(struct nm_so_basic_rq	 *p_so_rq) {
    struct nm_pkt_wrap *p_pw = NULL;
    struct nm_core *p_core = NULL;
    int err;

    if (!p_so_rq->lock)
        goto complete;

    err = nm_so_pw_get_pw(p_so_rq->p_so_pw, &p_pw);
    p_core = p_pw->p_proto->p_core;

    err = nm_schedule(p_core);
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_schedule returned err = %d", err);
        goto out;
    }

    if (!p_so_rq->lock)
        goto complete;

    err	= -NM_EAGAIN;
    goto out;

 complete:
    tbx_free(nm_so_basic_rq_mem, p_so_rq);
    err = NM_ESUCCESS;

 out:
    return err;
}

/* successful outgoing request
 */
int
nm_so_basic_out_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
    struct nm_so_basic_rq	 *p_so_rq	= NULL;
    int err;

    p_so_rq	= p_pw->proto_priv;
    p_so_rq->lock	= 0;
    p_so_rq->status	= NM_ESUCCESS;
    err = NM_ESUCCESS;

    return err;
}

/* failed outgoing request
 */
int
nm_so_basic_out_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
    struct nm_so_basic_rq	 *p_so_rq	= NULL;
    int err;

    p_so_rq	= p_pw->proto_priv;
    p_so_rq->lock	= 0;
    p_so_rq->status	= _err;
    err = NM_ESUCCESS;

    return err;
}

/* successful incoming request
 */
int
nm_so_basic_in_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
    struct nm_so_basic_rq	 *p_so_rq	= NULL;
    int err;

    p_so_rq	= p_pw->proto_priv;
    p_so_rq->lock	= 0;
    p_so_rq->status	= NM_ESUCCESS;
    err = NM_ESUCCESS;

    return err;
}

/* failed incoming request
 */
int
nm_so_basic_in_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
    struct nm_so_basic_rq	 *p_so_rq	= NULL;
    int err;

    p_so_rq	= p_pw->proto_priv;
    p_so_rq->lock	= 0;
    p_so_rq->status	= _err;
    err = NM_ESUCCESS;

    return err;
}

/* protocol initialisation
 */
int
nm_so_basic_proto_init	(struct nm_proto 	* const p_proto) {
    struct nm_core	 *p_core	= NULL;
    struct nm_so_basic	 *p_so_basic	= NULL;
    int i;
    int err;

    p_core	= p_proto->p_core;

    p_so_basic	= TBX_MALLOC(sizeof(struct nm_so_basic));
    if (!p_so_basic) {
        err = -NM_ENOMEM;
        goto out;
    }

    p_proto->priv	= p_so_basic;

    for (i = 0; i < 127; i++) {
        if (p_core->p_proto_array[128+i]) {
            NM_DISPF("nm_basic_proto_init: protocol entry %d already used",
                     128+i);

            err	= -NM_EALREADY;
            goto out;
        }
    }

    for (i = 0; i < 127; i++) {
        p_core->p_proto_array[128+i] = p_proto;
    }

    tbx_malloc_init(&nm_so_basic_rq_mem,
                    sizeof(struct nm_so_basic_rq),
                    INITIAL_BRQ_NUM,   "nmad/so_basic/rq");

    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_basic_load		(struct nm_proto_ops	*p_ops) {
    p_ops->init		= nm_so_basic_proto_init;

    p_ops->out_success	= nm_so_basic_out_success;
    p_ops->out_failed	= nm_so_basic_out_failed;

    p_ops->in_success	= nm_so_basic_in_success;
    p_ops->in_failed	= nm_so_basic_in_failed;

    return NM_ESUCCESS;
}
