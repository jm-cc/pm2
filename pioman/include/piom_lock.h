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

extern void piom_tasklet_mask(void);

extern void piom_tasklet_unmask(void);

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

#define piom_spinlock_t           pthread_spinlock_t
#define piom_spin_init(lock)      pthread_spin_init(lock, 0)


static inline int piom_spin_lock(piom_spinlock_t*lock)
{
    piom_tasklet_mask();
    return pthread_spin_lock(lock);
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
    int rc = pthread_spin_unlock(lock);
    piom_tasklet_unmask();
    return rc;
}
static inline int piom_spin_trylock(piom_spinlock_t *lock)
{
    piom_tasklet_mask();
    int rc = (pthread_spin_trylock(lock) == 0);
    if(!rc)
	{
	    piom_tasklet_unmask();
	}
    return rc;
}

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

#endif /* PIOM_LOCK_H */

