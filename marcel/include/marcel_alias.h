/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_ALIAS_H__
#define __MARCEL_ALIAS_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_utils.h"


/** Public macros **/
#define NAME_PREFIX
#define LPT_PREFIX lpt_
#define PTHREAD_PREFIX pthread_
#define MARCEL_PREFIX marcel_
#define PMARCEL_PREFIX pmarcel_

#define MA__MODE_MARCEL 1
#define MA__MODE_PMARCEL 2
#define MA__MODE_LPT 3
#define MA__MODE_PTHREAD 4

#define MARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, )
#define PMARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, PMARCEL_PREFIX, )
#define LPT_NAME(symbol) PREFIX_SYMBOL(symbol, LPT_PREFIX, )
#define PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, )
#define __PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, __)
#define LIBC_NAME(symbol) PREFIX_SYMBOL(symbol, , )
#define __LIBC_NAME(symbol) PREFIX_SYMBOL(symbol, , __)

#define PREFIX_SYMBOL(symbol, p3, p1) _PREFIX_SYMBOL(symbol, p1, NAME_PREFIX, p3)
#define _PREFIX_SYMBOL(symbol, p1, p2, p3) __PREFIX_SYMBOL(symbol, p1, p2, p3)
#define __PREFIX_SYMBOL(symbol, p1, p2, p3) p1##p2##p3##symbol

/****************************************************************
 * Déclarations
 */

/* Note: we usually do not declare pmarcel_name as they are declared in
 * include/marcel_pmarcel.h with a pthread API (which is not compatible with
 * the marcel API, notably marcel_t/pmarcel_t) */

/* Declare marcel_name */
#define DEC_MARCEL(rtype, name, proto)		\
	extern rtype MARCEL_NAME(name) proto


/****************************************************************
 * Définitions
 */
/* Define marcel_name */
#define DEF_MARCEL(rtype, name, proto, args, code)	\
	rtype MARCEL_NAME(name) proto code		\

/* Define pmarcel_name / marcel_name */
#ifdef MA__IFACE_PMARCEL
#define DEF_PMARCEL(rtype, name, proto, args, code)	\
	rtype PMARCEL_NAME(name) proto code
#define DEF_MARCEL_PMARCEL(rtype, name, proto, args, code)		\
	DEF_MARCEL(rtype, name, proto, args, code)			\
	TBX_FUN_ALIAS(rtype, MARCEL_NAME(name), PMARCEL_NAME(name), proto, args);
#else
#define DEF_PMARCEL(rtype, name, proto, args, code)
#define DEF_MARCEL_PMARCEL(rtype, name, proto, args, code)	\
	DEF_MARCEL(rtype, name, proto, args, code)
#endif

#define strong_alias_t(rtype, target, aliasname, proto, args)		\
	_strong_alias_t(rtype, target, aliasname, proto, args)
#define _strong_alias_t(type, target, aliasname, proto, args)		\
	extern TBX_FUN_ALIAS(type, target, aliasname, proto, args);
#define ma_strong_alias(target, aliasname)				\
	extern __typeof__ (target) aliasname __attribute__ ((alias (#target)));
#define weak_alias_t(rtype, target, aliasname, proto, args)	\
	_weak_alias_t (rtype, target, aliasname, proto, args)
#define _weak_alias_t(rtype, target, aliasname, proto, args)			\
	extern TBX_FUN_WEAKALIAS(rtype, target, aliasname, proto, args);
#define ma_weak_alias(target, aliasname)				\
	extern __typeof__ (target) aliasname __attribute__ ((weak, alias (#target)));

#ifdef MA__LIBPTHREAD
#  define DEF_STRONG_T(rtype, target, aliasname, proto, args)	\
	strong_alias_t(rtype, target, aliasname, proto, args)
#  define DEF_WEAK_T(rtype, target, aliasname, proto, args)	\
	weak_alias_t(rtype, target, aliasname, proto, args)
#  define DEF_STRONG_ALIAS(target, aliasname)	\
	ma_strong_alias(target, aliasname)
#  define DEF_WEAK_ALIAS(target, aliasname)	\
	ma_weak_alias(target, aliasname)
	
#else
#  define DEF_STRONG_T(rtype, target, aliasname, proto, args)
#  define DEF_WEAK_T(rtype, target, aliasname, proto, args)
#  define DEF_STRONG_ALIAS(target, aliasname)
#  define DEF_WEAK_ALIAS(target, aliasname)
#endif


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/*
 * To be used when the pmarcel ABI is the same as the system's libpthread ABI
 */
#ifdef MA__LIBPTHREAD
/* Define a strong alias pthread_name -> pmarcel_name */
#define DEF_PTHREAD_STRONG(rtype, name, proto, args)			\
	DEF_STRONG_T(rtype, PMARCEL_NAME(name), PTHREAD_NAME(name), proto, args)
/* Define a weak alias pthread_name -> pmarcel_name */
#define DEF_PTHREAD_WEAK(rtype, name, proto, args)			\
	DEF_WEAK_T(rtype, PMARCEL_NAME(name), PTHREAD_NAME(name), proto, args)
/* Define a strong alias __pthread_name -> pmarcel_name */
#define DEF___PTHREAD_STRONG(rtype, name, proto, args)			\
	extern __typeof__(PMARCEL_NAME(name)) __PTHREAD_NAME(name);	\
	DEF_STRONG_T(rtype, PMARCEL_NAME(name), __PTHREAD_NAME(name), proto, args)
/* Define a weak alias __pthread_name -> pmarcel_name */
#define DEF___PTHREAD_WEAK(rtype, name, proto, args)			\
	extern __typeof__(PMARCEL_NAME(name)) __PTHREAD_NAME(name);	\
	DEF_WEAK_T(rtype, PMARCEL_NAME(name), __PTHREAD_NAME(name), proto, args)
#else
#define DEF_PTHREAD_STRONG(rtype, name, proto, args)
#define DEF_PTHREAD_WEAK(rtype, name, proto, args)
#define DEF___PTHREAD_STRONG(rtype, name, proto, args)
#define DEF___PTHREAD_WEAK(rtype, name, proto, args)
#endif

/* Define strong aliases by default */
#define DEF_PTHREAD(rtype, name, proto, args)		\
	DEF_PTHREAD_STRONG(rtype, name, proto, args)
#define DEF___PTHREAD(rtype, name, proto, args)		\
	DEF___PTHREAD_STRONG(rtype, name, proto, args)


#ifdef MA__LIBPTHREAD
/* Define a strong alias name -> pmarcel_name */
#define DEF_C_STRONG(rtype, name, proto, args)				\
	DEF_STRONG_T(rtype, PMARCEL_NAME(name), LIBC_NAME(name), proto, args)
/* Define a weak alias name -> pmarcel_name */
#define DEF_C_WEAK(rtype, name, proto, args)				\
	DEF_WEAK_T(rtype, PMARCEL_NAME(name), LIBC_NAME(name), proto, args)
/* Define a strong __alias name -> pmarcel_name */
#define DEF___C_STRONG(rtype, name, proto, args)			\
	extern __typeof__(PMARCEL_NAME(name)) __LIBC_NAME(name);	\
	DEF_STRONG_T(rtype, PMARCEL_NAME(name), __LIBC_NAME(name), proto, args)
/* Define a weak __alias name -> pmarcel_name */
#define DEF___C_WEAK(rtype, name, proto, args)				\
	extern __typeof__(PMARCEL_NAME(name)) __LIBC_NAME(name);	\
	DEF_WEAK_T(rtype, PMARCEL_NAME(name), __LIBC_NAME(name), proto, args)
#else
#define DEF_C_STRONG(rtype, name, proto, args)
#define DEF_C_WEAK(rtype, name, proto, args)
#define DEF___C_STRONG(rtype, name, proto, args)
#define DEF___C_WEAK(rtype, name, proto, args)
#endif

/* Define strong aliases by default */
#define DEF_C(rtype, name, proto, args)		\
	DEF_C_STRONG(rtype, name, proto, args)
#define DEF___C(rtype, name, proto, args)		\
	DEF___C_STRONG(rtype, name, proto, args)

/*
 * To be used when the pmarcel ABI is NOT the same as the system's libpthread
 * ABI, when then define lpt_name functions to provide the system's ABI
 */
#ifdef MA__LIBPTHREAD
/* Define a strong alias pthread_name -> lpt_name */
#define DEF_LIBPTHREAD_STRONG(rtype, name, proto, args)			\
	DEF_STRONG_T(rtype, LPT_NAME(name), PTHREAD_NAME(name), proto, args)
/* Define a weak alias pthread_name -> lpt_name */
#define DEF_LIBPTHREAD_WEAK(rtype, name, proto, args)			\
	DEF_WEAK_T(rtype, LPT_NAME(name), PTHREAD_NAME(name), proto, args)
/* Define a strong alias __pthread_name -> lpt_name */
#define DEF___LIBPTHREAD_STRONG(rtype, name, proto, args)		\
	extern __typeof__(LPT_NAME(name)) __PTHREAD_NAME(name);		\
	DEF_STRONG_T(rtype, LPT_NAME(name), __PTHREAD_NAME(name), proto, args)
/* Define a weak alias __pthread_name -> lpt_name */
#define DEF___LIBPTHREAD_WEAK(rtype, name, proto, args)			\
	extern __typeof__(LPT_NAME(name)) __PTHREAD_NAME(name);		\
	DEF_WEAK_T(rtype, LPT_NAME(name), __PTHREAD_NAME(name), proto, args)
#else
#define DEF_LIBPTHREAD_STRONG(rtype, name, proto, args)
#define DEF_LIBPTHREAD_WEAK(rtype, name, proto, args)
#define DEF___LIBPTHREAD_STRONG(rtype, name, proto, args)
#define DEF___LIBPTHREAD_WEAK(rtype, name, proto, args)
#endif

/* Define strong aliases by default */
#define DEF_LIBPTHREAD(rtype, name, proto, args)	\
	DEF_LIBPTHREAD_STRONG(rtype, name, proto, args)
#define DEF___LIBPTHREAD(rtype, name, proto, args)		\
	DEF___LIBPTHREAD_STRONG(rtype, name, proto, args)
/* Note that libc always has versioned symbols, thus setting GLIBC_2_0 by
 * default (which is leveraged to the correct version automatically according
 * to the architecture) */
/* Define a strong alias name -> lpt_name */
#ifdef MA__LIBPTHREAD
#define DEF_LIBC_STRONG(rtype, name, proto, args)			\
	versioned_symbol(libpthread, LPT_NAME(name), LIBC_NAME(name), GLIBC_2_0);
/* Define a weak alias name -> lpt_name */
#define DEF_LIBC_WEAK(rtype, name, proto, args)				\
	DEF_WEAK_T(rtype, LPT_NAME(name), LIBC_NAME(name), proto, args)
/* Define a strong alias __name -> lpt_name */
#define DEF___LIBC_STRONG(rtype, name, proto, args)			\
	extern __typeof__(LPT_NAME(name)) __LIBC_NAME(name);		\
	DEF_STRONG_T(rtype, LPT_NAME(name), __LIBC_NAME(name), proto, args)
/* Define a weak alias __name -> lpt_name */
#define DEF___LIBC_WEAK(rtype, name, proto, args)			\
	extern __typeof__(LPT_NAME(name)) __LIBC_NAME(name);		\
	DEF_WEAK_T(rtype, LPT_NAME(name), __LIBC_NAME(name), proto, args)
#else
#define DEF_LIBC_STRONG(rtype, name, proto, args)
#define DEF_LIBC_WEAK(rtype, name, proto, args)
#define DEF___LIBC_STRONG(rtype, name, proto, args)
#define DEF___LIBC_WEAK(rtype, name, proto, args)
#endif

/* Define strong aliases by default */
#define DEF_LIBC(rtype, name, proto, args)		\
	DEF_LIBC_STRONG(rtype, name, proto, args)
#define DEF___LIBC(rtype, name, proto, args)		\
	DEF___LIBC_STRONG(rtype, name, proto, args)

/****************************************************************/
/* Fonctions internes à marcel
 */
#define MARCEL_INT(func)
#ifdef MA__LIBPTHREAD
/* Default minimum glibc versions */
#if defined(X86_ARCH)
#define MA_GLIBC_VERSION_MINIMUM	20000
#define GLIBC_MINI			GLIBC_2.0
#elif defined(IA64_ARCH)
#define MA_GLIBC_VERSION_MINIMUM	20200
#define GLIBC_MINI			GLIBC_2.2
#elif defined(X86_64_ARCH)
#define MA_GLIBC_VERSION_MINIMUM	20205
#define GLIBC_MINI			GLIBC_2.2.5
#endif
#if MA_GLIBC_VERSION_MINIMUM <= 20000
#define VERSION_libpthread_GLIBC_2_0    GLIBC_2.0
#define VERSION_libpthread_GLIBC_2_1    GLIBC_2.1
#define VERSION_libpthread_GLIBC_2_1_1  GLIBC_2.1.1
#define VERSION_libpthread_GLIBC_2_1_2  GLIBC_2.1.2
#endif
#if MA_GLIBC_VERSION_MINIMUM <= 20200
#define VERSION_libpthread_GLIBC_2_2    GLIBC_2.2
#define VERSION_libpthread_GLIBC_2_2_3  GLIBC_2.2.3
#endif
#define VERSION_libpthread_GLIBC_2_2_5  GLIBC_2.2.5
#define VERSION_libpthread_GLIBC_2_2_6  GLIBC_2.2.6
#define VERSION_libpthread_GLIBC_2_3_2  GLIBC_2.3.2
#define VERSION_libpthread_GLIBC_2_3_3  GLIBC_2.3.3
#define VERSION_libpthread_GLIBC_2_3_4  GLIBC_2.3.4
#define VERSION_libpthread_GLIBC_2_4    GLIBC_2.4
#define VERSION_libpthread_GLIBC_2_12   GLIBC_2.12
#define VERSION_libpthread_GLIBC_PRIVATE        GLIBC_PRIVATE
#ifndef VERSION_libpthread_GLIBC_2_0
#define VERSION_libpthread_GLIBC_2_0    GLIBC_MINI
#endif
#ifndef VERSION_libpthread_GLIBC_2_1
#define VERSION_libpthread_GLIBC_2_1    GLIBC_MINI
#endif
#ifndef VERSION_libpthread_GLIBC_2_1_1
#define VERSION_libpthread_GLIBC_2_1_1    GLIBC_MINI
#endif
#ifndef VERSION_libpthread_GLIBC_2_1_2
#define VERSION_libpthread_GLIBC_2_1_2    GLIBC_MINI
#endif
#ifndef VERSION_libpthread_GLIBC_2_2
#define VERSION_libpthread_GLIBC_2_2    GLIBC_MINI
#endif
#ifndef VERSION_libpthread_GLIBC_2_2_3
#define VERSION_libpthread_GLIBC_2_2_3  GLIBC_MINI
#endif

/* That header also defines symbols like `VERSION_libm_GLIBC_2_1' to
   the version set name to use for e.g. symbols first introduced into
   libm in the GLIBC_2.1 version.  Definitions of symbols with explicit
   versions should look like:
   versioned_symbol (libm, new_foo, foo, GLIBC_2_1);
   This will define the symbol `foo' with the appropriate default version,
   i.e. either GLIBC_2.1 or the "earliest version" specified in
   shlib-versions if that is newer.  */
# define versioned_symbol(lib, local, symbol, version)			\
	versioned_symbol_1 (local, symbol, VERSION_##lib##_##version)
# define versioned_symbol_1(local, symbol, name)	\
	default_symbol_version (local, symbol, name)
# define compat_symbol(lib, local, symbol, version)			\
	compat_symbol_1 (local, symbol, VERSION_##lib##_##version)
# define compat_symbol_1(local, symbol, name)	\
	symbol_version (local, symbol, name)
# ifdef __ASSEMBLER__
#  define symbol_version(real, name, version) \
	.symver real, name##@##version
#  define default_symbol_version(real, name, version) \
	.symver real, name##@##@##version
# else
#  define symbol_version(real, name, version) \
	__asm__ (".symver " #real "," #name "@" #version)
#  define default_symbol_version(real, name, version) \
	__asm__ (".symver " #real "," #name "@@" #version)
# endif
#endif				/* MA__LIBPTHREAD */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_ALIAS_H__ **/
