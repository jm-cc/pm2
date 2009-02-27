
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

/** \file
 * \brief Bubble steal scheduler functions
 * \defgroup marcel_bubble_steal Bubble Steal scheduler
 * @{
 */

#section marcel_functions
/** \brief Steal work
 *
 * This function may typically be called when a processor becomes idle. It will
 * look for bubbles to steal first locally (neighbour processors), then more
 * and more globally.
 *
 * \return 1 if it managed to steal work, 0 else.
 */
int marcel_bubble_steal_work(marcel_bubble_sched_t *self, unsigned vp);
/* @} */

#section variables
#depend "marcel_bubble_sched_interface.h[types]"
extern marcel_bubble_sched_t marcel_bubble_steal_sched;
