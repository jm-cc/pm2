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


/** Request for connecting/disconnecting a gate with a driver. */
struct nm_cnx_rq {

        struct	nm_gate		*p_gate;
        struct	nm_drv		*p_drv;
        struct	nm_trk		*p_trk;

        /** Host info for the queried driver.
         */
        char			*remote_host_url;


        /** Remote driver url.
         */
        char			*remote_drv_url;

        /** Remote track url.
         */
        char			*remote_trk_url;
};
