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

struct nm_cnx_rq;
struct nm_drv;
struct nm_pkt_wrap;
struct nm_trk;
struct nm_trk_rq;
struct nm_driver_query_param;

/** Driver commands. */
struct nm_drv_ops {

        /** Query and register resources for a driver.
         */
        int (*query)		(struct nm_drv 		*p_drv,
				 struct nm_driver_query_param *params,
				 int nparam);

        /** Initialize the driver using previously registered resources.
         */
        int (*init)		(struct nm_drv 		*p_drv);

        /** Shutdown the driver and release resources.
         */
        int (*exit)		(struct nm_drv 		*p_drv);

        /** Open a new track.
         */
        int (*open_trk)		(struct nm_trk_rq	*p_trk_rq);

        /** Close a track.
         */
        int (*close_trk)	(struct nm_trk 		*p_trk);

        /** Connect a gate to a remote process.
         */
        int (*connect)		(struct nm_cnx_rq 	*p_crq);

        /** Accept an incoming connection from a remote process.
         */
        int (*accept)		(struct nm_cnx_rq 	*p_crq);

        /** Close a connection.
         */
        int (*disconnect)	(struct nm_cnx_rq 	*p_crq);

        /** Post a new send request.
         */
        int (*post_send_iov)	(struct nm_pkt_wrap 	*p_pw);

        /** Post a new receive request.
         */
        int (*post_recv_iov)	(struct nm_pkt_wrap 	*p_pw);

        /** Poll a active send request.
         */
        int (*poll_send_iov)    (struct nm_pkt_wrap 	*p_pw);

        /** Poll an active receive request.
         */
        int (*poll_recv_iov)	(struct nm_pkt_wrap 	*p_pw);

        int (*wait_iov)         (struct nm_pkt_wrap 	*p_pw);
};
