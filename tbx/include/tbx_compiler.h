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
 * Définitions dépendantes du compilateur
 *
 * Fortement inspiré des sources du noyau Linux
 */
#ifndef TBX_COMPILER_H_EST_DEF
#define TBX_COMPILER_H_EST_DEF

/** \defgroup compiler_interface compiler dependent feature interface
 *
 * This is the compiler dependent feature interface
 *
 * @{
 */
#define _TBX_STRING(x) #x
#define TBX_STRING(x) _TBX_STRING(x)

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#ifndef __INTEL_COMPILER
#define tbx_barrier() __asm__ __volatile__("": : :"memory")
#else
void __memory_barrier(void);
#define tbx_barrier() __memory_barrier();
#endif

/* This macro obfuscates arithmetic on a variable address so that gcc
   shouldn't recognize the original var, and make assumptions about it */
#ifndef __INTEL_COMPILER
#if !defined(PPC_ARCH) || (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
#define TBX_RELOC_HIDE(ptr, off)					\
  ({ unsigned long __ptr;					\
    __asm__ ("" : "=g"(__ptr) : "0"(ptr));		\
    (__typeof__(ptr)) (__ptr + (off)); })
#else
#define TBX_RELOC_HIDE(ptr, off)					\
  ({ unsigned long __ptr;					\
    __asm__ ("" : "=r"(__ptr) : "0"(ptr));		\
    (__typeof__(ptr)) (__ptr + (off)); })
#endif
#else
#define TBX_RELOC_HIDE(ptr, off)                                   \
  ({ unsigned long __ptr;                                       \
     __ptr = (unsigned long) (ptr);                             \
    (__typeof__(ptr)) (__ptr + (off)); })
#endif
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
#define TBX_NORETURN    __attribute__ ((__noreturn__)) TBX_NOINST
#define TBX_CONST       __attribute__ ((__const__))
#if defined(__cplusplus)
#  if defined(PM2_INSTRUMENT)
#    error "Can't instrument PM2 code in C++"
#  else
#    define TBX_NOINST
#  endif
#else
#  define TBX_NOINST      __attribute__ ((__no_instrument_function__))
#endif
#define TBX_WEAK        __attribute__ ((__weak__))
#if defined(DARWIN_SYS) || defined(OSF_SYS) || defined(IRIX_SYS)
/* those systems don't support aliases, or only weak aliases */
#  define TBX_FUN_ALIAS(ret,name,alias,proto,args) \
     ret name proto { return alias args; }
#  define TBX_FUN_WEAKALIAS(ret,name,alias,proto,args) \
     ret name proto TBX_WEAK { return alias args; }
#else
#  define TBX_FUN_ALIAS(ret,name,alias,proto,args) \
     __typeof__(name) name __attribute__ ((__alias__(TBX_STRING(alias))))
#  define TBX_FUN_WEAKALIAS(ret,name,alias,proto,args) \
     TBX_FUN_ALIAS(ret, name, alias, proto, args) TBX_WEAK
#endif
#define TBX_FORMAT(...) __attribute__ ((__format__(__VA_ARGS__)))

#if defined __OPTIMIZE__
#define __TBX_ALWAYS_INLINE	__attribute__((always_inline))
#else
#define __TBX_ALWAYS_INLINE
#endif

#if defined __INTEL_COMPILER
#  define __TBX_FUNCTION__		__func__
#  define TBX_FMALLOC			__attribute__ ((__malloc__))
#  define __tbx_inline__	__inline__ __TBX_ALWAYS_INLINE
#  define __tbx_setjmp_inline__	__inline__
#  define __tbx_deprecated__		__attribute__((deprecated))
#  define __tbx_attribute_used__	__attribute__((__used__))
#  define __tbx_attribute_pure__	__attribute__((pure))
#  define __tbx_warn_unused_result__
#  define tbx_prefetch(a,...)		__builtin_prefetch(a, ## __VA_ARGS__)
#  define TBX_RETURNS_TWICE

#elif __GNUC__ >= 4
#  define __TBX_FUNCTION__		__func__
#  define TBX_FMALLOC			__attribute__ ((__malloc__))
#  define __tbx_inline__	__inline__ __TBX_ALWAYS_INLINE
#  define __tbx_setjmp_inline__	__inline__
#  define __tbx_deprecated__		__attribute__((deprecated))
#  define __tbx_attribute_used__	__attribute__((__used__))
#  define __tbx_attribute_pure__	__attribute__((pure))
#  define __tbx_warn_unused_result__	__attribute__((warn_unused_result))
#  define __tbx_attribute_nonnull__(list)	__attribute__((__nonnull__ list))
#  if !defined(AIX_SYS) && !defined(DARWIN_SYS)
#    define TBX_VISIBILITY(vis) __attribute__ ((__visibility__(vis)))
#    define TBX_VISIBILITY_PUSH_DEFAULT _Pragma("GCC visibility push(default)")
#    define TBX_VISIBILITY_PUSH_INTERNAL _Pragma("GCC visibility push(internal)")
#    define TBX_VISIBILITY_POP _Pragma("GCC visibility pop")
#  endif
#  define tbx_prefetch(a,...)		__builtin_prefetch(a, ## __VA_ARGS__)
#  if __GNUC_MINOR__ >= 1
#    define TBX_RETURNS_TWICE		__attribute__((returns_twice))
#  endif

#elif __GNUC__ == 3
#  define TBX_FMALLOC			__attribute__ ((__malloc__))
#  if __GNUC_MINOR__ >= 1
#    define __tbx_inline__	__inline__ __TBX_ALWAYS_INLINE
#    define tbx_prefetch(a,...)		__builtin_prefetch(a, ## __VA_ARGS__)
#    if __GNUC_MINOR < 4
#      define __tbx_setjmp_inline__	__inline__ __TBX_ALWAYS_INLINE
#    else
#      define __tbx_setjmp_inline__	__inline__
#    endif
#  else
#    define tbx_prefetch(a,...)		(void)0
#  endif

#  if __GNUC_MINOR__ > 0
#    define __TBX_FUNCTION__		__func__
#    define __tbx_deprecated__		__attribute__((deprecated))
#  else
#    define __TBX_FUNCTION__		__FUNCTION__
#  endif

#  if __GNUC_MINOR__ >= 3
#    define __tbx_attribute_used__	__attribute__((__used__))
#    define __tbx_attribute_nonnull__(list)	__attribute__((__nonnull__ list))
#  else
#    define __tbx_attribute_used__	__attribute__((__unused__))
#  endif

#  define __tbx_attribute_pure__	__attribute__((__pure__))
#  define __tbx_warn_unused_result__
#elif __GNUC__ == 2
#  define __TBX_FUNCTION__		__FUNCTION__
#  if __GNUC_MINOR__ < 96
#    ifndef __builtin_expect
#      define __builtin_expect(x, expected_value) (x)
#    endif
#  endif

#  define __tbx_attribute_used__	__attribute__((__unused__))

#  if __GNUC_MINOR__ >= 96
#    define TBX_FMALLOC			__attribute__((__malloc__))
#    define __tbx_attribute_pure__	__attribute__((__pure__))
#  else
#    define TBX_FMALLOC
#  endif

#  define __restrict
#  define tbx_prefetch(a,...)		(void)0
#  define __tbx_warn_unused_result__
#else
#  error Sorry, your compiler is too old/not recognized.
#endif
#if (__GNUC__ <= 2) && (__GNUC__ != 2 || __GNUC_MINOR__ <= 8)
#  error Sorry, your compiler is too old/not recognized.
#endif

#ifndef TBX_VISIBILITY
#  define TBX_VISIBILITY(vis)
#  define TBX_VISIBILITY_PUSH_DEFAULT
#  define TBX_VISIBILITY_PUSH_INTERNAL
#  define TBX_VISIBILITY_POP
#endif
#define TBX_EXTERN TBX_VISIBILITY("default")
#define TBX_HIDDEN TBX_VISIBILITY("hidden")
#define TBX_PROTECTED TBX_VISIBILITY("protected")
#define TBX_INTERNAL TBX_VISIBILITY("internal")

#ifndef __tbx_inline__
#define __tbx_inline__ __inline__
#define __tbx_setjmp_inline__ __inline__
#endif

#ifdef PM2_NOINLINE
#undef __tbx_inline__
#undef __tbx_setjmp_inline__
#define __tbx_inline__ TBX_UNUSED
#define __tbx_setjmp_inline__ TBX_UNUSED
#define __tbx_extern_inline_body__(body) ;
#else
#define __tbx_extern_inline_body__(body) { body }
#endif

#ifdef DARWIN_SYS
#  define TBX_DATASECTION(secname)	__attribute__((__section__("__DATA," secname)))
#  define TBX_TEXTSECTION(secname)	__attribute__((__section__("__TEXT," secname)))
#else
#  define TBX_DATASECTION(secname)	__attribute__((__section__(secname)))
#  define TBX_TEXTSECTION(secname)	TBX_DATASECTION(secname)
#endif
#  define TBX_SECTION(secname)		TBX_DATASECTION(secname)

#ifndef TBX_RETURNS_TWICE
#define TBX_RETURNS_TWICE
#endif

#if defined(LINUX_SYS) && (defined(X86_ARCH) || defined(X86_64_ARCH) || defined(IA64_ARCH))
#define TBX_SYM_WARN(sym,msg) static const char __evoke_link_warning_##sym[] \
	__tbx_attribute_used__ TBX_SECTION(".gnu.warning."#sym"\n\t#") \
	= msg
#else
#define TBX_SYM_WARN(sym,msg)
#endif

/*
 * Atomic builtins
 */

#if (__GNUC__ > 4)
#define tbx_mb __sync_synchronize()
#endif
#if defined(__GNUC_MINOR)
#  if (__GNUC__ == 4 && __GNUC_MINOR >=1)
#    define tbx_mb __sync_synchronize()
#  endif
#endif

/*
 * Generic compiler-dependent macros required for kernel
 * build go below this comment. Actual compiler/compiler version
 * specific implementations come from the above header files
 */

#define tbx_likely(x)	__builtin_expect((long)!!(x), 1L)
#define tbx_unlikely(x)	__builtin_expect((long)!!(x), 0L)

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

#ifndef __tbx_attribute_nonnull__
# define __tbx_attribute_nonnull__	/* unimplemented */
#endif

#define TBX_PURE __tbx_attribute_pure__

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({      type __dummy; \
        __typeof__(x) __dummy2; \
        (void)(&__dummy == &__dummy2); \
        1; \
})

/* Static named initialization can't be done the same in C and C++ */
#if defined __cplusplus
#define TBX_INIT(var,value) var: value
#define TBX_INIT_USELESS(var,value) var: value
#else
#define TBX_INIT(var,value) .var = value
#define TBX_INIT_USELESS(var,value)
#endif

/* Not all systems define __THROW */
#if !defined(__THROW)
# if defined __cplusplus && (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define __THROW throw ()
# else
#  define __THROW
# endif
#endif

/* C++ needs to know that types and declarations are C, not C++.  */
#ifdef  __cplusplus
# define __TBX_BEGIN_DECLS  extern "C" {
# define __TBX_END_DECLS    }
#else
# define __TBX_BEGIN_DECLS
# define __TBX_END_DECLS
#endif

/* @} */

#endif /* TBX_COMPILER_H_EST_DEF */
