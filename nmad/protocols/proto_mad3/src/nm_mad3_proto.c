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
#include "nm_mad3_private.h"

#include <nm_basic_public.h>

/* Macros
 */
#define INITIAL_RQ_NUM		4

/* fast allocator structs */
static p_tbx_memory_t		nm_mad3_rq_mem	= NULL;

/* NOT YET USED
 */
#if 0
/* main madeleine object
 */
static p_mad_madeleine_t	p_madeleine	= NULL;
#endif

/* successful outgoing request
 */
int
nm_mad3_out_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= NM_ESUCCESS;
	err = NM_ESUCCESS;

	return err;
}

/* failed outgoing request
 */
int
nm_mad3_out_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= _err;
	err = NM_ESUCCESS;

	return err;
}

/* successful incoming request
 */
int
nm_mad3_in_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= NM_ESUCCESS;
	err = NM_ESUCCESS;

	return err;
}

/* failed incoming request
 */
int
nm_mad3_in_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= _err;
	err = NM_ESUCCESS;

	return err;
}

/* protocol initialisation
 */
int
nm_mad3_proto_init	(struct nm_proto 	* const p_proto) {
        struct nm_core	 *p_core	= NULL;
        struct nm_mad3	 *p_mad3	= NULL;
        int i;
	int err;

        p_core	= p_proto->p_core;

        /* check if protocol may be registered
         */
        for (i = 0; i < 127; i++) {
                if (p_core->p_proto_array[128+i]) {
                        NM_DISPF("nm_mad3_proto_init: protocol entry %d already used",
                             128+i);

                        err	= -NM_EALREADY;
                        goto out;
                }
        }

        /* allocate private protocol part
         */
        p_mad3	= TBX_MALLOC(sizeof(struct nm_mad3));
        if (!p_mad3) {
                err = -NM_ENOMEM;
                goto out;
        }

        /* TODO: allocate the madeleine object wrapper
         */
#warning TODO
        p_mad3->p_madeleine	= NULL;

        /* setup allocator for request structs
         */
        tbx_malloc_init(&nm_mad3_rq_mem,   sizeof(struct nm_mad3_rq),
                        INITIAL_RQ_NUM,   "nmad/mad3/rq");

        /* register protocol
         */
        p_proto->priv	= p_mad3;
        for (i = 0; i < 127; i++) {
                p_core->p_proto_array[128+i] = p_proto;
        }

	err = NM_ESUCCESS;

 out:
	return err;
}

/* ---
 */

int
nm_mad3_load		(struct nm_proto_ops	*p_ops) {
        p_ops->init		= nm_mad3_proto_init;

        p_ops->out_success	= nm_mad3_out_success;
        p_ops->out_failed	= nm_mad3_out_failed;

        p_ops->in_success	= nm_mad3_in_success;
        p_ops->in_failed	= nm_mad3_in_failed;

        return NM_ESUCCESS;
}
