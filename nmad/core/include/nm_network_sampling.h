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

#ifndef NM_NETWORK_SAMPLING_H
#define NM_NETWORK_SAMPLING_H

int
nm_ns_network_sampling(struct nm_drv *driver,
                       nm_gate_id_t gate_id,
                       int connect_flag);

double
nm_ns_evaluate(struct nm_drv *driver,
               int length);

#endif
