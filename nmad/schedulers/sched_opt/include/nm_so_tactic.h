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

#ifndef NM_SO_TACTICS_H
#define NM_SO_TACTICS_H

#include "nm_so_strategies_private.h"

typedef int nm_so_tactic (struct nm_drv *driver,
                          struct nm_so_se_pending_tab *pw_tab,
                          struct nm_so_se_op_stack *op_stack,
                          int tree_altitude);
#endif
