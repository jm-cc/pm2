/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
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
#ifndef NM_LOCK_INLINE
#define NM_LOCK_INLINE

/** Lock entirely NewMadeleine */
static inline void nm_core_lock(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_lock(&p_core->lock);
#ifdef DEBUG
  p_core->lock_holder = PIOM_THREAD_SELF;
#endif
#endif
}

/** Try to lock NewMadeleine 
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int nm_core_trylock(struct nm_core*p_core)
{
#ifdef PIOMAN
  int rc = piom_spin_trylock(&p_core->lock);
#ifdef DEBUG
  if(rc == 1)
    p_core->lock_holder = PIOM_THREAD_SELF;
#endif
  return rc;
#else
  return 1;
#endif
}

/** Unlock NewMadeleine */
static inline void nm_core_unlock(struct nm_core*p_core)
{
#ifdef PIOMAN
#ifdef DEBUG
  assert(p_core->lock_holder == PIOM_THREAD_SELF);
  p_core->lock_holder = PIOM_THREAD_NULL;
#endif
  piom_spin_unlock(&p_core->lock);
#endif
}

static inline void nm_core_lock_init(struct nm_core*p_core)
{
#ifdef PIOMAN
  piom_spin_init(&p_core->lock);
#ifdef DEBUG
  p_core->lock_holder = PIOM_THREAD_NULL;
#endif
#endif
}

static inline void nm_core_lock_destroy(struct nm_core*p_core)
{
#ifdef PIOMAN
#ifdef DEBUG
  assert(p_core->lock_holder == PIOM_THREAD_NULL);
#endif
  piom_spin_destroy(&p_core->lock);
#endif
}

/** assert that current thread holds the lock */
static inline void nm_core_lock_assert(struct nm_core*p_core)
{
#ifdef PIOMAN
#ifdef DEBUG
  assert(PIOM_THREAD_SELF == p_core->lock_holder);
#endif
#endif
}

/** assert that current thread doesn't hold the lock */
static inline void nm_core_nolock_assert(struct nm_core*p_core)
{
#ifdef PIOMAN
#ifdef DEBUG
  assert(PIOM_THREAD_SELF != p_core->lock_holder);
#endif
#endif
}


#endif /* NM_LOCK_INLINE */
