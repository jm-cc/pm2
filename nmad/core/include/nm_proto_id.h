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

#ifndef NM_PROTO_ID_H
#define NM_PROTO_ID_H

/** Protocol IDs definition
 */
enum nm_proto_id {

        nm_pi_wildcard		=   0,	/* any protocol, cannot be used
                                           in send requests */


        /* Scheduler protocol flows  1-16				*/
        nm_pi_sched		=   1,	/* scheduler control pkt	*/


        /* General purpose protocol flows  17-127			*/

        /* RDV packets							*/
        nm_pi_rdv_req		=  17,	/* rdv request			*/
        nm_pi_rdv_ack		=  18,	/* rdv ack			*/


        /* User channels 128-255					*/
        nm_pi_user_channel	= 128,	/* first user channel		*/
};


#endif /* NM_PROTO_ID_H */
