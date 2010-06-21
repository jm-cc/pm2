/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#define MAX_SCHEDULING_HOOKS 8

static marcel_scheduling_hook_t * __ma_scheduling_hook[MAX_SCHEDULING_HOOKS][MARCEL_SCHEDULING_POINT_MAX];
static unsigned __ma_nb_scheduling_hooks[MARCEL_SCHEDULING_POINT_MAX];
static marcel_rwlock_t __ma_scheduling_hook_lock = MARCEL_RWLOCK_INITIALIZER;


void ma_init_scheduling_hook()
{
	unsigned i;

	for (i = 0; i < MARCEL_SCHEDULING_POINT_MAX; i++)
		__ma_nb_scheduling_hooks[i] = 0;
}

void marcel_register_scheduling_hook(marcel_scheduling_hook_t hook,
				     marcel_scheduling_point_t point)
{
	marcel_rwlock_wrlock(&__ma_scheduling_hook_lock);
	__ma_scheduling_hook[__ma_nb_scheduling_hooks[point]][point] = hook;
	__ma_nb_scheduling_hooks[point]++;
	marcel_rwlock_unlock(&__ma_scheduling_hook_lock);
}

int marcel_unregister_scheduling_hook(marcel_scheduling_hook_t hook,
				      marcel_scheduling_point_t point)
{
	unsigned i;
	int found = 0;

	/** ordre entre les hooks ??? **/
	marcel_rwlock_wrlock(&__ma_scheduling_hook_lock);
	for (i = 0; i < __ma_nb_scheduling_hooks[point]; i++)
		if ((__ma_scheduling_hook[i][point] == hook) || found) {
			__ma_scheduling_hook[i][point] =
			    __ma_scheduling_hook[i + 1][point];
			if (i < MAX_SCHEDULING_HOOKS)
				__ma_scheduling_hook[i + 1][point] = NULL;
			found = 1;
		}
	if (found)
		__ma_nb_scheduling_hooks[point]--;
	marcel_rwlock_unlock(&__ma_scheduling_hook_lock);
	return found;
}


int ma_schedule_hooks(marcel_scheduling_point_t scheduling_point)
{
	unsigned i;
	int nb_poll = 0;

	/* todo: maybe we can get rid of this lock. We could do something like
	   marcel_scheduling_point_t=f[i][scheduling_point];
	   if(f)
	   f(scheduling_point);
	   But this require that the callback may be called after it is unregistered
	 */
	marcel_rwlock_rdlock(&__ma_scheduling_hook_lock);
	for (i = 0; i < __ma_nb_scheduling_hooks[scheduling_point]; i++)
		nb_poll += (*__ma_scheduling_hook[i][scheduling_point]) (scheduling_point);
	marcel_rwlock_unlock(&__ma_scheduling_hook_lock);
	return nb_poll;
}
