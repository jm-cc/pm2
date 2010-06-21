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


#ifndef __MARCEL_DEBUG_H__
#define __MARCEL_DEBUG_H__


#include "sys/marcel_flags.h"
#include "marcel_config.h"
#include "tbx_compiler.h"
#include "tbx_types.h"
#include "tbx_macros.h"
#include "marcel_utils.h"
#include "tbx_log.h"
#include "marcel_profile.h"


/* Compile-time assertions.  Taken from Gnulib's LGPLv2+ `verify' module.  */
/* Verify requirement R at compile-time, as an integer constant expression.
   Return 1.  */
#ifdef __cplusplus

extern "C++" {
	template < int w > struct verify_type__ {
		unsigned int verify_error_if_negative_size__:w;
	};
#define MA_VERIFY_TRUE(R)				\
  (sizeof (verify_type__<(R) ? 1 : -1>) != 0)
}
#else
#define MA_VERIFY_TRUE(R) \
  (!!sizeof								\
   (struct { unsigned int verify_error_if_negative_size__: (R) ? 1 : -1; }))
#endif

/** \brief Verify requirement \param R at compile-time, as a declaration
 * without a trailing ';'.  */
# define MA_VERIFY(R) extern int (* verify_function__ (void)) [MA_VERIFY_TRUE (R)]


#ifdef PM2DEBUG

#ifdef MARCEL_LOGGER_DECLARE
tbx_log_t marcel_events;
tbx_log_t marcel_sched_events;
tbx_log_t marcel_alloc_events;
tbx_log_t marcel_timer_events;
#else
extern tbx_log_t marcel_events;
extern tbx_log_t marcel_sched_events;
extern tbx_log_t marcel_alloc_events;
extern tbx_log_t marcel_timer_events;
#endif /** LOGGER_DECLARE **/


/** logger constructors and destructors **/
#define MARCEL_LOG_ADD()                     tbx_logger_add(&marcel_events, "marcel")
#define MARCEL_SCHED_LOG_ADD()               tbx_logger_add(&marcel_sched_events, "marcel_sched")
#define MARCEL_ALLOC_LOG_ADD()               tbx_logger_add(&marcel_alloc_events, "marcel_alloc")
#define MARCEL_TIMER_LOG_ADD()               tbx_logger_add(&marcel_timer_events, "marcel_timer")

#define MARCEL_LOG_SET_LEVEL(level)          tbx_logger_set_level(&marcel_events, level)
#define MARCEL_SCHED_LOG_SET_LEVEL(level)    tbx_logger_set_level(&marcel_sched_events, level)
#define MARCEL_ALLOC_LOG_SET_LEVEL(level)    tbx_logger_set_level(&marcel_alloc_events, level)
#define MARCEL_TIMER_LOG_SET_LEVEL(level)    tbx_logger_set_level(&marcel_timer_events, level)

#define MARCEL_LOG_DEL()                     tbx_logger_del(&marcel_events)
#define MARCEL_SCHED_LOG_DEL()               tbx_logger_del(&marcel_sched_events)
#define MARCEL_ALLOC_LOG_DEL()               tbx_logger_del(&marcel_alloc_events)
#define MARCEL_TIMER_LOG_DEL()               tbx_logger_del(&marcel_timer_events)

/** LOG level macros **/
#define MARCEL_LOG(format, ...)              marcel_logger_print(&marcel_events, LOG, format, ##__VA_ARGS__)
#define MARCEL_SCHED_LOG(format, ...)        marcel_logger_print(&marcel_sched_events, LOG, format, ##__VA_ARGS__)
#define MARCEL_ALLOC_LOG(format, ...)        marcel_logger_print(&marcel_alloc_events, LOG, format, ##__VA_ARGS__)
#define MARCEL_TIMER_LOG(format, ...)        marcel_logger_print(&marcel_timer_events, LOG, format, ##__VA_ARGS__)
#define MARCEL_LOG_RETURN(val)               do { __typeof__(val) _ret=(val) ; MARCEL_LOG_OUT() ; return _ret; } while (0)

/** DISP level macros **/
#define MARCEL_DISP(format, ...)             marcel_logger_print(&marcel_events, DISP, format, ##__VA_ARGS__)
#define MARCEL_SCHED_DISP(format, ...)       marcel_logger_print(&marcel_sched_events, DISP, format, ##__VA_ARGS__)
#define MARCEL_ALLOC_DISP(format, ...)       marcel_logger_print(&marcel_alloc_events, DISP, format, ##__VA_ARGS__)
#define MARCEL_TIMER_DISP(format, ...)       marcel_logger_print(&marcel_timer_events, DISP, format, ##__VA_ARGS__)


#else


/** logger constructors and destructors **/
#define MARCEL_LOG_ADD()                    (void)0
#define MARCEL_SCHED_LOG_ADD()              (void)0
#define MARCEL_ALLOC_LOG_ADD()              (void)0
#define MARCEL_TIMER_LOG_ADD()              (void)0

#define MARCEL_LOG_SET_LEVEL(level)          (void)0
#define MARCEL_SCHED_LOG_SET_LEVEL(level)    (void)0
#define MARCEL_ALLOC_LOG_SET_LEVEL(level)    (void)0
#define MARCEL_TIMER_LOG_SET_LEVEL(level)    (void)0

#define MARCEL_LOG_DEL()                    (void)0
#define MARCEL_SCHED_LOG_DEL()              (void)0
#define MARCEL_ALLOC_LOG_DEL()              (void)0
#define MARCEL_TIMER_LOG_DEL()              (void)0

/** LOG level macros **/
#define MARCEL_LOG(format, ...)              (void)0
#define MARCEL_SCHED_LOG(format, ...)        (void)0
#define MARCEL_ALLOC_LOG(format, ...)        (void)0
#define MARCEL_TIMER_LOG(format, ...)        (void)0
#define MARCEL_LOG_RETURN(val)               return(val)

/** DISP level macros **/
#define MARCEL_DISP(format, ...)             (void)0
#define MARCEL_SCHED_DISP(format, ...)       (void)0
#define MARCEL_ALLOC_DISP(format, ...)       (void)0
#define MARCEL_TIMER_DISP(format, ...)       (void)0

#endif /** PM2DEBUG **/


/** PM2DEBUG & PROFILE **/
#define MARCEL_LOG_IN()                      do { MARCEL_LOG("%s: -->\n", __TBX_FUNCTION__) ; PROF_IN() ; } while(0)
#define MARCEL_LOG_OUT()                     do { MARCEL_LOG("%s: <--\n", __TBX_FUNCTION__) ; PROF_OUT() ; } while(0)
#define MARCEL_ALLOC_LOG_IN()                do { MARCEL_ALLOC_LOG("%s: -->\n", __TBX_FUNCTION__) ; PROF_IN() ; } while(0)
#define MARCEL_ALLOC_LOG_OUT()               do { MARCEL_ALLOC_LOG("%s: <--\n", __TBX_FUNCTION__) ; PROF_OUT() ; } while(0)



#ifndef MARCEL_TRACE
#  define marcel_trace_on() (void)0
#  define marcel_trace_off() (void)0
#else
#  define marcel_trace_on()      MARCEL_LOG_ADD(DISP, NULL)
#  define marcel_trace_off()     MARCEL_LOG_DEL()
#endif


/** display the bug / warning message and self-kill or not the program **/
#define MA_ALWAYS_BUG_ON(cond) \
  do {			\
    if (tbx_unlikely(cond)) {						\
      MARCEL_LOG("BUG on '" #cond "' at %s:%u\n", __FILE__, __LINE__);	\
      while(1) *(volatile int*)0 = 0;					\
      abort();								\
    }									\
  } while (0)

#define MA_ALWAYS_WARN_ON(cond) \
  do {			 \
    if (tbx_unlikely(cond)) {						\
      MARCEL_LOG("%s:%u:Warning on '" #cond "'\n", __FILE__, __LINE__); \
    }									\
  } while (0)

#ifdef PM2_BUG_ON
#define MA_BUG_ON(cond)  MA_ALWAYS_BUG_ON(cond)
#define MA_WARN_ON(cond) MA_ALWAYS_WARN_ON(cond)
#else
#define MA_BUG_ON(cond)  (void)0
#define MA_WARN_ON(cond) (void)0
#endif
#define MA_BUG()         MA_ALWAYS_BUG_ON(1)
#define MA_WARN()        MA_WARN_ON(1)

#ifdef MA__LWPS
#define MA_LWP_BUG_ON(cond) MA_BUG_ON(cond)
#else
#define MA_LWP_BUG_ON(cond) (void)0
#endif


/** Public functions **/
void marcel_debug_show_thread_info(tbx_bool_t show_info);
void marcel_logger_print(tbx_log_t * logd, tbx_msg_level_t level, char *format, ...);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#ifndef MARCEL_TRACE
#  define MTRACE(msg, pid) (void)0
#  define MTRACE_TIMER(msg, pid) (void)0
#else
#  define MTRACE(msg, pid)       MARCEL_DISP("[%-14s:%3d (pid=%p(%-15s):%2lX)  [%06x], %3d T]\n", \
				       msg, (pid)->number, (pid), (pid)->as_entity.name, (pid)->flags, \
				       pid->preempt_count,		\
				       marcel_nbthreads() + 1)
#  define MTRACE_TIMER(msg, pid) MARCEL_DISP("[%-14s:%3d (pid=%p(%-15s):%2lX)  %3d T]\n", \
					     msg, (pid)->number, (pid), (pid)->as_entity.name, (pid)->flags, \
					     marcel_nbthreads() + 1)
#endif
/** Internal functions **/
void marcel_debug_init(void);
void marcel_debug_exit(void);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_DEBUG_H__ **/
