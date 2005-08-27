
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

#section common
#include "tbx_compiler.h"

#section macros
#depend "marcel_utils.h[stringification]"

#define NAME_PREFIX
#define LPT_PREFIX lpt_
#define PTHREAD_PREFIX pthread_
#define MARCEL_PREFIX marcel_
#define POSIX_PREFIX pmarcel_

#define LOCAL_MARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, __)
#define MARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, )
#define MARCEL_INLINE_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, __inline_)
#define LOCAL_POSIX_NAME(symbol) PREFIX_SYMBOL(symbol, POSIX_PREFIX, __)
#define POSIX_NAME(symbol) PREFIX_SYMBOL(symbol, POSIX_PREFIX, )
#define LPT_NAME(symbol) PREFIX_SYMBOL(symbol, LPT_PREFIX, )
#define PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, )
#define __PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, __)

#define PREFIX_SYMBOL(symbol, p3, p1) _PREFIX_SYMBOL(symbol, p1, NAME_PREFIX, p3)
#define _PREFIX_SYMBOL(symbol, p1, p2, p3) __PREFIX_SYMBOL(symbol, p1, p2, p3)
#define __PREFIX_SYMBOL(symbol, p1, p2, p3) \
  p1##p2##p3##symbol


  /* Difficile à réaliser de manière portable...
#define LOCAL_SYMBOL(local_symbol, symbol) \
  extern __typeof(symbol) local_symbol \
    TBX_ALIAS(#symbol)
// l'attribut visibility n'est pas encore disponible...
//    TBX_ALIAS(#symbol) TBX_VISIBILITY("hidden")

#define ALIAS_SYMBOL(alias_symbol, symbol) \
  extern __typeof(symbol) alias_symbol \
    TBX_ALIAS(#symbol)
    */


#ifdef HAVE_VISIBILITY_ATTRIBUTE
#define LOCAL_ATTRIBUTE TBX_VISIBILITY("hidden")
#else
#define LOCAL_ATTRIBUTE //WEAK_ATTRIBUTE
#endif

#define ADD_WEAK_ATTRIBUTE ,weak
#define WEAK_ATTRIBUTE TBX_WEAK

/****************************************************************
 * Déclarations
 */

#define DEC_LOCAL_MARCEL(name) \
  extern typeof(MARCEL_NAME(name)) LOCAL_MARCEL_NAME(name) LOCAL_ATTRIBUTE

#ifdef MA__POSIX_FUNCTIONS_NAMES
# define DEC_LOCAL_POSIX(name) \
    extern typeof(POSIX_NAME(name)) LOCAL_POSIX_NAME(name);
#else
#  define DEC_LOCAL_POSIX(name)
#endif

#define DEC_MARCEL_POSIX(rtype, name, proto) \
  extern rtype MARCEL_NAME(name) proto; \
  DEC_LOCAL_POSIX(name) \
  DEC_LOCAL_MARCEL(name)

#define DEC_MARCEL(rtype, name, proto) \
  extern rtype MARCEL_NAME(name) proto; \
  DEC_LOCAL_MARCEL(name)

#ifdef MA__POSIX_FUNCTIONS_NAMES
#define DEC_POSIX(rtype, name, proto) \
  extern rtype POSIX_NAME(name) proto; \
  DEC_LOCAL_POSIX(name)
#else
#define DEC_POSIX(rtype, name, proto)
#endif

#define DECINLINE_MARCEL_POSIX(rtype, name, proto, code) \
  extern __tbx_inline__ rtype MARCEL_NAME(name) proto code; \
  DEC_LOCAL_POSIX(name) \
  DEC_LOCAL_MARCEL(name); \
  static __tbx_inline__ rtype MARCEL_INLINE_NAME(name) proto code

#define DECINLINE_MARCEL(rtype, name, proto) \
  static __tbx_inline__ rtype MARCEL_NAME(name) proto code \
  DEC_LOCAL_MARCEL(name); \
  static __tbx_inline__ rtype MARCEL_INLINE_NAME(name) proto code

#ifdef __USE_UNIX98
#  define DEC_MARCEL_UNIX98(rtype, name, proto) \
     DEC_MARCEL_POSIX(rtype, name, proto)
#  ifdef __USE_XOPEN2K
#    define DEC_MARCEL_XOPEN2K(rtype, name, proto) \
       DEC_MARCEL_POSIX(rtype, name, proto)
#  else /* __USE_XOPEN2K */
#    define DEC_MARCEL_XOPEN2K(rtype, name, proto) \
       DEC_MARCEL(rtype, name, proto)
#  endif /* __USE_XOPEN2K */
#else /* __USE_UNIX98 */
#  define DEC_MARCEL_UNIX98(rtype, name, proto) \
     DEC_MARCEL(rtype, name, proto)
#  define DEC_MARCEL_XOPEN2K(rtype, name, proto) \
     DEC_MARCEL(rtype, name, proto)
#endif /* __USE_UNIX98 */

#ifdef __USE_GNU
#  define DEC_MARCEL_GNU(rtype, name, proto) \
     DEC_MARCEL_POSIX(rtype, name, proto)
#else
#  define DEC_MARCEL_GNU(rtype, name, proto) \
     DEC_MARCEL(rtype, name, proto)
#endif

/****************************************************************
 * Définitions
 */

#ifdef MA__POSIX_FUNCTIONS_NAMES
# define DEF_ALIAS_POSIX_OF_MARCEL(rtype, name, proto, args) \
    TBX_FUN_ALIAS(rtype, LOCAL_POSIX_NAME(name), \
      LOCAL_MARCEL_NAME(name), proto, args);
# define DEF_ALIAS_POSIX(rtype, name, proto, args) \
    TBX_FUN_ALIAS(rtype, POSIX_NAME(name), \
      LOCAL_POSIX_NAME(name), proto, args);
    //TBX_FUN_WEAKALIAS(rtype, POSIX_NAME(name),
    //  LOCAL_POSIX_NAME(name), proto, args);
#else
# define DEF_ALIAS_POSIX_OF_MARCEL(rtype, name, proto, args)
# define DEF_ALIAS_POSIX(rtype, name, proto, args)
#endif
#define DEF_ALIAS_MARCEL(rtype, name, proto, args) \
  TBX_FUN_ALIAS(rtype, MARCEL_NAME(name), \
    LOCAL_MARCEL_NAME(name), proto, args);
#define DEF_ALIAS_LOCAL_MARCEL(rtype, name, proto, args) \
  TBX_FUN_ALIAS(rtype, LOCAL_MARCEL_NAME(name), \
    MARCEL_NAME(name), proto, args);


#define DEF_MARCEL_POSIX(rtype, name, proto, args) \
  DEF_ALIAS_MARCEL(rtype, name, proto, args) \
  DEF_ALIAS_POSIX(rtype, name, proto, args) \
  DEF_ALIAS_POSIX_OF_MARCEL(rtype, name, proto, args) \
  rtype LOCAL_MARCEL_NAME(name) proto

#define DEF_MARCEL(rtype, name, proto, args) \
  DEF_ALIAS_MARCEL(rtype, name, proto, args) \
  rtype LOCAL_MARCEL_NAME(name) proto

#ifdef MA__POSIX_FUNCTIONS_NAMES
#define DEF_POSIX(rtype, name, proto, args, code) \
  DEF_ALIAS_POSIX(rtype, name, proto, args) \
  rtype LOCAL_POSIX_NAME(name) proto code
#else
#define DEF_POSIX(rtype, name, proto, args, code)
#endif
  
#define DEFINLINE_MARCEL_POSIX(rtype, name, proto, args) \
  TBX_FUN_ALIAS(rtype, MARCEL_NAME(name), \
    MARCEL_INLINE_NAME(name), proto, args); \
  DEF_ALIAS_LOCAL_MARCEL(rtype, name, proto, args) \
  DEF_ALIAS_POSIX(rtype, name, proto, args) \
  DEF_ALIAS_POSIX_OF_MARCEL(rtype, name, proto, args)

#define DEFINLINE_MARCEL(rtype, name, proto, args) \
  DEF_ALIAS_LOCAL_MARCEL(rtype, name, proto, args)

#define strong_alias_t(rtype, name, alias, proto, args) \
  _strong_alias_t(rtype, name, alias, proto, args)
#define _strong_alias_t(type, name, alias, proto, args) \
  extern TBX_FUN_ALIAS(type, name, alias, proto, args);

#define weak_alias_t(rtype, name, alias, proto, args) \
  _weak_alias_t (rtype, name, alias, proto, args)
#define _weak_alias_t(rtype, name, alias, proto, args) \
  extern TBX_FUN_WEAKALIAS(rtype, name, alias, proto, args);

#ifdef MA__PTHREAD_FUNCTIONS
#  define DEF_STRONG_T(rtype, name, aliasname, proto, args) \
  strong_alias_t(rtype, aliasname, name, proto, args)
#  define DEF_WEAK_T(rtype, name, aliasname, proto, args) \
  weak_alias_t(rtype, aliasname, name, proto, args)
#else
#  define DEF_STRONG_T(rtype, name, aliasname, proto, args)
#  define DEF_WEAK_T(rtype, name, aliasname, proto, args)
#endif

#ifdef MA__PTHREAD_FUNCTIONS
#define DEF_PTHREAD_STRONG(rtype, name, proto, args) \
  DEF_STRONG_T(rtype, POSIX_NAME(name), PTHREAD_NAME(name), proto, args)
#define DEF_PTHREAD_WEAK(rtype, name, proto, args) \
  DEF_WEAK_T(rtype, POSIX_NAME(name), PTHREAD_NAME(name), proto, args)
#define DEF___PTHREAD_STRONG(rtype, name, proto, args) \
  extern typeof(POSIX_NAME(name)) __PTHREAD_NAME(name); \
  DEF_STRONG_T(rtype, POSIX_NAME(name), __PTHREAD_NAME(name), proto, args)
#define DEF___PTHREAD_WEAK(rtype, name, proto, args) \
  extern typeof(POSIX_NAME(name)) __PTHREAD_NAME(name); \
  DEF_WEAK_T(rtype, POSIX_NAME(name), __PTHREAD_NAME(name), proto, args)
#else
#define DEF_PTHREAD_STRONG(rtype, name, proto, args)
#define DEF_PTHREAD_WEAK(rtype, name, proto, args)
#define DEF___PTHREAD_STRONG(rtype, name, proto, args)
#define DEF___PTHREAD_WEAK(rtype, name, proto, args)
#endif

#define DEF_PTHREAD(rtype, name, proto, args) \
  DEF_PTHREAD_STRONG(rtype, name, proto, args)
#define DEF___PTHREAD(rtype, name, proto, args) \
  DEF___PTHREAD_STRONG(rtype, name, proto, args)

#define DEF_LIBPTHREAD_STRONG(rtype, name, proto, args) \
  DEF_STRONG_T(rtype, LPT_NAME(name), PTHREAD_NAME(name), proto, args)
#define DEF_LIBPTHREAD_WEAK(rtype, name, proto, args) \
  DEF_WEAK_T(rtype, LPT_NAME(name), PTHREAD_NAME(name), proto, args)
#define DEF___LIBPTHREAD_STRONG(rtype, name, proto, args) \
  extern typeof(LPT_NAME(name)) __PTHREAD_NAME(name); \
  DEF_STRONG_T(rtype, LPT_NAME(name), __PTHREAD_NAME(name), proto, args)
#define DEF___LIBPTHREAD_WEAK(rtype, name, proto, args) \
  extern typeof(LPT_NAME(name)) __PTHREAD_NAME(name); \
  DEF_WEAK_T(rtype, LPT_NAME(name), __PTHREAD_NAME(name), proto, args)

#define DEF_LIBPTHREAD(rtype, name, proto, args) \
  DEF_LIBPTHREAD_STRONG(rtype, name, proto, args)
#define DEF___LIBPTHREAD(rtype, name, proto, args) \
  DEF___LIBPTHREAD_STRONG(rtype, name, proto, args)

/****************************************************************/
/* Fonctions internes à marcel
 */
#define MARCEL_INT(func) 


#ifdef MA__PTHREAD_FUNCTIONS
/* start libpthread */
#define ABI_libpthread_GLIBC_2_0        1       /* support GLIBC_2.0 */
#define VERSION_libpthread_GLIBC_2_0    GLIBC_2.0
#define ABI_libpthread_GLIBC_2_1        2       /* support GLIBC_2.1 */
#define VERSION_libpthread_GLIBC_2_1    GLIBC_2.1
#define ABI_libpthread_GLIBC_2_1_1      3       /* support GLIBC_2.1.1 */
#define VERSION_libpthread_GLIBC_2_1_1  GLIBC_2.1.1
#define ABI_libpthread_GLIBC_2_1_2      4       /* support GLIBC_2.1.2 */
#define VERSION_libpthread_GLIBC_2_1_2  GLIBC_2.1.2
#define ABI_libpthread_GLIBC_2_2        5       /* support GLIBC_2.2 */
#define VERSION_libpthread_GLIBC_2_2    GLIBC_2.2
/* end libpthread */

/* Thread creation, initialization, and basic low-level routines */
# define SHLIB_COMPAT(lib, introduced, obsoleted) \
  (!(ABI_##lib##_##obsoleted - 0) \
   || ((ABI_##lib##_##introduced - 0) < (ABI_##lib##_##obsoleted - 0)))

/* That header also defines symbols like `VERSION_libm_GLIBC_2_1' to
   the version set name to use for e.g. symbols first introduced into
   libm in the GLIBC_2.1 version.  Definitions of symbols with explicit
   versions should look like:
        versioned_symbol (libm, new_foo, foo, GLIBC_2_1);
   This will define the symbol `foo' with the appropriate default version,
   i.e. either GLIBC_2.1 or the "earliest version" specified in
   shlib-versions if that is newer.  */

# define versioned_symbol(lib, local, symbol, version) \
  versioned_symbol_1 (local, symbol, VERSION_##lib##_##version)
# define versioned_symbol_1(local, symbol, name) \
  default_symbol_version (local, symbol, name)

# define compat_symbol(lib, local, symbol, version) \
  compat_symbol_1 (local, symbol, VERSION_##lib##_##version)
# define compat_symbol_1(local, symbol, name) \
  symbol_version (local, symbol, name)

#endif /* MA__PTHREAD_FUNCTIONS */

