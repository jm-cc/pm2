
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

# define MARCEL_RWLOCK_INITIALIZER \
  { .__rw_lock=MA_MARCEL_FASTLOCK_UNLOCKED, .__rw_readers=0, .__rw_writer=NULL,     \
    .__rw_read_waiting=NULL, .__rw_write_waiting=NULL,			      \
    .__rw_kind=MARCEL_RWLOCK_DEFAULT_NP, .__rw_pshared=MARCEL_PROCESS_PRIVATE }
# define MARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { .__rw_lock=MA_MARCEL_FASTLOCK_UNLOCKED, .__rw_readers=0, .__rw_writer=NULL,     \
    .__rw_read_waiting=NULL, .__rw_write_waiting=NULL,			      \
    .__rw_kind=MARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, 		      \
    .__rw_pshared=MARCEL_PROCESS_PRIVATE }

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
typedef struct _marcel_rwlock_t
{
  struct _marcel_fastlock __rw_lock; /* Lock to guarantee mutual exclusion */
  int __rw_readers;                   /* Number of readers */
  p_marcel_task_t __rw_writer;         /* Identity of writer, or NULL if none */
  p_marcel_task_t __rw_read_waiting;   /* Threads waiting for reading */
  p_marcel_task_t __rw_write_waiting;  /* Threads waiting for writing */
  int __rw_kind;                      /* Reader/Writer preference selection */
  int __rw_pshared;                   /* Shared between processes or not */
} marcel_rwlock_t, pmarcel_rwlock_t;


/* Attribute for read-write locks.  */
typedef struct
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

