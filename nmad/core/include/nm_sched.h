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
#include "tbx_pointers.h"

struct nm_sched {
        /* packet scheduler						*/

        /* nm_core object */
        struct	nm_core	*p_core;

        /* implementation-dependent scheduler operations */
        struct nm_sched_ops	ops;



        /*
         * implementation dependent fields *
         */

        /* scheduler private field */
        void *sch_private;



        /*
         * outgoing *
         */

        /* nothing for now... */


        /*
         * incoming *
         */

        /* request lists						*/

        /* aux requests pending */
        p_tbx_slist_t	 pending_aux_recv_req;

        /* aux requests to post */
        p_tbx_slist_t	 post_aux_recv_req;

        /* aux requests submitted to scheduler from external code */
        p_tbx_slist_t	 submit_aux_recv_req;


        /* pending permissive input request list
         */
        p_tbx_slist_t	 pending_perm_recv_req;

        /* permissive requests to post */
        p_tbx_slist_t	 post_perm_recv_req;

        /* permissive requests submitted to scheduler from external code */
        p_tbx_slist_t	 submit_perm_recv_req;


};
