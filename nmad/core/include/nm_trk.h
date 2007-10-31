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

#ifndef NM_TRK_H
#define NM_TRK_H

#include "nm_trk_cap.h"

/** Track.
 */
struct nm_trk {

        /** Id.
         */
        uint8_t			 id;

        /** Driver.
         */
        struct nm_drv		*p_drv;

        /** Capabilities.
         */
        struct nm_trk_cap	 cap;

        /** URL to connect to this track.
         */
        char			*url;

        /** Number of pending out requests on this track.
         */
        uint16_t 		 out_req_nb;

        /** Private data.
         */
        void			*priv;
};

#endif /* NM_TRK_H */
