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

/* DEBUG: main debug flag */
/* #define DEBUG */

/* TIMING: main timing flag */
/* #define TIMING */

/* TRACING: main trace flag */
/* #define TRACING */

/* OOPS: causes FAILURE to generate a segfault instead of a call to exit */
#define OOPS

/* TBX_MALLOC_CTRL: causes TBX to perform additional memory allocation
   verification */
#define TBX_MALLOC_CTRL

/*
 * Extension usage control  _____________________________________________
 * ________________________//////////////////////////////////////////
 */

// Extensions for safe macro writing
#if       (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 8)
#  define TBX_USE_SAFE_MACROS
static const int __tbx_using_safe_macros = 1;
#else  // (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 8)
static const int __tbx_using_safe_macros = 0;
#endif // (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 8)

// Ternary operator extension
#if       (__GNUC__ >= 2)
#  define TBX_USE_EXTENDED_TERNARY_OP
static const int __tbx_using_extended_ternary_operand = 1;
#else  // (__GNUC__ >= 2)
static const int __tbx_using_extended_ternary_operand = 0;
#endif // (__GNUC__ >= 2)

// GCC ATTRIBUTES
#if       (__GNUC__ >= 2)
#  define TBX_USE_GCC2_ATTRIBUTES
static const int __tbx_using_gcc2_attributes = 1;
#  if       (__GNUC__ >= 3)
#    define TBX_USE_GCC3_ATTRIBUTES
static const int __tbx_using_gcc3_attributes = 1;
#  else  // (__GNUC__ >= 3)
static const int __tbx_using_gcc3_attributes = 0;
#  endif // (__GNUC__ >= 3)
#else  // (__GNUC__ >= 2)
static const int __tbx_using_gcc2_attributes = 0;
#endif // (__GNUC__ >= 2)

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
#  define _TBXT_PRE  TBX_GET_TICK(tbx_new_event)
#  define _TBXT_DIFF tbx_tick2usec(TBX_TICK_DIFF(tbx_last_event, tbx_new_event)
#  define _TBXT_POST TBX_GET_TICK(tbx_last_event)

#  define TIME_INIT() _TBXT_POST
#  define TIME(str)
    _TBXT_PRE; pm2fulldebug(str " [%4f usecs]\n", _TBXT_DIFF); _TBXT_POST
#  define TIME_IN()  TIME_INIT()
#  define TIME_OUT() TIME(__FUNCTION__)
#  define TIME_VAL(str, val) \
    _TBXT_PRE; \
    pm2fulldebug(str " = %d [%4f usecs]\n", (int)(val), _TBXT_DIFF); \
    _TBXT_POST
#  define TIME_PTR(str, ptr) \
    _TBXT_PRE; \
    pm2fulldebug(str " = %p [%4f usecs]\n", (void *)(ptr), _TBXT_DIFF); \
    _TBXT_POST
#  define TIME_STR(str, str2) \
    _TBXT_PRE; \
    pm2fulldebug(str " : %s [%4f usecs]\n", (char *)(str2), _TBXT_DIFF); \
    _TBXT_POST
#else /* TIMING */
#  define TIME_INIT()          (void)(0)
#  define TIME(str)            (void)(0)
#  define TIME_IN()            (void)(0)
#  define TIME_OUT()           (void)(0)
#  define TIME_VAL(str, val)   (void)(0)
#  define TIME_PTR(str, ptr)   (void)(0)
#  define TIME_STR(str, str2)  (void)(0)
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
#  define TBX_VERIFY(op, val) \
  if((op) != (val))     \
    pm2fulldebug("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()
#else /* DEBUG */
#  define TBX_VERIFY(op, val) ((void)(op))
#endif /* DEBUG */

/*
 * FAILURE: display an error message and abort the program
 * -------------------------------------------------------
 */
#ifdef FAILURE_CLEANUP
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() FAILURE_CLEANUP(),abort()
#  else /* OOPS */
#    define _TBX_EXIT_FAILURE() FAILURE_CLEANUP(),exit(1)
#  endif /* OOPS */
#else // FAILURE_CLEANUP
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() abort()
#  else /* OOPS */
#    define _TBX_EXIT_FAILURE() exit(1)
#  endif /* OOPS */
#endif // FAILURE_CLEANUP

#define FAILURE(str) \
     (pm2debug_flush(), \
      pm2fulldebug("FAILURE: %s\n", (str)), \
      _TBX_EXIT_FAILURE())

#define ERROR(str) \
     (pm2debug_flush(), \
      pm2fulldebug("FAILURE: %s: %s\n\n", (str), strerror(errno)), \
      _TBX_EXIT_FAILURE())

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
#ifdef TBX_USE_SAFE_MACROS
#  define max(a, b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a > _b ? _a : _b; })
#  define min(a, b) \
       ({typedef _ta = (a), _tb = (b);  \
         _ta _a = (a); _tb _b = (b);     \
         _a < _b ? _a : _b; })
#else // TBX_USE_SAFE_MACROS
#  define max(a, b) (((a) > (b))?(a):(b))
#  define min(a, b) (((a) < (b))?(a):(b))
#endif // TBX_USE_SAFE_MACROS

/*
 * Flags control macros  ____________________________________________
 * _____________________/////////////////////////////////////////////
 */

#ifdef TBX_USE_SAFE_MACROS
#  define tbx_set(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_set;})
#  define tbx_clear(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = tbx_flag_clear;})
#  define tbx_toggle(f) \
        ({p_tbx_flag_t _pf = (f); \
          *_pf = 1 - !!*_pf;})
#else // TBX_USE_SAFE_MACROS
#  define tbx_set(f)    ((*(f)) = tbx_flag_set)
#  define tbx_clear(f)  ((*(f)) = tbx_flag_clear)
#  define tbx_toggle(f) ((*(f)) = 1 - (!!*(f)))
#endif // TBX_USE_SAFE_MACROS

#define tbx_test(f)   (!!*(f))

/*
 * Threads specific macros  _________________________________________
 * ________________________//////////////////////////////////////////
 */
#if defined (MARCEL)       //# Thread sync : Marcel mode
#  ifdef STANDARD_MAIN
#    define tbx_main main
#  else  // STANDARD_MAIN
#    define tbx_main marcel_main
#  endif // STANDARD_MAIN
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
#elif defined (_REENTRANT) //# Thread sync : Pthread mode
#  define tbx_main main
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
#  if defined (SOLARIS_SYS)     //# pthread yield : 'not available' case
#    define TBX_YIELD()
#  else                         //# pthread yield : 'available' case
#    define TBX_YIELD()           pthread_yield()
#  endif                        //# pthread yield : end
#else                         //# Threads sync : no thread mode
#  define tbx_main main

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
#endif                      //# Threads sync : end


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
#    define __TBX_MALLOC(s, f, l)     malloc  ((s))
#    define __TBX_CALLOC(n, s, f, l)  calloc  ((n), (s))
#    define __TBX_REALLOC(p, s, f, l) realloc ((p), (s))
#    define __TBX_FREE(s, f, l)       free    ((s))
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
#if (defined TBX_MALLOC_CTRL) && (!defined TBX_USE_EXTENDED_TERNARY_OP)
#  warning TBX_MALLOC_CTRL requires the GNU Compiler Collection
#  undef TBX_MALLOC_CTRL
#endif /* TBX_MALLOC_CTRL && !TBX_USE_EXTENDED_TERNARY_OP */

#ifdef TBX_MALLOC_CTRL
#  define TBX_MALLOC(s)     ((__TBX_MALLOC_OP  ((s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_CALLOC(n, s)  ((__TBX_CALLOC_OP  ((n)?:(FAILURE("attempt to alloc 0 bytes"), 0), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_REALLOC(p, s) ((__TBX_REALLOC_OP ((p)?:(FAILURE("attempt to realloc a NULL area"), NULL), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory reallocation error"), NULL))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)?:(FAILURE("attempt to free a NULL area"), NULL)))
#else /* TBX_MALLOC_CTRL */
#  define TBX_MALLOC(s)     (__TBX_MALLOC_OP  ((s)))
#  define TBX_CALLOC(n, s)  (__TBX_CALLOC_OP  ((n), (s)))
#  define TBX_REALLOC(p, s) (__TBX_REALLOC_OP ((p), (s)))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)))
#endif /* TBX_MALLOC_CTRL */

/*
 * Alignment macros  ________________________________________________
 * _________________/////////////////////////////////////////////////
 */
#ifdef TBX_USE_GCC2_ATTRIBUTES
#  define TBX_ALIGN(a)  __attribute__ ((__aligned__ (a)))
#else // TBX_USE_GCC2_ATTRIBUTES
#  define TBX_ALIGN(a)
#endif // TBX_USE_GCC2_ATTRIBUTES

#ifdef TBX_USE_SAFE_MACROS
#  define tbx_aligned(v, a) \
        ({typedef _tv = (v), _ta = (a); \
         _tv _v = (v); _ta _a = (a); \
         (_v + _a - 1) & ~(_a - 1);})
#else // TBX_USE_SAFE_MACROS
#  define tbx_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))
#endif // TBX_USE_SAFE_MACROS


/*
 * Arithmetic macros  _______________________________________________
 * __________________////////////////////////////////////////////////
 */
    // Bi-directional shifts
#ifdef TBX_USE_SAFE_MACROS
#  define tbx_lshift(x, n) ({     \
        typedef _tn = (n);        \
        typedef _tx = (x);        \
        _tn _n = (n);             \
        _tx _x = (x);             \
        _n > 0?_x << _n:_x >> -_n;\
})
#  define tbx_rshift(x, n) ({     \
        typedef _tn = (n);        \
        typedef _tx = (x);        \
        _tn _n = (n);             \
        _tx _x = (x);             \
        _n > 0?_x >> _n:_x << -_n;\
})
#else // TBX_USE_SAFE_MACROS
#  define tbx_lshift(x, n) (((_n) > 0)?((_x) << (_n)):((_x) >> (-(_n))))
#  define tbx_rshift(x, n) (((_n) > 0)?((_x) >> (_n)):((_x) << (-(_n))))
#endif // TBX_USE_SAFE_MACROS


/*
 * Fields access macros _____________________________________________
 * ____________________//////////////////////////////////////////////
 */
#define tbx_expand_field(h, f) (tbx_lshift((((unsigned int)(h)[f]) & f ## _mask), f ## _shift))

#define tbx_clear_field(h, f) ((h)[f] &= ~(f ## _mask))

#define tbx_contract_field(h, v, f) (tbx_clear_field((h), f), (h)[f] |= (unsigned char)((tbx_rshift((v), f ## _shift)) & f ## _mask))


/*
 * Attribute macros  ________________________________________________
 * _________________/////////////////////////////////////////////////
 */
#ifdef TBX_USE_GCC2_ATTRIBUTES
#  define TBX_BYTE        __attribute__ ((__mode__ (__byte__)))
#  define TBX_WORD        __attribute__ ((__mode__ (__word__)))
#  define TBX_POINTER     __attribute__ ((__mode__ (__pointer__)))
#  define TBX_ALIGNED     __attribute__ ((__aligned__))
#  define TBX_PACKED      __attribute__ ((__packed__))
#  define TBX_UNUSED      __attribute__ ((__unused__))
#  define TBX_NORETURN    __attribute__ ((__noreturn__))
#  define TBX_CONST       __attribute__ ((__const__))
#  define TBX_NOINST      __attribute__ ((__no_instrument_function__))
#else // TBX_USE_GCC2_ATTRIBUTES
#  define TBX_BYTE
#  define TBX_WORD
#  define TBX_POINTER
#  define TBX_ALIGNED
#  define TBX_PACKED
#  define TBX_UNUSED
#  define TBX_NORETURN
#  define TBX_CONST
#  define TBX_NOINST
#endif // TBX_USE_GCC2_ATTRIBUTES

#ifdef TBX_USE_GCC3_ATTRIBUTES
#  define TBX_PURE        __attribute__ ((__pure__))
#  define TBX_FMALLOC     __attribute__ ((__malloc__))
#else // TBX_USE_GCC3_ATTRIBUTES
#  define TBX_PURE
#  define TBX_FMALLOC
#endif // TBX_USE_GCC3_ATTRIBUTES


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

/*
 * Stringification  _________________________________________________
 * ________________//////////////////////////////////////////////////
 */
#define TBX_STR(s) #s
#define TBX_MACRO_TO_STR(s) TBX_STR(s)
