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

#ifndef NM_SO_STRATEGIES_H
#define NM_SO_STRATEGIES_H

struct nm_pkt_wrap *
nm_so_strategy_application(struct nm_gate *p_gate,
                           struct nm_drv *driver,
                           p_tbx_slist_t pre_list);

#endif
