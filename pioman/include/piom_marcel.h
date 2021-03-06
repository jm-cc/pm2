/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008-2017 "the PM2 team" (see AUTHORS file)
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

#ifndef PIOM_MARCEL_H
#define PIOM_MARCEL_H

#ifndef PIOM_CONFIG_H
#  error "Cannot include this file directly. Please inclued <pioman.h>."
#endif /* PIOM_CONFIG_H */

#ifndef PIOMAN_MARCEL
#error "inconsistency detected: PIOMAN_MARCEL not defined in piom_marcel.h"
#endif /* PIOMAN_MARCEL */

/* ** base marcel types ************************************ */

#define piom_thread_t                  marcel_t
#define piom_thread_attr_t             marcel_attr_t
#define PIOM_THREAD_NULL               NULL
#define PIOM_THREAD_SELF               marcel_self()

/* ** locks for Marcel ************************************* */

#define piom_spinlock_t                marcel_spinlock_t
#define piom_spin_init(lock)           marcel_spin_init(lock, 1)
#define piom_spin_destroy(lock)        marcel_spin_destroy(lock)
#define piom_spin_lock(lock) 	       marcel_spin_lock_tasklet_disable(lock)
#define piom_spin_unlock(lock) 	       marcel_spin_unlock_tasklet_enable(lock)
#define piom_spin_trylock(lock)	       marcel_spin_trylock_tasklet_disable(lock)

static inline void piom_spin_assert_locked(piom_spinlock_t*lock)
{
#ifdef PIOMAN_DEBUG
  /* not implemented */
#endif
}

static inline void piom_spin_assert_notlocked(piom_spinlock_t*lock)
{
#ifdef PIOMAN_DEBUG
  /* not implemented */
#endif
}
/* ** semaphores ******************************************* */

typedef marcel_sem_t piom_sem_t;


static inline void piom_sem_P(piom_sem_t *sem)
{
  marcel_sem_P(sem);
}

static inline void piom_sem_V(piom_sem_t *sem)
{
  marcel_sem_V(sem);
}

static inline void piom_sem_init(piom_sem_t *sem, int initial)
{
  marcel_sem_init(sem, initial);
}

static inline void piom_sem_destroy(piom_sem_t*sem)
{
  marcel_sem_destroy(sem);
}

static inline int piom_thread_create(piom_thread_t*thread, piom_thread_attr_t*attr,
				     void*(*thread_func)(void*), void*arg)
{
  return marcel_create((marcel_t*)thread, (marcel_attr_t*)attr, thread_func, arg);
}

static inline int piom_thread_join(piom_thread_t thread)
{
  return marcel_join((marcel_t)thread, NULL);
}


#endif /* PIOM_MARCEL_H */
