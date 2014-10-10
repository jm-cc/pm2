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

/** @defgroup  piom_lock PIOMan locking interface.
 * This interface manages locking and unified threading API.
 * It is used internally for ltasks locking, and is usable by endusers.
 * @ingroup pioman
 * @{
 */

#ifdef PIOMAN_LOCK_MARCEL

/* ** locks for Marcel ************************************* */

#define piom_spinlock_t marcel_spinlock_t

#define piom_spin_init(lock)           marcel_spin_init(lock, 1)
#define piom_spin_lock(lock) 	       marcel_spin_lock_tasklet_disable(lock)
#define piom_spin_unlock(lock) 	       marcel_spin_unlock_tasklet_enable(lock)
#define piom_spin_trylock(lock)	       marcel_spin_trylock_tasklet_disable(lock)

#define piom_thread_t     marcel_t
#define PIOM_THREAD_NULL NULL
#define PIOM_SELF       marcel_self()

#elif defined(PIOMAN_LOCK_PTHREAD)

/* ** locks for pthread ************************************ */

#ifdef PIOMAN_PTHREAD_SPINLOCK

#define piom_spinlock_t           pthread_spinlock_t
#define piom_spin_init(lock)      pthread_spin_init(lock, 0)

static inline int piom_spin_lock(piom_spinlock_t*lock)
{
    return pthread_spin_lock(lock);
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
    int rc = pthread_spin_unlock(lock);
    return rc;
}
static inline int piom_spin_trylock(piom_spinlock_t*lock)
{
    int rc = (pthread_spin_trylock(lock) == 0);
    return rc;
}
#else /* PIOMAN_PTHREAD_SPINLOCK */

typedef pthread_mutex_t piom_spinlock_t;

static inline void piom_spin_init(piom_spinlock_t*lock)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pthread_mutex_init(lock, &attr);
}
static inline int piom_spin_lock(piom_spinlock_t*lock)
{
    int rc = pthread_mutex_lock(lock);
    return rc;
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
    int rc = pthread_mutex_unlock(lock);
    return rc;
}
static inline int piom_spin_trylock(piom_spinlock_t*lock)
{
    int rc = (pthread_mutex_trylock(lock) == 0);
    return rc;
}

#endif /* PIOMAN_PTHREAD_SPINLOCK */

#define piom_thread_t pthread_t
#define PIOM_THREAD_NULL ((pthread_t)(-1))
#define PIOM_SELF pthread_self()

#elif defined(PIOMAN_LOCK_NONE)

/* ** no locks ********************************************* */

#define piom_spinlock_t		       int
#define piom_spin_init(lock)           (void) 0
#define piom_spin_lock(lock) 	       (void) 0
#define piom_spin_trylock(lock)        1
#define piom_spin_unlock(lock) 	       (void) 0

#define piom_thread_t     int
#define PIOM_THREAD_NULL 0
#define PIOM_SELF       1

#else
#  error "PIOMan: no lock scheme defined."
#endif

/** @} */

#endif /* PIOM_LOCK_H */

