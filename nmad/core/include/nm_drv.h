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

#ifndef NM_DRV_H
#define NM_DRV_H

struct nm_drv {
        /* driver		*/

        /* nm core object
         */
        struct nm_core		 *p_core;

        /* driver id
         */
        uint8_t			  id;

        /* driver name
         */
        char			 *name;

        /* commands
         */
        struct nm_drv_ops	  ops;

        /* capabilities
         */
        struct nm_drv_cap	  cap;

        /* number of gates using this driver
         */
        uint8_t			  nb_gates;

        /* number of tracks opened on this driver
         */
        uint8_t			  nb_tracks;

        /* track array
         */
        struct nm_trk		**p_track_array;

        /* url to connect to this driver
         */
        char			 *url;

        /* cumulated number of pending out requests on this driver
         */
        uint8_t			 out_req_nb;

        /* private data
         */
        void			 *priv;
};

#endif /* NM_DRV_H */
