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

#ifndef PIOM_LOCK_H
#define PIOM_LOCK_H

#include <assert.h>

#ifdef PIOMAN_LOCK_MARCEL

/* ** locks for Marcel ************************************* */

#define piom_spinlock_t marcel_spinlock_t

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

#define piom_thread_t     marcel_t
#define PIOM_THREAD_NULL NULL
#define PIOM_SELF       marcel_self()
#define PIOM_CURRENT_VP marcel_current_vp()
#define piom_ltask_current_vp() marcel_current_vp()
#define piom_ltask_nvp()        marcel_nbvps()

#elif defined(PIOMAN_LOCK_PTHREAD)

/* ** locks for pthread ************************************ */

extern volatile int __piom_ltask_handler_masked;

#define piom_spinlock_t                pthread_spinlock_t
#define piom_spin_lock_init(lock)      pthread_spin_init(lock, 0)


static inline int piom_spin_lock(piom_spinlock_t*lock)
{
    __sync_fetch_and_add(&__piom_ltask_handler_masked, 1);
    assert(__piom_ltask_handler_masked > 0);
    return pthread_spin_lock(lock);
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
    assert(__piom_ltask_handler_masked > 0);
    int rc = pthread_spin_unlock(lock);
    __sync_fetch_and_sub(&__piom_ltask_handler_masked, 1);
    return rc;
}
static inline int piom_spin_trylock(piom_spinlock_t *lock)
{
    __sync_fetch_and_add(&__piom_ltask_handler_masked, 1);
    int rc = (pthread_spin_trylock(lock) == 0);
    if(!rc)
	{
	    __sync_fetch_and_sub(&__piom_ltask_handler_masked, 1);
	}
    return rc;
}

#define piom_rwlock_t                  pthread_rwlock_t
#define PIOM_RW_LOCK_UNLOCKED          PTHREAD_RW_LOCK_UNLOCKED
#define piom_rwlock_init(lock)         pthread_rwlock_init(lock, NULL)
#define piom_rwlock_is_locked(lock)    pthread_rwlock_trywrlocked(lock)


#define piom_read_lock(lock)           pthread_rwlock_rdlock(lock)
#define piom_read_unlock(lock)         pthread_rwlock_unlock(lock)
#define piom_write_lock(lock)          pthread_rwlock_wrlock(lock)
#define piom_write_unlock(lock)        pthread_rwlock_unlock(lock)

#define piom_thread_t pthread_t
#define PIOM_THREAD_NULL ((pthread_t)(-1))
#define PIOM_SELF pthread_self()
#define PIOM_CURRENT_VP 0
#define piom_ltask_current_vp() (0)
#define piom_ltask_nvp()        (1)

#elif defined(PIOMAN_LOCK_NONE)

/* ** no locks ********************************************* */

#define piom_spinlock_t		       int
#define piom_spin_lock_init(lock)      (void) 0
#define piom_spin_lock(lock) 	       (void) 0
#define piom_spin_trylock(lock)        1
#define piom_spin_unlock(lock) 	       (void) 0

#define piom_rwlock_t   int
#define PIOM_RW_LOCK_UNLOCKED 0

#define piom_read_lock(lock)    (void) 0
#define piom_read_unlock(lock)  (void) 0
#define piom_write_lock(lock)   (void) 0
#define piom_write_unlock(lock) (void) 0

#define piom_thread_t     int
#define PIOM_THREAD_NULL 0
#define PIOM_SELF       1
#define PIOM_CURRENT_VP 1
#define piom_ltask_current_vp() (0)
#define piom_ltask_nvp()        (1)

#else
#  error "PIOMan: no lock scheme defined."
#endif

#endif /* PIOM_LOCK_H */

