
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
    _TBXT_PRE; pm2fulldebug(str " [%4f usecs]\n", _TBXT_DIFF); _TBXT_POST
#define TIME_IN()  TIME_INIT()
#define TIME_OUT() TIME(__FUNCTION__)
#define TIME_VAL(str, val) \
    _TBXT_PRE; \
    pm2fulldebug(str " = %d [%4f usecs]\n", (int)(val), _TBXT_DIFF); \
    _TBXT_POST
#define TIME_PTR(str, ptr) \
    _TBXT_PRE; \
    pm2fulldebug(str " = %p [%4f usecs]\n", (void *)(ptr), _TBXT_DIFF); \
    _TBXT_POST
#define TIME_STR(str, str2) \
    _TBXT_PRE; \
    pm2fulldebug(str " : %s [%4f usecs]\n", (char *)(str2), _TBXT_DIFF); \
    _TBXT_POST
#else /* TIMING */
#define TIME_INIT()
#define TIME(str) 
#define TIME_IN()
#define TIME_OUT()
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
    pm2fulldebug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()

/*
 * VERIFY: assertion verification macro activated by PM2DEBUG flag
 * ------------------------------------------------------------
 */
#ifdef PM2DEBUG
#define TBX_VERIFY(op, val) \
  if((op) != (val))     \
    pm2fulldebug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()
#else /* DEBUG */
#define TBX_VERIFY(op, val) ((void)(op))
#endif /* DEBUG */

/*
 * FAILURE: display an error message and abort the program
 * -------------------------------------------------------
 */
#ifdef OOPS
#define FAILURE(str) do { \
  pm2debug_flush(); \
  pm2fulldebug("FAILURE: %s\n", (str)); \
  abort(); \
} while(0)
#define ERROR(str) do { \
  pm2debug_flush(); \
  pm2fulldebug("FAILURE: %s: %s\n\n", (str), strerror(errno)); \
  abort(); \
} while(0)
#else /* OOPS */
#define FAILURE(str) do { \
  pm2debug_flush(); \
  pm2fulldebug("FAILURE: %s\n", (str)); \
  exit(1); \
} while(0)
#define ERROR(str) do { \
  pm2debug_flush(); \
  pm2fulldebug("FAILURE: %s: %s\n\n", (str), strerror(errno)); \
  exit(1); \
} while(0)
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
#ifdef __GNUC__
#define max(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a > _b ? _a : _b; })
#define min(a,b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a < _b ? _a : _b; })
#else // __GNUC__
#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))
#endif // __GNUC__

/*
 * Flags control macros  ____________________________________________
 * _____________________/////////////////////////////////////////////
 */

#ifdef __GNUC__
#define tbx_set(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_set;})
#define tbx_clear(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_clear;})
#define tbx_toggle(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = 1 - *_pf;})
#define tbx_test(f)   (*(f))
/*
#define tbx_test(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf;})
*/
#else // __GNUC__ 
#define tbx_set(f)    ((*(f)) = tbx_flag_set)
#define tbx_clear(f)  ((*(f)) = tbx_flag_clear)
#define tbx_toggle(f) ((*(f)) = 1 - (*(f)))
#define tbx_test(f)   (*(f))
#endif // __GNUC__

/*
 * Threads specific macros  _________________________________________
 * ________________________//////////////////////////////////////////
 */
#if defined (MARCEL)       //# Thread sync : Marcel mode
#define TBX_SHARED             marcel_mutex_t __tbx_mutex
#define TBX_INIT_SHARED(st)    marcel_mutex_init((&((st)->__tbx_mutex)), NULL)
#define TBX_LOCK_SHARED(st)    marcel_mutex_lock((&((st)->__tbx_mutex)))
#define TBX_TRYLOCK_SHARED(st) marcel_mutex_trylock((&((st)->__tbx_mutex)))
#define TBX_UNLOCK_SHARED(st)  marcel_mutex_unlock((&((st)->__tbx_mutex)))
#define TBX_LOCK()   lock_task()
#define TBX_UNLOCK() unlock_task()
#define TBX_YIELD()  marcel_yield()
#elif defined (_REENTRANT) //# Thread sync : Pthread mode
#define TBX_SHARED             pthread_mutex_t __tbx_mutex
#define TBX_INIT_SHARED(st)    pthread_mutex_init((&((st)->__tbx_mutex)), NULL)
#define TBX_LOCK_SHARED(st)    pthread_mutex_lock((&((st)->__tbx_mutex)))
#define TBX_TRYLOCK_SHARED(st) pthread_mutex_trylock((&((st)->__tbx_mutex)))
#define TBX_UNLOCK_SHARED(st)  pthread_mutex_unlock((&((st)->__tbx_mutex)))
#define TBX_LOCK()
#define TBX_UNLOCK()
#if defined (SOLARIS_SYS)     //# pthread yield : 'not available' case
#define TBX_YIELD()        
#else                         //# pthread yield : 'available' case
#define TBX_YIELD()        pthread_yield()
#endif                        //# pthread yield : end
#else                      //# Threads sync : no thread mode
#define TBX_SHARED 
#define TBX_INIT_SHARED(st) 
#define TBX_LOCK_SHARED(st) 
#define TBX_TRYLOCK_SHARED(st) 
#define TBX_UNLOCK_SHARED(st) 
#define TBX_LOCK()
#define TBX_UNLOCK()
#define TBX_YIELD()
#endif                      //# Threads sync : end


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
#define __TBX_MALLOC(s, f, l)     tbx_safe_malloc  ((s), (f), (l))
#define __TBX_CALLOC(n, s, f, l)  tbx_safe_calloc  ((n), (s), (f), (l))
#define __TBX_REALLOC(p, s, f, l) tbx_safe_realloc ((p), (s), (f), (l))
#define __TBX_FREE(s, f, l)       tbx_safe_free    ((s), (f), (l))
#else  /* TBX_SAFE_MALLOC */
#define __TBX_MALLOC(s, f, l)     malloc  ((s))
#define __TBX_CALLOC(n, s, f, l)  calloc  ((n), (s))
#define __TBX_REALLOC(p, s, f, l) realloc ((p), (s))
#define __TBX_FREE(s, f, l)       free    ((s))
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
#ifdef __GNUC__
#define tbx_aligned(v, a) \
        ({typedef _tv = (v), _ta = (a); \
         _tv _v = (v); _ta _a = (a); \
         (((_v) + (_a - 1)) & ~(_a - 1));})
#define TBX_ALIGN(a)  __attribute__ ((__aligned__ (a)))
#else // __GNUC__
#define tbx_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))
#endif // __GNUC__

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
