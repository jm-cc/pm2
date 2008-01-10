
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#section variables
#depend "marcel_bubble_sched_interface.h[types]"
extern marcel_bubble_sched_t marcel_bubble_affinity_sched;
extern marcel_bubble_t *registered_bubble;

#section functions
void marcel_bubble_affinity(marcel_bubble_t *b, struct marcel_topo_level *l);

