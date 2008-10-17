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

#ifndef INDEXED_OPTIMIZED_H
#define INDEXED_OPTIMIZED_H

#include "ping_optimized.h"

void pingpong_datatype_indexed(nm_core_t            p_core,
                               nm_gate_id_t         gate_id,
                               int                  number_of_elements,
                               int                  number_of_blocks,
                               int                  client);

void init_datatype_indexed(struct MPIR_DATATYPE *datatype,
                           int                   number_of_elements,
                           int                   number_of_blocks);

void pack_datatype_indexed(nm_core_t             p_core,
                           nm_gate_id_t          gate_id,
                           struct MPIR_DATATYPE *datatype,
                           float                *s_ptr);

void unpack_datatype_indexed(nm_core_t            p_core,
                             nm_gate_id_t         gate_id,
                             float               *r_ptr);

#endif /* INDEXED_OPTIMIZED_H */
