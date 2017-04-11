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

#ifndef PIOM_NOTHREAD_H
#define PIOM_NOTHREAD_H

#ifdef PIOMAN_MULTITHREAD
#  error "inconsistency detected: PIOMAN_MULTITHREAD defined in piom_nothread.h"
#endif /* PIOMAN_MULTITHREAD */

/* ** base dummy types ************************************* */

#define piom_thread_t     int
#define PIOM_THREAD_NULL  0
#define PIOM_THREAD_SELF  1

/* ** spinlocks for no-thread ****************************** */

typedef int piom_spinlock_t;

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
  assert(*lock == 0);
  *lock = 1;
  return 0;
}
static inline int piom_spin_unlock(piom_spinlock_t*lock)
{
  assert(*lock == 1);
  *lock = 0;
  return 0;
}
static inline int piom_spin_trylock(piom_spinlock_t*lock)
{
  if(*lock == 0)
    {
      *lock = 1;
      return 1;
    }
  else
    {
      return 0;
    }
}

/* ** semaphores ******************************************* */

/** dummy non-threaded semaphore */
typedef int piom_sem_t;

static inline void piom_sem_P(piom_sem_t*sem)
{
  (*sem)--;
  while((*sem) < 0)
    {
      piom_ltask_schedule(PIOM_POLL_POINT_BUSY);
    }
}

static inline void piom_sem_V(piom_sem_t*sem)
{
  (*sem)++;
}

static inline void piom_sem_init(piom_sem_t*sem, int initial)
{
  (*sem) = initial;
}

/* ** check wait while dispatching ************************* */

extern int piom_nothread_dispatching;

static inline void piom_nothread_check_wait(void)
{
  if(piom_nothread_dispatching > 0)
    {
      fprintf(stderr, "pioman: deadlock detected- cannot wait while dispatching ltask.\n");
      abort();
    }
}

#endif /* PIOM_NOTHREAD_H */
