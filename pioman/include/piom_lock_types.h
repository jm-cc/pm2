/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_LOCK_TYPES_H
#define PIOM_LOCK_TYPES_H

#ifdef MARCEL
#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#define piom_spinlock_t marcel_spinlock_t

#define PIOM_SPIN_LOCK_INITIALIZER	MARCEL_SPINLOCK_INITIALIZER
#define piom_spin_lock_init(lock)      marcel_spin_init(lock, 1)
#define piom_spin_lock(lock) 	       marcel_spin_lock_tasklet_disable(lock)
#define piom_spin_unlock(lock) 	       marcel_spin_unlock_tasklet_enable(lock)
#define piom_spin_trylock(lock)	       marcel_spin_trylock_tasklet_disable(lock)


#define piom_rwlock_t               marcel_rwlock_t
#define PIOM_RW_LOCK_UNLOCKED       MARCEL_RWLOCK_INITIALIZER
#define piom_rwlock_init(lock)      marcel_rwlock_init(lock)
#define piom_rwlock_is_locked(lock) marcel_rwlock_is_locked(lock)


#define piom_read_lock(lock)    marcel_rwlock_rdlock(lock)
#define piom_read_unlock(lock)  marcel_rwlock_unlock(lock)
#define piom_write_lock(lock)   marcel_rwlock_wrlock(lock)
#define piom_write_unlock(lock) marcel_rwlock_unlock(lock)

/* todo: what about preemption ? */
#define piom_read_lock_softirq(lock)    marcel_rwlock_rdlock(lock)
#define piom_read_unlock_softirq(lock)  marcel_rwlock_unlock(lock)
#define piom_write_lock_softirq(lock)   marcel_rwlock_wrlock(lock)
#define piom_write_unlock_softirq(lock) marcel_rwlock_unlock(lock)

#define piom_thread_t     marcel_t
#define PIOM_THREAD_NULL NULL
#define PIOM_SELF       marcel_self()
#define PIOM_CURRENT_VP marcel_current_vp()

#elif(defined(PIOM_PTHREAD_LOCK))
#include <pthread.h>

#define piom_spinlock_t                pthread_mutex_t
#define PIOM_SPIN_LOCK_INITIALIZER     PTHREAD_MUTEX_INITIALIZER
#define piom_spin_lock_init(lock)      pthread_mutex_init(lock, NULL)
#define piom_spin_lock(lock)           pthread_mutex_lock(lock)
#define piom_spin_unlock(lock)         pthread_mutex_unlock(lock)
#define piom_spin_trylock(lock)        (pthread_mutex_trylock(lock)==0)

#define piom_rwlock_t                  pthread_rwlock_t
#define PIOM_RW_LOCK_UNLOCKED          PTHREAD_RW_LOCK_UNLOCKED
#define piom_rwlock_init(lock)         pthread_rwlock_init(lock, NULL)
#define piom_rwlock_is_locked(lock)    pthread_rwlock_trywrlocked(lock)


#define piom_read_lock(lock)           pthread_rwlock_rdlock(lock)
#define piom_read_unlock(lock)         pthread_rwlock_unlock(lock)
#define piom_write_lock(lock)          pthread_rwlock_wrlock(lock)
#define piom_write_unlock(lock)        pthread_rwlock_unlock(lock)

#define piom_read_lock_softirq(lock)    pthread_rwlock_rdlock(lock)
#define piom_read_unlock_softirq(lock)  pthread_rwlock_unlock(lock)
#define piom_write_lock_softirq(lock)   pthread_rwlock_wrlock(lock)
#define piom_write_unlock_softirq(lock) pthread_rwlock_unlock(lock)

#define piom_thread_t pthread_t
#define PIOM_THREAD_NULL ((pthread_t)(-1))
#define PIOM_SELF pthread_self()
#define PIOM_CURRENT_VP 0

#else
#define piom_spinlock_t		       int
#define piom_spin_lock_init(lock)      (void) 0
#define piom_spin_lock(lock) 	       (void) 0
#define piom_spin_trylock(lock)        1
#define piom_spin_unlock(lock) 	       (void) 0

#define piom_rwlock_t   int
#define PIOM_RW_LOCK_UNLOCKED 0
#define PIOM_SPIN_LOCK_INITIALIZER 1

#define piom_read_lock(lock)    (void) 0
#define piom_read_unlock(lock)  (void) 0
#define piom_write_lock(lock)   (void) 0
#define piom_write_unlock(lock) (void) 0

#define piom_read_lock_softirq(lock)    (void) 0
#define piom_read_unlock_softirq(lock)  (void) 0
#define piom_write_lock_softirq(lock)   (void) 0
#define piom_write_unlock_softirq(lock) (void) 0

#define piom_thread_t     int
#define PIOM_THREAD_NULL 0
#define PIOM_SELF       1
#define PIOM_CURRENT_VP 1

#endif /* PIOM_THREAD_ENABLED */
#endif
