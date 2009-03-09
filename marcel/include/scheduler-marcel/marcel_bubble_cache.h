
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006, 2008, 2009 "the PM2 team" (see AUTHORS file)
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

MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS (cache);

/** \brief The class of a `cache' bubble scheduler.  */
extern marcel_bubble_sched_class_t marcel_bubble_cache_sched_class;

/** \brief Initialize \e scheduler, a `cache' bubble scheduler.  If \param
 * work_stealing is true, then work stealing is enabled, otherwise it is
 * disabled.  */
extern int
marcel_bubble_cache_sched_init (struct marcel_bubble_cache_sched *scheduler,
				tbx_bool_t work_stealing);


