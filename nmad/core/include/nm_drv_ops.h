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

struct nm_drv_ops {
        /* driver commands */

        int (*init)		(struct nm_drv 		*p_drv);
        int (*exit)		(struct nm_drv 		*p_drv);

        int (*open_trk)		(struct nm_trk_rq	*p_trk_rq);
        int (*close_trk)	(struct nm_trk 		*p_trk);

        int (*connect)		(struct nm_cnx_rq 	*p_crq);
        int (*accept)		(struct nm_cnx_rq 	*p_crq);
        int (*disconnect)	(struct nm_cnx_rq 	*p_crq);

        int (*post_send_iov)	(struct nm_pkt_wrap 	*p_pw);
        int (*post_recv_iov)	(struct nm_pkt_wrap 	*p_pw);

        int (*poll_send_iov)    (struct nm_pkt_wrap 	*p_pw);
        int (*poll_recv_iov)	(struct nm_pkt_wrap 	*p_pw);

        int (*wait_iov)         (struct nm_pkt_wrap 	*p_pw);
};
