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


#include "marcel.h"
#include "marcel_pmarcel.h"


#if defined(MARCEL_LIBPTHREAD) && defined(__GLIBC__)


/* Check at compile-time whether the `lpt_' types are smaller than or as
 * large as NPTL's types.  */
MA_VERIFY(sizeof(pmarcel_sem_t) <= sizeof(sem_t));
MA_VERIFY(sizeof(lpt_attr_t) <= sizeof(pthread_attr_t));
MA_VERIFY(sizeof(lpt_mutex_t) <= sizeof(pthread_mutex_t));
MA_VERIFY(sizeof(lpt_mutexattr_t) <= sizeof(pthread_mutexattr_t));
MA_VERIFY(sizeof(lpt_cond_t) <= sizeof(pthread_cond_t));
MA_VERIFY(sizeof(lpt_condattr_t) <= sizeof(pthread_condattr_t));
MA_VERIFY(sizeof(lpt_key_t) <= sizeof(pthread_key_t));
MA_VERIFY(sizeof(lpt_once_t) <= sizeof(pthread_once_t));
MA_VERIFY(sizeof(lpt_rwlock_t) <= sizeof(pthread_rwlock_t));
MA_VERIFY(sizeof(lpt_rwlockattr_t) <= sizeof(pthread_rwlockattr_t));
MA_VERIFY(sizeof(lpt_spinlock_t) <= sizeof(pthread_spinlock_t));
MA_VERIFY(sizeof(lpt_barrier_t) <= sizeof(pthread_barrier_t));
MA_VERIFY(sizeof(lpt_barrierattr_t) <= sizeof(pthread_barrierattr_t));

/* Check at compile-time whether the alignment constraints of the `lpt_'
 * types are compatible with those of NPTL's types.  */
MA_VERIFY(__alignof(pmarcel_sem_t) <= __alignof(sem_t));
MA_VERIFY(__alignof(lpt_attr_t) <= __alignof(pthread_attr_t));
MA_VERIFY(__alignof(lpt_mutex_t) <= __alignof(pthread_mutex_t));
MA_VERIFY(__alignof(lpt_mutexattr_t) <= __alignof(pthread_mutexattr_t));
MA_VERIFY(__alignof(lpt_cond_t) <= __alignof(pthread_cond_t));
MA_VERIFY(__alignof(lpt_condattr_t) <= __alignof(pthread_condattr_t));
MA_VERIFY(__alignof(lpt_key_t) <= __alignof(pthread_key_t));
MA_VERIFY(__alignof(lpt_once_t) <= __alignof(pthread_once_t));
MA_VERIFY(__alignof(lpt_rwlock_t) <= __alignof(pthread_rwlock_t));
MA_VERIFY(__alignof(lpt_rwlockattr_t) <= __alignof(pthread_rwlockattr_t));
MA_VERIFY(__alignof(lpt_spinlock_t) <= __alignof(pthread_spinlock_t));
MA_VERIFY(__alignof(lpt_barrier_t) <= __alignof(pthread_barrier_t));
MA_VERIFY(__alignof(lpt_barrierattr_t) <= __alignof(pthread_barrierattr_t));


#endif /** MARCEL_LIBPTHREAD & GLIBC **/
