
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

#include "tbx_compiler.h"
#include "tbx_debug.h"


#define XPAUL_CONSTRAINT_ERROR "XPAUL_CONSTRAINT_ERROR"
#define XPAUL_TASKING_ERROR   "XPAUL_TASKING_ERROR: A non-handled exception occurred in a task"
#define XPAUL_DEADLOCK_ERROR   "XPAUL_DEADLOCK_ERROR: Global Blocking Situation Detected"
#define XPAUL_STORAGE_ERROR   "XPAUL_STORAGE_ERROR: No space left on the heap"
#define XPAUL_CONSTRAINT_ERROR   "XPAUL_CONSTRAINT_ERROR"
#define XPAUL_PROGRAM_ERROR   "XPAUL_PROGRAM_ERROR"
#define XPAUL_STACK_ERROR "XPAUL_STACK_ERROR: Stack Overflow"
#define XPAUL_TIME_OUT "TIME OUT while being blocked on a semaphor"
#define XPAUL_NOT_IMPLEMENTED "XPAUL_NOT_IMPLEMENTED (sorry)"
#define XPAUL_USE_ERROR "XPAUL_USE_ERROR: XPaulette was not compiled to enable this functionality"
#define XPAUL_CALLBACK_ERROR "XPAUL_CALLBACK_ERROR: no callback defined"

/*
#ifndef MODULE
#define MODULE xpaulette
#endif // MODULE 
*/

#define XPAUL_DEBUG_VAR_ATTRIBUTE \
    TBX_SECTION(".xpaul.debug.var") TBX_ALIGNED

#ifdef PM2DEBUG
#define XPAUL_DEBUG_VAR_NAME(name)   XPAUL_DEBUG_VAR_NAME_S(name)
#define XPAUL_DEBUG_VAR_NAME_S(name) xpaul_debug_##name

#define XPAUL_DECLARE_DEBUG_NAME(name) XPAUL_DECLARE_DEBUG_NAME_S(name)
#define XPAUL_DECLARE_DEBUG_NAME_S(name) \
  extern debug_type_t XPAUL_DEBUG_VAR_NAME(name)

#define XPAUL_DEBUG_DEFINE_NAME_DEPEND(name, dep) \
  XPAUL_DEBUG_DEFINE_NAME_S(name, dep)
#define XPAUL_DEBUG_DEFINE_NAME(name) \
  XPAUL_DEBUG_DEFINE_NAME_S(name, &xpaul_debug)
#define XPAUL_DEBUG_DEFINE_NAME_S(name, dep) \
  XPAUL_DEBUG_VAR_ATTRIBUTE debug_type_t XPAUL_DEBUG_VAR_NAME(name) \
   NEW_DEBUG_TYPE_DEPEND("MA-"#name": ", "xpaul-"#name, dep)

#define xpaul_debug(name, fmt, ...) \
  xpaul_debugl(name, PM2DEBUG_STDLEVEL, fmt , ##__VA_ARGS__)
#define xpaul_debugl(name, level, fmt, ...) \
  debug_printfl(&XPAUL_DEBUG_VAR_NAME(name), level, fmt , ##__VA_ARGS__)

#if !defined(XPAUL_FILE_DEBUG)
#  define XPAUL_DEBUG_NO_DEFINE
#  define XPAUL_FILE_DEBUG default
#endif

XPAUL_DECLARE_DEBUG_NAME(default);

#ifdef XPAUL_FILE_DEBUG

#  undef DEBUG_NAME
#  define DEBUG_NAME CONCAT3(MODULE,_,XPAUL_FILE_DEBUG)
#  define XDEBUG_STR_NAME CONCAT3(MODULE,3,XPAUL_FILE_DEBUG)
extern debug_type_t DEBUG_NAME_DISP(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_LOG(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME);

#  define DEBUG_NAME_DISP_MODULE  DEBUG_NAME_DISP(DEBUG_NAME_MODULE)
#  define DEBUG_NAME_LOG_MODULE   DEBUG_NAME_LOG(DEBUG_NAME_MODULE)
#  define DEBUG_NAME_TRACE_MODULE DEBUG_NAME_TRACE(DEBUG_NAME_MODULE)
#  define XPAUL_DEBUG_DEFINE_STANDARD(DEBUG_VAR_NAME, XDEBUG_STR_NAME) \
XPAUL_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_DISP(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(XDEBUG_STR_NAME "-disp: ", \
		          XDEBUG_STR_NAME "-disp", &DEBUG_NAME_DISP_MODULE); \
XPAUL_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_LOG(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(XDEBUG_STR_NAME "-log: ", \
		          XDEBUG_STR_NAME "-log", &DEBUG_NAME_LOG_MODULE); \
XPAUL_DEBUG_VAR_ATTRIBUTE debug_type_t DEBUG_NAME_TRACE(DEBUG_VAR_NAME) \
  = NEW_DEBUG_TYPE_DEPEND(XDEBUG_STR_NAME "-trace: ", \
		          XDEBUG_STR_NAME "-trace", &DEBUG_NAME_TRACE_MODULE)

/* Une variable de debug a definir pour ce fichier C */
#  ifndef XPAUL_DEBUG_NO_DEFINE
/* On peut avoir envie de le definir nous meme (�1 par exemple) */
XPAUL_DEBUG_DEFINE_NAME_DEPEND(XPAUL_FILE_DEBUG, &xpaul_xdebug);
XPAUL_DEBUG_DEFINE_STANDARD(DEBUG_NAME, XDEBUG_STR_NAME);
#  endif
#  undef xdebugl
#  define xdebugl(level, fmt, ...) \
  xpaul_debugl(XPAUL_FILE_DEBUG, level, fmt , ##__VA_ARGS__)
#endif

extern debug_type_t xpaul_debug;
extern debug_type_t xpaul_xdebug;
extern debug_type_t xpaul_debug_state;
extern debug_type_t xpaul_debug_work;
extern debug_type_t xpaul_debug_deviate;
extern debug_type_t xpaul_xdebug_sched_q;

extern debug_type_t xpaul_mtrace;
extern debug_type_t xpaul_mtrace_timer;

#ifndef xdebugl
#  define xdebugl(level, fmt, ...) \
      debug_printfl(&xpaul_xdebug, level, fmt , ##__VA_ARGS__)
#endif
#define xdebug(fmt, ...) \
    xdebugl(PM2DEBUG_STDLEVEL, fmt , ##__VA_ARGS__)
#define xdebug_state(fmt, ...) \
    debug_printf(&xpaul_debug_state, fmt , ##__VA_ARGS__)
#define xdebug_work(fmt, ...) \
    debug_printf(&xpaul_debug_work, fmt , ##__VA_ARGS__)
#define xdebug_deviate(fmt, ...) \
    debug_printf(&xpaul_debug_deviate, fmt , ##__VA_ARGS__)
#define xdebug_sched_q(fmt, ...) \
    debug_printf(&xpaul_xdebug_sched_q, fmt , ##__VA_ARGS__)

#else				/* PM2DEBUG */
#ifndef xdebugl
#  define xdebugl(level, fmt, ...) (void) 0
#endif
#define xdebug(fmt, ...) (void) 0
#define xdebug_state(fmt, ...) (void) 0
#define xdebug_work(fmt, ...) (void) 0
#define xdebug_deviate(fmt, ...) (void) 0
#define xdebug_sched_q(fmt, ...) (void) 0

#endif				/* PM2DEBUG */


#include <signal.h>
#ifdef PM2_OPT
#define XPAUL_BUG_ON(cond) (void)(cond)
#define XPAUL_WARN_ON(cond) (void)(cond)
#else
#define XPAUL_BUG_ON(cond) \
  do { \
	if (cond) { \
		xdebug("BUG on '" #cond "' at %s:%u\n", __FILE__, __LINE__); \
		raise(SIGABRT); \
	} \
  } while (0)
#define XPAUL_WARN_ON(cond) \
  do { \
	if (cond) { \
		xdebug("%s:%u:Warning on '" #cond "'\n", __FILE__, __LINE__); \
	} \
  } while (0)
#endif
#define XPAUL_BUG() XPAUL_BUG_ON(1)

#ifdef DEBUG_SCHED
#  define sched_debug(fmt, ...) debug_printf(&xpaul_sched_debug, \
        fmt, ##__VA_ARGS__)
#else
#  define sched_debug(fmt, ...) (void)0
#endif

#ifdef DEBUG_BUBBLE_SCHED
#  define bubble_sched_debug(fmt, ...) debug_printf(&xpaul_bubble_sched_debug, \
        (fmt), ##__VA_ARGS__)
#  define bubble_sched_debugl(level, fmt, ...) debug_printfl(&xpaul_bubble_sched_debug, (level), \
        (fmt), ##__VA_ARGS__)
#else
#  define bubble_sched_debug(fmt, ...) (void)0
#  define bubble_sched_debugl(level, fmt, ...) (void)0
#endif

#ifndef XPAUL_TRACE
#  define MTRACE(msg, pid) (void)0
#  define MTRACE_TIMER(msg, pid) (void)0
#  define xpaul_trace_on() (void)0
#  define xpaul_trace_off() (void)0
#else
#  define MTRACE(msg, pid) \
    (msg[0] ? debug_printf(&xpaul_mtrace, \
            "[%-14s:%3d (pid=%p(%-15s):%2lX)." \
            " [%06x], %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->name, (pid)->flags, \
            pid->preempt_count, \
            xpaul_activethreads(), \
            xpaul_sleepingthreads(), \
            xpaul_blockedthreads(), \
            xpaul_frozenthreads(), \
            xpaul_nbthreads() + 1) : 0)
#  define MTRACE_TIMER(msg, pid) \
    debug_printf(&xpaul_mtrace_timer, \
            "[%-14s:%3d (pid=%p(%-15s):%2lX)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            msg, (pid)->number, (pid), (pid)->name, (pid)->flags, \
            xpaul_activethreads(), \
            xpaul_sleepingthreads(), \
            xpaul_blockedthreads(), \
            xpaul_frozenthreads(), \
            xpaul_nbthreads() + 1)
#  define xpaul_trace_on() pm2debug_setup(&xpaul_mtrace, PM2DEBUG_SHOW, 1)
#  define xpaul_trace_off() pm2debug_setup(&xpaul_mtrace, PM2DEBUG_SHOW, 0)
#endif

void xpaul_debug_init(int *argc, char **argv, int debug_flags);
