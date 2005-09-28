
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

#section marcel_macros
#include "tbx_compiler.h"

#define MA_DEBUG_VAR_ATTRIBUTE \
    TBX_SECTION(".ma.debug.var") TBX_ALIGNED

#ifdef PM2DEBUG
#define MA_DEBUG_VAR_NAME(name)   MA_DEBUG_VAR_NAME_S(name)
#define MA_DEBUG_VAR_NAME_S(name) ma_debug_##name

#define MA_DECLARE_DEBUG_NAME(name) MA_DECLARE_DEBUG_NAME_S(name)
#define MA_DECLARE_DEBUG_NAME_S(name) \
  extern debug_type_t MA_DEBUG_VAR_NAME(name)

#define MA_DEBUG_DEFINE_NAME_DEPEND(name, dep) \
  MA_DEBUG_DEFINE_NAME_S(name, dep)
#define MA_DEBUG_DEFINE_NAME(name) \
  MA_DEBUG_DEFINE_NAME_S(name, &marcel_debug)
#define MA_DEBUG_DEFINE_NAME_S(name, dep) \
  MA_DEBUG_VAR_ATTRIBUTE debug_type_t MA_DEBUG_VAR_NAME(name) \
  = NEW_DEBUG_TYPE_DEPEND("MA-"#name": ", "marcel-"#name, dep)

#define ma_debug(name, fmt, ...) \
  ma_debugl(name, PM2DEBUG_STDLEVEL, fmt , ##__VA_ARGS__)
#define ma_debugl(name, level, fmt, ...) \
  debug_printfl(&MA_DEBUG_VAR_NAME(name), level, fmt , ##__VA_ARGS__)

#if defined (MARCEL_KERNEL) && !defined(MA_FILE_DEBUG)
#  define MA_DEBUG_NO_DEFINE
#  define MA_FILE_DEBUG default
#endif

MA_DECLARE_DEBUG_NAME(default);

#ifdef MA_FILE_DEBUG
#  depend "tbx_debug.h"

#  undef DEBUG_NAME
#  define DEBUG_NAME CONCAT3(MODULE,_,MA_FILE_DEBUG)
#  define DEBUG_STR_NAME marcel_xstr(MODULE) "-" marcel_xstr(MA_FILE_DEBUG)
extern debug_type_t DEBUG_NAME_DISP(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_LOG(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME);

#  define DEBUG_NAME_DISP_MODULE  DEBUG_NAME_DISP(DEBUG_NAME_MODULE)
#  define DEBUG_NAME_LOG_MODULE   DEBUG_NAME_LOG(DEBUG_NAME_MODULE)
#  define DEBUG_NAME_TRACE_MODULE DEBUG_NAME_TRACE(DEBUG_NAME_MODULE)
#  define MA_DEBUG_DEFINE_STANDARD(DEBUG_VAR_NAME, DEBUG_STR_NAME) \
MA_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_DISP(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-disp: ", \
		          DEBUG_STR_NAME "-disp", &DEBUG_NAME_DISP_MODULE); \
MA_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_LOG(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-log: ", \
		          DEBUG_STR_NAME "-log", &DEBUG_NAME_LOG_MODULE); \
MA_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_TRACE(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-trace: ", \
		          DEBUG_STR_NAME "-trace", &DEBUG_NAME_TRACE_MODULE)

/* Une variable de debug � d�finir pour ce fichier C */
#  ifndef MA_DEBUG_NO_DEFINE
/* On peut avoir envie de le d�finir nous m�me (� 1 par exemple) */
MA_DEBUG_DEFINE_NAME_DEPEND(MA_FILE_DEBUG, &marcel_mdebug);
MA_DEBUG_DEFINE_STANDARD(DEBUG_NAME, DEBUG_STR_NAME);
#  endif
#  undef mdebugl
#  define mdebugl(level, fmt, ...) \
  ma_debugl(MA_FILE_DEBUG, level, fmt , ##__VA_ARGS__)
#endif
#endif /* PM2DEBUG */


#section types
#depend "tbx_debug.h"

#section variables
#ifdef PM2DEBUG
extern debug_type_t marcel_debug;
extern debug_type_t marcel_mdebug;
extern debug_type_t marcel_debug_state;
extern debug_type_t marcel_debug_work;
extern debug_type_t marcel_debug_deviate;
extern debug_type_t marcel_mdebug_sched_q;

extern debug_type_t marcel_lock_task_debug;

extern debug_type_t marcel_sched_debug;
extern debug_type_t marcel_bubble_sched_debug;

extern debug_type_t marcel_mtrace;
extern debug_type_t marcel_mtrace_timer;
#endif

#section macros
#ifndef mdebugl
#  define mdebugl(level, fmt, ...) \
      debug_printfl(&marcel_mdebug, level, fmt , ##__VA_ARGS__)
#endif
#define mdebug(fmt, ...) \
    mdebugl(PM2DEBUG_STDLEVEL, fmt , ##__VA_ARGS__)
#define mdebug_state(fmt, ...) \
    debug_printf(&marcel_debug_state, fmt , ##__VA_ARGS__)
#define mdebug_work(fmt, ...) \
    debug_printf(&marcel_debug_work, fmt , ##__VA_ARGS__)
#define mdebug_deviate(fmt, ...) \
    debug_printf(&marcel_debug_deviate, fmt , ##__VA_ARGS__)
#define mdebug_sched_q(fmt, ...) \
    debug_printf(&marcel_mdebug_sched_q, fmt , ##__VA_ARGS__)

#include <signal.h>
#ifdef PM2_OPT
#define MA_BUG_ON(cond) (void)(cond)
#define MA_WARN_ON(cond) (void)(cond)
#else
#define MA_BUG_ON(cond) \
  do { \
	if (cond) { \
		mdebug("BUG at %s:%u\n", __FILE__, __LINE__); \
		raise(SIGABRT); \
	} \
  } while (0)
#define MA_WARN_ON(cond) \
  do { \
	if (cond) { \
		mdebug("%s:%u:Warning on '" #cond "'\n", __FILE__, __LINE__); \
	} \
  } while (0)
#endif
#define MA_BUG() MA_BUG_ON(1)

#ifdef DEBUG_LOCK_TASK
#  define lock_task_debug(fmt, ...) debug_printf(&marcel_lock_task_debug, \
        fmt, ##__VA_ARGS__)
#endif
#ifdef DEBUG_SCHED
#  define sched_debug(fmt, ...) debug_printf(&marcel_sched_debug, \
        fmt, ##__VA_ARGS__)
#else
#  define sched_debug(fmt, ...) (void)0
#endif

#ifdef DEBUG_BUBBLE_SCHED
#  define bubble_sched_debug(fmt, ...) debug_printf(&marcel_bubble_sched_debug, \
        (fmt), ##__VA_ARGS__)
#  define bubble_sched_debugl(level, fmt, ...) debug_printfl(&marcel_bubble_sched_debug, (level), \
        (fmt), ##__VA_ARGS__)
#else
#  define bubble_sched_debug(fmt, ...) (void)0
#  define bubble_sched_debugl(level, fmt, ...) (void)0
#endif

#ifndef MARCEL_TRACE
#  define MTRACE(msg, pid) (void)0
#  define MTRACE_TIMER(msg, pid) (void)0
#  define marcel_trace_on() (void)0
#  define marcel_trace_off() (void)0
#else
#  define MTRACE(msg, pid) \
    (msg[0] ? debug_printf(&marcel_mtrace, \
            "[%-14s:%3d (pid=%p(%-15s):%2lX)." \
            " [%06x], %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->name, (pid)->flags, \
            pid->preempt_count, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1) : (void)0)
#  define MTRACE_TIMER(msg, pid) \
    debug_printf(&marcel_mtrace_timer, \
            "[%-14s:%3d (pid=%p(%-15s):%2lX)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->name, (pid)->flags, \
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

