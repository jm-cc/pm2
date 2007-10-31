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

#include "nm_drv_ops.h"
#include "nm_drv_cap.h"

/** Driver.
 */
struct nm_drv {

        /* NM core object.
         */
        struct nm_core		 *p_core;

        /* Driver id.
         */
        uint8_t			  id;

        /** Driver name.
         */
        char			 *name;

        /** Commands.
         */
        struct nm_drv_ops	  ops;

        /** Capabilities.
         */
        struct nm_drv_cap	  cap;

        /** Number of gates using this driver.
         */
        uint8_t			  nb_gates;

        /** Number of tracks opened on this driver.
         */
        uint8_t			  nb_tracks;

        /** Track array.
         */
        struct nm_trk		**p_track_array;

        /** Url to connect to this driver.
         */
        char			 *url;

        /** Cumulated number of pending out requests on this driver.
         */
        uint8_t			 out_req_nb;

        /** Private data.
         */
        void			 *priv;
};

#endif /* NM_DRV_H */
