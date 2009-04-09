
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

#section macros

/** \brief Static initializer for `marcel_rwlock_t' objects.  */
#define MARCEL_RWLOCK_INITIALIZER				\
  { { 0, 0,							\
      MA_LPT_FASTLOCK_UNLOCKED, MA_LPT_FASTLOCK_UNLOCKED,	\
      0, 0, 0, 0, 0, 0, 0 } }

/* FIXME: `MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP' not provided
 * yet.  Being architecture-dependent, it's not a good fit for this header
 * (see Glibc's `nptl/sysdep/pthread/pthread.h').  */
/* #undef MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP */

enum
{
  MARCEL_RWLOCK_PREFER_READER_NP,
  MARCEL_RWLOCK_PREFER_WRITER_NP,
  MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
  MARCEL_RWLOCK_DEFAULT_NP = MARCEL_RWLOCK_PREFER_WRITER_NP
};

#section structures
#depend "marcel_threads.h[types]"

/* Read-write locks.  */
typedef lpt_rwlock_t marcel_rwlock_t, pmarcel_rwlock_t;

/* Attribute for read-write locks (from glibc's
 * `sysdeps/unix/sysv/linux/internaltypes.h').  */
typedef struct marcel_rwlockattr
{
  int __lockkind;
  int __pshared;
} marcel_rwlockattr_t, pmarcel_rwlockattr_t;

#section functions
#depend "marcel_alias.h[macros]"

DEC_MARCEL_POSIX(int, rwlock_init, (marcel_rwlock_t * __restrict rwlock,
			       const marcel_rwlockattr_t * __restrict attr)
		 __THROW);

DEC_MARCEL_POSIX(int, rwlock_destroy, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlock_rdlock, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlock_timedrdlock, (marcel_rwlock_t * __restrict rwlock,
				      const struct timespec * __restrict abstime)
		 __THROW);

DEC_MARCEL_POSIX(int, rwlock_timedwrlock, (marcel_rwlock_t * __restrict rwlock,
				      const struct timespec * __restrict abstime)
		 __THROW);

DEC_MARCEL_POSIX(int, rwlock_tryrdlock, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlock_wrlock, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlock_trywrlock, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlock_unlock, (marcel_rwlock_t *rwlock) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_init, (marcel_rwlockattr_t *attr) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_destroy, (marcel_rwlockattr_t *attr) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_getpshared,
		 (const marcel_rwlockattr_t * __restrict attr,
		  int * __restrict pshared) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_setpshared,
		 (marcel_rwlockattr_t *attr, int pshared) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_getkind_np,
		 (const marcel_rwlockattr_t * __restrict attr,
		  int * __restrict pref) __THROW);

DEC_MARCEL_POSIX(int, rwlockattr_setkind_np, (marcel_rwlockattr_t *attr, int pref) __THROW);

