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


#ifndef __MARCEL_SCHED_TYPES_H__
#define __MARCEL_SCHED_TYPES_H__


#include "tbx_compiler.h"
#include "tbx_types.h"
#include "marcel_types.h"
#include "marcel_config.h"
#include "scheduler-marcel/marcel_bubble_sched_types.h"


/** Public data types **/
typedef struct marcel_sched_attr marcel_sched_attr_t;
typedef unsigned (*marcel_schedpolicy_func_t)(marcel_t pid,
					      unsigned current_lwp);
struct marcel_sched_param {
	int __sched_priority;
};
#ifndef sched_priority
#define sched_priority __sched_priority
#endif


/** Public data structures **/
/** \brief Scheduler-specific attributes.
 *
 * TODO: merge with main attributes structs */
struct marcel_sched_attr {

	/** \brief SMP scheduling policy code for controlling how to select a VP for the newly created thread. */
	int sched_policy;

	/** \brief Holder to explicitely use as the created thread's natural
	 * holder.
	 *
	 * \attention Override to the marcel_sched_attr#inheritholder flag
	 * and marcel_task#default_children_bubble implicit holder selection mechanisms. */
	ma_holder_t *natural_holder;

	/** \brief Flag indicating whether the newly created thread should inherit the natural holder of its creating thread (flag set) or not (flag unset).
	 *
	 * \attention Override the marcel_task#default_children_bubble implicit holder selection mechanism.
	 *
	 * \attention Is overridden by the marcel_sched_attr#natural_holder explicite holder selection mechanism. */
	tbx_bool_t inheritholder;
};


#ifdef __MARCEL_KERNEL__


/** Internal data structures **/
struct ma_lwp_usage_stat {
	unsigned long long user;
	unsigned long long nice;
	//unsigned long long system;
	unsigned long long softirq;
	unsigned long long irq;
	unsigned long long idle;
	//unsigned long long iowait;
        unsigned long long disabled;
};


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_SCHED_TYPES_H__ **/
