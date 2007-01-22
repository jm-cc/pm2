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

#ifndef NM_CNX_RQ_H
#define NM_CNX_RQ_H

struct nm_cnx_rq {
        /* request for connecting/disconnecting a gate with a driver */

        struct	nm_gate		*p_gate;
        struct	nm_drv		*p_drv;
        struct	nm_trk		*p_trk;

        /* host info for the queried driver				*/
        char			*remote_host_url;


        /* process info for the queried driver/track			*/

        /* remote driver url						*/
        char			*remote_drv_url;

        /* remote track url						*/
        char			*remote_trk_url;
};

#endif /* NM_CNX_RQ_H */
