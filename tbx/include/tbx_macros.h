/*! \file tbx_macros.h
 *  \brief TBX general-purpose macrocommands
 *
 *  This file contains the TBX general-purpose macrocommands.
 *
 */

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

#include "tbx_debug.h"
#include <alloca.h>

/** \defgroup macros general purpose macros
 *
 * These are the general purpose macrocommands
 *
 * @{
 */

/* DEBUG: main debug flag */
/* #define DEBUG */

/* TIMING: main timing flag */
/* #define TIMING */

/* TRACING: main trace flag */
/* #define TRACING */

/* HAVE_BACKTRACE: controls usage of the backtracing features
 * provided by the GNU C library
 */
#if defined(LINUX_SYS) || defined(GNU_SYS)
#  define HAVE_BACKTRACE
#else
#  undef  HAVE_BACKTRACE
#endif

/* OOPS: causes TBX_FAILURE to generate a segfault instead of a call to exit */
#define OOPS

/* TBX_MALLOC_CTRL: causes TBX to perform additional memory allocation
   verification */
/* 0 allocation/désallocation is correct */
/*#define TBX_MALLOC_CTRL*/

/*
 * Constant definition  _____________________________________________
 * ____________________//////////////////////////////////////////////
 */
#define TBX_FILE_TRANSFER_BLOCK_SIZE 1024

#ifndef NULL
# ifdef __cplusplus
#  define NULL  0
# else
#  define NULL  ((void*) 0)
# endif
#endif

/*
 * Timing macros  __________________________________________________
 * ______________///////////////////////////////////////////////////
 */
#ifdef TBX_TIMING
#  define _TBXT_PRE  TBX_GET_TICK(tbx_new_event)
#  define _TBXT_DIFF tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event)
#  define _TBXT_POST TBX_GET_TICK(tbx_last_event)

#  define TBX_TIME_INIT() _TBXT_POST
#  define TBX_TIME(str)
    _TBXT_PRE; pm2debug("%s [%4f usecs]\n", str, _TBXT_DIFF); _TBXT_POST
#  define TBX_TIME_IN()  TIME_INIT()
#  define TBX_TIME_OUT() TIME(__TBX_FUNCTION__)
#  define TBX_TIME_VAL(str, val) \
    _TBXT_PRE; \
    pm2debug(str " = %d [%4f usecs]\n", (int)(val), _TBXT_DIFF); \
    _TBXT_POST
#  define TBX_TIME_PTR(str, ptr) \
    _TBXT_PRE; \
    pm2debug(str " = %p [%4f usecs]\n", (void *)(ptr), _TBXT_DIFF); \
    _TBXT_POST
#  define TBX_TIME_STR(str, str2) \
    _TBXT_PRE; \
    pm2debug(str " : %s [%4f usecs]\n", (char *)(str2), _TBXT_DIFF); \
    _TBXT_POST
#else /* TBX_TIMING */
#  define TBX_TIME_INIT()          (void)(0)
#  define TBX_TIME(str)            (void)(0)
#  define TBX_TIME_IN()            (void)(0)
#  define TBX_TIME_OUT()           (void)(0)
#  define TBX_TIME_VAL(str, val)   (void)(0)
#  define TBX_TIME_PTR(str, ptr)   (void)(0)
#  define TBX_TIME_STR(str, str2)  (void)(0)
#endif /* TBX_TIMING */

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
    pm2debug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()

/*
 * VERIFY: assertion verification macro activated by PM2DEBUG flag
 * ------------------------------------------------------------
 */
#ifdef PM2DEBUG
#  define TBX_VERIFY(op, val) \
  if((op) != (val))     \
    pm2debug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()
#else /* DEBUG */
#  define TBX_VERIFY(op, val) ((void)(op))
#endif /* DEBUG */

#ifdef PM2DEBUG
#  define TBX_ASSERT(op) \
  if(!(op))     \
    pm2debug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op, __FILE__, __LINE__),   abort()
#else /* DEBUG */
#  define TBX_ASSERT(op) ((void)(op))
#endif /* DEBUG */



/*
 * TBX_FAILURE: display an error message and abort the program
 * -------------------------------------------------------
 */
#ifdef TBX_FAILURE_CLEANUP
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() TBX_FAILURE_CLEANUP(),abort()
#  else /* OOPS */
#    define _TBX_EXIT_FAILURE() TBX_FAILURE_CLEANUP(),exit(1)
#  endif /* OOPS */
#else /* TBX_FAILURE_CLEANUP */
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() abort()
#  else /* OOPS */
#    define _TBX_EXIT_FAILURE() exit(1)
#  endif /* OOPS */
#endif /* TBX_FAILURE_CLEANUP */

#ifdef HAVE_BACKTRACE
#  include <execinfo.h>
#  include <stdio.h>
#  include <stdlib.h>

#  ifndef TBX_BACKTRACE_DEPTH
#    define TBX_BACKTRACE_DEPTH 10
#  endif /* TBX_BACKTRACE_DEPTH */

#  define __TBX_RECORD_SOME_TRACE(array, size) pm2debug_backtrace(array, size);
#  define __TBX_PRINT_SOME_TRACE(array, size) \
do { \
       int      trace_i;\
       char   **strings;\
       strings = backtrace_symbols (array, size);\
\
       pm2debug("Obtained %d stack frames.\n", (int) size);\
\
       for (trace_i = 0; trace_i < size; trace_i++)\
          pm2debug ("%s\n", strings[trace_i]);\
\
       free (strings);\
} while(0)

#else  /* HAVE_BACKTRACE */
#  ifdef TBX_BACKTRACE_DEPTH
#    undef TBX_BACKTRACE_DEPTH
#  endif /* TBX_BACKTRACE_DEPTH */
#  define TBX_BACKTRACE_DEPTH 0
#  define __TBX_RECORD_SOME_TRACE(array, size) ((void)(array), (void)(size), 0)
#  define __TBX_PRINT_SOME_TRACE(array, size) ((void)(array), (void)(size))
#endif /* HAVE_BACKTRACE */

#  define __TBX_PRINT_TRACE()\
     ({\
       void    *array[TBX_BACKTRACE_DEPTH];\
       int      trace_size;\
\
       trace_size    = __TBX_RECORD_SOME_TRACE (array, TBX_BACKTRACE_DEPTH);\
       __TBX_PRINT_SOME_TRACE (array, trace_size); \
     })

#ifndef TBX_FAILURE_CONTEXT
#  define TBX_FAILURE_CONTEXT
#endif /* TBX_FAILURE_CONTEXT */

#define TBX_FAILURE(str) \
     (pm2debug("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT "%s\n", __FILE__, __LINE__, __TBX_FUNCTION__, (str)), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_FAILUREF(fmt, ...) \
     (pm2debug("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT fmt "\n" , __FILE__, __LINE__, __TBX_FUNCTION__, ## __VA_ARGS__), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_ERROR(str) \
     (pm2debug("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT "%s: %s\n\n", __FILE__, __LINE__, __TBX_FUNCTION__, (str), strerror(errno)), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_ERRORF(fmt, ...) \
     (pm2debug("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT ": %s" fmt "\n", __FILE__, __LINE__, __TBX_FUNCTION__, strerror(errno) , ## __VA_ARGS__), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

/*
 * CTRL_ALLOC
 * ----------
 */
#define CTRL_ALLOC(ptr) \
  { \
    if ((ptr) == NULL) \
      { \
	TBX_FAILURE("not enough memory"); \
      } \
  }

/*
 * Min/Max macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */
#define tbx_max(a, b) \
       ({__typeof__ ((a)) _a = (a); \
	 __typeof__ ((b)) _b = (b); \
         _a > _b ? _a : _b; })
#define tbx_min(a, b) \
       ({__typeof__ ((a)) _a = (a); \
	 __typeof__ ((b)) _b = (b); \
         _a < _b ? _a : _b; })

/*
 * Flags control macros  ____________________________________________
 * _____________________/////////////////////////////////////////////
 */

#define tbx_flag_set(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_set;})
#define tbx_flag_clear(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_clear;})
#define tbx_flag_toggle(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = 1 - !!*_pf;})

#define tbx_flag_test(f)   (!!*(f))
#define tbx_flag_is_set(f)   (!!*(f))
#define tbx_flag_is_clear(f)   (!*(f))

#if defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
#define TBX_REAL_SYMBOL_NAME(name) _ ## name
#else
#define TBX_REAL_SYMBOL_NAME(name) name
#endif

/*
 * Threads specific macros  _________________________________________
 * ________________________//////////////////////////////////////////
 */
#if defined (MARCEL)       /*# Thread sync : Marcel mode */

#  define TBX_CRITICAL_SECTION(name)              marcel_mutex_t __tbx_section_ ## name = MARCEL_MUTEX_INITIALIZER
#  define TBX_CRITICAL_SECTION_ENTER(name)        marcel_mutex_lock(&(__tbx_section_ ## name))
#  define TBX_CRITICAL_SECTION_TRY_ENTERING(name) marcel_mutex_trylock(&(__tbx_section_ ## name))
#  define TBX_CRITICAL_SECTION_LEAVE(name)        marcel_mutex_unlock(&(__tbx_section_ ## name))

#  define TBX_SHARED             marcel_mutex_t __tbx_mutex
#  define TBX_INIT_SHARED(st)    marcel_mutex_init((&((st)->__tbx_mutex)), NULL)
#  define TBX_LOCK_SHARED(st)    marcel_mutex_lock((&((st)->__tbx_mutex)))
#  define TBX_TRYLOCK_SHARED(st) marcel_mutex_trylock((&((st)->__tbx_mutex)))
#  define TBX_UNLOCK_SHARED(st)  marcel_mutex_unlock((&((st)->__tbx_mutex)))
#  define TBX_LOCK()             lock_task()
#  define TBX_UNLOCK()           unlock_task()
#  define TBX_YIELD()            marcel_yield()

#  define TBX_CRITICAL_SHARED             marcel_spinlock_t __tbx_spinlock
#  define TBX_INIT_CRITICAL_SHARED(st)    marcel_spin_init((&((st)->__tbx_spinlock)), 0)
#  define TBX_LOCK_CRITICAL_SHARED(st)    marcel_spin_lock((&((st)->__tbx_spinlock)))
#  define TBX_TRYLOCK_CRITICAL_SHARED(st) marcel_spin_trylock((&((st)->__tbx_spinlock)))
#  define TBX_UNLOCK_CRITICAL_SHARED(st)  marcel_spin_unlock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_LOCK()             marcel_spin_lock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_UNLOCK()           marcel_spin_unlock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_YIELD()            (void) 0

#elif defined (_REENTRANT) /*# Thread sync : Pthread mode */

#  define TBX_CRITICAL_SECTION(name)              pthread_mutex_t __tbx_section_ ## name = PTHREAD_MUTEX_INITIALIZER
#  define TBX_CRITICAL_SECTION_ENTER(name)        pthread_mutex_lock(&(__tbx_section_ ## name))
#  define TBX_CRITICAL_SECTION_TRY_ENTERING(name) pthread_mutex_trylock(&(__tbx_section_ ## name))
#  define TBX_CRITICAL_SECTION_LEAVE(name)        pthread_mutex_unlock(&(__tbx_section_ ## name))

#  define TBX_SHARED             pthread_mutex_t __tbx_mutex
#  define TBX_INIT_SHARED(st)    pthread_mutex_init((&((st)->__tbx_mutex)), NULL)
#  define TBX_LOCK_SHARED(st)    pthread_mutex_lock((&((st)->__tbx_mutex)))
#  define TBX_TRYLOCK_SHARED(st) pthread_mutex_trylock((&((st)->__tbx_mutex)))
#  define TBX_UNLOCK_SHARED(st)  pthread_mutex_unlock((&((st)->__tbx_mutex)))
#  define TBX_LOCK()
#  define TBX_UNLOCK()
#  if defined (SOLARIS_SYS)     /*# pthread yield : 'not available' case */
#    define TBX_YIELD()
#  else                         /*# pthread yield : 'available' case */
#    define TBX_YIELD()           pthread_yield()
#  endif                        /*# pthread yield : end */
#  define TBX_CRITICAL_SHARED             pthread_spinlock_t __tbx_spinlock
#  define TBX_INIT_CRITICAL_SHARED(st)    pthread_spin_init((&((st)->__tbx_spinlock)), 0)
#  define TBX_LOCK_CRITICAL_SHARED(st)    pthread_spin_lock((&((st)->__tbx_spinlock)))
#  define TBX_TRYLOCK_CRITICAL_SHARED(st) pthread_spin_trylock((&((st)->__tbx_spinlock)))
#  define TBX_UNLOCK_CRITICAL_SHARED(st)  pthread_spin_unlock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_LOCK()             pthread_spin_lock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_UNLOCK()           pthread_spin_unlock((&((st)->__tbx_spinlock)))
#  define TBX_CRITICAL_YIELD()            (void) 0
#else                         /*# Threads sync : no thread mode */

#  define TBX_CRITICAL_SHARED             TBX_SHARED
#  define TBX_INIT_CRITICAL_SHARED(st)    TBX_INIT_SHARED(st)
#  define TBX_LOCK_CRITICAL_SHARED(st)    TBX_LOCK_SHARED(st)
#  define TBX_TRYLOCK_CRITICAL_SHARED(st) TBX_TRYLOCK_SHARED(st)
#  define TBX_UNLOCK_CRITICAL_SHARED(st)  TBX_UNLOCK_SHARED(st)
#  define TBX_CRITICAL_LOCK()             TBX_LOCK()
#  define TBX_CRITICAL_UNLOCK()           TBX_UNLOCK()
#  define TBX_CRITICAL_YIELD()            TBX_YIELD()

#  define tbx_main DEFINE_TBX_MAIN(main)

#  define TBX_CRITICAL_SECTION(name) const int __tbx_section_ ## name = 0
#  define TBX_CRITICAL_SECTION_ENTER(name)
#  define TBX_CRITICAL_SECTION_TRY_ENTERING(name)
#  define TBX_CRITICAL_SECTION_LEAVE(name)

#  define TBX_SHARED
#  define TBX_INIT_SHARED(st)    (void)(0)
#  define TBX_LOCK_SHARED(st)    (void)(0)
#  define TBX_TRYLOCK_SHARED(st) (void)(0)
#  define TBX_UNLOCK_SHARED(st)  (void)(0)
#  define TBX_LOCK()             (void)(0)
#  define TBX_UNLOCK()           (void)(0)
#  define TBX_YIELD()            (void)(0)

#  define TBX_CRITICAL_SHARED             TBX_SHARED
#  define TBX_INIT_CRITICAL_SHARED(st)    TBX_INIT_SHARED(st)
#  define TBX_LOCK_CRITICAL_SHARED(st)    TBX_LOCK_SHARED(st)
#  define TBX_TRYLOCK_CRITICAL_SHARED(st) TBX_TRYLOCK_SHARED(st)
#  define TBX_UNLOCK_CRITICAL_SHARED(st)  TBX_UNLOCK_SHARED(st)
#  define TBX_CRITICAL_LOCK()             TBX_LOCK()
#  define TBX_CRITICAL_UNLOCK()           TBX_UNLOCK()
#  define TBX_CRITICAL_YIELD()            TBX_YIELD()
#endif                      /*# Threads sync : end */


/*
 * Memory allocation macros  ________________________________________
 * _________________________/////////////////////////////////////////
 */

    /* Marcel-internal specific allocation */
#ifdef MARCEL
#  ifdef TBX_SAFE_MALLOC
#    define __TBX_MALLOC(s, f, l)     tbx_safe_malloc  ((s), (f), (l))
#    define __TBX_CALLOC(n, s, f, l)  tbx_safe_calloc  ((n), (s), (f), (l))
#    define __TBX_REALLOC(p, s, f, l) tbx_safe_realloc ((p), (s), (f), (l))
#    define __TBX_FREE(s, f, l)       tbx_safe_free    ((s), (f), (l))
#  else  /* TBX_SAFE_MALLOC */
#    define __TBX_MALLOC(s, f, l)     ((void) f, (void)l, malloc  ((s)))
#    define __TBX_CALLOC(n, s, f, l)  ((void) f, (void)l, calloc  ((n), (s)))
#    define __TBX_REALLOC(p, s, f, l) ((void) f, (void)l, realloc ((p), (s)))
#    define __TBX_FREE(s, f, l)       ((void) f, (void)l, free    ((s)))
#  endif /* TBX_SAFE_MALLOC */
#endif /* MARCEL */

    /* Allocation operation */
#ifdef MARCEL
#  define __TBX_MALLOC_OP(s)     tmalloc  ((s))
#  define __TBX_CALLOC_OP(n, s)  tcalloc  ((n), (s))
#  define __TBX_REALLOC_OP(p, s) trealloc ((p), (s))
#  define __TBX_FREE_OP(s)       tfree    ((s))
#else /* MARCEL */
#  ifdef TBX_SAFE_MALLOC
#    define __TBX_MALLOC_OP(s)     tbx_safe_malloc  ((s), __FILE__, __LINE__)
#    define __TBX_CALLOC_OP(n, s)  tbx_safe_calloc  ((n), (s), __FILE__, __LINE__)
#    define __TBX_REALLOC_OP(p, s) tbx_safe_realloc ((p), (s), __FILE__, __LINE__)
#    define __TBX_FREE_OP(s)       tbx_safe_free    ((s), __FILE__, __LINE__)
#  else /* TBX_SAFE_MALLOC */
#    define __TBX_MALLOC_OP(s)     malloc  ((s))
#    define __TBX_CALLOC_OP(n, s)  calloc  ((n), (s))
#    define __TBX_REALLOC_OP(p, s) realloc ((p), (s))
#    define __TBX_FREE_OP(s)       free    ((s))
#  endif /* TBX_SAFE_MALLOC */
#endif /* MARCEL */

    /* Regular allocation macro */
#ifdef TBX_MALLOC_CTRL
#  define TBX_MALLOC(s)     ((__TBX_MALLOC_OP  ((s)?:(TBX_FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_CALLOC(n, s)  ((__TBX_CALLOC_OP  ((n)?:(TBX_FAILURE("attempt to alloc 0 bytes"), 0), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_REALLOC(p, s) ((__TBX_REALLOC_OP ((p)?:(TBX_FAILURE("attempt to realloc a NULL area"), NULL), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory reallocation error"), NULL))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)?:(TBX_FAILURE("attempt to free a NULL area"), NULL)))
#else /* TBX_MALLOC_CTRL */
#  define TBX_MALLOC(s)     (__TBX_MALLOC_OP  ((s)))
#  define TBX_CALLOC(n, s)  (__TBX_CALLOC_OP  ((n), (s)))
#  define TBX_REALLOC(p, s) (__TBX_REALLOC_OP ((p), (s)))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)))
#endif /* TBX_MALLOC_CTRL */


#define TBX_CFREE(s)  ((s)?__TBX_FREE_OP((s)): )
#define TBX_FREE2(s)  (__TBX_FREE_OP((s)), (s)=NULL)
#define TBX_CFREE2(s) ((s)?(__TBX_FREE_OP((s)), (s)=NULL):NULL)


/*
 * Alignment macros  ________________________________________________
 * _________________/////////////////////////////////////////////////
 */
#define TBX_ALIGN(a)  __attribute__ ((__aligned__ (a)))

#define tbx_aligned(v, a) \
        ({__typeof__ ((v)) _v = (v); \
	 __typeof__ ((a)) _a = (a); \
         (_v + _a - 1) & ~(_a - 1);})


/*
 * Arithmetic macros  _______________________________________________
 * __________________////////////////////////////////////////////////
 */
    /* Bi-directional shifts */
#define tbx_lshift(x, n) ({     \
	__typeof__ ((n)) _n = (n); \
	__typeof__ ((x)) _x = (x); \
        _n > 0?_x << _n:_x >> -_n;\
})
#define tbx_rshift(x, n) ({     \
	__typeof__ ((n)) _n = (n); \
	__typeof__ ((x)) _x = (x); \
        _n > 0?_x >> _n:_x << -_n;\
})
 
#define tbx_c2b(c) ({ \
		unsigned char _c = (c); \
		unsigned int  _b = 0; \
		unsigned int  _m = 1; \
		while (_c) { \
			if (_c & 1) { \
				_b += _m; \
			} \
			_c >>= 1; \
			_m *= 10; \
		} \
		_b; \
	})
#define tbx_i2b(v) ({ \
		__typeof__ ((MARCEL_VPSET_CONST_0)) _v = (v); \
		char *a; \
		int i; \
\
		a = alloca(sizeof(_v)*8+1); \
		\
		for (i = 0; i < sizeof(_v); i++) { \
			sprintf(a + i*8, "%08u", tbx_c2b((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)); \
		} \
		a; \
	})
#define tbx_i2mb(v) ({ \
		__typeof__ ((MARCEL_VPSET_CONST_0)) _v = (v); \
		char *a; \
		int i; \
		int j;\
\
		a = alloca(sizeof(_v)*8+1); \
		\
		for (i = 0, j = 0; i < sizeof(_v); i++) { \
			if ((i < sizeof(_v) - 1) && !((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)) \
				continue; \
			sprintf(a + j*8, "%08u", tbx_c2b((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)); \
			j++;\
		} \
		a; \
	})
#define tbx_i2sb(v) ({ \
		__typeof__ ((MARCEL_VPSET_CONST_0)) _v = (v); \
		char *a; \
		int i; \
\
		a = alloca(sizeof(_v)*9); \
		\
		for (i = 0; i < sizeof(_v); i++) { \
			if (i) \
				*(a + i*9 - 1) = '_'; \
			sprintf(a + i*9, "%08u", tbx_c2b((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)); \
		} \
		a; \
	})
#define tbx_i2smb(v) ({ \
		__typeof__ ((MARCEL_VPSET_CONST_0)) _v = (v); \
		char *a; \
		int i; \
		int j; \
\
		a = alloca(sizeof(_v)*9); \
		\
		for (i = 0, j = 0; i < sizeof(_v); i++) { \
			if ((i < sizeof(_v) - 1) && !((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)) \
				continue; \
			if (j) \
				*(a + j*9 - 1) = '_'; \
			sprintf(a + j*9, "%08u", tbx_c2b((_v >> (8*(sizeof(_v) - i - 1))) & 0xff)); \
		} \
		a; \
	})
/*
 * Fields access macros _____________________________________________
 * ____________________//////////////////////////////////////////////
 */
#define tbx_expand_field(h, f) (tbx_lshift((((unsigned int)(h)[f]) & f ## _mask), f ## _shift))

#define tbx_clear_field(h, f) ((h)[f] &= ~(f ## _mask))

#define tbx_contract_field(h, v, f) (tbx_clear_field((h), f), (h)[f] |= (unsigned char)((tbx_rshift((v), f ## _shift)) & f ## _mask))

#define tbx_offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
							  
/**
 * tbx_container_of - cast a member of a structure out to the containing structure
 *
 * @param ptr        the pointer to the member.
 * @param type       the type of the container struct this is embedded in.
 * @param member     the name of the member within the struct.
 *
 */
#define tbx_container_of(ptr, type, member) \
	((type *)((char *)(__typeof__ (&((type *)0)->member))(ptr)- \
		  tbx_offset_of(type,member)))


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
	TBX_FAILURE("system call failed"); \
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
	TBX_FAILURE("system call failed"); \
      } \
  }

/* @} */
#endif /* TBX_MACROS_H */

/** \addtogroup macros
 *
 * @{
 */
/*
 * Stringification  _________________________________________________
 * ________________//////////////////////////////////////////////////
 */
#define TBX_STR(s) #s
#define TBX_MACRO_TO_STR(s) TBX_STR(s)

/*
 * Assembly   _______________________________________________________
 * __________////////////////////////////////////////////////////////
 */
#ifdef AIX_SYS
#define TBX_LOCAL_LBL(num) "Lma%=" #num
#define TBX_LOCAL_LBLF(num) TBX_LOCAL_LBL(num)
#define TBX_LOCAL_LBLB(num) TBX_LOCAL_LBL(num)
#else
#define TBX_LOCAL_LBL(num) #num
#define TBX_LOCAL_LBLF(num) TBX_LOCAL_LBL(num) "f"
#define TBX_LOCAL_LBLB(num) TBX_LOCAL_LBL(num) "b"
#endif

/*
 * On opteron, it's better to use the prefix lock for setting highly shared
 * variables (the new value is directly given to other caches)
 */
#if defined(MARCEL) && defined(X86_64_ARCH)
#define TBX_SHARED_SET(v,n) asm("lock add %1,%0":"=m"(v):"r"((unsigned long)n-(unsigned long)v))
#else
#define TBX_SHARED_SET(v,n) (v) = (n);
#endif

#ifdef PM2_NUIOA
#define PM2_NUIOA_ANY_NODE (-1) /* we don't like any numa node more than the others */
#define PM2_NUIOA_CONFLICTING_NODES (-2) /* we like several different numa nodes, no solution */
#endif
/* @} */
