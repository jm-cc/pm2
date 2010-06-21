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


#ifndef __MARCEL_BUBBLE_GANG_H__
#define __MARCEL_BUBBLE_GANG_H__


#include "scheduler-marcel/marcel_bubble_sched.h"


/** Public global variables **/
MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(gang);


/** Public functions **/
/** \brief Gang scheduler thread function
 *
 * This is the "thread function" for gang scheduling.
 *
 * \param runqueue is the runqueue on which the gang scheduler will put bubbles
 * to be scheduled.
 *
 * You may for instance use
 * \code
 * marcel_create(NULL, NULL, marcel_gang_scheduler, &marcel_topo_levels[0][0].sched);
 * \endcode
 * to spawn one Gang scheduler which will put bubbles for the whole machine,
 * or use
 * \code
 * marcel_create(NULL, NULL, marcel_gang_scheduler, &marcel_topo_levels[1][0].sched);
 * marcel_create(NULL, NULL, marcel_gang_scheduler, &marcel_topo_levels[1][1].sched);
 * \endcode
 * to spawn two Gang schedulers which will put bubbles for one part of the
 * machine each, etc.
 *
 * You should probably use
 * \code 
 * marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
 * \endcode
 * for making sure that the Gang Scheduler can preempt other threads.
 */
any_t marcel_gang_scheduler(any_t runqueue);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal global variables **/
/** \brief Gang scheduler runqueue
 *
 * This is the common runqueue where all Gang schedulers take and put back
 * bubbles.
 */
extern ma_runqueue_t ma_gang_rq;


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_BUBBLE_GANG_H__ **/
