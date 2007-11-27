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

#ifndef NM_SO_PROCESS_UNEXPECTED_H
#define NM_SO_PROCESS_UNEXPECTED_H

#include <tbx.h>
struct gate;

int treat_unexpected(tbx_bool_t is_any_src, struct nm_gate *p_gate,
                     uint8_t tag, uint8_t seq, uint32_t len, void *data);

#endif
