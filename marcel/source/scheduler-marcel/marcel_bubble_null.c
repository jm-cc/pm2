/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2009 "the PM2 team" (see AUTHORS file)
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

struct marcel_bubble_null_sched {
	marcel_bubble_sched_t scheduler;
};

/* The canonical instance of the `null' scheduler.  */
marcel_bubble_sched_t marcel_bubble_null_sched = {
	.klass = &marcel_bubble_null_sched_class
};

static int
make_default_scheduler (marcel_bubble_sched_t *scheduler) {
	/* This scheduler class doesn't define any method.  */
  scheduler->klass = &marcel_bubble_null_sched_class;
	return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS (null, make_default_scheduler);

#endif /* MA__BUBBLES */
