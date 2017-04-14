/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001-2017 "the PM2 team" (see AUTHORS file)
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

#ifndef PIOM_CONFIG_H
#  error "Cannot include this file directly. Please inclued <pioman.h>."
#endif /* PIOM_CONFIG_H */

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

/** value (bitmask) of cond */
typedef uint16_t piom_cond_value_t;

#if defined(PIOMAN_MULTITHREAD)
/** a mask type, used as a spinlock with only trylock/unlock */
typedef volatile int piom_mask_t;

struct piom_waitsem_s
{
  piom_sem_t sem;
  piom_cond_value_t mask;
};
typedef struct
{
  volatile piom_cond_value_t value;
  struct piom_waitsem_s*p_waitsem;
  piom_spinlock_t lock;
} piom_cond_t;
#else /* PIOMAN_MULTITHREAD */
typedef piom_cond_value_t piom_cond_t;
typedef int piom_mask_t;
#endif	/* PIOMAN_MULTITHREAD */

/* ** mask ************************************************* */

#ifdef PIOMAN_MULTITHREAD

static inline void piom_mask_init(piom_mask_t*mask)
{
  *mask = 0;
}
static inline int piom_mask_release(piom_mask_t*mask)
{
  __sync_fetch_and_sub(mask, 1);
  return 0;
}
/** @return 0 for success, non-zero else */
static inline int piom_mask_acquire(piom_mask_t*mask)
{
  if(*mask != 0)
    return 1;
  int rc = __sync_val_compare_and_swap(mask, 0, 1);
  return rc;
}

/** add a bit to the value, without signaling */
static inline void piom_cond_add(piom_cond_t*cond, piom_cond_value_t mask)
{
  __sync_fetch_and_or(&cond->value, mask);  /* cond->value |= mask; */
}
/** add a bit and signal */
static inline void piom_cond_signal(piom_cond_t*cond, piom_cond_value_t mask)
{
  /* WARNING- order matters here!
   * 1. check whether somebody is waiting on the cond with spinlock.
   *    (to ensure waiter atomically checks for status & sets waitsem)
   * 2. add bitmask to cond value. Do not ever access cond after
   *    (not even a spin unlock). Waiter may be busy-waiting and may
   *    free the cond immediately after the bit is set.
   * 3. signal on the semaphore. The semaphore must not have been freed
   *    since a waiter that sets waitsem _must_ wait on a sem_P.
   */
  piom_spin_lock(&cond->lock);
  struct piom_waitsem_s*p_waitsem = cond->p_waitsem;
  const piom_cond_value_t waitmask = p_waitsem ? (p_waitsem->mask & mask) : 0;
  __sync_fetch_and_or(&cond->value, mask);  /* cond->value |= mask; */
  piom_spin_unlock(&cond->lock);
  if(waitmask)
    {
      piom_sem_V(&p_waitsem->sem);
    }
}
/** tests whether a bit is set */
static inline piom_cond_value_t piom_cond_test(const piom_cond_t*cond, piom_cond_value_t mask)
{
  return cond->value & mask;
}
/** tests whether a bit is set, whit lock 
 * (needed to prevent race condition between cond destroy and 'finalized' state signal)
 */
static inline piom_cond_value_t piom_cond_test_locked(piom_cond_t*cond, piom_cond_value_t mask)
{
  piom_spin_lock(&cond->lock);
  piom_cond_value_t val = cond->value & mask;
  piom_spin_unlock(&cond->lock);
  return val;
}

/** initialize a piom cond, with the given value */
static inline void piom_cond_init(piom_cond_t*cond, piom_cond_value_t initial)
{
  cond->value = initial;
  cond->p_waitsem = NULL;
  piom_spin_init(&cond->lock);
}

/** destroy a piom cond */
static inline void piom_cond_destroy(piom_cond_t*cond)
{
  piom_spin_destroy(&cond->lock);
}

/** remove bits from cond value (does not trigger any signal) */
static inline void piom_cond_mask(piom_cond_t*cond, piom_cond_value_t mask)
{
  __sync_fetch_and_and(&cond->value, mask); /* cond->value &= mask; */
}

/** wait for the given bits in the cond to be set */
extern void piom_cond_wait(piom_cond_t*cond, piom_cond_value_t mask);

/** wait for the given bits in mutiple conds to be set */
extern void piom_cond_wait_all(void**pp_conds, int n, uintptr_t offset, piom_cond_value_t mask);

#else /* PIOMAN_MULTITHREAD */

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
static inline void piom_cond_wait_all(void**pp_conds, int n, uintptr_t offset, piom_cond_value_t mask)
{
  int i;
  for(i = 0; i < n; i++)
    {
      piom_cond_t*p_cond = (pp_conds[i] + offset);
      piom_cond_wait(p_cond, mask);
    }  
}
static inline void piom_cond_add(piom_cond_t*cond, piom_cond_value_t mask)
{
  *cond |= mask;
}
static inline void piom_cond_signal(piom_cond_t*cond, piom_cond_value_t mask)
{
  *cond |= mask;
}
static inline piom_cond_value_t piom_cond_test(const piom_cond_t*cond, piom_cond_value_t mask)
{
  return *cond & mask;
}
static inline piom_cond_value_t piom_cond_test_locked(piom_cond_t*cond, piom_cond_value_t mask)
{
  return *cond & mask;
}
static inline void piom_cond_init(piom_cond_t*cond, piom_cond_value_t initial)
{
  *cond = initial;
}
static inline void piom_cond_destroy(piom_cond_t*cond)
{
}
static inline void piom_cond_mask(piom_cond_t*cond, piom_cond_value_t mask)
{
  *cond &= mask;
}

#endif /* PIOMAN_MULTITHREAD */


/** @} */

#endif /* PIOM_LOCK_H */

