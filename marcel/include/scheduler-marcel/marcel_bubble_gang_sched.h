
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

/** \file
 * \brief Bubble Gang scheduler
 * \defgroup marcel_bubble_gang Bubble Gang scheduler
 *
 * Gang Scheduling was proposed back in the 1980's by Ousterhout for
 * efficiently exploiting machines by taking affinities into account.  The idea
 * is to schedule together the threads of the same "gang".
 *
 * This is here realized by expressing gangs thanks to bubbles: a bubble just
 * holds the threads of a gang, and a daemon thread is spawned for actually
 * performing the scheduling.
 *
 * The daemon thread takes a bubble from the ::ma_gang_rq runqueue, puts it on
 * a scheduling runqueue, and leave it there for some time during which the
 * basic Self-Scheduler can schedule the threads of the bubble. The daemon
 * thread then puts the bubble back into ::ma_gang_rq, and loops with another
 * bubble.
 *
 * If (for instance), the gangs are not big enough for filling the whole
 * machine, it is possible to start several concurrent bubble gang schedulers
 * by just spawning several daemon threads.
 *
 * @{
 */

#section functions
#depend "marcel_utils.h[types]"
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

#section marcel_variables
/** \brief Gang scheduler runqueue
 *
 * This is the common runqueue where all Gang schedulers take and put back
 * bubbles.
 */
extern ma_runqueue_t ma_gang_rq;

#section variables
#depend "marcel_bubble_sched_interface.h[macros]"

MARCEL_DECLARE_BUBBLE_SCHEDULER_CLASS(gang);

/* @} */
