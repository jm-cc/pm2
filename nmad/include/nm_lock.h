/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2009-2017 (see AUTHORS file)
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

/** @file nm_lock.h
 * Locking interface used in nm core. This is not part of the public API.
 */

#ifndef NM_LOCK_H
#define NM_LOCK_H

#include <nm_private.h>

/** acquire the nm core lock */
static inline void nm_core_lock(struct nm_core*p_core);

/** try to lock the nm core core
 * return 1 is lock is successfully acquired, 0 otherwise
 */
static inline int nm_core_trylock(struct nm_core*p_core);

/** unlock the nm core lock */
static inline void nm_core_unlock(struct nm_core*p_core);

/** init the core lock */
static inline void nm_core_lock_init(struct nm_core*p_core);

/** destroy the core lock */
static inline void nm_core_lock_destroy(struct nm_core*p_core);

/** assert that current thread doesn't hold the lock */
static inline void nm_core_nolock_assert(struct nm_core*p_core);

/** assert that current thread holds the lock */
static inline void nm_core_lock_assert(struct nm_core*p_core);


/* ********************************************************* */
/* inline definition */


static inline void nm_core_lock(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_lock(&p_core->lock);
  nm_profile_inc(p_core->profiling.n_locks);
#endif /* PIOMAN */
}

static inline int nm_core_trylock(struct nm_core*p_core)
{
#ifdef PIOMAN
  int rc = piom_spin_trylock(&p_core->lock);
  if(rc)
    nm_profile_inc(p_core->profiling.n_locks);
  return rc;
#else /* PIOMAN*/
  return 1;
#endif /* PIOMAN */
}

static inline void nm_core_unlock(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_unlock(&p_core->lock);
#endif /* PIOMAN */
}

static inline void nm_core_lock_init(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_init(&p_core->lock);
#endif /* PIOMAN */
}

static inline void nm_core_lock_destroy(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_destroy(&p_core->lock);
#endif /* PIOMAN */
}

static inline void nm_core_lock_assert(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_assert_locked(&p_core->lock);
#endif /* PIOMAN */
}

static inline void nm_core_nolock_assert(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_assert_notlocked(&p_core->lock);
#endif /* PIOMAN */
}

/** memory fence only when multithread */
static inline void nm_mem_fence(void)
{
#if defined(PIOMAN_MULTITHREAD)
  __sync_synchronize();
#endif /* PIOMAN_MULTITHREAD */
}

/** increment int, atomic only when multithread */
static inline int nm_atomic_inc(int*v)
{
#if defined(PIOMAN_MULTITHREAD)
  return __sync_fetch_and_add(v, 1);
#else
  return (*v)++;
#endif /* PIOMAN_MULTITHREAD */
}

/** decrement int, atomic only when multithread */
static inline int nm_atomic_dec(int*v)
{
#if defined(PIOMAN_MULTITHREAD)
  return __sync_sub_and_fetch(v, 1);
#else
  return --(*v);
#endif /* PIOMAN_MULTITHREAD */
}

/** int add, atomic only when multithread */
static inline void nm_atomic_add(int*v, int v2)
{
#if defined(PIOMAN_MULTITHREAD)
  __sync_fetch_and_add(v, v2);
#else
  (*v) += v2;
#endif /* PIOMAN_MULTITHREAD */
}

/** boolean int compare and swap */
static inline int nm_atomic_compare_and_swap(int*v, int oldval, int newval)
{
#if defined(PIOMAN_MULTITHREAD)
  return __sync_bool_compare_and_swap(v, oldval, newval);
#else
  if(*v == oldval)
    {
      *v = newval;
      return 1;
    }
  else
    {
      return 0;
    }
#endif /* PIOMAN_MULTITHREAD */
}

#endif	/* NM_LOCK_H */
