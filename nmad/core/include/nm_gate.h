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

#ifndef NM_GATE_H
#define NM_GATE_H

#include "nm_public.h"

struct nm_drv;
struct nm_trk;
struct nm_core;
struct nm_sched;
struct nm_gate_drv;
struct nm_pkt_wrap;
struct nm_gate_trk;

/** Per track gate related data.
 */
struct nm_gate_trk {

        /** Track.
         */
        struct nm_trk 		*p_trk;

        /** Related gate driver.
         */
        struct nm_gate_drv	*p_gdrv;

        /** Preallocated outgoing request.
         */
        struct nm_pkt_wrap	*p_out_rq;

        /** Reference to current incoming request, or NULL.
         */
        struct nm_pkt_wrap		*p_in_rq;
};

/** Per driver gate related data. */
struct nm_gate_drv {

        /** Driver.
         */
        struct nm_drv		 *p_drv;

        /** Array of gate track struct.
         */
        struct nm_gate_trk	**p_gate_trk_array;

        void			 *info;

        /** Cumulated number of pending out requests on this driver for
           this gate.
         */
        uint16_t		   out_req_nb;
};

/** Connexion to another process.
 */
struct nm_gate {

        /** NM core object.
         */
        struct nm_core		 *p_core;

         /** Gate id.
          */
        uint8_t			  id;



        /*
         * implementation dependent fields *
         */

        /** Scheduler managing this gate.
         */
        struct nm_sched *p_sched;

        /** Scheduler private field.
         */
        void *sch_private;



        /*
         * connectivity structures *
         */

        /** Gate data for each driver.

         */
        struct nm_gate_drv	 *p_gate_drv_array[NUMBER_OF_GATES];

        /* packet scheduling structures					*/



        /*
         * outgoing *
         */

        /** Pre-scheduler outgoing packet wrap.
           - list filled from outside the scheduler
	*/
        p_tbx_slist_t pre_sched_out_list;

        /** Post-scheduler outgoing packet wrap lists.
           - list filled by the implementation-dependent scheduling code
	*/
        p_tbx_slist_t post_sched_out_list;


        /** Outgoing active requests.
         */
        struct nm_pkt_wrap *out_req_list[256];

        /** number of outgoing active rq */
        uint16_t out_req_nb;



        /*
         * incoming *
         */

        /* nothing for now */
};

#endif /* NM_GATE_H */
