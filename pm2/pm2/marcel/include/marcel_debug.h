
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

#ifdef PM2DEBUG
#define MA_DEBUG_VAR_NAME(name)   MA_DEBUG_VAR_NAME_S(name)
#define MA_DEBUG_VAR_NAME_S(name) ma_debug_##name

#define MA_DECLARE_DEBUG_NAME(name) MA_DECLARE_DEBUG_NAME_S(name)
#define MA_DECLARE_DEBUG_NAME_S(name) \
  extern debug_type_t MA_DEBUG_VAR_NAME(name)

#define MA_DEFINE_DEBUG_NAME_DEPEND(name, dep) \
  MA_DEFINE_DEBUG_NAME_S(name, dep)
#define MA_DEFINE_DEBUG_NAME(name) \
  MA_DEFINE_DEBUG_NAME_S(name, &marcel_debug)
#define MA_DEFINE_DEBUG_NAME_S(name, dep) \
  debug_type_t MA_DEBUG_VAR_NAME(name) \
    __attribute__((section(".ma.debug.var"))) \
  = NEW_DEBUG_TYPE_DEPEND("MA-"#name": ", "marcel-"#name, dep)

#define ma_debug(name, fmt, args...) \
  ma_debugl(name, PM2DEBUG_STDLEVEL, fmt , ##args)
#define ma_debugl(name, level, fmt, args...) \
  debug_printfl(&MA_DEBUG_VAR_NAME(name), level, fmt , ##args)

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
#  define MA_DEBUG_DECLARE_STANDARD(DEBUG_VAR_NAME, DEBUG_STR_NAME) \
debug_type_t DEBUG_NAME_DISP(DEBUG_VAR_NAME) \
    __attribute__((section(".ma.debug.var"))) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-disp: ", \
		          DEBUG_STR_NAME "-disp", &DEBUG_NAME_DISP_MODULE); \
debug_type_t DEBUG_NAME_LOG(DEBUG_VAR_NAME) \
    __attribute__((section(".ma.debug.var"))) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-log: ", \
		          DEBUG_STR_NAME "-log", &DEBUG_NAME_LOG_MODULE); \
debug_type_t DEBUG_NAME_TRACE(DEBUG_VAR_NAME) \
    __attribute__((section(".ma.debug.var"))) \
  = NEW_DEBUG_TYPE_DEPEND(DEBUG_STR_NAME "-trace: ", \
		          DEBUG_STR_NAME "-trace", &DEBUG_NAME_TRACE_MODULE)

/* Une variable de debug � d�finir pour ce fichier C */
#  ifndef MA_DEBUG_NO_DEFINE
/* On peut avoir envie de le d�finir nous m�me (� 1 par exemple) */
MA_DEFINE_DEBUG_NAME_DEPEND(MA_FILE_DEBUG, &marcel_mdebug);
MA_DEBUG_DECLARE_STANDARD(DEBUG_NAME, DEBUG_STR_NAME);
#  endif
#  undef mdebugl
#  define mdebugl(level, fmt, args...) \
  ma_debugl(MA_FILE_DEBUG, level, fmt , ##args)
#endif
#endif


#section types
#depend "tbx_debug.h"

#section variables
#ifdef PM2DEBUG
extern debug_type_t marcel_debug;
extern debug_type_t marcel_mdebug;
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
#ifndef mdebugl
#  define mdebugl(level, fmt, args...) \
      debug_printfl(&marcel_mdebug, level, fmt , ##args)
#endif
#define mdebug(fmt, args...) \
    mdebugl(PM2DEBUG_STDLEVEL, fmt , ##args)
#define mdebug_state(fmt, args...) \
    debug_printf(&marcel_debug_state, fmt , ##args)
#define mdebug_work(fmt, args...) \
    debug_printf(&marcel_debug_work, fmt , ##args)
#define mdebug_deviate(fmt, args...) \
    debug_printf(&marcel_debug_deviate, fmt , ##args)
#define mdebug_upcall(fmt, args...) \
    debug_printf(&marcel_debug_upcall, fmt , ##args)
#define mdebug_sched_q(fmt, args...) \
    debug_printf(&marcel_mdebug_sched_q, fmt , ##args)

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

