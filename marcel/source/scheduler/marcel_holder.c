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
#include "scheduler/marcel_holder.h"


#ifdef MARCEL_MAMI_ENABLED


void marcel_get_task_memory_areas(marcel_t task, struct tbx_fast_list_head **memory_areas)
{
        *memory_areas = &((&(task)->as_entity)->memory_areas);
}

void marcel_get_task_memory_areas_lock(marcel_t task, marcel_spinlock_t **memory_areas_lock)
{
        *memory_areas_lock = &((&(task)->as_entity)->memory_areas_lock);
}

void marcel_get_bubble_memory_areas(marcel_bubble_t *bubble, struct tbx_fast_list_head **memory_areas)
{
#ifdef MA__BUBBLES
        *memory_areas = &((&(bubble)->as_entity)->memory_areas);
#endif
}

void marcel_get_bubble_memory_areas_lock(marcel_bubble_t *bubble, marcel_spinlock_t **memory_areas_lock)
{
#ifdef MA__BUBBLES
        *memory_areas_lock = &((&(bubble)->as_entity)->memory_areas_lock);
#endif
}


#endif
