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

#ifndef PIOM_PTHREAD_H
#define PIOM_PTHREAD_H

#ifndef PIOM_CONFIG_H
#  error "Cannot include this file directly. Please inclued <pioman.h>."
#endif /* PIOM_CONFIG_H */

#ifndef PIOMAN_PTHREAD
#  error "inconsistency detected: PIOMAN_PTHREAD not defined in piom_pthread.h"
#endif /* PIOMAN_PTHREAD */

#include <errno.h>

/* ** base pthread types *********************************** */

#define piom_thread_t      pthread_t
#define piom_thread_attr_t pthread_attr_t
#define PIOM_THREAD_NULL ((pthread_t)(-1))
#define PIOM_THREAD_SELF          pthread_self()

/* ** spinlocks for pthread ******************************** */

#if defined(PIOMAN_PTHREAD_SPINLOCK)

struct piom_spinlock_s
{
  pthread_spinlock_t spinlock;
#ifdef PIOMAN_DEBUG
  pthread_t owner;
#endif
};
typedef struct piom_spinlock_s piom_spinlock_t;

static inline void piom_spin_init(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
  err = pthread_spin_init(&lock->spinlock, 0);
  assert(!err);
#ifdef PIOMAN_DEBUG
  lock->owner = PIOM_THREAD_NULL;
#endif
}

static inline void piom_spin_destroy(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
#ifdef PIOMAN_DEBUG
  assert(lock->owner == PIOM_THREAD_NULL);
#endif
  err = pthread_spin_destroy(&lock->spinlock);
  assert(!err);
}

static inline void piom_spin_lock(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
  err = pthread_spin_lock(&lock->spinlock);
#ifdef PIOMAN_DEBUG
  if(err != 0)
    PIOM_FATAL("cannot get spinlock- rc = %d\n", err);
  if(PIOM_THREAD_SELF == lock->owner)
    {
      PIOM_FATAL("deadlock detected. Self already holds spinlock.\n");
    }
  assert(lock->owner == PIOM_THREAD_NULL);
  lock->owner = PIOM_THREAD_SELF;
#endif
}
static inline void piom_spin_unlock(piom_spinlock_t*lock)
{
#ifdef PIOMAN_DEBUG
  assert(lock->owner == PIOM_THREAD_SELF);
  lock->owner = PIOM_THREAD_NULL;
#endif
  int err __attribute__((unused));
  err = pthread_spin_unlock(&lock->spinlock);
#ifdef PIOMAN_DEBUG
  if(err != 0)
    PIOM_FATAL("error in spin_unlock- rc = %d\n", err);
#endif
}
static inline int piom_spin_trylock(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
  err = pthread_spin_trylock(&lock->spinlock);
  int rc = (err == 0);
#ifdef PIOMAN_DEBUG
  assert((err == 0) || (err == EBUSY));
  if(rc)
    {
      assert(lock->owner == PIOM_THREAD_NULL);
      lock->owner = PIOM_THREAD_SELF;
    }
#endif
  return rc;
}

#elif defined(PIOMAN_MUTEX_SPINLOCK)

typedef pthread_mutex_t piom_spinlock_t;

static inline void piom_spin_init(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
  err = pthread_mutex_init(lock, &attr);
  assert(!err);
}
static inline void piom_spin_destroy(piom_spinlock_t*lock)
{
  int err __attribute__((unused));
  err = pthread_mutex_destory(lock);
  assert(!err);
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
static inline void piom_spin_destroy(piom_spinlock_t*lock)
{
  assert(*lock == 0);
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


static inline void piom_sem_P(piom_sem_t*sem)
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
  int err = sem_wait(sem);
  if(err)
    PIOM_FATAL("sem_wait() - err = %d; errno = %d\n", err, errno);
#endif /* PIOMAN_SEM_COND */
}

static inline void piom_sem_V(piom_sem_t*sem)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_lock(&sem->mutex);
  sem->n++;
  pthread_cond_signal(&sem->cond);
  pthread_mutex_unlock(&sem->mutex);
#else /* PIOMAN_SEM_COND */
  int err = sem_post(sem);
  if(err)
    PIOM_FATAL("sem_post() - err = %d; errno = %d\n", err, errno);
#endif /* PIOMAN_SEM_COND */
}

static inline void piom_sem_init(piom_sem_t*sem, int initial)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_init(&sem->mutex, NULL);
  pthread_cond_init(&sem->cond, NULL);
  sem->n = initial;
#else /* PIOMAN_SEM_COND */
  int err = sem_init(sem, 0, initial);
  if(err)
    PIOM_FATAL("sem_init() - err = %d; errno = %d\n", err, errno);
#endif /* PIOMAN_SEM_COND */
}

static inline void piom_sem_destroy(piom_sem_t*sem)
{
#ifdef PIOMAN_SEM_COND
  pthread_mutex_destroy(&sem->mutex);
  pthread_cond_destroy(&sem->cond);
#else /* PIOMAN_SEM_COND */
  int err = sem_destroy(sem);
  if(err)
    PIOM_FATAL("sem_destroy() - err = %d; errno = %d\n", err, errno);
#endif /* PIOMAN_SEM_COND */
}

/* ** Thread creation and join ***************************** */

static inline int piom_thread_create(piom_thread_t*thread, piom_thread_attr_t*attr,
				     void*(*thread_func)(void*), void*arg)
{
  return pthread_create((pthread_t*)thread, (pthread_attr_t*)attr, thread_func, arg);
}

static inline int piom_thread_join(piom_thread_t thread)
{
  return pthread_join((pthread_t)thread, NULL);
}

#endif /* PIOM_PTHREAD_H */
