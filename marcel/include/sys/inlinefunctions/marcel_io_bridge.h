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


#ifndef __INLINE_SYS_MARCEL_IO_BRIDGE_H__
#define __INLINE_SYS_MARCEL_IO_BRIDGE_H__


#include "sys/marcel_io_bridge.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


static __tbx_inline__ int ma_schedule_hooks(marcel_scheduling_point_t scheduling_point)
{
	unsigned i;
	int nb_poll = 0;

	/* todo: maybe we can get rid of this lock. We could do something like
	   marcel_scheduling_point_t=f[i][scheduling_point];
	   if(f)
	   f(scheduling_point);
	   But this require that the callback may be called after it is unregistered
	 */

	if (__ma_nb_scheduling_hooks[scheduling_point]) {
		ma_read_lock(&__ma_scheduling_hook_lock);
		for (i = 0; i < __ma_nb_scheduling_hooks[scheduling_point]; i++)
			nb_poll += (*__ma_scheduling_hook[i][scheduling_point]) (scheduling_point);
		ma_read_unlock(&__ma_scheduling_hook_lock);
	}

	return nb_poll;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINE_SYS_MARCEL_IO_BRIDGE_H__ **/
