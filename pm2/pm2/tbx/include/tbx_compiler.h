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

/****************************************************************
 * D�finitions d�pendantes du compilateur
 *
 * Fortement inspir� des sources du noyau Linux
 */
#ifndef TBX_COMPILER_H_EST_DEF
#define TBX_COMPILER_H_EST_DEF

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define tbx_barrier() __asm__ __volatile__("": : :"memory")

/* This macro obfuscates arithmetic on a variable address so that gcc
   shouldn't recognize the original var, and make assumptions about it */
#define TBX_RELOC_HIDE(ptr, off)					\
  ({ unsigned long __ptr;					\
    __asm__ ("" : "=g"(__ptr) : "0"(ptr));		\
    (typeof(ptr)) (__ptr + (off)); })

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
#define TBX_NORETURN    __attribute__ ((__noreturn__,__no_instrument_function__))
#define TBX_CONST       __attribute__ ((__const__))
#define TBX_NOINST      __attribute__ ((__no_instrument_function__))

#if __GNUC__ > 3
#  define __TBX_FUNCTION__		__func__
#  define TBX_FMALLOC			__attribute__ ((__malloc__))
#  define __tbx_inline__	__inline__ __attribute__((always_inline))
#  define __tbx_deprecated__		__attribute__((deprecated))
#  define __tbx_attribute_used__	__attribute__((__used__))
#  define __tbx_attribute_pure__	__attribute__((pure))
#elif __GNUC__ == 3
#  define TBX_FMALLOC			__attribute__ ((__malloc__))
#  if __GNUC_MINOR__ >= 1
#    define __tbx_inline__	__inline__ __attribute__((always_inline))
#  endif

#  if __GNUC_MINOR__ > 0
#    define __TBX_FUNCTION__		__func__
#    define __tbx_deprecated__		__attribute__((deprecated))
#  else
#    define __TBX_FUNCTION__		__FUNCTION__
#  endif

#  if __GNUC_MINOR__ >= 3
#    define __tbx_attribute_used__	__attribute__((__used__))
#  else
#    define __tbx_attribute_used__	__attribute__((__unused__))
#  endif

#  define __tbx_attribute_pure__	__attribute__((pure))
#elif __GNUC__ == 2
#  define __TBX_FUNCTION__		__FUNCTION__
#  define TBX_FMALLOC
#  if __GNUC_MINOR__ < 96
#    ifndef __builtin_expect
#      define __builtin_expect(x, expected_value) (x)
#    endif
#  endif

#  define __tbx_attribute_used__	__attribute__((__unused__))

#  if __GNUC_MINOR__ >= 96
#    define __tbx_attribute_pure__	__attribute__((pure))
#  endif
#else
#  error Sorry, your compiler is too old/not recognized.
#endif
#if (__GNUC__ <= 2) && (__GNUC__ != 2 || __GNUC_MINOR__ <= 8)
#  error Sorry, your compiler is too old/not recognized.
#endif


/*
 * Generic compiler-dependent macros required for kernel
 * build go below this comment. Actual compiler/compiler version
 * specific implementations come from the above header files
 */

#define tbx_likely(x)	__builtin_expect(!!(x), 1)
#define tbx_unlikely(x)	__builtin_expect(!!(x), 0)

/*
 * Allow us to mark functions as 'deprecated' and have gcc emit a nice
 * warning for each use, in hopes of speeding the functions removal.
 * Usage is:
 * 		int __tbx_deprecated__ foo(void)
 */
#ifndef __tbx_deprecated__
# define __tbx_deprecated__		/* unimplemented */
#endif

/*
 * Allow us to avoid 'defined but not used' warnings on functions and data,
 * as well as force them to be emitted to the assembly file.
 *
 * As of gcc 3.3, static functions that are not marked with attribute((used))
 * may be elided from the assembly file.  As of gcc 3.3, static data not so
 * marked will not be elided, but this may change in a future gcc version.
 *
 * In prior versions of gcc, such functions and data would be emitted, but
 * would be warned about except with attribute((unused)).
 */
#ifndef __tbx_attribute_used__
# define __tbx_attribute_used__	/* unimplemented */
#endif

/*
 * From the GCC manual:
 *
 * Many functions have no effects except the return value and their
 * return value depends only on the parameters and/or global
 * variables.  Such a function can be subject to common subexpression
 * elimination and loop optimization just as an arithmetic operator
 * would be.
 * [...]
 */
#ifndef __tbx_attribute_pure__
# define __tbx_attribute_pure__	/* unimplemented */
#endif
#define TBX_PURE __tbx_attribute_pure__

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({      type __dummy; \
        typeof(x) __dummy2; \
        (void)(&__dummy == &__dummy2); \
        1; \
})

/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef  __cplusplus
# define __TBX_BEGIN_DECLS  extern "C" {
# define __TBX_END_DECLS    }
#else
# define __TBX_BEGIN_DECLS
# define __TBX_END_DECLS
#endif

#endif /* TBX_COMPILER_H_EST_DEF */
