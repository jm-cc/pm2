/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: pm2debug.h,v $
Revision 1.12  2000/09/18 14:22:43  rnamyst
Few re-arrangements in .h files

Revision 1.11  2000/09/14 02:08:27  rnamyst
Put profile.h into common/include and added few FUT_SWITCH_TO calls

Revision 1.10  2000/09/13 21:57:35  rnamyst
Improvements to the profile stuff: .fut files are now only built for modules that have the -DDO_PROFILE option set.

Revision 1.9  2000/09/13 00:07:17  rnamyst
Support for profiling + minor bug fixes

Revision 1.8  2000/09/12 16:59:09  rnamyst
Few bug fixes + support for .fut files

Revision 1.7  2000/09/12 14:55:13  rnamyst
Added support for generating .i files in Makefiles

Revision 1.6  2000/06/13 12:32:50  vdanjean
debug_flush

Revision 1.5  2000/06/05 15:31:54  vdanjean
adaptation of debug messages

Revision 1.4  2000/05/25 00:23:56  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.3  2000/05/15 14:06:26  vdanjean
syntaxe bug fixes

Revision 1.2  2000/05/10 13:08:03  vdanjean
minor bugs fixes

Revision 1.1  2000/05/09 10:49:36  vdanjean
pm2debug module

Revision 1.4  2000/04/28 18:33:36  vdanjean
debug actsmp + marcel_key

Revision 1.2  2000/04/11 09:07:17  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/03/31 09:04:12  rnamyst
This is the old "debug.h" file.

______________________________________________________________________________
*/

#ifndef PM2DEBUG_EST_DEF
#define PM2DEBUG_EST_DEF

#include "profile.h"

typedef enum {
	DEBUG_SHOW,
	DEBUG_SHOW_PREFIX,
	DEBUG_SHOW_FILE,
	DEBUG_CRITICAL,
	DEBUG_TRYONLY,
	DEBUG_REGISTER,
} debug_action_t;

typedef struct struct_debug_type_t debug_type_t;

typedef void (*debug_action_func_t)(debug_type_t*, debug_action_t, int);

struct struct_debug_type_t {
	int show;
	char* prefix;
	char* option_name;
	int show_prefix;
	int show_file;
	int critical;
	int try_only;
	debug_action_func_t setup;
	void* data;
};

enum {
        PM2DEBUG_CLEAROPT=1,
        PM2DEBUG_DO_OPT=2,
};

#define NEW_DEBUG_TYPE(show, prefix, name) \
   {show, prefix, name, 0, 0, 0, 0, NULL, NULL}

extern debug_type_t debug_type_default;
void debug_setup_default(debug_type_t*, debug_action_t, int);

#ifdef PM2DEBUG
#ifdef MARCEL
extern int pm2debug_marcel_launched;
#endif
void pm2debug_init_ext(int *argc, char **argv, int debug_flags);
int pm2debug_printf(debug_type_t *type, int line, char* file, 
		     const char *format, ...);
void pm2debug_register(debug_type_t *type);
void pm2debug_setup(debug_type_t* type, debug_action_t action, int value);
void pm2debug_flush();
#else /* PM2DEBUG */
#define pm2debug_init_ext(argc, argv, debug_flags)
#define pm2debug_printf(type, line, file, format...)
#define pm2debug_register(type)
#define pm2debug_setup(type, action, value)
#define pm2debug_flush() ((void)0)
#endif

#define pm2debug_init(argc, argv) \
   pm2debug_init_ext((argc), (argv), PM2DEBUG_CLEAROPT|PM2DEBUG_DO_OPT)
#define debug_printf(type, fmt, args...) \
   ((type) && ((type)->show) ? \
    pm2debug_printf(type, __LINE__, __BASE_FILE__, fmt, ##args) \
    : (void)0 )

#define debug(fmt, args...) \
   debug_printf(&debug_type_default, fmt, ##args)

#endif /* PM2DEBUG */

#ifdef PM2DEBUG
#ifndef DEBUG_NAME
#ifndef MODULE
#define DEBUG_NAME app
#else
#define DEBUG_NAME MODULE
#endif
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

extern debug_type_t DEBUG_NAME_DISP(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_LOG(DEBUG_NAME);
extern debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME);

#define DEBUG_DECLARE(DEBUG_NAME) \
debug_type_t DEBUG_NAME_DISP(DEBUG_NAME)= \
  NEW_DEBUG_TYPE(1, #DEBUG_NAME "-disp: ", \
		    #DEBUG_NAME "-disp"); \
debug_type_t DEBUG_NAME_LOG(DEBUG_NAME)= \
  NEW_DEBUG_TYPE(0, #DEBUG_NAME "-log: ", \
		    #DEBUG_NAME "-log"); \
debug_type_t DEBUG_NAME_TRACE(DEBUG_NAME)= \
  NEW_DEBUG_TYPE(0, #DEBUG_NAME "-trace: ", \
		    #DEBUG_NAME "-trace"); 
#define DEBUG_INIT(DEBUG_NAME) \
  { pm2debug_register(&DEBUG_NAME_DISP(DEBUG_NAME)); \
    pm2debug_register(&DEBUG_NAME_LOG(DEBUG_NAME)); \
    pm2debug_register(&DEBUG_NAME_TRACE(DEBUG_NAME)); }

#else /* PM2DEBUG */
#define DEBUG_DECLARE(DEBUG_NAME)
#endif /* PM2DEBUG */

/*
 * Display  macros  _________________________________________________
 * ________________//////////////////////////////////////////////////
 */
#if defined(PM2DEBUG)
#define DISP(str, args...)   debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   str "\n" , ## args)
#define DISP_IN()            debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   __FUNCTION__": -->\n")
#define DISP_OUT()           debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   __FUNCTION__": <--\n")
#define DISP_VAL(str, val)   debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   str " = %d\n" , (int)(val))
#define DISP_PTR(str, ptr)   debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   str " = %p\n" , (void *)(ptr))
#define DISP_STR(str, str2)  debug_printf(&DEBUG_NAME_DISP(DEBUG_NAME), \
					   str " : %s\n" , (char *)(str2))
#else /* PM2DEBUG */
#define DISP(str, args...)
#define DISP_IN()
#define DISP_OUT()
#define DISP_VAL(str, val)
#define DISP_PTR(str, ptr)
#define DISP_STR(str, str2)
#endif /* DEBUG */


/*
 * Logging macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */

#if defined(PM2DEBUG)
#ifdef DEBUG
#define LOG(str, args...)     debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
                                           str "\n" , ## args)
#define LOG_IN()              do { debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
					   __FUNCTION__": -->\n"); \
                              PROF_IN(); } while(0)
#define LOG_OUT()             do { debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
					   __FUNCTION__": <--\n"); \
                              PROF_OUT(); } while(0)
#define LOG_VAL(str, val)     debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
					   str " = %d\n" , (int)(val))
#define LOG_PTR(str, ptr)     debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
					   str " = %p\n" , (void *)(ptr))
#define LOG_STR(str, str2)    debug_printf(&DEBUG_NAME_LOG(DEBUG_NAME), \
					   str " : %s\n" , (char *)(str2))
#else // if not DEBUG

#define LOG(str, args...) 
#define LOG_IN()              PROF_IN()
#define LOG_OUT()             PROF_OUT()
#define LOG_VAL(str, val) 
#define LOG_PTR(str, ptr) 
#define LOG_STR(str, str2)

#endif

#else // else if not PM2DEBUG

#define LOG(str, args...) 
#define LOG_IN()              PROF_IN()
#define LOG_OUT()             PROF_OUT()
#define LOG_VAL(str, val) 
#define LOG_PTR(str, ptr) 
#define LOG_STR(str, str2)

#endif // PM2DEBUG


/*
 * Tracing macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */
#if defined(PM2DEBUG) && defined(DEBUG)
#define TRACE(str, args...)   debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   str "\n" , ## args)
#define TRACE_IN()            debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   __FUNCTION__": -->\n")
#define TRACE_OUT()           debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   __FUNCTION__": <--\n")
#define TRACE_VAL(str, val)   debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   str " = %d\n" , (int)(val))
#define TRACE_PTR(str, ptr)   debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   str " = %p\n" , (void *)(ptr))
#define TRACE_STR(str, str2)  debug_printf(&DEBUG_NAME_TRACE(DEBUG_NAME), \
					   str " : %s\n" , (char *)(str2))
#else /* PM2DEBUG */
#define TRACE(str, args...) 
#define TRACE_IN()
#define TRACE_OUT()
#define TRACE_VAL(str, val) 
#define TRACE_PTR(str, ptr) 
#define TRACE_STR(str, str2) 
#endif /* PM2DEBUG */






