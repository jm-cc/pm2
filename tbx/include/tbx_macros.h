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

#include "tbx_log.h"
#include <stdlib.h>

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
 * Control macros  __________________________________________________
 * _______________///////////////////////////////////////////////////
 */

/*
 * CTRL: assertion verification macro
 * ----------------------------------
 */
#define TBX_CTRL(op, val) \
  if((op) != (val))     \
    PM2_LOG("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()

/*
 * VERIFY: assertion verification macro activated by TBX_DEBUG flag
 * ------------------------------------------------------------
 */
#ifdef TBX_DEBUG
#  define TBX_VERIFY(op, val) \
  if((op) != (val))     \
    PM2_LOG("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__),   abort()
#else				/* DEBUG */
#  define TBX_VERIFY(op, val) ((void)(op))
#endif				/* DEBUG */

#ifdef TBX_DEBUG
#  define TBX_ASSERT(op) \
  if(!(op))     \
    PM2_LOG("ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op, __FILE__, __LINE__),   abort()
#else				/* DEBUG */
#  define TBX_ASSERT(op) ((void)(op))
#endif				/* DEBUG */



/*
 * TBX_FAILURE: display an error message and abort the program
 * -------------------------------------------------------
 */
#ifdef TBX_FAILURE_CLEANUP
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() TBX_FAILURE_CLEANUP(),abort()
#  else				/* OOPS */
#    define _TBX_EXIT_FAILURE() TBX_FAILURE_CLEANUP(),exit(1)
#  endif			/* OOPS */
#else				/* TBX_FAILURE_CLEANUP */
#  ifdef OOPS
#    define _TBX_EXIT_FAILURE() abort()
#  else				/* OOPS */
#    define _TBX_EXIT_FAILURE() exit(1)
#  endif			/* OOPS */
#endif				/* TBX_FAILURE_CLEANUP */

#ifdef HAVE_BACKTRACE
#  include <execinfo.h>
#  include <stdio.h>
#  include <stdlib.h>

#  ifndef TBX_BACKTRACE_DEPTH
#    define TBX_BACKTRACE_DEPTH 10
#  endif			/* TBX_BACKTRACE_DEPTH */

#  define __TBX_RECORD_SOME_TRACE(array, size) backtrace(array, size);
#  define __TBX_PRINT_SOME_TRACE(array, size) \
do { \
       unsigned     trace_i;\
       char       **strings;\
       strings = backtrace_symbols (array, size);\
\
       PM2_LOG("Obtained %d stack frames.\n", (int) size);\
\
       for (trace_i = 0; trace_i < (unsigned)size; trace_i++)	\
          PM2_LOG ("%s\n", strings[trace_i]);\
\
       free (strings);\
} while(0)

#else				/* HAVE_BACKTRACE */
#  ifdef TBX_BACKTRACE_DEPTH
#    undef TBX_BACKTRACE_DEPTH
#  endif			/* TBX_BACKTRACE_DEPTH */
#  define TBX_BACKTRACE_DEPTH 0
#  define __TBX_RECORD_SOME_TRACE(array, size) ((void)(array), (void)(size), 0)
#  define __TBX_PRINT_SOME_TRACE(array, size) ((void)(array), (void)(size))
#endif				/* HAVE_BACKTRACE */

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
#endif				/* TBX_FAILURE_CONTEXT */

#define TBX_FAILURE(str) \
     (PM2_ERR("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT "%s\n", __FILE__, __LINE__, __TBX_FUNCTION__, (str)), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_FAILUREF(fmt, ...) \
     (PM2_ERR("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT fmt "\n" , __FILE__, __LINE__, __TBX_FUNCTION__, ## __VA_ARGS__), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_ERROR(str) \
     (PM2_ERR("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT "%s: %s\n\n", __FILE__, __LINE__, __TBX_FUNCTION__, (str), strerror(errno)), \
      __TBX_PRINT_TRACE(), \
      _TBX_EXIT_FAILURE())

#define TBX_ERRORF(fmt, ...) \
     (PM2_ERR("TBX_FAILURE(%s:%d in %s): " TBX_FAILURE_CONTEXT ": %s" fmt "\n", __FILE__, __LINE__, __TBX_FUNCTION__, strerror(errno) , ## __VA_ARGS__), \
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
 * Memory allocation macros  ________________________________________
 * _________________________/////////////////////////////////////////
 */
#  define __TBX_MALLOC_OP(s)     malloc  ((s))
#  define __TBX_CALLOC_OP(n, s)  calloc  ((n), (s))
#  define __TBX_REALLOC_OP(p, s) realloc ((p), (s))
#  define __TBX_FREE_OP(s)       free    ((s))

    /* Regular allocation macro */
#ifdef TBX_MALLOC_CTRL
#  define TBX_MALLOC(s)     ((__TBX_MALLOC_OP  ((s)?:(TBX_FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_CALLOC(n, s)  ((__TBX_CALLOC_OP  ((n)?:(TBX_FAILURE("attempt to alloc 0 bytes"), 0), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
#  define TBX_REALLOC(p, s) ((__TBX_REALLOC_OP ((p)?:(TBX_FAILURE("attempt to realloc a NULL area"), NULL), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory reallocation error"), NULL))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)?:(TBX_FAILURE("attempt to free a NULL area"), NULL)))
#else				/* TBX_MALLOC_CTRL */
#  define TBX_MALLOC(s)     (__TBX_MALLOC_OP  ((s)))
#  define TBX_CALLOC(n, s)  (__TBX_CALLOC_OP  ((n), (s)))
#  define TBX_REALLOC(p, s) (__TBX_REALLOC_OP ((p), (s)))
#  define TBX_FREE(s)       (__TBX_FREE_OP    ((s)))
#endif				/* TBX_MALLOC_CTRL */


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
#endif				/* TBX_MACROS_H */

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
#if defined(X86_64_ARCH)
#define TBX_SHARED_SET(v,n) asm("lock add %1,%0":"=m"(v):"r"((unsigned long)n-(unsigned long)v))
#else
#define TBX_SHARED_SET(v,n) (v) = (n);
#endif

/* @} */
