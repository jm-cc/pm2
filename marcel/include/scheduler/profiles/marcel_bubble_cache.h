/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_BUBBLE_CACHE_H__
#define __MARCEL_BUBBLE_CACHE_H__


#include "scheduler/marcel_bubble_sched.h"


MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(cache);

/** \brief The class of a `cache' bubble scheduler.  */
extern marcel_bubble_sched_class_t marcel_bubble_cache_sched_class;

/** \brief Initialize \e scheduler, a `cache' bubble scheduler. \param
 * root_level represents the root level of the topology tree this
 * scheduler is instantiated on. If \param work_stealing is true, then
 * work stealing is enabled, otherwise it is disabled.  */
extern int marcel_bubble_cache_sched_init(struct marcel_bubble_cache_sched *scheduler,
					  struct marcel_topo_level *root_level,
					  tbx_bool_t work_stealing);


#endif /** __MARCEL_BUBBLE_CACHE_H__ **/
