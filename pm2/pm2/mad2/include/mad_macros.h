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
$Log: mad_macros.h,v $
Revision 1.3  2000/01/04 16:47:30  oaumage
- ajout du flag OOPS et modification de la macro FAILURE:
  -> quand OOPS est defini, FAILURE genere un `segfault' pour arreter
     l'execution sous debugger

Revision 1.2  1999/12/15 17:31:23  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_macros.h
 * ============
 */
#ifndef MAD_MACROS_H
#define MAD_MACROS_H

/* DEBUG: main debug flag */
/* #define DEBUG */

/* TIMING: main timing flag */
/* #define TIMING */

/* TRACE: main trace flag */
/* #define TRACE */

/* OOPS: causes FAILURE to generate a segfault instead of a call to exit */
#define OOPS

/*
 * Classical boolean type
 * ----------------------
 */
typedef enum
{
  mad_false = 0,
  mad_true
} mad_bool_t, *p_mad_bool_t;

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

#ifdef DEBUG
#define LOG_IN() \
    fprintf(stderr, __FUNCTION__": -->\n")
#else /* DEBUG */
#define LOG_IN() 
#endif /* DEBUG */

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
  fprintf(stderr, "MADELEINE FAILURE: %s\nFILE: %s\nLINE: %d\n", \
            str, __FILE__, __LINE__),   *(int *)0 = 0,   exit(1)
#else /* OOPS */
#define FAILURE(str) \
  fprintf(stderr, "MADELEINE FAILURE: %s\nFILE: %s\nLINE: %d\n", \
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
#define mad_set(f)    ((f) = mad_flag_set)
#define mad_clear(f)  ((f) = mad_flag_clear)
#define mad_toggle(f) ((f) = 1 - (f))
#define mad_test(f)   (f)

/* PM2 specific macros */
#ifdef PM2
#define PM2_SHARED marcel_mutex_t __pm2_mutex
#define PM2_INIT_SHARED(st) marcel_mutex_init((&((st)->__pm2_mutex)), NULL)
#define PM2_LOCK_SHARED(st) marcel_mutex_lock((&((st)->__pm2_mutex)))
#define PM2_UNLOCK_SHARED(st) marcel_mutex_unlock((&((st)->__pm2_mutex)))
#define PM2_LOCK() lock_task()
#define PM2_UNLOCK() unlock_task()
#define PM2_YIELD() marcel_givehandback()
#else /* PM2 */
#define PM2_SHARED 
#define PM2_INIT_SHARED(st) 
#define PM2_LOCK_SHARED(st) 
#define PM2_UNLOCK_SHARED(st) 
#define PM2_LOCK()
#define PM2_UNLOCK()
#define PM2_YIELD()
#endif

/* Trace macros */
#ifdef TRACE
#define TRACE(str) \
    fprintf(stderr, str "\n")
#define TRACE_VAL(str, val) \
    fprintf(stderr, str " = %d\n", (int)(val))
#define TRACE_PTR(str, ptr) \
    fprintf(stderr, str " = %p\n", (void *)(ptr))
#define TRACE_STR(str, str2) \
    fprintf(stderr, str " : %s\n", (char *)(str2))
#else /* TRACE */
#define TRACE(str) 
#define TRACE_VAL(str, val) 
#define TRACE_PTR(str, ptr) 
#define TRACE_STR(str, str2) 
#endif /* TRACE */

/* Timing macros */
#ifdef TIMING
#define TIME_INIT() \
    MAD_GET_TICK(mad_last_event);
#define TIME(str) \
    MAD_GET_TICK(mad_new_event); \
    fprintf(stderr, str " [%4f usecs]\n", \
            mad_tick2usec(MAD_TICK_DIFF(mad_last_event, mad_new_event))); \
    MAD_GET_TICK(mad_last_event);
#define TIME_VAL(str, val) \
    MAD_GET_TICK(mad_new_event); \
    fprintf(stderr, str " = %d [%4f usecs]\n", (int)(val), \
            mad_tick2usec(MAD_TICK_DIFF(mad_last_event, mad_new_event))); \
    MAD_GET_TICK(mad_last_event);
#define TIME_PTR(str, ptr) \
    MAD_GET_TICK(mad_new_event); \
    fprintf(stderr, str " = %p [%4f usecs]\n", (void *)(ptr), \
            mad_tick2usec(MAD_TICK_DIFF(mad_last_event, mad_new_event))); \
    MAD_GET_TICK(mad_last_event);
#define TIME_STR(str, str2) \
    MAD_GET_TICK(mad_new_event); \
    fprintf(stderr, str " : %s [%4f usecs]\n", (char *)(str2, \
            mad_tick2usec(MAD_TICK_DIFF(mad_last_event, mad_new_event))); \
    MAD_GET_TICK(mad_last_event);
#else /* TIMING */
#define TIME_INIT()
#define TIME(str) 
#define TIME_VAL(str, val) 
#define TIME_PTR(str, ptr) 
#define TIME_STR(str, str2) 
#endif /* TIMING */

#define mad_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))

#endif /* MAD_MACROS_H */
