
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008 "the PM2 team" (see AUTHORS file)
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
extern marcel_bubble_sched_t marcel_bubble_memory_sched;

#section functions

#ifdef MARCEL_MAMI_ENABLED

#depend "marcel_memory.h[types]"
extern void marcel_bubble_set_memory_manager (marcel_memory_manager_t *);

#endif
