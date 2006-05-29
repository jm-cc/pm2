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


struct nm_core;
struct nm_pkt_wrap;

struct nm_core_ops {
        int
        (*trk_alloc)		(struct nm_core		 * p_core,
                                 struct nm_drv		 * p_drv,
                                 struct nm_trk		**pp_trk);

        int
        (*trk_free)		(struct nm_core		*p_core,
                                 struct nm_trk		*p_trk);

};
