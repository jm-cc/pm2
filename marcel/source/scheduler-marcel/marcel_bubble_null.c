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

#include "marcel.h"

#ifdef MA__BUBBLES

#if 0
static int
null_sched_vp_is_idle(unsigned vp)
{
#ifdef MARCEL_BUBBLE_STEAL
  /* TODO: avoir un scheduler steal plutôt ? */
  return marcel_bubble_steal_work(vp);
#else
  return 0;
#endif
}
#endif

MARCEL_DEFINE_BUBBLE_SCHEDULER (null,
//	.vp_is_idle = null_sched_vp_is_idle,
);

#endif /* MA__BUBBLES */
