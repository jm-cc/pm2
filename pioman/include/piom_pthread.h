/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008-2015 "the PM2 team" (see AUTHORS file)
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

#ifndef PIOM_PTHREAD_H
#define PIOM_PTHREAD_H

#ifndef PIOMAN_PTHREAD
#error "inconsistency detected: PIOMAN_PTHREAD not defined in piom_pthread.h"
#endif /* PIOMAN_PTHREAD */

/* ** base pthread types *********************************** */

#define piom_thread_t      pthread_t
#define PIOM_THREAD_NULL ((pthread_t)(-1))
#define PIOM_SELF          pthread_self()

/* ** spinlocks for pthread ******************************** */

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

#elif defined(PIOMAN_MUTEX_SPINLOCK)

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
#elif defined (PIOMAN_MINI_SPINLOCK)

/* ** minimalistic spinlocks */

typedef volatile int piom_spinlock_t;

static inline void piom_spin_init(piom_spinlock_t*lock)
{
  *lock = 0;
}
static inline int piom_spin_lock(piom_spinlock_t*lock)
{
  int k = 0;
  while(__sync_fetch_and_add(lock, 1) != 0)
    {
      __sync_fetch_and_sub(lock, 1);
      k++;
      if(k > 100)
	{
	  sched_yield();
	}
    }
  return 0;
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
  __sync_fetch_and_sub(lock, 1);
  return 0;
}
static inline int piom_spin_trylock(piom_spinlock_t*lock)
{
  if(*lock != 0)
    return 0;
  if(__sync_fetch_and_add(lock, 1) != 0)
    {
      __sync_fetch_and_sub(lock, 1);
      return 0;
    }
  return 1;
}

#else
#error "PIOMan: no spinlock scheme defined."
#endif /* PIOMAN_PTHREAD_SPINLOCK */

/* ** semaphores ******************************************* */

#ifdef PIOMAN_SEM_COND
typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int n;
} piom_sem_t;
#else /* PIOMAN_SEM_COND */
/** use system semaphores as piom_sem_t  */
typedef sem_t piom_sem_t;
#endif /* PIOMAN_SEM_COND */


static inline void piom_sem_P(piom_sem_t *sem)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_lock(&sem->mutex);
  sem->n--;
  while(sem->n < 0)
    {
      pthread_cond_wait(&sem->cond, &sem->mutex);
    }
  pthread_mutex_unlock(&sem->mutex);
#else /* PIOMAN_SEM_COND */
  while(sem_wait(sem) == -1)
    ;
#endif /* PIOMAN_SEM_COND */
}

static inline void piom_sem_V(piom_sem_t *sem)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_lock(&sem->mutex);
  sem->n++;
  pthread_cond_signal(&sem->cond);
  pthread_mutex_unlock(&sem->mutex);
#else /* PIOMAN_SEM_COND */
  sem_post(sem);
#endif /* PIOMAN_SEM_COND */
}

static inline void piom_sem_init(piom_sem_t *sem, int initial)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_init(&sem->mutex, NULL);
  pthread_cond_init(&sem->cond, NULL);
  sem->n = initial;
#else /* PIOMAN_SEM_COND */
  sem_init(sem, 0, initial);
#endif /* PIOMAN_SEM_COND */
}

#endif /* PIOM_PTHREAD_H */
