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

/** Request for opening a new track on a driver.
 */
struct nm_trk_rq {

        /** Track (and indirectly driver).
         */
        struct nm_trk		*p_trk;

        /** Requested capabilities.
         */
        struct nm_trk_cap	 cap;

        /** Flags.
           - unspecified for now
         */
        uint32_t		 flags;
};
