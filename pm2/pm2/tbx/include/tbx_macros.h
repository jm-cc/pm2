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
$Log: tbx_macros.h,v $
Revision 1.1  2000/01/13 14:51:30  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_macros.h
 * ============
 */
#ifndef TBX_MACROS_H
#define TBX_MACROS_H

/* DEBUG: main debug flag */
/* #define DEBUG */

/* TIMING: main timing flag */
/* #define TIMING */

/* TRACING: main trace flag */
/* #define TRACING */

/* OOPS: causes FAILURE to generate a segfault instead of a call to exit */
#define OOPS

/*
 * Classical boolean type
 * ----------------------
 */
typedef enum
{
  tbx_false = 0,
  tbx_true
} tbx_bool_t, *p_tbx_bool_t;

/* DISP: display a string on stderr */
#define DISP(str) fprintf(stderr, str "\n")


/* CTRL: assertion verification macro. */
#define CTRL(op, val) \
  if((op) != (val))     \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   exit(1)


/* VERIFY: assertion verification macro.
   activated by DEBUG flag. */
#ifdef DEBUG
#define VERIFY(op, val) \
  if((op) != (val))     \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   exit(1)
#else /* DEBUG */
#define VERIFY(op, val) ((void)(op))
#endif /* DEBUG */


/* LOG */
#ifdef DEBUG
#define LOG(str) \
    fprintf(stderr, str "\n")
#else /* DEBUG */
#define LOG(str) 
#endif /* DEBUG */


/* LOG_IN */
#ifdef DEBUG
#define LOG_IN() \
    fprintf(stderr, __FUNCTION__": -->\n")
#else /* DEBUG */
#define LOG_IN() 
#endif /* DEBUG */


/* LOG_OUT */
#ifdef DEBUG
#define LOG_OUT() \
    fprintf(stderr, __FUNCTION__": <--\n")
#else /* DEBUG */
#define LOG_OUT() 
#endif /* DEBUG */


/* LOG_VAL */
#ifdef DEBUG
#define LOG_VAL(str, val) \
    fprintf(stderr, str " = %d\n", (int)(val))
#else /* DEBUG */
#define LOG_VAL(str, val) 
#endif /* DEBUG */


/* LOG_PTR */
#ifdef DEBUG
#define LOG_PTR(str, ptr) \
    fprintf(stderr, str " = %p\n", (void *)(ptr))
#else /* DEBUG */
#define LOG_PTR(str, ptr) 
#endif /* DEBUG */


/* LOG_STR */
#ifdef DEBUG
#define LOG_STR(str, str2) \
    fprintf(stderr, str " : %s\n", (char *)(str2))
#else /* DEBUG */
#define LOG_STR(str, str2) 
#endif /* DEBUG */


/* FAILURE: display an error message and abort the program */
#ifdef OOPS
#define FAILURE(str) \
  fprintf(stderr, "FAILURE: %s\nFILE: %s\nLINE: %d\n", \
            str, __FILE__, __LINE__),   *(int *)0 = 0,   exit(1)
#else /* OOPS */
#define FAILURE(str) \
  fprintf(stderr, "FAILURE: %s\nFILE: %s\nLINE: %d\n", \
            str, __FILE__, __LINE__),   exit(1)
#endif /* OOPS */

#define CTRL_ALLOC(ptr) \
  { \
    if (ptr == NULL) \
      { \
	FAILURE("not enough memory"); \
      } \
  }

/* max: maximum of a & b */
#define max(a, b) (((a) >= (b))?(a):(b))

/* min: minimum of a & b */
#define min(a, b) (((a) <= (b))?(a):(b))

/* flags */
#define tbx_set(f)    ((f) = tbx_flag_set)
#define tbx_clear(f)  ((f) = tbx_flag_clear)
#define tbx_toggle(f) ((f) = 1 - (f))
#define tbx_test(f)   (f)

/* PM2 specific macros */
#ifdef PM2
#define PM2_SHARED marcel_mutex_t __pm2_mutex
#define PM2_INIT_SHARED(st) marcel_mutex_init((&((st)->__pm2_mutex)), NULL)
#define PM2_LOCK_SHARED(st) marcel_mutex_lock((&((st)->__pm2_mutex)))
#define PM2_TRYLOCK_SHARED(st) marcel_mutex_trylock((&((st)->__pm2_mutex)))
#define PM2_UNLOCK_SHARED(st) marcel_mutex_unlock((&((st)->__pm2_mutex)))
#define PM2_LOCK() lock_task()
#define PM2_UNLOCK() unlock_task()
#define PM2_YIELD() marcel_givehandback()
#else /* PM2 */
#define PM2_SHARED 
#define PM2_INIT_SHARED(st) 
#define PM2_LOCK_SHARED(st) 
#define PM2_TRYLOCK_SHARED(st) 
#define PM2_UNLOCK_SHARED(st) 
#define PM2_LOCK()
#define PM2_UNLOCK()
#define PM2_YIELD()
#endif

/* Trace macros */
#ifdef TRACING
#define TRACE(str) \
    fprintf(stderr, str "\n")
#define TRACE_VAL(str, val) \
    fprintf(stderr, str " = %d\n", (int)(val))
#define TRACE_PTR(str, ptr) \
    fprintf(stderr, str " = %p\n", (void *)(ptr))
#define TRACE_STR(str, str2) \
    fprintf(stderr, str " : %s\n", (char *)(str2))
#else /* TRACING */
#define TRACE(str) 
#define TRACE_VAL(str, val) 
#define TRACE_PTR(str, ptr) 
#define TRACE_STR(str, str2) 
#endif /* TRACING */

/* Timing macros */
#ifdef TIMING
#define TIME_INIT() \
    TBX_GET_TICK(tbx_last_event);
#define TIME(str) \
    TBX_GET_TICK(tbx_new_event); \
    fprintf(stderr, str " [%4f usecs]\n", \
            tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event))); \
    TBX_GET_TICK(tbx_last_event);
#define TIME_VAL(str, val) \
    TBX_GET_TICK(tbx_new_event); \
    fprintf(stderr, str " = %d [%4f usecs]\n", (int)(val), \
            tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event))); \
    TBX_GET_TICK(tbx_last_event);
#define TIME_PTR(str, ptr) \
    TBX_GET_TICK(tbx_new_event); \
    fprintf(stderr, str " = %p [%4f usecs]\n", (void *)(ptr), \
            tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event))); \
    TBX_GET_TICK(tbx_last_event);
#define TIME_STR(str, str2) \
    TBX_GET_TICK(tbx_new_event); \
    fprintf(stderr, str " : %s [%4f usecs]\n", (char *)(str2, \
            tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event))); \
    TBX_GET_TICK(tbx_last_event);
#else /* TIMING */
#define TIME_INIT()
#define TIME(str) 
#define TIME_VAL(str, val) 
#define TIME_PTR(str, ptr) 
#define TIME_STR(str, str2) 
#endif /* TIMING */

#define tbx_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))

#endif /* TBX_MACROS_H */
