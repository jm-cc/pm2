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


struct nm_trk {
        /* track				*/

        /* track id
         */
        uint8_t			 id;

        /* driver
         */
        struct nm_drv		*p_drv;

        /* track capabilities
         */
        struct nm_trk_cap	 cap;

        /* url to connect to this track
         */
        char			*url;

        /* number of pending out requests on this track
         */
        uint16_t 		 out_req_nb;

        /* private data
         */
        void			*priv;
};
