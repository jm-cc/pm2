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

#ifndef NM_SO_INTERFACE_OPS_H
#define NM_SO_INTERFACE_OPS_H

struct nm_core;
struct nm_gate;

/* Initialization */
typedef int (*nm_so_interface_init_gate)(struct nm_gate *p_gate);

/* Handle the success of an operation. */
typedef int (*nm_so_interface_pack_success)(struct nm_gate *p_gate,
					    uint8_t tag, uint8_t seq);
typedef int (*nm_so_interface_unpack_success)(struct nm_gate *p_gate,
					      uint8_t tag, uint8_t seq);

typedef int (*nm_so_interface_any_source)(struct nm_gate *p_gate, uint8_t tag);

typedef int (*nm_so_interface_unexpected)(struct nm_gate *p_gate,
                                           uint8_t tag, uint8_t seq);

struct nm_so_interface_ops {
  nm_so_interface_init_gate init_gate;
  nm_so_interface_pack_success pack_success;
  nm_so_interface_unpack_success unpack_success;
  nm_so_interface_unexpected unexpected;
};

#endif
