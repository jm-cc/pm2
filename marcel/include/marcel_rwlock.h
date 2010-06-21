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


#ifndef __MARCEL_RWLOCK_H__
#define __MARCEL_RWLOCK_H__


#include <sys/time.h>
#include "sys/marcel_flags.h"
#include "marcel_types.h"
#include "marcel_alias.h"
#ifdef MA__IFACE_PMARCEL
#include "marcel_pmarcel.h"
#endif


/** Public macros **/
/** \brief Static initializer for `marcel_rwlock_t' objects.  */
#define MARCEL_RWLOCK_INITIALIZER				\
  {.__data = { \
    .__readers_wakeup=MA_LPT_FASTLOCK_UNLOCKED, \
    .__writer_wakeup=MA_LPT_FASTLOCK_UNLOCKED, \
  }}

/* FIXME: `MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP' not provided
 * yet.  Being architecture-dependent, it's not a good fit for this header
 * (see Glibc's `nptl/sysdep/pthread/pthread.h').  */
/* #undef MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP */
enum {
	MARCEL_RWLOCK_PREFER_READER_NP,
	MARCEL_RWLOCK_PREFER_WRITER_NP,
	MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
	MARCEL_RWLOCK_DEFAULT_NP = MARCEL_RWLOCK_PREFER_WRITER_NP
};


/** Public functions **/
DEC_MARCEL(int, rwlock_init, (marcel_rwlock_t * __restrict rwlock,
			      const marcel_rwlockattr_t * __restrict attr) __THROW);

DEC_MARCEL(int, rwlock_destroy, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlock_rdlock, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlock_timedrdlock, (marcel_rwlock_t * __restrict rwlock,
				     const struct timespec * __restrict abstime) __THROW);

DEC_MARCEL(int, rwlock_timedwrlock, (marcel_rwlock_t * __restrict rwlock,
				     const struct timespec * __restrict abstime) __THROW);

DEC_MARCEL(int, rwlock_tryrdlock, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlock_wrlock, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlock_trywrlock, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlock_unlock, (marcel_rwlock_t * rwlock) __THROW);

DEC_MARCEL(int, rwlockattr_init, (marcel_rwlockattr_t * attr) __THROW);

DEC_MARCEL(int, rwlockattr_destroy, (marcel_rwlockattr_t * attr) __THROW);

DEC_MARCEL(int, rwlockattr_getpshared,
	   (const marcel_rwlockattr_t * __restrict attr,
	    int *__restrict pshared) __THROW);

DEC_MARCEL(int, rwlockattr_setpshared, (marcel_rwlockattr_t * attr, int pshared) __THROW);

DEC_MARCEL(int, rwlockattr_getkind_np,
	   (const marcel_rwlockattr_t * __restrict attr, int *__restrict pref) __THROW);

DEC_MARCEL(int, rwlockattr_setkind_np, (marcel_rwlockattr_t * attr, int pref) __THROW);


#endif /** __MARCEL_RWLOCK_H__ **/
