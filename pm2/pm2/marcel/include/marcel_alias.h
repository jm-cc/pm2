
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
#define PTHREAD_PREFIX pthread_
#define MARCEL_PREFIX marcel_
#define POSIX_PREFIX pmarcel_

#define LOCAL_MARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, __)
#define MARCEL_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, )
#define MARCEL_INLINE_NAME(symbol) PREFIX_SYMBOL(symbol, MARCEL_PREFIX, __inline_)
#define LOCAL_POSIX_NAME(symbol) PREFIX_SYMBOL(symbol, POSIX_PREFIX, __)
#define POSIX_NAME(symbol) PREFIX_SYMBOL(symbol, POSIX_PREFIX, )
#define PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, )
#define __PTHREAD_NAME(symbol) PREFIX_SYMBOL(symbol, PTHREAD_PREFIX, __)

#define PREFIX_SYMBOL(symbol, p3, p1) _PREFIX_SYMBOL(symbol, p1, NAME_PREFIX, p3)
#define _PREFIX_SYMBOL(symbol, p1, p2, p3) __PREFIX_SYMBOL(symbol, p1, p2, p3)
#define __PREFIX_SYMBOL(symbol, p1, p2, p3) \
  p1##p2##p3##symbol


#define LOCAL_SYMBOL(local_symbol, symbol) \
  extern __typeof(symbol) local_symbol \
    TBX_ALIAS(#symbol)
// l'attribut visibility n'est pas encore disponible...
//    TBX_ALIAS(#symbol) TBX_VISIBILITY("hidden")

#define ALIAS_SYMBOL(alias_symbol, symbol) \
  extern __typeof(symbol) alias_symbol \
    TBX_ALIAS(#symbol)


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
  extern typeof(MARCEL_NAME(name)) LOCAL_MARCEL_NAME(name) LOCAL_ATTRIBUTE;

#ifdef MA__POSIX_FUNCTIONS_NAMES
# define DEC_LOCAL_POSIX(name) \
    typeof(POSIX_NAME(name)) LOCAL_POSIX_NAME(name);
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
  extern inline rtype MARCEL_NAME(name) proto code; \
  DEC_LOCAL_POSIX(name) \
  DEC_LOCAL_MARCEL(name) \
  static inline rtype MARCEL_INLINE_NAME(name) proto code

#define DECINLINE_MARCEL(rtype, name, proto) \
  static inline rtype MARCEL_NAME(name) proto code \
  DEC_LOCAL_MARCEL(name) \
  static inline rtype MARCEL_INLINE_NAME(name) proto code

/****************************************************************
 * Définitions
 */

#ifdef MA__POSIX_FUNCTIONS_NAMES
# define DEF_ALIAS_POSIX_OF_MARCEL(name) \
    typeof(LOCAL_POSIX_NAME(name)) LOCAL_POSIX_NAME(name) \
      TBX_ALIAS(marcel_xstr(LOCAL_MARCEL_NAME(name)));
# define DEF_ALIAS_POSIX(name) \
    typeof(POSIX_NAME(name)) POSIX_NAME(name) \
      TBX_ALIAS(marcel_xstr(LOCAL_POSIX_NAME(name)));
//      TBX_ALIAS(xstr(LOCAL_POSIX_NAME(name))) TBX_WEAK;
#else
# define DEF_ALIAS_POSIX_OF_MARCEL(name)
# define DEF_ALIAS_POSIX(name)
#endif
#define DEF_ALIAS_MARCEL(name) \
  typeof(MARCEL_NAME(name)) MARCEL_NAME(name) \
    TBX_ALIAS(marcel_xstr(LOCAL_MARCEL_NAME(name)));
#define DEF_ALIAS_LOCAL_MARCEL(name) \
  typeof(LOCAL_MARCEL_NAME(name)) LOCAL_MARCEL_NAME(name) \
    TBX_ALIAS(marcel_xstr(MARCEL_NAME(name)));


#define DEF_MARCEL_POSIX(rtype, name, proto) \
  DEF_ALIAS_MARCEL(name) \
  DEF_ALIAS_POSIX(name) \
  DEF_ALIAS_POSIX_OF_MARCEL(name) \
  rtype LOCAL_MARCEL_NAME(name) proto

#define DEF_MARCEL(rtype, name, proto) \
  DEF_ALIAS_MARCEL(name) \
  rtype LOCAL_MARCEL_NAME(name) proto

#ifdef MA__POSIX_FUNCTIONS_NAMES
#define DEF_POSIX(rtype, name, proto, code) \
  DEF_ALIAS_POSIX(name) \
  rtype LOCAL_POSIX_NAME(name) proto code
#else
#define DEF_POSIX(rtype, name, proto, code)
#endif
  
#define DEFINLINE_MARCEL_POSIX(rtype, name, proto) \
  typeof(MARCEL_NAME(name)) MARCEL_NAME(name) \
    TBX_ALIAS(marcel_xstr(MARCEL_INLINE_NAME(name))); \
  DEF_ALIAS_LOCAL_MARCEL(name) \
  DEF_ALIAS_POSIX(name) \
  DEF_ALIAS_POSIX_OF_MARCEL(name)

#define DEFINLINE_MARCEL(rtype, name, proto) \
  DEF_ALIAS_LOCAL_MARCEL(name)

# define strong_alias_t(t, name, aliasname) _strong_alias_t(t, name, aliasname)
# define _strong_alias_t(type, name, aliasname) \
  extern __typeof (type) aliasname TBX_ALIAS (#name);

#  define weak_alias_t(t, name, aliasname) _weak_alias_t (t, name, aliasname)
#  define _weak_alias_t(type, name, aliasname) \
  extern __typeof (type) aliasname TBX_WEAK TBX_ALIAS(#name);

#ifdef MA__PTHREAD_FUNCTIONS
#define DEF_STRONG_T(type, name, aliasname) \
  strong_alias_t(type, name, aliasname)
#define DEF_WEAK_T(type, name, aliasname) \
  weak_alias_t(type, name, aliasname)
#else
#define DEF_STRONG_T(type, name, aliasname)
#define DEF_WEAK_T(type, name, aliasname)
#endif

#define DEF_PTHREAD_STRONG(name) \
  DEF_STRONG_T(PTHREAD_NAME(name), \
               POSIX_NAME(name), PTHREAD_NAME(name))
#define DEF_PTHREAD_WEAK(name) \
  DEF_WEAK_T(PTHREAD_NAME(name), \
             POSIX_NAME(name), PTHREAD_NAME(name))
#define DEF___PTHREAD_STRONG(name) \
  DEF_STRONG_T(PTHREAD_NAME(name), \
               POSIX_NAME(name), __PTHREAD_NAME(name))
#define DEF___PTHREAD_WEAK(name) \
  DEF_WEAK_T(PTHREAD_NAME(name), \
             POSIX_NAME(name), __PTHREAD_NAME(name))

#define DEF_PTHREAD(name) DEF_PTHREAD_STRONG(name)
#define DEF___PTHREAD(name) DEF___PTHREAD_STRONG(name)

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

