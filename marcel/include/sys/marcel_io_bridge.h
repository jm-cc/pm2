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

/**
 * Defines a callback interface that allow internal/external
 * libraries to be scheduled during particular scheduling points
 * 
 * For example, Marcel Poll or PIOMan use it for IO/communication 
 * progression.
 *
 **/


#ifndef __MARCEL_IO_BRIDGE_H__
#define __MARCEL_IO_BRIDGE_H__


enum {
	MARCEL_SCHEDULING_POINT_TIMER = 0,
	MARCEL_SCHEDULING_POINT_YIELD = 1,
	MARCEL_SCHEDULING_POINT_LIB_ENTRY = 2,
	MARCEL_SCHEDULING_POINT_IDLE = 3,
	MARCEL_SCHEDULING_POINT_CTX_SWITCH = 4,
	MARCEL_SCHEDULING_POINT_MAX = 5
};

typedef unsigned marcel_scheduling_point_t;
typedef int (marcel_scheduling_hook_t) (marcel_scheduling_point_t scheduling_point);

/* Register a callback that will be called each type Marcel reaches a particular scheduling point */
void marcel_register_scheduling_hook(marcel_scheduling_hook_t hook,
				     marcel_scheduling_point_t point);

/* Unregister a callback for a specific scheduling point 
 * Return 1 if the callback is successfully unregister and 0 otherwise
 */
int marcel_unregister_scheduling_hook(marcel_scheduling_hook_t hook,
				      marcel_scheduling_point_t point);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


void ma_init_scheduling_hook(void);

/* Call all the callbacks registered for a particular scheduling point */
int ma_schedule_hooks(marcel_scheduling_point_t scheduling_point);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_IO_BRIDGE_H__ **/
