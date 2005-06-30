
/*
 * CVS Id: $Id: trace_debug.h,v 1.1 2002/10/24 12:58:34 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA, May 2002 */

/**********************************************************************/
/* TRACING AND DEBUGGING SYSTEM                                       */
/**********************************************************************/

/* Before include this header file, TRCSL, DBGSL, NDEBUG must have
 * been #define'd or #undef'd.  If TRCSL is defined, the TRACE
 * messages are printed.  If DBGSL is defined, a message is printed
 * while entering / exiting a function.  If NDEBUG is defined, the
 * assertions "assert()" are replaced with NOPs (for better
 * performance). */


#ifndef TRACE_DEBUG_H
#define TRACE_DEBUG_H


#include "marcel_stdio.h"   /* for marcel_fprintf() */

#include <assert.h>


#undef PROFIN
#undef PROFOUT
#define PROFIN(x) ((void)0)
#define PROFOUT(x) ((void)0)

#if defined(TRCSL) || defined(DBGSL)
static int depth = 0;
static void spaces (const int i) { int j; for (j=0; j<i; j++) marcel_fprintf(stderr, "   "); return; }
#endif

#undef IN
#undef OUT
#ifdef DBGSL
#define IN { spaces(depth++); marcel_fprintf(stderr, "--> [%d:%x:%s]\n", pm2_self(), marcel_self(), __FUNCTION__); }
#define OUT { spaces(--depth); marcel_fprintf(stderr, "[%d:%x:%s] -->\n", pm2_self(), marcel_self(), __FUNCTION__); }
#else
#define IN ((void)0)
#define OUT ((void)0)
#endif

#undef TRACE
#ifdef TRCSL
#define TRACE(strng, ...) { spaces(depth); marcel_fprintf(stderr, "[%d:%x:%s] " strng "\n", pm2_self(), marcel_self(), __FUNCTION__, ##__VA_ARGS__); }
#else
#define TRACE(...) ((void)0)
#endif

/* special trace, always active */
#define STRACE(strng, ...) { marcel_fprintf(stderr, "[%d:%x:%s] *** " strng "\n", pm2_self(), marcel_self(), __FUNCTION__, ##__VA_ARGS__); }

#endif   /* TRACE_DEBUG_H */

