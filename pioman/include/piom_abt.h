/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2015 "the PM2 team" (see AUTHORS file)
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

#ifndef PIOM_ABT_H
#define PIOM_ABT_H

#ifndef PIOMAN_ABT
#error "inconsistency detected: PIOMAN_ABT not defined in piom_abt.h"
#endif /* PIOMAN_ABT */

/* ** base ABT types *************************************** */

typedef ABT_thread piom_thread_t;
#define PIOM_THREAD_NULL  ABT_THREAD_NULL
#define PIOM_THREAD_SELF         piom_self()

static inline ABT_thread piom_self(void)
{
  ABT_thread t = ABT_THREAD_NULL;
  ABT_thread_self(&t);
  assert(t != ABT_THREAD_NULL);
  return t;
}

/* ** locks for ABT **************************************** */

typedef ABT_mutex piom_spinlock_t;
#define piom_spin_init(lock)           ABT_mutex_create(lock)
#define piom_spin_lock(lock) 	       ABT_mutex_lock(*(lock))
#define piom_spin_unlock(lock) 	       ABT_mutex_unlock(*(lock))
#define piom_spin_trylock(lock)	       (!ABT_mutex_trylock(*(lock)))

/* ** semaphores ******************************************* */

typedef struct
{
  ABT_mutex mutex;
  ABT_cond cond;
  int n;
} piom_sem_t;

static inline void piom_sem_P(piom_sem_t *sem)
{
  ABT_mutex_lock(sem->mutex);
  sem->n--;
  while(sem->n < 0)
    {
      ABT_cond_wait(sem->cond, sem->mutex);
    }
  ABT_mutex_unlock(sem->mutex);
}

static inline void piom_sem_V(piom_sem_t *sem)
{
  ABT_mutex_lock(sem->mutex);
  sem->n++;
  ABT_cond_signal(sem->cond);
  ABT_mutex_unlock(sem->mutex);
}

static inline void piom_sem_init(piom_sem_t *sem, int initial)
{
  ABT_mutex_create(&sem->mutex);
  ABT_cond_create(&sem->cond);
  sem->n = initial;
}

#endif /* PIOM_ABT_H */
