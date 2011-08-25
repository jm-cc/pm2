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


/* TODO: this is a contention point, implement RCU lists and use them instead */
marcel_scheduling_hook_t * __ma_scheduling_hook[MAX_SCHEDULING_HOOKS][MARCEL_SCHEDULING_POINT_MAX];
unsigned __ma_nb_scheduling_hooks[MARCEL_SCHEDULING_POINT_MAX];
ma_rwlock_t __ma_scheduling_hook_lock = MA_RW_LOCK_UNLOCKED;


void ma_init_scheduling_hook()
{
	unsigned i;

	for (i = 0; i < MARCEL_SCHEDULING_POINT_MAX; i++)
		__ma_nb_scheduling_hooks[i] = 0;
}

void marcel_register_scheduling_hook(marcel_scheduling_hook_t hook,
				     marcel_scheduling_point_t point)
{
	ma_write_lock(&__ma_scheduling_hook_lock);
	__ma_scheduling_hook[__ma_nb_scheduling_hooks[point]][point] = hook;
	__ma_nb_scheduling_hooks[point]++;
	ma_write_unlock(&__ma_scheduling_hook_lock);
}

int marcel_unregister_scheduling_hook(marcel_scheduling_hook_t hook,
				      marcel_scheduling_point_t point)
{
	unsigned i;
	int found = 0;

	/** ordre entre les hooks ??? **/
	ma_write_lock(&__ma_scheduling_hook_lock);
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
	ma_write_unlock(&__ma_scheduling_hook_lock);
	return found;
}
