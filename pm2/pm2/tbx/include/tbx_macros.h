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
Revision 1.13  2000/07/11 09:01:26  oaumage
- Suppression de la collision sur la macro CTRL
- Suppression de la collision potentielle sur la macro VERIFY

Revision 1.12  2000/06/09 18:15:04  vdanjean
minor update

Revision 1.11  2000/06/08 13:57:09  oaumage
- Macros d'attributs

Revision 1.10  2000/06/05 15:41:58  vdanjean
adaptation of debug messages

Revision 1.9  2000/05/25 00:23:58  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.8  2000/05/18 13:27:52  oaumage
- correction de l'expansion conditionnelle des macros de synchronisation
  multithread

Revision 1.7  2000/04/27 09:05:52  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.6  2000/04/20 12:17:39  oaumage
- corrections diverses

Revision 1.5  2000/04/05 16:10:09  oaumage
- remplacement de marcel_givehandback par marcel_yield

Revision 1.4.2.2  2000/04/03 08:55:51  oaumage
- ajout d'une constante (taille des blocs lors d'un transfert de fichier)

Revision 1.4.2.1  2000/03/30 14:13:51  oaumage
- correction d'un bug au niveau de la macro DISP (il faut un espace avant la ,)

Revision 1.4  2000/03/13 09:48:18  oaumage
- ajout de l'option TBX_SAFE_MALLOC
- support de safe_malloc
- extension des macros de logging

Revision 1.3  2000/03/08 17:16:03  oaumage
- support de Marcel sans PM2
- support de tmalloc en mode `Marcel'

Revision 1.2  2000/01/31 15:59:11  oaumage
- detection de l'absence de GCC
- ajout de aligned_malloc

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

#include "pm2debug.h"

/*
 * Classical boolean type
 * ----------------------
 */
typedef enum
{
  tbx_false = 0,
  tbx_true
} tbx_bool_t, *p_tbx_bool_t;


/*
 * Constant definition  _____________________________________________
 * ____________________//////////////////////////////////////////////
 */
#define TBX_FILE_TRANSFER_BLOCK_SIZE 1024


/*
 * Timing macros  __________________________________________________
 * ______________///////////////////////////////////////////////////
 */
#ifdef TIMING
#define _TBXT_PRE  TBX_GET_TICK(tbx_new_event)
#define _TBXT_DIFF tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event)
#define _TBXT_POST TBX_GET_TICK(tbx_last_event)

#define TIME_INIT() _TBXT_POST
#define TIME(str)
    _TBXT_PRE; fprintf(stderr, str " [%4f usecs]\n", _TBXT_DIFF); _TBXT_POST
#define TIME_VAL(str, val) \
    _TBXT_PRE; \
    fprintf(stderr, str " = %d [%4f usecs]\n", (int)(val), _TBXT_DIFF); \
    _TBXT_POST
#define TIME_PTR(str, ptr) \
    _TBXT_PRE; \
    fprintf(stderr, str " = %p [%4f usecs]\n", (void *)(ptr), _TBXT_DIFF); \
    _TBXT_POST
#define TIME_STR(str, str2) \
    _TBXT_PRE; \
    fprintf(stderr, str " : %s [%4f usecs]\n", (char *)(str2), _TBXT_DIFF); \
    _TBXT_POST
#else /* TIMING */
#define TIME_INIT()
#define TIME(str) 
#define TIME_VAL(str, val) 
#define TIME_PTR(str, ptr) 
#define TIME_STR(str, str2) 
#endif /* TIMING */


/*
 * Control macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */

/*
 * CTRL: assertion verification macro
 * ----------------------------------
 */
#define TBX_CTRL(op, val) \
  if((op) != (val))     \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   exit(1)

/*
 * VERIFY: assertion verification macro activated by DEBUG flag
 * ------------------------------------------------------------
 */
#ifdef DEBUG
#define TBX_VERIFY(op, val) \
  if((op) != (val))     \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   exit(1)
#else /* DEBUG */
#define TBX_VERIFY(op, val) ((void)(op))
#endif /* DEBUG */

/*
 * FAILURE: display an error message and abort the program
 * -------------------------------------------------------
 */
#ifdef OOPS
#define FAILURE(str) \
  pm2debug_flush(), fprintf(stderr, "FAILURE: %s\nFILE: %s\nLINE: %d\n", \
            (str), __FILE__, __LINE__),   abort()
#else /* OOPS */
#define FAILURE(str) \
  pm2debug_flush(), fprintf(stderr, "FAILURE: %s\nFILE: %s\nLINE: %d\n", \
            (str), __FILE__, __LINE__),   exit(1)
#endif /* OOPS */

/*
 * CTRL_ALLOC
 * ----------
 */
#define CTRL_ALLOC(ptr) \
  { \
    if ((ptr) == NULL) \
      { \
	FAILURE("not enough memory"); \
      } \
  }


/*
 * Min/Max macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */
#define max(a, b) (((a) >= (b))?(a):(b))
#define min(a, b) (((a) <= (b))?(a):(b))


/*
 * Flags control macros  ____________________________________________
 * _____________________/////////////////////////////////////////////
 */
#define tbx_set(f)    ((f) = tbx_flag_set)
#define tbx_clear(f)  ((f) = tbx_flag_clear)
#define tbx_toggle(f) ((f) = 1 - (f))
#define tbx_test(f)   (f)


/*
 * Threads specific macros  _________________________________________
 * ________________________//////////////////////////////////////////
 */
#ifdef MARCEL
#define TBX_SHARED marcel_mutex_t __pm2_mutex
#define TBX_INIT_SHARED(st) marcel_mutex_init((&((st)->__pm2_mutex)), NULL)
#define TBX_LOCK_SHARED(st) marcel_mutex_lock((&((st)->__pm2_mutex)))
#define TBX_TRYLOCK_SHARED(st) marcel_mutex_trylock((&((st)->__pm2_mutex)))
#define TBX_UNLOCK_SHARED(st) marcel_mutex_unlock((&((st)->__pm2_mutex)))
#define TBX_LOCK() lock_task()
#define TBX_UNLOCK() unlock_task()
#define TBX_YIELD() marcel_yield()
#else /* MARCEL */
#define TBX_SHARED 
#define TBX_INIT_SHARED(st) 
#define TBX_LOCK_SHARED(st) 
#define TBX_TRYLOCK_SHARED(st) 
#define TBX_UNLOCK_SHARED(st) 
#define TBX_LOCK()
#define TBX_UNLOCK()
#define TBX_YIELD()
#endif /* MARCEL */


/*
 * Memory allocation macros  ________________________________________
 * _________________________/////////////////////////////////////////
 */
#ifdef MARCEL
#define TBX_MALLOC(s)     tmalloc  ((s))
#define TBX_CALLOC(n, s)  tcalloc  ((n), (s))
#define TBX_REALLOC(p, s) trealloc ((p), (s))
#define TBX_FREE(s)       tfree    ((s))
#ifdef TBX_SAFE_MALLOC
#define __TBX_MALLOC(s)     tbx_safe_malloc  ((s), __FILE__, __LINE__)
#define __TBX_CALLOC(n, s)  tbx_safe_calloc  ((n), (s), __FILE__, __LINE__)
#define __TBX_REALLOC(p, s) tbx_safe_realloc ((p), (s), __FILE__, __LINE__)
#define __TBX_FREE(s)       tbx_safe_free    ((s), __FILE__, __LINE__)
#else  /* TBX_SAFE_MALLOC */
#define __TBX_MALLOC(s)     malloc  ((s))
#define __TBX_CALLOC(n, s)  calloc  ((n), (s))
#define __TBX_REALLOC(p, s) realloc ((p), (s))
#define __TBX_FREE(s)       free    ((s))
#endif /* TBX_SAFE_MALLOC */
#else /* MARCEL */
#ifdef TBX_SAFE_MALLOC
#define TBX_MALLOC(s)     tbx_safe_malloc  ((s), __FILE__, __LINE__)
#define TBX_CALLOC(n, s)  tbx_safe_calloc  ((n), (s), __FILE__, __LINE__)
#define TBX_REALLOC(p, s) tbx_safe_realloc ((p), (s), __FILE__, __LINE__)
#define TBX_FREE(s)       tbx_safe_free    ((s), __FILE__, __LINE__)
#else /* TBX_SAFE_MALLOC */
#define TBX_MALLOC(s)     malloc  ((s))
#define TBX_CALLOC(n, s)  calloc  ((n), (s))
#define TBX_REALLOC(p, s) realloc ((p), (s))
#define TBX_FREE(s)       free    ((s))
#endif /* TBX_SAFE_MALLOC */
#endif /* MARCEL */


/*
 * Alignment macros  ________________________________________________
 * _________________/////////////////////////////////////////////////
 */
#define tbx_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))
#define TBX_ALIGN(a)  __attribute__ ((__aligned__ (a)))

/*
 * Attribute macros  ________________________________________________
 * _________________/////////////////////////////////////////////////
 */
#define TBX_BYTE        __attribute__ ((__mode__ (__byte__)))
#define TBX_WORD        __attribute__ ((__mode__ (__word__)))
#define TBX_POINTER     __attribute__ ((__mode__ (__pointer__)))
#define TBX_ALIGNED     __attribute__ ((__aligned__))
#define TBX_PACKED      __attribute__ ((__packed__))
#define TBX_UNUSED      __attribute__ ((__unused__))
#define TBX_NORETURN    __attribute__ ((__noreturn__))
#define TBX_CONST       __attribute__ ((__const__))

/*
 * System calls wrappers  ___________________________________________
 * ______________________////////////////////////////////////////////
 */

/*
 * SYSCALL
 * -------
 */
#define SYSCALL(op) \
  while ((op) == -1) \
  { \
    if (errno != EINTR) \
      { \
	perror(#op); \
	FAILURE("system call failed"); \
      } \
  }

/*
 * SYSTEST
 * -------
 */
#define SYSTEST(op) \
  if ((op) == -1) \
  { \
    if (errno != EINTR) \
      { \
	perror(#op); \
	FAILURE("system call failed"); \
      } \
  }

#endif /* TBX_MACROS_H */
