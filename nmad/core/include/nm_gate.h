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


struct nm_gate_trk {
        /* per track gate related data */

        /* track */
        struct nm_trk 		*p_trk;

        /* related gate driver */
        struct nm_gate_drv	*p_gdrv;

        /* preallocated outgoing request */
        struct nm_pkt_wrap	*p_out_rq;

        /* reference to current incoming request, or NULL */
        struct nm_pkt_wrap		*p_in_rq;
};

struct nm_gate_drv {
        /* per driver gate related data */

        /* driver */
        struct nm_drv		 *p_drv;

        /* array of gate track struct */
        struct nm_gate_trk	**p_gate_trk_array;

        void			 *info;

        /* cumulated number of pending out requests on this driver for
           this gate
         */
        uint16_t		   out_req_nb;
};

struct nm_gate {

        /* connexion to another process
         */

        /* nm core object
         */
        struct nm_core		 *p_core;

         /* gate id
         */
        uint8_t			  id;



        /*
         * implementation dependent fields *
         */

        /* scheduler managing this gate */
        struct nm_sched *p_sched;

        /* scheduler private field */
        void *sch_private;



        /*
         * connectivity structures *
         */

        /* gate data for each driver
         */
        struct nm_gate_drv	 *p_gate_drv_array[255];

        /* packet scheduling structures					*/



        /*
         * outgoing *
         */

        /* pre-scheduler outgoing packet wrap
           - list filled from outside the scheduler
	*/
        p_tbx_slist_t pre_sched_out_list;

        /* post-scheduler outgoing packet wrap lists
           - list filled by the implementation-dependent scheduling code
	*/
        p_tbx_slist_t post_sched_out_list;


        /* outgoing active requests
         */
        struct nm_pkt_wrap *out_req_list[256];

        /* number of outgoing active rq */
        uint16_t out_req_nb;



        /*
         * incoming *
         */

        /* nothing for now */
};
