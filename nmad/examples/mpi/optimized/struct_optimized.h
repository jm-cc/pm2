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

#ifndef STRUCT_OPTIMIZED_H
#define STRUCT_OPTIMIZED_H

#include "ping_optimized.h"

typedef struct {
  float x, y;
  int c;
  float z;
} particle_t;

void pingpong_datatype_struct(nm_so_pack_interface interface,
                              nm_gate_id_t         gate_id,
                              int                  number_of_elements,
                              int                  client);

void init_datatype_struct(struct MPIR_DATATYPE *datatype,
                          int                   number_of_elements);

void pack_datatype_struct(nm_so_pack_interface  interface,
                          nm_gate_id_t          gate_id,
                          struct MPIR_DATATYPE *datatype,
                          particle_t           *s_ptr);

void unpack_datatype_struct(nm_so_pack_interface interface,
                            nm_gate_id_t         gate_id,
                            particle_t          *r_ptr);

#endif /* STRUCT_OPTIMIZED_H */
