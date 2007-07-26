/*! \file tbx_debug.h
 *  \brief TBX debug support definitions and macros
 *
 *  This file contains the macros and support functions declaration for the
 *  TBX debugging facilities.
 *
 */

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#ifndef PM2DEBUG_EST_DEF
#define PM2DEBUG_EST_DEF
/** \defgroup debug_interface debug logs interface
 *
 * This is the debug logs interface
 *
 * @{
 */

#ifndef PM2DEBUG
/* for fprintf & stderr */
#include <stdio.h>
#endif

#include "pm2_profile.h"
#include "tbx_compiler.h"

typedef enum {
	PM2DEBUG_SHOW,
	PM2DEBUG_SHOW_PREFIX,
	PM2DEBUG_SHOW_FILE,
	PM2DEBUG_SHOW_THREAD,
	PM2DEBUG_SHOW_LWP,
	PM2DEBUG_SHOW_MAX,
} debug_action_t;

typedef struct struct_debug_type_t debug_type_t;

typedef void (*debug_action_func_t)(debug_type_t*, debug_action_t, int);

#define PM2DEBUG_AUTO_REGISTER 100
#define PM2DEBUG_DEFAULT -1

#define PM2DEBUG_ON 5
#define PM2DEBUG_OFF 0
#define PM2DEBUG_DEFAULTLEVEL  2

#define PM2DEBUG_STDLEVEL      4
#define PM2DEBUG_PM2DEBUGLEVEL 0
#define PM2DEBUG_DISPLEVEL     1
#define PM2DEBUG_WARNLEVEL     1
#define PM2DEBUG_LOGLEVEL      PM2DEBUG_STDLEVEL
#define PM2DEBUG_TRACELEVEL    PM2DEBUG_STDLEVEL

struct struct_debug_type_t {
	int show;
	int actions[PM2DEBUG_SHOW_MAX];
	const char* prefix;
	const char* option_name;
	debug_type_t *depend;
	int register_shown;
};

enum {
        PM2DEBUG_CLEAROPT=1,
        PM2DEBUG_DO_OPT=2,
};

#define NEW_DEBUG_TYPE_DEPEND(_prefix, _name, _dep) \
   {  .show=PM2DEBUG_AUTO_REGISTER, \
      .actions={PM2DEBUG_DEFAULT, \
		PM2DEBUG_DEFAULT, PM2DEBUG_DEFAULT, \
                PM2DEBUG_DEFAULT, PM2DEBUG_DEFAULT }, \
      .prefix=_prefix, \
      .option_name=_name, \
      .depend=_dep, \
      .register_shown=0, \
   }

#define NEW_DEBUG_TYPE(_prefix, _name) \
  NEW_DEBUG_TYPE_DEPEND(_prefix, _name, NULL)

extern debug_type_t debug_pm2debug;

enum {
	PM2DEBUG_MARCEL_PRINTF_DENY,
	PM2DEBUG_MARCEL_PRINTF_ALLOWED
};

void pm2debug_printf_state(int state);

#define PM2DEBUG_MAXLINELEN 512

#ifdef PM2DEBUG

void pm2debug_init_ext(int *argc, char **argv, int debug_flags);
int TBX_FORMAT (printf, 5, 6) 
pm2debug_printf(debug_type_t *type, int level, int line, const char* file,
		const char *format, ...);
void pm2debug_register(debug_type_t *type);
void pm2debug_setup(debug_type_t* type, debug_action_t action, int value);
#define debug_printfl(type, level, fmt, ...) \
   ({ register int _level=(level); \
   ((type != NULL) && ((type)->show >= _level) ? \
    pm2debug_printf(type, _level, __LINE__, __FILE__, fmt , ##__VA_ARGS__) \
    : 0 ); \
   })
#define pm2debug(fmt, ...) \
   debug_printfl(&debug_pm2debug, PM2DEBUG_PM2DEBUGLEVEL, fmt , ##__VA_ARGS__)

#else /* PM2DEBUG */

#define pm2debug_init_ext(argc, argv, debug_flags)
#define pm2debug_printf(type, level, line, file, ...) ((void)0)
#define pm2debug_register(type)
#define pm2debug_setup(type, action, value)
#define debug_printfl(type, level, fmt, ...) ((void)0)
#define pm2debug(fmt, ...) fprintf(stderr, fmt , ##__VA_ARGS__)

#endif /* PM2DEBUG */

int pm2debug_backtrace(void ** array, int size);

#define debug_printf(type, fmt, ...) \
   debug_printfl(type, PM2DEBUG_STDLEVEL, fmt , ##__VA_ARGS__)
#define pm2debug_init(argc, argv) \
   pm2debug_init_ext((argc), (argv), PM2DEBUG_CLEAROPT|PM2DEBUG_DO_OPT)


#ifndef DEBUG_NAME_MODULE
#  ifndef MODULE
#    define DEBUG_NAME_MODULE app
#  else
#    define DEBUG_NAME_MODULE MODULE
#  endif // MODULE
#endif // DEBUG_NAME
#ifndef DEBUG_NAME
#  define DEBUG_NAME DEBUG_NAME_MODULE
#endif

#define CONCAT3(a,b,c) subconcat3(a,b,c)
#define subconcat3(a,b,c) a##b##c
#ifndef DEBUG_NAME_DISP
#define DEBUG_NAME_DISP(DEBUG_NAME) CONCAT3(debug_, DEBUG_NAME, _disp)
#endif
#ifndef DEBUG_NAME_LOG
#define DEBUG_NAME_LOG(DEBUG_NAME) CONCAT3(debug_, DEBUG_NAME, _log)
#endif
#ifndef DEBUG_NAME_TRACE
#define DEBUG_NAME_TRACE(DEBUG_NAME) CONCAT3(debug_, DEBUG_NAME, _trace)
#endif
#ifndef DEBUG_NAME_WARN
#define DEBUG_NAME_WARN(DEBUG_NAME) CONCAT3(debug_, DEBUG_NAME, _warn)
#endif

extern debug_type_t debug_disp;
extern debug_type_t debug_log;
extern debug_type_t debug_trace;
extern debug_type_t debug_warn;

extern debug_type_t DEBUG_NAME_DISP(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_LOG(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_WARN(DEBUG_NAME);

#if defined(PM2DEBUG) || defined(TBX_KERNEL)

#define DEBUG_DECLARE(DEBUG_NAME) \
debug_type_t DEBUG_NAME_DISP(DEBUG_NAME)= \
  NEW_DEBUG_TYPE_DEPEND(#DEBUG_NAME "-disp: ", \
		        #DEBUG_NAME "-disp", &debug_disp); \
debug_type_t DEBUG_NAME_LOG(DEBUG_NAME)= \
  NEW_DEBUG_TYPE_DEPEND(#DEBUG_NAME "-log: ", \
		        #DEBUG_NAME "-log", &debug_log); \
debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME)= \
  NEW_DEBUG_TYPE_DEPEND(#DEBUG_NAME "-trace: ", \
		        #DEBUG_NAME "-trace", &debug_trace); \
debug_type_t DEBUG_NAME_WARN(DEBUG_NAME)= \
  NEW_DEBUG_TYPE_DEPEND(#DEBUG_NAME "-warn: ", \
		        #DEBUG_NAME "-warn", &debug_warn);
#else /* PM2DEBUG */

#define DEBUG_DECLARE(DEBUG_NAME)

#endif /* PM2DEBUG */

#define DEBUG_INIT(DEBUG_NAME) \
  { pm2debug_register(&DEBUG_NAME_DISP(DEBUG_NAME)); \
    pm2debug_register(&DEBUG_NAME_LOG(DEBUG_NAME)); \
    pm2debug_register(&DEBUG_NAME_TRACE(DEBUG_NAME)); \
    pm2debug_register(&DEBUG_NAME_WARN(DEBUG_NAME)); }

/*
 * Display  macros  _________________________________________________
 * ________________//////////////////////////////////////////////////
 */
#if defined(PM2DEBUG)
#define DISP(str, ...)       debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   str "\n" , ## __VA_ARGS__)
#define DISP_NO_NL(str, ...) debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   str , ## __VA_ARGS__)
#define DISP_IN()            debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   __TBX_FUNCTION__": -->\n")
#define DISP_OUT()           debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   __TBX_FUNCTION__": <--\n")
#define DISP_VAL(str, val)   debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   str " = %d\n" , (int)(val))
#define DISP_CHAR(val)       debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   "%c" , (char)(val))
#define DISP_PTR(str, ptr)   debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   str " = %p\n" , (void *)(ptr))
#define DISP_STR(str, str2)  debug_printfl(&DEBUG_NAME_DISP(DEBUG_NAME), \
                                           PM2DEBUG_DISPLEVEL, \
					   str ": %s\n" , (char *)(str2))
#else /* PM2DEBUG */

#define DISP(str, ...)       fprintf(stderr, str "\n" , ## __VA_ARGS__)
#define DISP_NO_NL(str, ...) fprintf(stderr, str, ## __VA_ARGS__)
#define DISP_IN()            fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define DISP_OUT()           fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define DISP_VAL(str, val)   fprintf(stderr, str " = %d\n" , (int)(val))
#define DISP_CHAR(val)       fprintf(stderr, "%c" , (char)(val))
#define DISP_PTR(str, ptr)   fprintf(stderr, str " = %p\n" , (void *)(ptr))
#define DISP_STR(str, str2)  fprintf(stderr, str ": %s\n" , (char *)(str2))

#endif /* PM2DEBUG */


/*
 * Logging macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */

#if defined(PM2DEBUG)

#define LOG(str, ...)         debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
                                           str "\n" , ## __VA_ARGS__)

#if defined(__PROF_APP__) || defined(NO_IMPLICIT_PROFILE)
#define LOG_IN()              do { debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   "%s: -->\n", __TBX_FUNCTION__); \
                              } while(0)

#define LOG_OUT()             do { debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   "%s: <--\n", __TBX_FUNCTION__); \
                              } while(0)
#else // __PROF_APP__
#define LOG_IN()              do { debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   "%s: -->\n", __TBX_FUNCTION__); \
                                   PROF_IN(); \
                              } while(0)

#define LOG_OUT()             do { debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   "%s: <--\n", __TBX_FUNCTION__); \
                                   PROF_OUT(); \
                              } while(0)
#endif // __PROF_APP__

#define LOG_RETURN(val)       do { __typeof__(val) _ret=(val) ; \
                                   LOG_OUT() ;\
                                   return _ret; \
                              } while (0)
#define LOG_CHAR(val)         debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   "%c" , (char)(val))
#define LOG_VAL(str, val)     debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   str " = %d\n" , (int)(val))
#define LOG_PTR(str, ptr)     debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   str " = %p\n" , (void *)(ptr))
#define LOG_STR(str, str2)    debug_printfl(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           PM2DEBUG_LOGLEVEL, \
					   str ": %s\n" , (char *)(str2))

#else // else if not PM2DEBUG
#define LOG(str, ...)      
#if defined (__PROF_APP__) || defined(NO_IMPLICIT_PROFILE)
#define LOG_IN()           (void)(0)
#define LOG_OUT()          (void)(0)
#define LOG_RETURN(val)    return (val)
#else
#define LOG_IN()           PROF_IN()
#define LOG_OUT()          PROF_OUT()
#define LOG_RETURN(val)    do { PROF_OUT(); return (val); } while (0)
#endif
#define LOG_CHAR(val)      (void)(0)
#define LOG_VAL(str, val)  (void)(0)
#define LOG_PTR(str, ptr)  (void)(0)
#define LOG_STR(str, str2) (void)(0)

#endif // PM2DEBUG


/*
 * Tracing macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */
#ifdef PM2DEBUG

#define TRACE(str, ...)       debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   str "\n" , ## __VA_ARGS__)
#define TRACE_IN()            debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   "%s: -->\n", __TBX_FUNCTION__)
#define TRACE_OUT()           debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   "%s: <--\n", __TBX_FUNCTION__)
#define TRACE_VAL(str, val)   debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   str " = %d\n" , (int)(val))
#define TRACE_CHAR(val)       debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   "%c" , (char)(val))
#define TRACE_PTR(str, ptr)   debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   str " = %p\n" , (void *)(ptr))
#define TRACE_STR(str, str2)  debug_printfl(&DEBUG_NAME_TRACE(DEBUG_NAME), \
                                           PM2DEBUG_TRACELEVEL, \
					   str ": %s\n" , (char *)(str2))
#else /* PM2DEBUG */

#define TRACE(str, ...)      (void)(0)
#define TRACE_IN()           (void)(0)
#define TRACE_OUT()          (void)(0)
#define TRACE_VAL(str, val)  (void)(0)
#define TRACE_CHAR(val)      (void)(0)
#define TRACE_PTR(str, ptr)  (void)(0)
#define TRACE_STR(str, str2) (void)(0)

#endif /* PM2DEBUG */

/*
 * Warning  macros  _________________________________________________
 * ________________//////////////////////////////////////////////////
 */
#if defined(PM2DEBUG)
#define WARN(str, ...)       debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   str "\n" , ## __VA_ARGS__)
#define WARN_NO_NL(str, ...) debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   str , ## __VA_ARGS__)
#define WARN_IN()            debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   __TBX_FUNCTION__": -->\n")
#define WARN_OUT()           debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   __TBX_FUNCTION__": <--\n")
#define WARN_VAL(str, val)   debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   str " = %d\n" , (int)(val))
#define WARN_CHAR(val)       debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   "%c" , (char)(val))
#define WARN_PTR(str, ptr)   debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   str " = %p\n" , (void *)(ptr))
#define WARN_STR(str, str2)  debug_printfl(&DEBUG_NAME_WARN(DEBUG_NAME), \
                                           PM2DEBUG_WARNLEVEL, \
					   str ": %s\n" , (char *)(str2))
#else /* PM2DEBUG */

#define WARN(str, ...)       fprintf(stderr, str "\n" , ## __VA_ARGS__)
#define WARN_NO_NL(str, ...) fprintf(stderr, str, ## __VA_ARGS__)
#define WARN_IN()            fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define WARN_OUT()           fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define WARN_VAL(str, val)   fprintf(stderr, str " = %d\n" , (int)(val))
#define WARN_CHAR(val)       fprintf(stderr, "%c" , (char)(val))
#define WARN_PTR(str, ptr)   fprintf(stderr, str " = %p\n" , (void *)(ptr))
#define WARN_STR(str, str2)  fprintf(stderr, str ": %s\n" , (char *)(str2))

#endif /* PM2DEBUG */

/* @} */
#endif
