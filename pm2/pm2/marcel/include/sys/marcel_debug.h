
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

#section types
#include "tbx_debug.h"
#include "sys/marcel_flags.h"

#section variables
#ifdef PM2DEBUG
extern debug_type_t marcel_mdebug;
extern debug_type_t marcel_trymdebug;
extern debug_type_t marcel_debug_state;
extern debug_type_t marcel_debug_work;
extern debug_type_t marcel_debug_deviate;
extern debug_type_t marcel_debug_upcall;
extern debug_type_t marcel_mdebug_sched_q;

extern debug_type_t marcel_lock_task_debug;

extern debug_type_t marcel_sched_lock_debug;

extern debug_type_t marcel_mtrace;
extern debug_type_t marcel_mtrace_timer;
#endif

#section macros
#ifdef PM2DEBUG
#  define mdebug(fmt, args...) \
    debug_printf(&marcel_mdebug, fmt , ##args)
#  define try_mdebug(fmt, args...) \
    debug_printf(&marcel_trymdebug, fmt , ##args)
#  define mdebug_state(fmt, args...) \
    debug_printf(&marcel_debug_state, fmt , ##args)
#  define mdebug_work(fmt, args...) \
    debug_printf(&marcel_debug_work, fmt , ##args)
#  define mdebug_deviate(fmt, args...) \
    debug_printf(&marcel_debug_deviate, fmt , ##args)
#  define mdebug_upcall(fmt, args...) \
    debug_printf(&marcel_debug_upcall, fmt , ##args)
#  define mdebug_sched_q(fmt, args...) \
    debug_printf(&marcel_mdebug_sched_q, fmt , ##args)
#else // PM2DEBUG
#  define mdebug(fmt, args...)     (void)0
#  define try_mdebug(fmt, args...)     (void)0
#  define mdebug_state(fmt, args...)     (void)0
#  define mdebug_work(fmt, args...)     (void)0
#  define mdebug_deviate(fmt, args...)     (void)0
#  define mdebug_upcall(fmt, args...)     (void)0
#  define mdebug_sched_q(fmt, args...)     (void)0
#endif // PM2DEBUG

#define MA_BUG() RAISE(PROGRAM_ERROR)
#define MA_BUG_ON(cond) \
  do { \
	if (cond) { \
		RAISE(PROGRAM_ERROR); \
	} \
  } while (0);

#define MA_WARN_ON(cond) \
  do { \
	if (cond) { \
		mdebug("%s:%l:Warning on '" #cond "'", __FILE__, __LINE__); \
	} \
  } while (0);

#ifdef DEBUG_LOCK_TASK
#  define lock_task_debug(fmt, args...) debug_printf(&marcel_lock_task_debug, \
        fmt, ##args)
#endif
#ifdef DEBUG_SCHED_LOCK
#  define sched_lock_debug(fmt, args...) debug_printf(&marcel_sched_lock_debug, \
        fmt, ##args)
#endif

#ifndef MARCEL_TRACE
#  define MTRACE(msg, pid) (void)0
#  define MTRACE_TIMER(msg, pid) (void)0
#  define marcel_trace_on() (void)0
#  define marcel_trace_off() (void)0
#else
#  define MTRACE(msg, pid) \
    (msg[0] ? debug_printf(&marcel_mtrace, \
            "[%-14s:%3ld (pid=%p:%2lX)." \
            " [%1d], %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->flags, \
            pid->preempt_count, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1) : (void)0)
#  define MTRACE_TIMER(msg, pid) \
    debug_printf(&marcel_mtrace_timer, \
            "[%-14s:%3ld (pid=%p:%2lX)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->flags, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1)
#  define marcel_trace_on() pm2debug_setup(&marcel_mtrace, PM2DEBUG_SHOW, 1)
#  define marcel_trace_off() pm2debug_setup(&marcel_mtrace, PM2DEBUG_SHOW, 0)
#endif

#section marcel_functions
void marcel_debug_init(int* argc, char** argv, int debug_flags);





