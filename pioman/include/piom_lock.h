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

#if defined(PIOMAN_MARCEL)
#  include "piom_marcel.h"
#elif defined(PIOMAN_PTHREAD)
#  include "piom_pthread.h"
#elif defined(PIOMAN_ABT)
#  include "piom_abt.h"
#else
#  include "piom_nothread.h"
#endif

/* ** cond ************************************************* */

typedef uint8_t piom_cond_value_t;

#if defined(PIOMAN_MULTITHREAD)
typedef struct
{
    volatile piom_cond_value_t value;
    piom_sem_t sem;
} piom_cond_t;
#else /* PIOMAN_MULTITHREAD */
typedef piom_cond_value_t piom_cond_t;
#endif	/* PIOMAN_MULTITHREAD */

/* ** mask ************************************************* */

#ifdef PIOMAN_MULTITHREAD

typedef volatile int piom_mask_t;

static inline void piom_mask_init(piom_mask_t*mask)
{
  *mask = 0;
}
static inline int piom_mask_release(piom_mask_t*mask)
{
  __sync_fetch_and_sub(mask, 1);
  return 0;
}
/** @return 0 for success, 1 else */
static inline int piom_mask_acquire(piom_mask_t*mask)
{
  if(*mask != 0)
    return 1;
  if(__sync_fetch_and_add(mask, 1) != 0)
    {
      __sync_fetch_and_sub(mask, 1);
      return 1;
    }
  return 0;
}

static inline void piom_cond_signal(piom_cond_t *cond, piom_cond_value_t mask)
{
  __sync_fetch_and_or(&cond->value, mask);  /* cond->value |= mask; */
  piom_sem_V(&cond->sem);
}

static inline int piom_cond_test(piom_cond_t *cond, piom_cond_value_t mask)
{
  return cond->value & mask;
}

static inline void piom_cond_init(piom_cond_t *cond, piom_cond_value_t initial)
{
  cond->value = initial;
  piom_sem_init(&cond->sem, 0);
}

static inline void piom_cond_mask(piom_cond_t *cond, piom_cond_value_t mask)
{
  __sync_fetch_and_and(&cond->value, mask); /* cond->value &= mask; */
}

extern void piom_cond_wait(piom_cond_t *cond, piom_cond_value_t mask);

#else /* PIOMAN_MULTITHREAD */

typedef int piom_mask_t;

static inline void piom_mask_init(piom_mask_t*mask)
{
  *mask = 0;
}
static inline int piom_mask_release(piom_mask_t*mask)
{
  assert(*mask == 1);
  *mask = 0;
  return 0;
}
/** @return 0 for success, 1 else */
static inline int piom_mask_acquire(piom_mask_t*mask)
{
  if(*mask != 0)
    {
      return 1;
    }
  else
    {
      *mask = 1;
      return 0;
    }
}

static inline void piom_cond_wait(piom_cond_t*cond, piom_cond_value_t mask)
{
  while(!(*cond & mask))
    piom_ltask_schedule(PIOM_POLL_POINT_BUSY);		
}
static inline void piom_cond_signal(piom_cond_t*cond, piom_cond_value_t mask)
{
  *cond |= mask;
}
static inline int piom_cond_test(piom_cond_t*cond, piom_cond_value_t mask)
{
  return *cond & mask;
}
static inline void piom_cond_init(piom_cond_t*cond, piom_cond_value_t initial)
{
  *cond = initial;
}

static inline void piom_cond_mask(piom_cond_t*cond, piom_cond_value_t mask)
{
  *cond &= mask;
}

#endif /* PIOMAN_MULTITHREAD */


/** @} */

#endif /* PIOM_LOCK_H */

