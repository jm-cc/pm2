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


#ifndef __MARCEL_BUBBLE_MEMORY_H__
#define __MARCEL_BUBBLE_MEMORY_H__


#include "scheduler-marcel/marcel_bubble_sched.h"


MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(memory);

#ifdef MARCEL_MAMI_ENABLED
typedef int (*bubble_migrate_all_func_t) (void *memory_manager, marcel_bubble_t * bubble, int node);
typedef int (*task_migrate_all_func_t) (void *memory_manager, marcel_t task, int node);

/** \brief Use \param memory_manager as the memory manager for \param
 * scheduler.  */
extern void marcel_bubble_set_memory_manager(marcel_bubble_memory_sched_t * scheduler,
					     void *memory_manager,
					     bubble_migrate_all_func_t bubble_migrate_all_func,
					     task_migrate_all_func_t task_migrate_all_func);

/** \brief Initialize \param scheduler as a `memory' scheduler.  \param *
 * memory_manager is used to determine the amount of memory associated with a
 * thread on a given node, which then directs thread/memory migration
 * decisions.  If \param work_stealing is true, then work stealing is
 * enabled.  */
extern int marcel_bubble_memory_sched_init(struct marcel_bubble_memory_sched *scheduler,
					   void *memory_manager,
					   bubble_migrate_all_func_t bubble_migrate_all_func,
					   task_migrate_all_func_t task_migrate_all_func,
					   tbx_bool_t work_stealing);
#endif


#endif /** __MARCEL_BUBBLE_MEMORY_H__ **/
