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

#ifndef NM_SCHED_H
#define NM_SCHED_H

#include "nm_sched_ops.h"

struct nm_core;

/** Packet scheduler.
 */
struct nm_sched {

        /** nm_core object. */
        struct	nm_core	*p_core;

        /** Implementation-dependent scheduler operations. */
        struct nm_sched_ops	ops;



        /*
         * Implementation dependent fields. *
         */

        /** Scheduler private field. */
        void *sch_private;



        /*
         * Outgoing *
         */

        /* nothing for now... */


        /*
         * Incoming *
         */

        /* Request lists						*/

        /** Aux requests pending. */
        p_tbx_slist_t	 pending_aux_recv_req;

        /** Aux requests to post. */
        p_tbx_slist_t	 post_aux_recv_req;

        /** Aux requests submitted to scheduler from external code. */
        p_tbx_slist_t	 submit_aux_recv_req;


        /** Pending permissive input request list.
         */
        p_tbx_slist_t	 pending_perm_recv_req;

        /** Permissive requests to post. */
        p_tbx_slist_t	 post_perm_recv_req;

        /** Permissive requests submitted to scheduler from external code. */
        p_tbx_slist_t	 submit_perm_recv_req;


};

#endif /* NM_SCHED_H */
