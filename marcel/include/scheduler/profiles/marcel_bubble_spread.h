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


#ifndef __MARCEL_BUBBLE_SPREAD_H__
#define __MARCEL_BUBBLE_SPREAD_H__


#include "scheduler/marcel_bubble_sched.h"


/** Public global variables **/
MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(spread);


/** Public functions **/
extern void marcel_bubble_spread(marcel_bubble_t * b, struct marcel_topo_level *l);


#endif /** __MARCEL_BUBBLE_SPREAD_H__ **/
